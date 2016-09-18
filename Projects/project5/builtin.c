/* builtin.c
 * contains the switch and the functions for builtin commands
 */

#include	<stdio.h>
#include	<string.h>
#include	<ctype.h>
#include	<unistd.h>
#include	<stdlib.h>
#include	"smsh.h"
#include	"varlib.h"

int assign(char *);
int okname(char *);
int execute_read(char **);
int execute_cd(char **);
int execute_exit(char **);

int builtin_command(char **args, int *resultp)
/*
 * purpose: run a builtin command 
 * returns: 1 if args[0] is builtin, 0 if not
 * details: test args[0] against all known builtins.  Call functions
 */
{
	int rv = 0;

	if (**args == '#'){ 	/* ignore comments */
		rv = 1;
		*resultp = 0;
	}
	else if ( strcmp(args[0],"set") == 0 ){	     /* 'set' command? */
		VLlist();
		*resultp = 0;
		rv = 1;
	}
	else if ( strchr(args[0], '=') != NULL ){   /* assignment cmd */
		*resultp = assign(args[0]);
		if ( *resultp != -1 )		    /* x-y=123 not ok */
			rv = 1;
	}
	else if ( strcmp(args[0], "export") == 0 ){
		if ( args[1] != NULL && okname(args[1]) )
			*resultp = VLexport(args[1]);
		else
			*resultp = 1;
		rv = 1;
	}
	else if(strcmp(args[0],"cd") == 0){
		rv = 1;
		*resultp = execute_cd(args + 1);
	}
	else if(strcmp(args[0], "exit") == 0){
		rv = 1;
		*resultp = execute_exit(args + 1);
	}
	else if(strcmp(args[0], "read") == 0){
		rv = 1;
		*resultp = execute_read(args + 1);
	}
	return rv;
}

int assign(char *str)
/*
 * purpose: execute name=val AND ensure that name is legal
 * returns: -1 for illegal lval, or result of VLstore 
 * warning: modifies the string, but retores it to normal
 */
{
	char	*cp;
	int	rv ;

	cp = strchr(str,'=');
	*cp = '\0';
	rv = ( okname(str) ? VLstore(str,cp+1) : -1 );
	*cp = '=';
	return rv;
}
int okname(char *str)
/*
 * purpose: determines if a string is a legal variable name
 * returns: 0 for no, 1 for yes
 */
{
	char	*cp;

	for(cp = str; *cp; cp++ ){
		if ( (isdigit((int)*cp) && cp==str) || !(isalnum((int)*cp) || *cp=='_' ))
			return 0;
	}
	return ( cp != str );	/* no empty strings, either */
}

int oknamechar(char c, int pos)
/*
 * purpose: used to determine if a char is ok for a name
 */
{
	return ( (isalpha((int)c) || (c=='_' ) ) || ( pos>0 && isdigit((int)c) ) );
}


/**
 * execute_read( get_var )
 *	purpose: Reads one line of input from stdin to the input variable 
 *		 get_var using next_cmd. It then uses varlib to store the 
 *		 variable as a local variable.
 *	inputs:  A pointer to a string holding the name of the var to store
 *	returns: 0 if successful, -1 otherwise
 */
int execute_read(char** args)
{
	if(args[0] == NULL || args[1] != NULL){ /* ensure only 1 arg to read */
		fprintf(stderr, "Error: read takes 1 arg");
		return -1;
	}
	char *tmp;	// buffer to hold string;
	int  rv, check ;
	tmp = next_cmd("", stdin);

	check = okname(args[0]);
	if (!check){
		fprintf(stderr, "Command not recognized check spelling\n");
		rv = -1;
	}
	else rv = VLstore(args[0],tmp);
	return rv;
}


/**
 * execute_cd(args)
 *	purpose: checks number of args and then attempts to change to the 
 *		 specified directory
 *	inputs:  A pointer to a string that holds the args to cd. Should either be
 *	         empty or contain one string
 *	returns: 0 on success, -1 otherwise
 */
int execute_cd(char** args)
{
	int   rv = 0; 

	if(args[1] != NULL){ /* ensure max 1 arg to cd */
		fprintf(stderr, "Error: cd takes max 1 arg");
		return -1;
	}
	if(args[0] == NULL){
		char* home;
		home = VLlookup("HOME");
		rv = chdir( home );
	}
	else 
		rv = chdir(args[0]);
	if(rv == -1)
		fprintf(stderr, "%s not found\n", args[0]);
	return rv;
}

/**
 * execute_exit(args)
 *	purpose: Exits the shell with the exit code specified in args. If 
 *		 alphabetic char or string is supplied versus a number
 *		 for the exit status, an error is thrown.
 *	returns: -1 for error, nothing otherwise.
 */
int execute_exit(char** args)
{
	int i, status = 0; 	// exit status
	if ( args[1] != NULL ){
		fprintf(stderr, "Error: exit has 1 arg max");
		return -1;
	}
	if ( args[0] != NULL){
		for(i=0; args[0][i] != '\0'; i++){
			if( isdigit(args[0][i]) != 0 )
				status = (status * 10) + args[0][i] - '0';
			else
				return -1;
		}
	}
	exit(status);
	return -1;
}



