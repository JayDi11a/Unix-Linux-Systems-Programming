#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<signal.h>
#include	<sys/wait.h>
#include	"smsh.h"
#include	"splitline.h"
#include	"varlib.h"

/**
 **	small-shell 
 **		first really useful version after prompting shell
 **		this one parses the command line into strings
 **		uses fork, exec, wait, and ignores signals
 **		It also uses the flexstr library to handle
 **		a the command line and list of args
 **/

#define	DFL_PROMPT	"> "

int main(int argc, char** argv)
{
	char	*cmdline, *prompt, **arglist;
	int	process(char **);
	void	setup();

	prompt = DFL_PROMPT ;
	setup();
	
	FILE* input = stdin;

	if(argc==2){
		prompt ="";

		if( (input = fopen(argv[1], "r")) == NULL){
			fprintf(stderr,"Failed to open file %s", argv[1]);
			return 1;
		}
	}
	if(argc > 2){
		fprintf(stderr, "One arg max permited to smsh");
		return 1;
	}
	while ( (cmdline = next_cmd(prompt, input)) != NULL )
	{
		cmdline = varsub( &cmdline );
		if ( (arglist = splitline(cmdline)) != NULL  ){

			process(arglist);

			freelist(arglist);
		}
		free(cmdline);
	}
	return 0;
}

void setup()
/*
 * purpose: initialize shell
 * returns: nothing. calls fatal() if trouble
 */
{
	extern char **environ;

	VLstore("SCRIPT","no");

	VLenviron2table(environ);
	signal(SIGINT,  SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
}

void fatal(char *s1, char *s2, int n)
{
	fprintf(stderr,"Error: %s,%s\n", s1, s2);
	exit(n);
}

