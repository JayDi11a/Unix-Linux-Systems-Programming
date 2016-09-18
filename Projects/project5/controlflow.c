/* controlflow.c
 *
 * "if" processing is done with two state variables
 *    if_state and if_result
 */
#include	<stdio.h>
#include	<stdlib.h>
#include	"smsh.h"
#include	"string.h"
#include	"splitline.h"
#include 	"varlib.h"

enum states   { NEUTRAL, WANT_THEN, THEN_BLOCK, ELSE_BLOCK };
enum results  { SUCCESS, FAIL };
enum keywords { NONE, K_IF, K_THEN, K_ELSE, K_FI };
struct kw { char *str; int tok; };
struct cb { int state; int result; struct cb* outer; };

struct kw words[] = { {"if", K_IF}, {"then", K_THEN}, {"else", K_ELSE }, 
			{"fi", K_FI}, {NULL,0} };

static struct cb* flow = NULL;	// linked list of control flow blocks

int is_a_control_word(char *s);

int	syn_err(char *);

/*
 * purpose: determine the shell should execute a command
 * returns: 1 for yes, 0 for no
 * details: if in THEN_BLOCK and result was SUCCESS then yes
 *          if in THEN_BLOCK and result was FAIL    then no
 *          if in ELSE_BLOCK and result was SUCCESS then no
 *          if in ELSE_BLOCK and result was FAIL    then yes
 *          if in WANT_THEN  then syntax error (sh is different)
 */
int ok_to_execute()
{
	int	rv = 1;		/* default is positive */
	if(flow == NULL) return rv;

	if ( flow->state == WANT_THEN ){
		syn_err("then expected");
		rv = 0;
	}
	else if ( flow->state == THEN_BLOCK && flow->result == SUCCESS )
		rv = 1;
	else if ( flow->state == THEN_BLOCK && flow->result == FAIL )
		rv = 0;
	else if (flow->state == ELSE_BLOCK && flow->result == SUCCESS)
		rv = 0;
	else if (flow->state == ELSE_BLOCK && flow->result == FAIL)
		rv = 1;
	return rv;
}

/*
 * purpose: boolean to report if the command is a shell control command
 * returns: 0 or 1
 */
int is_control_command(char *s)
{
	if(*s == '\0' && flow != NULL)
		return syn_err("unexpected EOF during control flow");
	
	return ( is_a_control_word(s) != NONE );
}

int is_a_control_word(char *s)
{
	int	i;

	for(i=0; words[i].str != NULL ; i++ )
	{
		if ( strcmp(s, words[i].str) == 0 )
			return words[i].tok;
	}
	return NONE;
}

/*
 * purpose: Process "if", "then", "else", "fi" - change state or detect error
 * returns: 0 if ok, -1 for syntax error
 * effects: Potentially adds/removes a control flow block from the linked list
 *		of flow blocks.
 *   notes: I would have put returns all over the place, Barry says "no"
 */
int do_control_command(char **args)
{
	char	*cmd = args[0];
	int	status, rv = -1;

	if( strcmp(cmd,"if")==0 ){	// add new block to flow list	
		struct cb* newBlock = (struct cb*)emalloc(sizeof(struct cb));
		newBlock->outer = flow;
		flow = newBlock;

		status = process(args+1);
		flow->result = (status == 0 ? SUCCESS : FAIL );
		flow->state = WANT_THEN;
		rv = 0;
		
	}
	else if ( strcmp(cmd,"then")==0 ){
		if ( !flow || flow->state != WANT_THEN )
			rv = syn_err("then unexpected");
		else {
			flow->state = THEN_BLOCK;
			rv = 0;
		}
	}
	else if ( strcmp(cmd,"else")==0){
		if ( !flow || flow->state != THEN_BLOCK )
			rv = syn_err("else unexpected");
		else {
			flow->state = ELSE_BLOCK;
			rv = 0;
		}
	}
	else if ( strcmp(cmd,"fi")==0 ){
		if ( !flow || ( flow->state != THEN_BLOCK && 
			flow->state != ELSE_BLOCK ))
			rv = syn_err("fi unexpected");
		else {		// return to outer control flow
			struct cb* tmp = flow;
			flow = flow->outer;
			free(tmp);
			rv = 0;
		}
	}
	else 
		fatal("internal error processing:", cmd, 2);
	return rv;
}

/* purpose: handles syntax errors in control structures
 * details: resets state to NEUTRAL
 * returns: -1 in interactive mode. Should call fatal in scripts
 */
int syn_err(char *msg)
{
	fprintf(stderr,"syntax error: %s\n", msg);
	return -1;
}

