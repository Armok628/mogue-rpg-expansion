#include <termios.h>
#include <stdio.h>
#ifndef M_DISPLAY
#define M_DISPLAY
enum COLOR_NAMES {BLACK,GRAY,RED,LRED,GREEN,LGREEN,BROWN,YELLOW
	,BLUE,LBLUE,PURPLE,PINK,TEAL,CYAN,LGRAY,WHITE};
static char *RESET_COLOR="\e[0;38;39m";
static char TERM_COLORS_40M[16][11]={"\e[0;30;40m","\e[1;30;40m" // Black
	,"\e[0;31;40m","\e[1;31;40m" // Red
	,"\e[0;32;40m","\e[1;32;40m" // Green
	,"\e[0;33;40m","\e[1;33;40m" // Yellow
	,"\e[0;34;40m","\e[1;34;40m" // Blue
	,"\e[0;35;40m","\e[1;35;40m" // Magenta
	,"\e[0;36;40m","\e[1;36;40m" // Cyan
	,"\e[0;37;40m","\e[1;37;40m"}; // White
static char TERM_COLORS_41M[16][11]={"\e[0;30;41m","\e[1;30;41m" // Black
	,"\e[0;31;41m","\e[1;31;41m" // Red
	,"\e[0;32;41m","\e[1;32;41m" // Green
	,"\e[0;33;41m","\e[1;33;41m" // Yellow
	,"\e[0;34;41m","\e[1;34;41m" // Blue
	,"\e[0;35;41m","\e[1;35;41m" // Magenta
	,"\e[0;36;41m","\e[1;36;41m" // Cyan
	,"\e[0;37;41m","\e[1;37;41m"}; // White
void clear_screen()
{
	printf("%s\e[2J",RESET_COLOR);
}
void clear_line()
{
	printf("%s\e[2K",RESET_COLOR);
}
void move_cursor(int x,int y)
{
	printf("\e[%d;%dH",y+1,x+1);
}
void set_cursor_visibility(int visible)
{
	if (visible)
		printf("\e[?25h");
	else
		printf("\e[?25l");
}
void set_terminal_canon(int canon)
{
	static struct termios old_term,new_term;
	if (!canon) {
		if (!old_term.c_lflag)
			tcgetattr(0,&old_term);
		new_term=old_term;
		new_term.c_lflag&=(~ICANON&~ECHO);
		tcsetattr(0,TCSANOW,&new_term);
	} else {
		tcsetattr(0,TCSANOW,&old_term);
	}
}
#endif
