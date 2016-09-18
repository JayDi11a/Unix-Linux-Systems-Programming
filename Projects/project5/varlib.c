/* varlib.c
 *
 * a simple storage system to store name=value pairs
 * with facility to mark items as part of the environment
 *
 * interface:
 *     VLstore( name, value )    returns 0 for OK, 1 for no
 *     VLlookup( name )          returns string or "" if not there
 *     VLlist()			 prints out current table
 *
 * environment-related functions
 *     VLexport( name )		 adds name to list of env vars
 *     VLtable2environ()	 copy from table to environ
 *     VLenviron2table()         copy from environ to table
 *
 * details:
 *	the table is stored as an array of structs that
 *	contain a flag for `global' and a single string of
 *	the form name=value.  This allows EZ addition to the
 *	environment.  It makes searching pretty easy, as
 *	long as you search for "name=" 
 *
 */

#include	<stdio.h>
#include	<stdlib.h>
#include	"varlib.h"
#include	<string.h>
#include	<ctype.h>
#include	"splitline.h"

#define	MAXVARS	200		/* a linked list would be nicer */

struct var {
		char *str;		/* name=val string	*/
		int  global;		/* a boolean		*/
	};

static struct var tab[MAXVARS];			/* the table	*/

static char *new_string( char *, char *);	/* private methods	*/
static struct var *find_item(char *, int);

char* execute_sub(char**, char*);
void unspecial(char*, char*);

/**
 * varsub(string)
 *	purpose: Subsitutes any variables present if the chars following '$'
 *		 represent a valid variable. Allows for escaped '$' to appear
 *		 preceded by a '\'
 *	inputs:  The string to substitute on
 *	returns: Either the same string or the modified string
 */
char* varsub(char **string)
{
	char *ptr;	
	for(ptr = *string; *ptr != '\0'; ptr++){
		if(*ptr == '\\')
			unspecial(*string, ptr);
		else if(*ptr == '$'){
			ptr = execute_sub(string, ptr);
		}
	}
	return *string;
}
/** 
 * unspecial( string, escape )
 *	purpose: Acknowledges and removes the escaped sequence in input string
 *	inputs:  String to operate on, and pointer to escape char
 */
void unspecial(char* string, char* escape)
{
	*escape = '\0';
	strcat(string, (escape +1));
}
/**
 * execute_sub(string, subString)
 *	purpose: Checks if var is on list and substitutes it. If var is not 
 *		 on the list, or is not a valid var name, it does nothing.
 *	inputs:  The string to work on
 *	returns: If substitution made, returns pointer to new string, otherwise
 *		 returns pointer to the same string
 */
char* execute_sub(char **fullString, char* subString)
{
	char tmp, *ptr = subString + 1,    // vars for tmp char, 
	     *val; 		// iteration ptr, and var value

	char *newString;
	
	*subString = '\0'; 	// start breaking full string into parts
	int length = strlen(*fullString);	// length of beginning 
	
	// iterate while valid var name chars are found
	for(; (*ptr != '\0' && (isalnum(*ptr) || *ptr=='_')) ; ptr++){
		if(isdigit(*ptr) != 0 && ptr == (subString + 1)){
			ptr++; break;	// if first char is numeric, stop
		}
	}
	tmp = *ptr;
	*ptr = '\0';
	val = VLlookup(subString + 1);
	*ptr = tmp;

	// substitute & put string back together
	newString = (char*)emalloc(sizeof(char) * 
			(length + strlen(ptr) + strlen(val) + 1));
	strcpy(newString, *fullString);
	strcat(newString, val);
	strcat(newString, ptr);
	free(*fullString);
	*fullString = newString;
	ptr = newString + length + strlen(val);

	// returning ptr - 1, to account for iteration in calling funcion
	return ptr - 1 ; 
}

int VLstore( char *name, char *val )
/*
 * traverse list, if found, replace it, else add at end
 * since there is no delete, a blank one is a free one
 * return 1 if trouble, 0 if ok (like a command)
 */
{
	struct var *itemp;
	char	*s;
	int	rv = 1;

	/* find spot to put it              and make new string */
	if ((itemp=find_item(name,1))!=NULL && (s=new_string(name,val))!=NULL) 
	{
		if ( itemp->str )		/* has a val?	*/
			free(itemp->str);	/* y: remove it	*/
		itemp->str = s;
		rv = 0;				/* ok! */
	}
	return rv;
}

char * new_string( char *name, char *val )
/*
 * returns new string of form name=value or NULL on error
 */
{
	char	*retval;

	retval = malloc( strlen(name) + strlen(val) + 2 );
	if ( retval != NULL )
		sprintf(retval, "%s=%s", name, val );
	return retval;
}

char * VLlookup( char *name )
/*
 * returns value of var or empty string if not there
 */
{
	struct var *itemp;

	if ( (itemp = find_item(name,0)) != NULL )
		return itemp->str + 1 + strlen(name);
	return "";

}

int VLexport( char *name )
/*
 * marks a var for export, adds it if not there
 * returns 1 for no, 0 for ok
 */
{
	struct var *itemp;
	int	rv = 1;

	if ( (itemp = find_item(name,0)) != NULL ){
		itemp->global = 1;
		rv = 0;
	}
	else if ( VLstore(name, "") == 0 )
		rv = VLexport(name);
	return rv;
}

static struct var * find_item( char *name , int first_blank )
/*
 * searches table for an item
 * returns ptr to struct or NULL if not found
 * OR if (first_blank) then ptr to first blank one
 */
{
	int	i;
	int	len = strlen(name);
	char	*s;

	for( i = 0 ; i<MAXVARS && tab[i].str != NULL ; i++ )
	{
		s = tab[i].str;
		if ( strncmp(s,name,len) == 0 && s[len] == '=' ){
			return &tab[i];
		}
	}
	if ( i < MAXVARS && first_blank )
		return &tab[i];
	return NULL;
}


void VLlist()
/*
 * performs the shell's  `set'  command
 * Lists the contents of the variable table, marking each
 * exported variable with the symbol  '*' 
 */
{
	int	i;
	for(i = 0 ; i<MAXVARS && tab[i].str != NULL ; i++ )
	{
		if ( tab[i].global )
			printf("  * %s\n", tab[i].str);
		else
			printf("    %s\n", tab[i].str);
	}
}

int VLenviron2table(char *env[])
/*
 * initialize the variable table by loading array of strings
 * return 1 for ok, 0 for not ok
 */
{
	int     i;
	char	*newstring;

	for(i = 0 ; env[i] != NULL ; i++ )
	{
		if ( i == MAXVARS )
			return 0;
		newstring = malloc(1+strlen(env[i]));
		if ( newstring == NULL )
			return 0;
		strcpy(newstring, env[i]);
		tab[i].str = newstring;
		tab[i].global = 1;
	}
	while( i < MAXVARS ){		/* I know we don't need this	*/
		tab[i].str = NULL ;	/* static globals are nulled	*/
		tab[i++].global = 0;	/* by default			*/
	}
	return 1;
}

char ** VLtable2environ()
/*
 * build an array of pointers suitable for making a new environment
 * note, you need to free() this when done to avoid memory leaks
 */
{
	int	i,			/* index			*/
		j,			/* another index		*/
		n = 0;			/* counter			*/
	char	**envtab;		/* array of pointers		*/

	/*
	 * first, count the number of global variables
	 */

	for( i = 0 ; i<MAXVARS && tab[i].str != NULL ; i++ )
		if ( tab[i].global == 1 )
			n++;

	/* then, allocate space for that many variables	*/
	envtab = (char **) malloc( (n+1) * sizeof(char *) );
	if ( envtab == NULL )
		return NULL;

	/* then, load the array with pointers		*/
	for(i = 0, j = 0 ; i<MAXVARS && tab[i].str != NULL ; i++ )
		if ( tab[i].global == 1 )
			envtab[j++] = tab[i].str;
	envtab[j] = NULL;
	return envtab;
}

