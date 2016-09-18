#include	<stdio.h>
#include	<stdlib.h>
#include	<strings.h>
#include	<string.h>
#include	<netdb.h>
#include	<errno.h>
#include	<unistd.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/param.h>
#include	<signal.h>
#include	<time.h>
#include	<dirent.h>
#include	"socklib.h"

/*
 * wsng.c - a web server
 *
 *    usage: wsng [ -c configfilenmame ]
 * features: supports the GET and HEAD commands only
 *           runs in the current directory
 *           forks a new child to handle each request
 *           added required additional features 
 *
 */


#define	PORTNUM	80
#define	SERVER_ROOT	"."
#define	CONFIG_FILE	"wsng.conf"
#define	VERSION		"1"

#define	MAX_RQ_LEN	4096
#define	LINELEN		1024
#define	PARAM_LEN	128
#define	VALUE_LEN	512
#define OPT_VAL_LEN	15

char	myhost[MAXHOSTNAMELEN];
int	myport;
char	*full_hostname();
char * rfc822_time(time_t);	/* gets time in correct format */

// struct for holding entension/content-type pairs from config file
struct ec { char * ext; char* content; struct ec* next; };
struct ec* type_table = NULL;

#define	oops(m,x)	{ perror(m); exit(x); }

/*
 * prototypes
 */

int	startup(int, char *a[], char [], int *);
void	read_til_crnl(FILE *);
void	process_rq( char *, FILE *);
void	bad_request(FILE *);
void	cannot_do(FILE *fp);
void	do_404(char *item, FILE *fp, int);
void	do_cat(char *f, FILE *fpsock, int);
void	do_exec( char *prog, FILE *fp, int);
void	do_ls(char *dir, FILE *fp, int);
int	ends_in_cgi(char *f);
char 	*file_type(char *f);
void	header( FILE *fp, int code, char *msg, char *content_type );
int	isadir(char *f);
int	index_used(char *, FILE *, int);
char	*modify_argument(char *arg, int len);
int	not_exist(char *f);
void	fatal(char *, char *);
void	handle_call(int);
int	read_request(FILE *, char *, int);
char	*readline(char *, int, FILE *);


int
main(int ac, char *av[])
{
	int 	sock, fd;

	/* set up */
	sock = startup(ac, av, myhost, &myport);

	/* sign on */
	printf("wsng%s started.  host=%s port=%d\n", VERSION, myhost, myport);

	/* main loop here */
	while(1)
	{
		fd    = accept( sock, NULL, NULL );	/* take a call	*/
		if ( fd == -1 )
			perror("accept");
		else
			handle_call(fd);		/* handle call	*/
	}
	return 0;
	/* never end */
}

/*
 * handle_call(fd) - serve the request arriving on fd
 * summary: fork, then get request, then process request
 *    rets: child exits with 1 for error, 0 for ok
 *    note: closes fd in parent
 */
void handle_call(int fd)
{
	int	pid = fork();
	FILE	*fpin, *fpout;
	char	request[MAX_RQ_LEN];

	if ( pid == -1 ){
		perror("fork");
		return;
	}

	/* child: buffer socket and talk with client */
	if ( pid == 0 )
	{
		fpin  = fdopen(fd, "r");
		fpout = fdopen(fd, "w");
		if ( fpin == NULL || fpout == NULL )
			exit(1);

		if ( read_request(fpin, request, MAX_RQ_LEN) == -1 )
			exit(1);
		printf("got a call: request = %s", request);

		process_rq(request, fpout);
		fflush(fpout);		/* send data to client	*/
		exit(0);		/* child is done	*/
					/* exit closes files	*/
	}
	/* parent: close fd and return to take next call	*/
	close(fd);
}

/*
 * read the http request into rq not to exceed rqlen
 * return -1 for error, 0 for success
 */
int read_request(FILE *fp, char rq[], int rqlen)
{
	/* null means EOF or error. Either way there is no request */
	if ( readline(rq, rqlen, fp) == NULL )
		return -1;
	read_til_crnl(fp);
	return 0;
}

void read_til_crnl(FILE *fp)
{
        char    buf[MAX_RQ_LEN];
        while( readline(buf,MAX_RQ_LEN,fp) != NULL 
			&& strcmp(buf,"\r\n") != 0 )
                ;
}

/*
 * readline -- read in a line from fp, stop at \n 
 *    args: buf - place to store line
 *          len - size of buffer
 *          fp  - input stream
 *    rets: NULL at EOF else the buffer
 *    note: will not overflow buffer, but will read until \n or EOF
 *          thus will lose data if line exceeds len-2 chars
 *    note: like fgets but will always read until \n even if it loses data
 */
char *readline(char *buf, int len, FILE *fp)
{
        int     space = len - 2;
        char    *cp = buf;
        int     c;

        while( ( c = getc(fp) ) != '\n' && c != EOF ){
                if ( space-- > 0 )
                        *cp++ = c;
        }
        if ( c == '\n' )
                *cp++ = c;
        *cp = '\0';
        return ( c == EOF && cp == buf ? NULL : buf );
}
/*
 * initialization function
 * 	1. process command line args
 *		handles -c configfile
 *	2. open config file
 *		read rootdir, port
 *	3. chdir to rootdir
 *	4. open a socket on port
 *	5. gets the hostname
 *	6. return the socket
 *       later, it might set up logfiles, check config files,
 *         arrange to handle signals
 *
 *  returns: socket as the return value
 *	     the host by writing it into host[]
 *	     the port by writing it into *portnump
 */
int startup(int ac, char *av[],char host[], int *portnump)
{
	int	sock;
	int	portnum     = PORTNUM;
	char	*configfile = CONFIG_FILE ;
	int	pos;
	void	process_config_file(char *, int *);

	for(pos=1;pos<ac;pos++){
		if ( strcmp(av[pos],"-c") == 0 ){
			if ( ++pos < ac )
				configfile = av[pos];
			else
				fatal("missing arg for -c",NULL);
		}
	}
	process_config_file(configfile, &portnum);
			
	sock = make_server_socket( portnum );
	if ( sock == -1 ) 
		oops("making socket",2);
	signal(SIGCHLD, SIG_IGN);		// no zombies
	strcpy(myhost, full_hostname());
	*portnump = portnum;
	return sock;
}


/*
 * opens file or dies
 * reads file for lines with the format
 *   port ###
 *   server_root path
 * at the end, return the portnum by loading *portnump
 * and chdir to the rootdir
 * (Assignment Part #3)
 */
void process_config_file(char *conf_file, int *portnump)
{
	FILE	*fp;
	char	rootdir[VALUE_LEN] = SERVER_ROOT;
	char	param[PARAM_LEN];
	char	value[VALUE_LEN];
	char	opt_val[OPT_VAL_LEN];	/* optional value content-type */
	int	port, num_args;	/* num_args stores output from read_param */
	void	update_typetable(char*, char*);
	int	read_param(FILE *, char *, int, char *, int, char*, int);

	/* open the file */
	if ( (fp = fopen(conf_file,"r")) == NULL )
		fatal("Cannot open config file %s", conf_file);

	/* extract the settings */
	while( (num_args=read_param(fp, param, PARAM_LEN, value, VALUE_LEN, 
			 opt_val, OPT_VAL_LEN)) != EOF)
	{
		if ( strcasecmp(param,"server_root") == 0 )
			strcpy(rootdir, value);
		if ( strcasecmp(param,"port") == 0 )
			port = atoi(value);
		if ( num_args == 3 && strcasecmp(param,"type") == 0 ){
			update_typetable(value, opt_val);
		}
	}
	fclose(fp);

	/* act on the settings */
	if (chdir(rootdir) == -1)
		oops("cannot change to rootdir", 2);
	*portnump = port;
	return;
}

/**
 * update_typetable:
 *	purpose: implements a table-driven system to associate  
 *		 content-type with file extensions read from the wsng.conf file
 *		 (Assignment Part #3)
 */
void update_typetable(char* extension, char* type)
{
	struct ec* new_entry;	
	new_entry = (struct ec*)malloc(sizeof(struct ec));
	if(new_entry == NULL)
		oops("Couldnt add to type table", 1);

	new_entry->ext = (char*)malloc(sizeof(char)*(strlen(extension)+1));

	new_entry->content = (char*)malloc(sizeof(char)*(strlen(type)+1));

	if(new_entry->content == NULL || new_entry->ext == NULL){
		free(new_entry->content);
		free(new_entry->ext);
		oops("Couldn't update type table", 2);
	}
	strcpy(new_entry->ext, extension);
	strcpy(new_entry->content, type);
	new_entry->next = type_table;
	type_table = new_entry;
}

/*
 * read_param:
 *   purpose -- read next parameter setting line from fp
 *   details -- a param-setting line looks like  name value opt_value
 *		for example:  port 4444
 *     extra -- skip over lines that start with # and those
 *		that do not contain either two or three strings
 *   returns -- EOF at eof and number of args scanned on good data
 *   (Assignment Part #2)
 */
int read_param(FILE *fp, char *name, int nlen, char* value, int vlen, char* opt, int olen)
{
	char	line[LINELEN];
	int	c, rv;
	char	fmt[100] ;

	sprintf(fmt, "%%%ds%%%ds%%%ds", nlen, vlen, olen);

	/* read in next line and if the line is too long, read until \n */
	while( fgets(line, LINELEN, fp) != NULL )
	{
		if ( line[strlen(line)-1] != '\n' )
			while( (c = getc(fp)) != '\n' && c != EOF )
				;
		rv = sscanf(line, fmt, name, value, opt);
		if( (rv == 2 || rv == 3) && *name != '#')
			return rv;
	}
	return EOF;
}
	


/* ------------------------------------------------------ *
   process_rq( char *rq, FILE *fpout)
   do what the request asks for and write reply to fp
   rq is HTTP commands:  GET  /foo/bar.html HTTP/1.0
			 HEAD /foo/bar.html HTTP/1.0
   (Assignment Part #2)
   ------------------------------------------------------ */

void process_rq(char *rq, FILE *fp)
{
	char	cmd[MAX_RQ_LEN], arg[MAX_RQ_LEN];
	char	*item, *modify_argument();
	int 	is_head = 0;	/* if true, flag causes output functions */
				/* to stop execution after sending outputing */
				/* the message head */
	if ( sscanf(rq, "%s%s", cmd, arg) != 2 ){
		bad_request(fp);
		return;
	}
	
	if(strcmp(cmd, "HEAD") == 0) is_head = 1;

	item = modify_argument(arg, MAX_RQ_LEN);
	if ( strcmp(cmd,"GET") != 0 && !is_head )
		cannot_do(fp);
	else if ( not_exist( item ) )
		do_404( item, fp, is_head );
	else if ( isadir( item ) ){
		if ( !index_used(item, fp, is_head ) )
			do_ls( item, fp, is_head );
	}
	else if ( ends_in_cgi( item ) )
		do_exec( item, fp, is_head );
	else
		do_cat( item, fp, is_head );
}

/*
 * modify_argument
 *  purpose: many roles
 *		security - remove all ".." components in paths
 *		cleaning - if arg is "/" convert to "."
 *  returns: pointer to modified string
 *     args: array containing arg and length of that array
 *  (Assignment Part #8) 
 */

char *
modify_argument(char *arg, int len)
{
	char	*nexttoken, *querystring;
	char	*copy = malloc(len);

	if ( copy == NULL )
		oops("memory error", 1);

	/* remove all ".." components from path */
	/* by tokeninzing on "/" and rebuilding */
	/* the string without the ".." items	*/

	*copy = '\0';

	querystring = strtok(arg, "?");
	/* if QUERY_STRING exists, add it to the environment and remove */
	/* it from arg by null terminating at the '?'  */
	/* (Assignment Part #8) */
	if( (querystring = strtok(NULL, "?")) != NULL){
		if( setenv("QUERY_STRING", querystring, 1) != 0)
			oops("QUERY_STRING not stored", 2);
		querystring[-1] = '\0';
	}
	nexttoken = strtok(arg, "/");
	while( nexttoken != NULL )
	{
		if ( strcmp(nexttoken,"..") != 0 )
		{
			if ( *copy )
				strcat(copy, "/");
			strcat(copy, nexttoken);
		}
		nexttoken = strtok(NULL, "/");
	}
	strcpy(arg, copy);
	free(copy);

	/* the array is now cleaned up */
	/* handle a special case       */

	if ( strcmp(arg,"") == 0 )
		strcpy(arg, ".");
	return arg;
}
/* ------------------------------------------------------ *
   the reply header thing: all functions need one
   if content_type is NULL then don't send content type
   ------------------------------------------------------ */

void
header( FILE *fp, int code, char *msg, char *content_type )
{
	/* (Assignment Part #1) */
	time_t current_time = time(NULL);	/* variable holds server time */
	fprintf(fp, "HTTP/1.0 %d %s\r\n", code, msg);
	fprintf(fp, "Date: %s\r\n", rfc822_time(current_time));
	fprintf(fp, "Server: myWSNG/0.1\r\n");
	if ( content_type )
		fprintf(fp, "Content-type: %s\r\n", content_type );
}

/* ------------------------------------------------------ *
   simple functions first:
	bad_request(fp)     bad request syntax
        cannot_do(fp)       unimplemented HTTP command
    and do_404(item,fp)     no such object
   ------------------------------------------------------ */

void
bad_request(FILE *fp)
{
	header(fp, 400, "Bad Request", "text/plain");
	fprintf(fp, "\r\nI cannot understand your request\r\n");
}

void
cannot_do(FILE *fp)
{
	header(fp, 501, "Not Implemented", "text/plain");
	fprintf(fp, "\r\n");

	fprintf(fp, "That command is not yet implemented\r\n");
}


void
do_404(char *item, FILE *fp, int is_head)
{
	header(fp, 404, "Not Found", "text/plain");
	fprintf(fp, "\r\n");

	if(is_head) return;

	fprintf(fp, "The item you requested: %s\r\nis not found\r\n", 
			item);
}

/* (Assignment Part #6) */
void 
do_403(char* msg, FILE *fp, int is_head)
{
	header(fp, 403, "Forbidden", "text/plain");
	fprintf(fp, "\r\n");
	fflush(fp);

	if(is_head) return;

	printf("403: Forbidden\r\n");
	perror(msg);
}

/* ------------------------------------------------------ *
   the directory listing section
   all functions use stat to get the appropriate info 
   index_used and do_ls also use the dirent struct to 
   to get info on the files in the directory
   (Assignments Part #2, Part #4, and Part #5)
   ------------------------------------------------------ */

int
isadir(char *f)
{
	struct stat info;
	return ( stat(f, &info) != -1 && S_ISDIR(info.st_mode) );
}

int
not_exist(char *f)
{
	struct stat info;

	return( stat(f,&info) == -1 && errno == ENOENT );
}
/**
 * index_used(dir, fp, is_head)
 *	purpose: Search for an index file in the named directory. If index.html
 *		 is found, it uses do_cat. If index.cgi is found
 *		 it uses do_exec.
 * 	returns: 1 if an index file is found and used, 0 otherwise
 */
int index_used(char* dir, FILE* fp, int is_head)
{
	int rv = 0;		/* return value */
	DIR * dir_ptr;		/* directory pointer */
	char* index_path;	/* string for holding index path */
	struct dirent *entry; 	/* hold a directory entry */

	if( (dir_ptr = opendir(dir)) == NULL)
		oops(dir, 1);

	while( (entry = readdir(dir_ptr)) != NULL){
		if( strcmp(entry->d_name, "index.html") == 0 ){
			index_path = (char*)malloc(sizeof(char)*(strlen(dir) +
				strlen("index.html")+2));
			sprintf(index_path, "%s/index.html", dir); 
			rv = 1;
			do_cat(index_path, fp, is_head);
		}
		else if( strcmp(entry->d_name, "index.cgi") == 0 ){
			index_path = (char*)malloc(sizeof(char)*(strlen(dir) +
				strlen("index.cgi")+2));
			sprintf(index_path, "%s/index.cgi", dir); 
			rv = 1;
			do_exec(index_path, fp, is_head);
		}
	}
 
	return rv;
}

/*
 * lists the directory named by 'dir' 
 * sends the listing to the stream at fp
 * skips hidden files
 */
void
do_ls(char *dir, FILE *fp, int is_head)
{
	int	fd;		/* file descriptor of stream */
	DIR	*dir_ptr;	/* the directory */
	struct dirent *entry; 	/* hold a directory entry */
	struct stat info;	/* directory entry info */

	fd = fileno(fp);
	dup2(fd,1);
	dup2(fd,2);

	if( (dir_ptr = opendir(dir)) == NULL)
		oops(dir, 1);
	if(chdir(dir) != 0){	/* know it exists, so throw error if fails */
		closedir(dir_ptr);
		do_403("Request refused", fp, is_head);
		return;
	}		
	header(fp, 200, "OK", "text/html");
	fprintf(fp,"\r\n");
	fflush(fp);

	if( is_head ) return;
	
	/* prints directory name at top of page */
	char* temp;
	if(strcmp(dir, ".") ==0)
		temp = "";
	else temp = dir;
	
	printf("<!DOCTYPE HTML>\n<html>\n <head>\n");
	printf("  <title>Index of /~geraldtrotman/wsng/%s</title>\n </head>", temp);
	printf(" <body>\n<h1>Index of /~geraldtrotman/wsng/%s</h1>\n<table>", temp);
	
	// while entries available show info for all but hidden files
	while( (entry = readdir(dir_ptr) ) != NULL ){
		if(entry->d_name[0] == '.') continue;
		if(stat(entry->d_name, &info) == -1) continue;

		printf("<tr><td><a href=\"");
		printf("%s",entry->d_name);
		if(S_ISDIR(info.st_mode)) /* print trailing '/' if dir */
			printf("/");
		printf("\">%s</a></td>", entry->d_name);
		printf("<td>%s</td>", rfc822_time(info.st_mtime));
		printf("<td>%ld</td></tr>\n", ((long)(info.st_size)));
	}	
	closedir(dir_ptr);
	printf("</table></body></html>\n");
}

/* ------------------------------------------------------ *
   the cgi stuff.  function to check extension and
   one to run the program.
   ------------------------------------------------------ */

char *
file_type(char *f)
/* returns 'extension' of file */
{
	char	*cp;
	if ( (cp = strrchr(f, '.' )) != NULL )
		return cp+1;
	return "";
}

int
ends_in_cgi(char *f)
{
	return ( strcmp( file_type(f), "cgi" ) == 0 );
}

void
do_exec( char *prog, FILE *fp, int is_head)
{
	int	fd = fileno(fp);

	header(fp, 200, "OK", NULL);
	fflush(fp);

	if(is_head) return;

	dup2(fd, 1);
	dup2(fd, 2);
	execl(prog,prog,NULL);
	perror(prog);
}
/* ------------------------------------------------------ *
   do_cat(filename,fp,is_head)
   sends back contents after a header
   if is_head is true, it stops after sending the header
   ------------------------------------------------------ */

void
do_cat(char *f, FILE *fpsock, int is_head)
{
	char	*extension = file_type(f);
	char	*content = "text/plain";
	FILE	*fpfile;
	int	c;

	struct ec *ptr = type_table;

	while(ptr != NULL){
		if(strcmp(extension, ptr->ext) == 0){
			content = ptr->content; break;
		}
		ptr = ptr->next;
	}

	fpfile = fopen( f , "r");
	if ( fpfile != NULL )
	{
		header( fpsock, 200, "OK", content );
		fprintf(fpsock, "\r\n");

		if(!is_head){
			while( (c = getc(fpfile) ) != EOF )
				putc(c, fpsock);
		}
		fclose(fpfile);
	}
}

char *
full_hostname()
/*
 * returns full `official' hostname for current machine
 * NOTE: this returns a ptr to a static buffer that is
 *       overwritten with each call. ( you know what to do.)
 */
{
	struct	hostent		*hp;
	char	hname[MAXHOSTNAMELEN];
	static  char fullname[MAXHOSTNAMELEN];

	if ( gethostname(hname,MAXHOSTNAMELEN) == -1 )	/* get rel name	*/
	{
		perror("gethostname");
		exit(1);
	}
	hp = gethostbyname( hname );		/* get info about host	*/
	if ( hp == NULL )			/*   or die		*/
		return NULL;
	strcpy( fullname, hp->h_name );		/* store foo.bar.com	*/
	return fullname;			/* and return it	*/
}


void fatal(char *fmt, char *str)
{
	fprintf(stderr, fmt, str);
	exit(1);
}

