#include <stdio.h>
#include <termios.h>
#include <stddef.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <sys/ioctl.h>

#define isBackspace 127
#define IOFFSET offsetof(struct termios,c_iflag)
#define OOFFSET offsetof(struct termios,c_oflag)
#define COFFSET offsetof(struct termios,c_cflag)
#define LOFFSET offsetof(struct termios,c_lflag) 


void  show_settings();
void show_control_char(struct termios *);
void show_bit_field(struct termios *);
int execute_args(char **, struct termios *);
void show_baud(int);
void  print_terminal_dimensions();
int set_bit_field(struct termios *, char *);
int set_control_char(struct termios *, char **);



struct bit_content {
	const char *name;
	const unsigned int offset;
	const unsigned int mask;
};

struct bit_content bit_table[] =
{
	{  "icrnl",  IOFFSET, ICRNL },
	{ "-icrnl",  IOFFSET, ICRNL },
	{  "hupcl",  COFFSET, HUPCL },
	{ "-hupcl",  COFFSET, HUPCL },
	{  "echo",   LOFFSET, ECHO },
	{ "-echo",   LOFFSET, ECHO },
	{  "echoe",  LOFFSET, ECHOE },
	{ "-echoe",  LOFFSET, ECHOE },
	{  "opost",  OOFFSET, OPOST },
	{ "-opost",  OOFFSET, OPOST },
	{  "icanon", LOFFSET, ICANON },
	{ "-icanon", LOFFSET, ICANON },
	{  "isig",   LOFFSET, ISIG },
	{ "-isig",   LOFFSET, ISIG },
	{   NULL, 0, 0 }
};

struct cc_info { unsigned int field; char *name; };

struct cc_info cc_table[] = {
	{ VINTR,   "intr" },
	{ VERASE,  "erase" },
	{ VKILL,   "kill" },
	{ VSTART,  "start" },
	{ VSTOP,   "stop" },
	{ VWERASE, "werase" },
	{ 0, NULL }
};

/**
 *
 */
int main(int argc, char** argv)
{
	int ok = 0, fd = 0;	// file descriptor zero causes read from stdin
	struct termios settings;
	if( tcgetattr(fd, &settings) != 0 ){
		printf("Error: Failed to read system settings");
		return 1;
	}

	if(argc > 1){
		show_settings( &settings );
		ok = execute_args(argv, &settings);
	}
	if (ok == 0){
		ok = tcsetattr(fd, TCSANOW, &settings);
	}
	show_settings( &settings );
	
	return ok;
}


void show_settings(struct termios* settings)
/**
 * show_settings
 *	purpose: gets and prints to stdout the current status 
 *		 of the terminal settings
 *	returns: takes no input and returns nothing
 */
{
	show_baud( cfgetospeed( settings ) );

	print_terminal_dimensions(); 

	show_control_char(settings);

	show_bit_field(settings);
}



void show_control_char(struct termios *settings)
/**
 * show_control_char(settings)
 *	purpose: Function prints out the current settings for the control
 *		 characters as provided by the struct termios object input
 *		 settings. The field values of the supported settings are 
 * 		 read from the the global ccTable object.
 */
{
	char cc[3];
	unsigned int i = 0, field;


	for(i=0; cc_table[i].name != NULL ; i++){
		field = cc_table[i].field;

		if( iscntrl(settings->c_cc[field]) ){
			cc[0] = '^';
			cc[1] = (settings->c_cc[field] + 'A' - 1);
			cc[2] = '\0';
		} else if(settings->c_cc[field] == _POSIX_VDISABLE){
			cc[0] = _POSIX_VDISABLE;
			cc[1] = '\0';
		} else{
			cc[0] = settings->c_cc[field];
			cc[1] = '\0';
		}

		if( cc_table[i].field == field ){
			printf("%s = %s", cc_table[i].name, cc);
		}

		if(i && (i%4) == 0) printf("\n");
		else printf("; ");
	}
	printf("\n");
}

void show_bit_field(struct termios *settings)
/**
 * show_bit_field(settings)
 *	purpose: Walks through the global array of bit table entries and uses 
 *		 it to extract the current settings from the termios struct.
 *	input:	 A termios struct representing the current settings is taken 
 *		 as input.
 */
{
	unsigned int i, *ptr;

	for( i=0; bit_table[i].mask != 0 ; i+=2 ){
		ptr = (unsigned int*)((char*)settings + bit_table[i].offset);
		if(*ptr & bit_table[i].mask){
			printf("%s ", bit_table[i].name);
		} else {
			printf("%s ", bit_table[i+1].name);
		}
		if(i && (i%6) ==0){
			printf("\n");
		}
	}
}


int execute_args(char **av, struct termios *settings)
/**
 * execute_args(av, settings)
 *	purpose: The function carries prepares to set the current mode of 
 *		 the terminal based on the arguments given. The new settings
 *		 are then stored in the input settings pointer.
 *	inputs:  Function takes as input av representing the argument strings
 *		 to the calling function and the termios struct settings which
 *		 represents the current tterminal settings
 *	returns: Returns 0 on success. Returns non-zero for failure if an 
 *		 unsupoorted argument is encountered.
 */
{
	int i, retVal = 0;
	char *ptr[2] = { av[1], av[2] };
	struct termios newMode = *settings;

	for(i=1; *ptr != '\0' ; ptr[0]=av[++i]){
		ptr[1] = av[i + 1];
		if(set_bit_field(&newMode, *ptr) != 0){
			if(set_control_char(&newMode, ptr) != 0){
				fprintf(stderr, 
				"%s is an unrecognized or ill-formed argument\n",
					ptr[0]);
				retVal = -1;
			} else{
				i++;
			}
		}
		*settings = newMode;
	}
	return retVal;
}


void show_baud(int speed)
/**
 * show_baud(speed)
 *	purpose: Function prints out the baud rate of the terminal in human 
 *		 readable format
 *	input:	 A terminal speed value from a termios struct.
 *	notes:	 Exactly as provided to us by the teaching staff
 */
{
	printf("speed ");
	switch( speed ){
		case B300:   printf("300");   break;
		case B600:   printf("600");   break;
		case B1200:  printf("1200");  break;
		case B1800:  printf("1800");  break;
		case B2400:  printf("2400");  break;
		case B4800:  printf("4800");  break;
		case B9600:  printf("9600");  break;
		case B19200: printf("19200"); break;
		case B38400: printf("38400"); break;
		default:     printf("Fast");  break;
	}
	printf(" baud; ");
}


void print_terminal_dimensions()
/**
 * print_terminal_dimensions
 *	purpose: prints the rows and columns of the current terminal session
 *
 */
{
	struct winsize ws;

	if (ioctl(0, TIOCGWINSZ, &ws) < 0) { 
		perror("ioctl failed"); 
		return; 
	}

	printf("rows %u; cols %u\n", ws.ws_row, ws.ws_col);
}


int set_bit_field(struct termios *newMode, char *string)
/**
 * set_bit_field(newMode, string)
 *	purpose: Sets the bitfield for terminal setting value whose name 
 * 		 represented by the string.
 *	inputs:	 A struct termios object representing the new settings to 
 *		 be applied and an aray of characters representing the field 
 *	returns: 0 on success and non-zero otherwise
 */
{
	int retVal = -1;		
	unsigned int i, fieldVal;
	
	for(i=0; bit_table[i].name != NULL; i++){

		if(strcmp(bit_table[i].name, string) == 0){
			fieldVal = *((int*)((char*)newMode + bit_table[i].offset));

			if(string[0] == '-') 
				fieldVal &= ~bit_table[i].mask;
			else 
				fieldVal |= bit_table[i].mask;

			*((int*)((char*)newMode + bit_table[i].offset)) = fieldVal;

			retVal = 0;	break;
		}
	}
	return retVal;
}


int set_control_char(struct termios *newMode, char **string)
/**
 * set_control_char(newMode, string)
 *	purpose: Sets the control char for terminal setting whose name is 
 * 		 represented by the string. Fails if more than one control char
 *		 is specified as input.
 *	inputs:	 A struct termios object representing the new settings to 
 *		 be applied and an aray of two strings characters representing 
 *		 first the setting to update and then the character to set.
 *	returns: 0 on success and non-zero otherwise.
 *
 */
{
	int i, retVal = -1;
	unsigned char cc;

	for(i=0; cc_table[i].name != NULL; i++){
		if(strcmp(cc_table[i].name, *string) == 0){
			if(strlen(string[1]) != 1){
				fprintf(stderr,
				"Control chars must be a single char\n");
				printf("arg is %s\n", string[1]);
			} else{
				cc = **(++string);
				newMode->c_cc[cc_table[i].field] = cc;
				retVal = 0;
			}			
		}
	}
	return retVal;
}

