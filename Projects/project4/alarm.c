#include	<sys/time.h>
#include	<signal.h>
#include	<errno.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>

/*
 *	alarm.c	
 *			timer functions for higher resolution clock
 *
 *			set_ticker( number_of_milliseconds )
 *				arranges for the interval timer to issue
 *				SIGALRM's at regular intervals
 *			millisleep( number_of_milliseconds )
 *
 * 
 */

static void my_handler(int);


/**
 * set_ticker(n_msecs)
 *	purpose: Function aranges for the interval timer to issue SIGALARM's at 
 *		 regular intervals.
 *	input:   Function takes the argument in milliseconds and converts it 
 *		 into micro seconds.
 *	returns: Returns negative one on error and zero if there was no error.
 */
int set_ticker( int n_msecs )
{
	struct itimerval new_timeset, old_timeset;
	long	n_sec, n_usecs;

	n_sec = n_msecs / 1000 ;
	n_usecs = ( n_msecs % 1000 ) * 1000L ;

	new_timeset.it_interval.tv_sec  = n_sec;	/* set reload  */
	new_timeset.it_interval.tv_usec = n_usecs;	/* new ticker value */
	new_timeset.it_value.tv_sec     = n_sec  ;	/* store this	*/
	new_timeset.it_value.tv_usec    = n_usecs ;	/* and this 	*/

	if ( setitimer( ITIMER_REAL, &new_timeset, &old_timeset ) != 0 ){
		printf("Error with timer..errno=%d\n", errno );
		return -1;
	}
	return 0;
}

/**
 * millisleep(n)
 *	purpose: Function used to suspend action upon being given a handler
 *		 and applying a signal to a timer.
 *	input:   Function takes in an integer that represents the amount of time
 *		 to set the alarm.
 */
void millisleep( int n )
{
	signal( SIGALRM , my_handler);		/* set handler		*/
	set_ticker( n );			/* set alarm timer	*/
	pause();				/* wait for sigalrm	*/
}

/**
 * my_handler(s)
 *	purpose: Function used to wrap another function that is to be assigned to
 *		 a signal alarm.
 *	input:	 Function takes in an integer argument that set_ticker function 
 *		 used to set an amount of time.
 */
static void my_handler(int s)
{
	set_ticker( 0 );			/* turns off ticker */
}

