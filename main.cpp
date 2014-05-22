#include <ncurses.h>

int main()
{
	initscr();
	raw();
	keypad(stdscr, TRUE);
	noecho();
	curs_set(0);

	cbreak();
	echo();
	curs_set(1);
	endwin();
	return 0;
}
