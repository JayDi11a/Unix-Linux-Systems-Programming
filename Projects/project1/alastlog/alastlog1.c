#include	<stdio.h>
#include	<sys/types.h>
#include	<utmp.h>
#include	<fcntl.h>
#include	<time.h>
#include	<stdlib.h>
#include	"utmplib.h"	/* include interface def for utmplib.c	*/

/*
 *	alastlog 1.0		- read  and list info therein
 *				- surpresses empty records
 *				- formats time nicely
 *				- buffers input (using utmplib)
 */

#define MAX_FNAME_LEN	64	/* max filename length */
#define TEXTDATE
#ifndef	DATE_FMT
#ifdef	TEXTDATE
#define	DATE_FMT	"%a %b %e %H:%M:%S %z %Y"		/* text format	*/
#else
#define	DATE_FMT	"%Y-%m-%d %H:%M"	/* the default	*/
#endif
#endif

int main(int ac, char** av) {
	struct utmp	*utbufp[2],				/* holds pointer to next rec	*/
			*utmp_next(),				/* returns pointer to next	*/
			*compare_info(struct utmp**, char*);	/* returns ptr to newest	*/
	void		show_info( struct utmp * );
	
	int check, check_args(int, char**, char*);		/* checks args & opens utmp */

	char *user = (char *)malloc(sizeof(char)*2*UT_NAMESIZE);

	if ((check = check_args(ac, av, user)) != 0) {
		printf("Cannot open file \n");
		return 1;
	}

	if ((utbufp[0] = utmp_next() ) == ((struct utmp *) NULL)) {
		utmp_close();
		return 0;
	}


	/*if ( utmp_open( UTMP_FILE ) == -1 ){
	//	fprintf(stderr,"%s: cannot open %s\n", av, UTMP_FILE);
		printf("Cannot open file \n");
		exit(1);
	}*/
	
	printf("%-16.16s %-5s %6.16s %18.16s\n", "Username", "Port", "From", "Latest");
	while ( ( utbufp[1] = utmp_next() ) != ((struct utmp *) NULL) )
		*utbufp = compare_info(utbufp, user);
	show_info( utbufp );

	fprintf(stdout, "%s user \n", user);
	utmp_close( );
	return 0;
}
/*
 *	show info()
 *			displays the contents of the utmp struct
 *			in human readable form
 *			* displays nothing if record has no user name
 */
void show_info( struct utmp *utbufp ) {
	void	showtime( time_t , char *);

	if ( utbufp->ut_type != USER_PROCESS )
		return;

	printf("%-16.16s", utbufp->ut_name);		/* the logname	*/
	printf(" ");					/* a space 	*/
	printf("%-5.6s", utbufp->ut_line);		/* the tty	*/
	printf("  ");					/* a space	*/
	if ( utbufp->ut_host[0] != '\0' )
		printf(" %6.16s", utbufp->ut_host);	/* the host	*/
	printf(" ");					/* a space 	*/
	showtime( utbufp->ut_time, DATE_FMT );		/* display time	*/
	printf("\n");					/* newline	*/
}

#define	MAXDATELEN	100

void showtime( time_t timeval , char *fmt )
/*
 * displays time in a format fit for human consumption.
 * Uses localtime to convert the timeval into a struct of elements
 * (see localtime(3)) and uses strftime to format the data
 */
{
	char	result[MAXDATELEN];

	struct tm *tp = localtime(&timeval);		/* convert time	*/
	strftime(result, MAXDATELEN, fmt, tp);		/* format it	*/
	fputs(result, stdout);
}

int check_args(int ac, char** av, char* user)
/*
 *
 *
 */
{
	int check = -1, process_ops(char **, char *, char *);	 /* process options */
	char *file = (char *)malloc(sizeof(char)*MAX_FNAME_LEN); /* filename */
	
	/*if( ac == 0 ){         					 * use default utmp file */ 				/* set the user equal to the argument given */
           if( ac == 4 ){
                if(process_ops(av, user, file) != 0){ 		/* if problems, error */
                        fprintf(stderr, "%s: bad argument format", *av);
                        return -1;
                }
                // check that f option is selected or output error message
                // check that the string after -f is a filename that opens
                check = 0;
        }
        fprintf(stdout, "%s confirmed as user?\n", user);
        if( check == 0 && utmp_open( file ) == -1){
                fprintf(stderr, "%s: cannot open %s\n",
                        *av, file);
                return -1;
        }
        free(file);
        return check;
}

int process_ops(char** av, char* user, char* file)
/*
 *
 *
 *
 */
{
	#define NUM_ARGS 4

        char **ptr = av + 1;    					/* start with 1st arg to function */
        int opt_flag = 0, user_flag = 0;
        while(ptr < (av + NUM_ARGS)){   				/* terminate when args done */
                if(**ptr == '-' && *(*ptr+1) != 'f'){ 			/* if bad option */
                        fprintf(stderr, "%s: unrecognized option", *av);
                        return -1;
                } else if(**ptr != '-'){        			/* if not an option */
                        strcpy(user, *ptr);     			/* arg is the user string */
                        user_flag = 1;
                        fprintf(stdout,"%s assigned as user\n", user);
                } else if(**ptr =='-' && ptr< (av + NUM_ARGS -1)){  	/* option */
                        opt_flag = 1;
                        strcpy(file, *(++ptr)); 			/* file name is arg after option */
                }
                ptr++;
        }
        if(!opt_flag || !user_flag){
                fprintf(stderr,
                        "%s: bad arguments; no option and/or username supplied",
                        *av);
                return -1;
        }
        return 0;
}

struct utmp * compare_info(struct utmp** utbufp, char* user) 
/*
 *
 *
 *
 */
{
	struct utmp *temp1 = *utbufp,
		    *temp2 = *(utbufp + 1);
	char tempName [2*UT_NAMESIZE];
	strcpy(tempName, temp1->ut_name);
        if(temp1 && temp1->ut_name){
                if(strcmp(user, tempName) != 0)         		/* if user match */
                        temp1 = NULL;                   		/* else NULL	 */
        }else{
                temp1 = NULL;
        }
        strcpy(tempName, temp2->ut_name);
        if(temp2 && temp2->ut_name){
                if(strcmp(temp2->ut_name, user) != 0)
                        temp2 = NULL;
        }else{
                temp2 = NULL;
        }// temp1 only NULL if temp2 is also
        if((temp1 == NULL) || (temp1 && temp2 &&
                (temp1->ut_time < temp2->ut_time)))

                temp1 = temp2;  					/* temp1 gets temp2 if temp2 is newer */
        return temp1;
}

 
	

