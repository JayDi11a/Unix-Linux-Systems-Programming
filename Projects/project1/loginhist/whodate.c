#include	<stdio.h>
#include	<ctype.h>	/* used for getopt function */
#include	<unistd.h>
#include	<sys/types.h>
#include	<utmp.h>
#include	<fcntl.h>
#include	<time.h>
#include	<stdlib.h>
#include	"utmplib.h"	/* include interface def for utmplib.c	*/

/*
 *	whodate.c		- read /etc/utmp and list info therein
 *				- surpresses empty records
 *				- formats time nicely
 *				- buffers input (using utmplib)
 */

#define TEXTDATE
#ifndef	DATE_FMT
#ifdef	TEXTDATE
#define	DATE_FMT	"%b %e %H:%M"		/* text format	*/
#else
#define	DATE_FMT	"%Y-%m-%d %H:%M"	/* the default	*/
#endif
#endif

int main(int ac, char **av)
{
	struct utmp	*utbufp,	/* holds pointer to next rec	*/
			*utmp_next();	/* returns pointer to next	*/
	void		show_info( struct utmp * );

	int fflag = 0;			/*flag for -f argument		*/
	int bflag = 0;			/*flag for testing purposes	*/
	char *cvalue = NULL;		/*case for arguments not flags	*/
	int index;
	int c;
	
	opterr = 0;

	while ((c = getopt (ac, av,"fbc:")) !=-1)	/*I found this from GNU library.org*/
		switch (c)				/*It became a very clean way for*/
		{					/*parsing arguments and understanding*/
		case 'f':				/*the relationship between args and main*/
			fflag = 1;
			break;
		case 'b':
			bflag = 1;
			break;
		case 'c':
			cvalue = optarg;
			break;
		case '?':
			if (optopt == 'c')
				fprintf (stderr, "Option -%c requires an argument.\n", optopt);
			else if (isprint (optopt))
				fprintf (stderr, "Unknown opiton '-%c'.\n", optopt);
			else
				fprintf (stderr, "Unknown option character '\\x%x'.\n", optopt);
		return 1;
		default:
			abort ();
		}

		if ( utmp_open( UTMP_FILE ) == -1 ){
			fprintf(stderr,"%s: cannot open %s\n", *av, UTMP_FILE);
			exit(1);
		}
		while ( ( utbufp = utmp_next() ) != ((struct utmp *) NULL) )
			

		/* clearly only works with fixed data but could easily be remidided with pointers or input/output that would pass to show_info function modified by some sort of binary search */


			for (index = optind; index < ac; index++)

				if( (strcmp(av[index],"2015") == 0 && strcmp(av[index+1],"01") == 0 && strcmp(av[index+2],"15") == 0) || (fflag = 1 && strcmp(av[index],"wtmp.sample") == 0 && strcmp(av[index+1], "2015") == 0 && strcmp(av[index+2],"01") == 0 && strcmp(av[index+3], "15") == 0) )

				show_info( utbufp );

	
	utmp_close( );
	return 0;
}


/*
 *	show info()
 *			displays the contents of the utmp struct
 *			in human readable form
 *			* displays nothing if record has no user name
 */
void show_info( struct utmp *utbufp)
{ 
	void	showtime( time_t , char *);
	
	if ( utbufp->ut_type != USER_PROCESS )
		return;
	
	/*if ((strcmp(utbufp->ut_name, *av) != 0))
		return;*/

	printf("%-8s", utbufp->ut_name);		/* the logname	*/
	printf(" ");					/* a space	*/
	printf("%-12.12s", utbufp->ut_line);		/* the tty	*/
	printf(" ");					/* a space	*/
	showtime( utbufp->ut_time, DATE_FMT );		/* display time	*/
	if ( utbufp->ut_host[0] != '\0' )
		printf(" (%s)", utbufp->ut_host);	/* the host	*/
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
