#include	<stdio.h>
#include	<signal.h>
#include	<unistd.h>
#include	<stdlib.h>
#include	<string.h>
#include	"pong.h"
#include	"paddle.h"
#include	"alarm.h"

/*
 *	pong.c	
 *
 *	bounces a character inside three boundaries and uses
 *	the paddle to keep it from going out of bounds
 *
 *	user input:
 *		 	k: move paddle up 
 *			m: move paddle down
 *		shift + q: quit
 *
 *	The program blocks on read, but the timer tick sets SIGALRM which gets caught
 *	by the ball_move function.
 */

static struct ppball the_ball ; 	// the ball
static struct ptime  timer;		// the clock
int balls_left = NUM_EXTRA_BALLS;

int  set_up();
void wrap_up();
void ball_move(int);
void update_timer();
int  bounce_or_lose();
void show_environment();
void serve();
void lose();
void placech(int, int, char);
void placestr(int, int, char*);

/** 
 * main()
 * 	purpose: The main loop function
 * 		 
 *	returns: Returns zero if the contents of set up are successful
 *		 which then serve the ball and allows paddle movement
 *		 and non-zero otherwise.
 */
int main()
{
	int c;

	if( set_up() != 0 )	/* init everything  */
		return -1;
	serve();		/* begins movement of ball */

	while ( balls_left >= 0 && ( c = getch()) != 'Q' ){
		if ( c== 'k' )
			paddle_up();
		else if ( c== 'm' )
			paddle_down();
	}
	wrap_up();
	return 0;
}


/**
 * set_up()
 *	purpose: Initializes the paddle, the environment, signal handler,
 *		  and curses
 *	returns: Returns zero if the dimensions are appropriate for setting up
 *		 the game and non-zero otherwise.	
 */
int set_up()
{
	the_ball.symbol = DFL_SYMBOL ;

	timer.hr = 0, timer.min = 0, timer.sec = 0, timer.msec = 0;

	initscr();		/* turns on curses	*/
	noecho();		/* turns off echo	*/
	cbreak();		/* turns off buffering	*/

	signal(SIGINT, SIG_IGN);	/* ignore SIGINT	*/

	if(LINES < 10 || 
	  COLS < (6 + strlen("balls left: _  total time: __:__:__ "))){
		wrap_up();
		fprintf(stderr, "Window dimensions too small for game\n");
		return -1;
	}

	show_environment();			/* display environment */
	srand(getpid());		/* seeds random number generator */
	paddle_init();

	move( LINES-1, COLS-1 );	/* park cursor	*/
	refresh();

	return 0;
}

/** 
 * wrap_up()
 *	purporse: Function stops ticker and curses. 
 * 
 */
void wrap_up()
{
	set_ticker( 0 );
	endwin();		/* put back to normal	*/
}


/** 
 * bal_move(s)
 *	purporse: Function is responsible for tracking the position of the ping
 * 		  pong ball and created the illusion of the animation of its
 *		  movement as well as what happens when in contact with the 
 *		  wall or paddle. The movement of the ball relies on the
 *		  update_timer function.
 *	input:	  Function takes in an integer that represents if the
 *		  position of the ball has moved or not.
 */
void ball_move(int s)
{
	int	y_cur, x_cur, moved;

	signal( SIGALRM , SIG_IGN );		/* dont get caught now 	*/
	y_cur = the_ball.y_pos ;		/* old spot		*/
	x_cur = the_ball.x_pos ;
	moved = 0 ;

	update_timer();/* update timer */

	if ( the_ball.y_delay > 0 && --the_ball.y_count == 0 ){
		the_ball.y_pos += the_ball.y_dir ;	/* move	*/
		the_ball.y_count = the_ball.y_delay  ;	/* reset*/
		moved = 1;
	}
	if ( the_ball.x_delay > 0 && --the_ball.x_count == 0 ){
		the_ball.x_pos += the_ball.x_dir ;	/* move	*/
		the_ball.x_count = the_ball.x_delay  ;	/* reset*/
		moved = 1;
	}
	if ( moved ){
		if( paddle_contact(y_cur, x_cur) == TRUE )
			placech( y_cur, x_cur, PDL_SYMBOL );
		else	placech( y_cur, x_cur, BLANK );
		placech( the_ball.y_pos, the_ball.x_pos, the_ball.symbol );
		if( bounce_or_lose( ) == LOSE ){
			lose();
		}
		refresh();
	}
	signal( SIGALRM, ball_move );		/* re-enable handler	*/
}

/**
 * update_timer()
 *	purpose: Function updates the internal state of game timer and updates it's
 *		 dispay value. A new timer value will show upon a next game refresh.
 */
void update_timer()
{
	timer.msec++;
	if( (timer.msec / TICKS_PER_SEC) < 1 )
		return;
	timer.msec = timer.msec % TICKS_PER_SEC;
	timer.sec++;
	timer.min += timer.sec / 60;
	timer.sec  = timer.sec % 60;
	timer.hr  += timer.min / 60;
	timer.min  = timer.min % 60;
	char clock[] = "  :  :  ";
	clock[0] = (timer.hr / 10)  + '0';
	clock[1] = (timer.hr % 10)  + '0';
	clock[3] = (timer.min / 10) + '0';
	clock[4] = (timer.min % 10) + '0';
	clock[6] = (timer.sec / 10) + '0';
	clock[7] = (timer.sec % 10) + '0';
	placestr(1, ((RIGHT_EDGE / 2) + strlen("total time: ")), clock);
}


/* bounce_or_lose()
 *	purpose: Function is responsible for the bounce of the ball against either
 *		 a wall or the paddle and changing its direction accordingly.
 *   	returns: Returns a one if a bounce happened, a zero if no bounce happened, 
 *		 and a negative one if ball is out of bounds.
 */
int bounce_or_lose()
{
	struct 	ppball *bp = &the_ball;
	int	return_val = 0 ;

	if ( bp->y_pos == TOP_ROW )
		bp->y_dir = 1 , return_val = BOUNCE ;
	else if ( bp->y_pos == BOT_ROW )
		bp->y_dir = -1 , return_val = BOUNCE;

	if ( bp->x_pos == LEFT_EDGE )
		bp->x_dir = 1 , return_val = BOUNCE ;
	else if(bp->x_pos == RIGHT_EDGE){
		if ( paddle_contact(bp->y_pos, bp->x_pos) == TRUE ){
			bp->x_dir = -1 , return_val = BOUNCE;
			bp->y_delay += (rand() % 3);	/* slight speed */
			bp->x_delay += (rand() % 3);	/* modification */
		}
	} else if ( bp->x_pos > RIGHT_EDGE )
		return_val = LOSE;

	return return_val;
}

/**
 * show_environment()
 *	purpose: Function displays the boundary walls for the game, as well as the 
 *		 clock and balls left.
 */
void show_environment()
{
	int i;
	placestr(1, LEFT_EDGE, "BALLS LEFT: ");
	placech(1, (LEFT_EDGE + strlen("balls left: ")), (balls_left + '0'));
	placestr(1, (RIGHT_EDGE / 2), "TOTAL TIME: ");
	for(i = TOP_ROW; i <= BOT_ROW; i++){
		placech(i, (LEFT_EDGE -1), '|');
	}
	for(i=(LEFT_EDGE-1); i <= RIGHT_EDGE; i++){
		placech((TOP_ROW -1), i, '-');
		placech((BOT_ROW +1), i, '-');
	}
}

/**
 * serve()
 *	purpose: Function puts a ball into play giving it a random-like x and y 
 *		 velocity. It also calls the set_ticker function and begins a timer.
 *		 
 */
void serve()
{
	placestr( (BOT_ROW+2), LEFT_EDGE, "        ");	/* erase lose msg */
	the_ball.y_pos = Y_INIT;
	the_ball.x_pos = X_INIT;
	the_ball.y_count = the_ball.y_delay = (rand() % 6) + 5 ;
	the_ball.x_count = the_ball.x_delay = (rand() % 3) + 3 ;
	the_ball.y_dir = 1  ;
	the_ball.x_dir = 1  ;
	placech(the_ball.y_pos, the_ball.x_pos, the_ball.symbol);
	
	signal( SIGALRM, ball_move );
	set_ticker( 1000 / TICKS_PER_SEC );	/* send millisecs per tick */
}


/**
 * lose()
 *	purpose: Function pauses the timer, displays a game loss indicator, decrements 
 *		 number of balls left, and calls the serve() function if any balls are
 *		 remaining.
 */
void lose()
{
	set_ticker( 0 );		/* pause timer 	*/
	placestr((BOT_ROW+2), LEFT_EDGE, "YOU LOST. PRESS ANY KEY TO CONTINUE");
	balls_left--;
	placech(1, (LEFT_EDGE+strlen("balls left: ")), (balls_left+'0'));
	refresh();

	getchar();			/* block on user input to give */
					/* player a pause */

	placestr((BOT_ROW+2), LEFT_EDGE,"                                    ");
	
	if(balls_left >= 0) serve();
}


/**
 * placech(y, x, ch)
 *	purpose: Function is a wrapper around mvaddch that allows the 
 *		 calling function to reliably change the cursor and place 
 *		 characters on the screen without disturbing the location of 
 *		 the cursor for any other interrupted processes. It saves the 
 *		 location of the cursor before writing to the screen and 
 *		 then restore that location afterward.
 */
void placech(int y, int x, char ch)
{
	int y_cur = getcury(stdscr), x_cur = getcurx(stdscr);
	mvaddch(y, x, ch);
	move(y_cur, x_cur);
}



/**
 * placestr(y, x, string)
 *	purpose: Function is a wrapper around mvaddstr that allows the 
 *		 calling function to reliably change the cursor and place 
 *		 strings on the screen without disturbing the location of 
 *		 the cursor for any other interrupted processes. It saves the 
 *		 location of the cursor before writing to the screen and 
 *		 then restore that location afterward.
 */
void placestr(int y, int x, char* string)
{
	int y_cur = getcury(stdscr), x_cur = getcurx(stdscr);
	mvaddstr(y, x, string);
	move(y_cur, x_cur);
}

