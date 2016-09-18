#include <stdio.h>
#include <curses.h>
#include "paddle.h"
#include "pong.h"

struct ppaddle {
	int	paddle_top, paddle_bottom, paddle_column;
	char 	paddle_char;
};

static struct ppaddle the_paddle;

static void put_paddle_char( char );

/**
 * paddle_init()
 *	purpose: Function draws the intial state of the paddle object on the screen. 
 *		 It uses the ppaddle struct and gets initialized to the corresponding 
 *		 parameters of the "pong table" which is located in the pong.h file. 
 *
 */
void paddle_init()
{
	the_paddle.paddle_top  = TOP_ROW ;
	the_paddle.paddle_bottom  = the_paddle.paddle_top + (BOT_ROW - TOP_ROW)/3;
	the_paddle.paddle_column  = RIGHT_EDGE;
	the_paddle.paddle_char = PDL_SYMBOL;

	put_paddle_char( the_paddle.paddle_char );	// draw on screen
	refresh();
}

/**
 * paddle_up() 
 *	purpose: Function moves the location of the paddle up one row on the
 *		 screen. If the paddle is has no more space between it and the 
 *		 top of the screen, the function does nothing. Otherwise, it 
 *		 writes spaces over the current paddle location, changes the 
 *		 state of the paddle object, then writes the paddle characters 
 *		 on the screen at the appropriate location.
 */
void paddle_up()
{
	if( the_paddle.paddle_top <= TOP_ROW ){
		printf("\a");
		return;
	}
	put_paddle_char(BLANK);
	the_paddle.paddle_bottom--;
	the_paddle.paddle_top--;
	bounce_or_lose();
	put_paddle_char(the_paddle.paddle_char);
	refresh();
}


/**
 * paddle_down() 
 *	purpose: Function moves the location of the paddle down one row on the
 *		 screen. If the paddle is has no more space between it and the 
 *		 bottom of the screen, the function does nothing. Otherwise, it 
 *		 writes spaces over the current paddle location, changes the 
 *		 state of the paddle object, then writes the paddle characters 
 *		 on the screen at the appropriate location.
 */
void paddle_down()
{
	if( the_paddle.paddle_bottom >= BOT_ROW ){
		printf("\a");
		return;
	}

	put_paddle_char(BLANK);
	the_paddle.paddle_bottom++;
	the_paddle.paddle_top++;
	bounce_or_lose();
	put_paddle_char(the_paddle.paddle_char);
	refresh();
}


/**
 * paddle_contact(y, x)
 *	purpose: Function returns an int value indicating whether or not the 
 *		 screen co-ordinates provided match a location occupied by the 
 *		 paddle.
 *	input:	 Function takes in two integers, x and y, which represent the
 *		 x and y co-ordinate foe the screen location given.
 *	returns: Returns zero if the location is not occupied by the paddle, 
 *		 and non-zero otherwise.
 */
int paddle_contact(int y, int x)
{
	if( x == (the_paddle.paddle_column ) && y <= the_paddle.paddle_bottom && 
	    y >= the_paddle.paddle_top ){
		return TRUE;
	}
	return FALSE;
}

/**
 * put_paddle_char(the_char)
 *	purpose: Function is responsible for creating the illusion of animation
 *		 by keeping track of the paddle character and either drawing or
 *		 deleting it based on the paddle movement upward or downward.
 *	input:	 Function takes a character argument which is defined in the 
 *		 paddle.h file of parameters.		 
 */
void put_paddle_char(char the_char){
	int i = the_paddle.paddle_top, y_cur = getcury(stdscr), 
	    x_cur = getcurx(stdscr);

	for(; i <= the_paddle.paddle_bottom; i++)
		mvaddch(i, the_paddle.paddle_column, the_char);

	move(y_cur, x_cur);
}


