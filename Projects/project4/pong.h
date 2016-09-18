#include <curses.h>
/**
 **	some parameters
 **/

#define	BLANK		' '
#define	DFL_SYMBOL	'O'
#define	TOP_ROW		3		/* top row in bounds */
#define	BOT_ROW 	(LINES - 4)	/* bot row in bounds */
#define	LEFT_EDGE	3	
#define	RIGHT_EDGE	(COLS - 4)
#define	X_INIT		10		/* starting col		*/
#define	Y_INIT		10		/* starting row		*/
#define	TICKS_PER_SEC	75		/* affects speed	*/
#define MIN_SCRN_SIZE	11		/* min num screen rows  */
#define PDL_SYMBOL	'#'
#define NUM_EXTRA_BALLS	2
#define BOUNCE		1
#define LOSE		-1

#define	X_DELAY		5
#define	Y_DELAY		8

/**
 **	the objects
 **/

struct ppball {
	int	x_pos, x_dir,
		y_pos, y_dir,
		y_delay, y_count,
		x_delay, x_count;
	char	symbol ;
};

struct ptime {
	int	hr, min, sec, msec;
};

int  bounce_or_lose();

