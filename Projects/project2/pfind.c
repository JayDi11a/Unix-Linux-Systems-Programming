#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fnmatch.h>	// for fnmatch(const char*, const char*, int)
#include <stdlib.h>
#include <dirent.h>
#include <string.h>

#define MAX_LEN	100	// MAX SEARCH STRING LENGTH
#define MAX_ARGS 6

void user_error(char*);
void check_args(int,char**,char*,char**,char*);
void searchdir(char*,char*,char);
int process_ops(char**,char*,char*,char**);
int directory_check(char*);
int type_check(char*, char entry);

/**
 *	pfind - This is a pseudo version of the find utility. pfind supports
 *		an initial directory, an initial directory and a name flag,
 *		an initial directory and name flag combined with a wild card and type flag, 
 *		or a file argument.
 *
 */
int main(int argc, char** argv){
	char type=0, *name=NULL, dir[BUFSIZ]="\0";
	check_args(argc, argv, &type, &name, dir);
	
	if( dir[0] == '\0'){
		user_error("Unexpected argument format");
	}
	
	// open dir, check inodes, compare to selected field to search string
	searchdir(dir, name, type);

	return 0;
}

void user_error(char *string)
/**
 * user_error(string)
 *	purpose: used for error handling turning return values into human readable errors 
 *
 */

{
	fprintf(stderr,"Error: %s\n", string);
	exit(1);
}

void check_args(int ac, char** av, char* type, char** name, char* dir)
/**
 * check_args(ac, av, type, name)
 *	purpose: Function checks the arguments to main are as expected and
 *		 parses args assigning pointers to the appropriate args
 *	actions: Sets pointers to strings representing name, type and starting 
 *	         directory
 *	returns: pointer to directory string on success, NULL otherwise
 */
{
	if( ac == 2 )				// if only 2 args
		strncpy(dir,av[1],(BUFSIZ-1));	// use argument given as dir
	else if( ac <= MAX_ARGS ){
		if(process_ops(av,dir,type,name) != 0){ // if problems, error
			user_error("Bad argument format");
		};
	}
}


int process_ops(char** av, char* dir, char *type, char** name)
/**
 * process_ops(av, dir, type, name)
 *	purpose: Processes arguments from check_args found in the array of
 *	         strings av. Dir is set to point to the starting directory, 
 *	         name is pointed to the search pattern and type to the type.
 *	returns: 0 on success, -1 on error
 */
{
	char **ptr = av + 1; 	// start with 1st arg to function
	int nameFlag=0, dirFlag=0, typeFlag=0, dupl=0;

	while(*ptr != NULL){	// terminate when args done
		if(**ptr == '-'){
			if(strcmp((*ptr+1),"name") == 0){ // if name option
				*name = *(++ptr);	  // assign next arg
				nameFlag==0 ? (nameFlag=1) : (dupl=1);
			} else if(strcmp((*ptr+1),"type") == 0){ // if type
				// assign next arg or null char to type
				//*(++ptr) ? (*type=**(ptr)) : (*type='\0');
				*(ptr) ? (*type=**(ptr)) : (*type='\0');
				typeFlag==0 ? (typeFlag=1) : (dupl=1);
				printf("%s is printing out\n", *(++ptr));
			} else // if bad option return
				user_error("Unrecognized option");
		} else if(**ptr != '-'){		// if not an option
			strncpy(dir,*ptr,(BUFSIZ-1));	// arg is the start dir
			dirFlag==0 ? (dirFlag=1) : (dupl=1);
		}
		ptr++;
	}
	if(!dirFlag)
		user_error("Bad arguments; no start directory");
	else if(dupl)
		user_error("Syntax error; duplicate criterion");
	return 0;
}

void searchdir(char* dirName, char* findMe, char type)
/**
 * searchdir(dirName, findMe, type)
 *	purpose: Searches a directory dirName and its sub-directories for files
 *		 that meet the criteria specified by findMe and type
 */
{
	char* newPath;
	DIR* dirPtr = NULL;	
	struct dirent *direntp;	  // placeholder for files in directory
	if( (dirPtr=opendir(dirName)) == NULL ){   // if it doesnt open
		fprintf(stderr,"Cannot open %s\n", dirName); // print error
		return;
	}
	while( (direntp = readdir(dirPtr)) != NULL){
		if(direntp->d_name[0] == '.')
			continue; // ignore hidden files
		// path for new directory
		newPath = (char*)malloc( sizeof(char) * (strlen(dirName)
					+ strlen(direntp->d_name)+2));
		sprintf(newPath,"%s/%s", dirName,direntp->d_name);

		/*if((type_check(newPath, (char)**)) == 0){
			searchdir(newPath, findMe, type);
		}*/

		if(directory_check(newPath)==0){ //  if directory

			// build new path with directory name
			searchdir(newPath, findMe, type); // recurse
		}

		if(findMe!=NULL && fnmatch(findMe,direntp->d_name,
		    (FNM_PATHNAME|FNM_PERIOD))!=0){
			continue; // if pattern given and doesnt match 
		}else if(type != '\0' && type != direntp->d_type)
			continue; // if type given and doesnt match, skip
		printf("%s/%s\n",dirName,direntp->d_name);
	}
	free(newPath);
	closedir(dirPtr);
}

int directory_check(char* dir)
/**
 * directory_check(dir)
 * 	purpose: checks to see if the named file is a directory
 *	returns: 0 if directory, non-zero otherwise
 */
{
	struct stat info;
	if(lstat(dir, &info) == -1){
		perror(dir);
		exit(2);
	}
	// check if it is a directory
	if( S_ISDIR(info.st_mode))
		return 	0;
	return -1;
}


int type_check(char* dir, char entry)
/**
 * type_check(dir, entry)
 *	purpose: check to see if named file is everything else based on type
 *	returns: 0 if a match is found, non-zero otherwise
 */
{
	struct stat info;
	if(lstat(dir, &info) == -1){
		perror(dir);
		exit(2);
	}


	if( (S_ISREG(info.st_mode)) && (entry = 'f'))
		return 0;
	else if( (S_ISBLK(info.st_mode)) && (entry = 'b'))
		return 0;
	else if( (S_ISCHR(info.st_mode)) && (entry = 'c'))
		return 0;
	else if( (S_ISFIFO(info.st_mode)) && (entry = 'p'))
		return 0;
	else if( (S_ISLNK(info.st_mode)) && (entry = 'l'))
		return 0;
	else if( (S_ISSOCK(info.st_mode)) && (entry = 's'))
		return 0;
	return -1; 


}


