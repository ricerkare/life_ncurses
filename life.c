/* TODO.
 * Figure out use of time.h
 * (Non-urgent) Use two adjacent terminal cells per game of life cell
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <locale.h>
#include <time.h>
#include "macro.h"

int looping = 0;

int ** cells;
int ** buffer;

static int lifecols = 0, liferows = 0;

static WINDOW *help = NULL;
static WINDOW *life = NULL;
/* Holds the sleep time */
static struct timespec sleeptime = {0, 250000000};

void activate(int y, int x);
void deactivate(int y, int x);
void tick();
void print_info();
void init();
void init_ncurses();
void finish();
int count_neighbors(int y, int x);

WINDOW *createwin(int height, int width, int begy, int begx);

int main()
{
    srand(time(NULL));
    int ch;
    int y, x;
    init();
    while ((ch = getch()) != 'q' && ch != 'Q') {
	getyx(life, y, x);
	if (ch == 'r' || ch == 'R') {
	    randomize_grid();
	}
	else if (ch == 'l' || ch == 'L') {
	    looping = !looping;
	}
	else if (ch == 't' || ch == 'T') {
	    tick();
	}
	else {
	    switch(ch) {
	    case ' ':
		if (!ISALIVE(y, x)) {
		    activate(y, x);
		}
		else {
		    deactivate(y, x);
		}
	    case KEY_UP:
		y = (y - 1) % Y_MAX;
	    case KEY_DOWN:
		y = (y + 1) % Y_MAX;
	    case KEY_LEFT:
		x = (x - 1) % X_MAX;
	    case KEY_RIGHT:
		x = (x + 1) % X_MAX;
		/*  Wait time increments/decrements by 50000000 nanoseconds; 20 fps is the maximum speed and 1 fps the infimum */
	    case ']':
		if (wait > 50000000) { 
		    wait -= 50000000;
		}
	    case '[':
		if (wait < 999999999) {
		    wait += 50000000;
		}
	    case 10:
		do {
		    tick();
		    //TODO: DELAY
		}
		while ((ch = getch()) != 10);
	    }
	}
    }
}

int count_neighbors(int x, int y)
{
    /* While this does take two more steps than is strictly necessary,
     * writing out the eight operands in the return statement would be
     * both cumbersome to write and rather hideous to read.
     */
    int n = 0;
    for (int j = -1; int j <= 1; j++) {
	for (int i = -1; int i <= 1; i++) {
	    n += ISALIVE((y + j) % Y_MAX, (x + i) % X_MAX); 
	}
    }
    n -= ISALIVE(y, x);
    return n;
}

void randomize_grid()
{
    int r = rand();
    for (int j = 0; j < Y_MAX; j++) {
	for (int i = 0; i < X_MAX; i++) {
	    if (r % 8 >= 4) {
		activate(j, i, "cells");
	    }
	    else {
		deactivate(j, i, "cells");
	    }
	}
    }
}

void tick()
{
    /* We alter buffer according to the Game of Life rules, based on the
     * states of cells; display the buffer; then copy buffer to cells.
     * While this is a little backwards (the buffer, intuitively, should
     * be copied to cells and then the cells displayed), it makes use of
     * one set of for loops rather than two, so, ya know . . .
     */
    struct timespec *remaining
    COPYCELLS;
    for (int y = 0; y <= Y_MAX; y++) {
	for (int x = 0; x <= X_MAX; x++) {
	    if (buffer[y][x] != cells[y][x]) { //This check gets rid of redundant calculations.
		n = count_neighbors(y, x);
		if (n < 2 || n > 3) {
		    deactivate(y, x, buffer);
		}
		else if (n == 3) {
		    activate(y, x, buffer);
		}
	    }
	}
    }
    COPYBUF;
    wrefresh(life);
    //TODO: DELAY
}

WINDOW *createwin(int height, int width, int begy, int begx)
{
    WINDOW *local_win = NULL;
    local_win = newwin(height, width, begy, begx);
    if (local_win == NULL) {
	exit(EXIT_FAILURE);
    }
    wmove(local_win, 1, 1);
    wrefresh(local_win);
    return local_win;
}

void init_ncurses()
{
    initscr();
    noecho();
    raw();
    keypad(stdscr, TRUE);
    refresh();
    lifecols = COLS - INFOCOLS - 1;
    lifelines = LINES;

    life = createwin(lifelines, lifecols, 0, 0);
    info = createwin(INFOLINES, INFOCOLS, 0, lifecols + 1);

    box(info, 0, 0);
    mvwprintw(info, 1, 1, "INFO");

    wmove(life, 1, 1);

    wrefresh(info);
    wrefresh(life);
}

void init()
{
    sleeptime.tv_nsec = SLEEP;
    initcurses();
    setlocale(LC_ALL, "");

    cells = (int **)malloc(lifelines * sizeof(int *));
    cells[0] = (int *)malloc(lifecols * lifelines * sizeof(int));
    if (cells == NULL) {
	exit(1);
    }
    for (int j = 0; j < lifelines; j++) {
	memset(cells[j], 0, lifecols);
    }
    
    buffer = (int **)malloc(lifelines * sizeof(int *));
    buffer[0] = (int *)malloc(lifecols * lifelines * sizeof(int));
    if (buffer == NULL) {
	exit(1);
    }
    for (int j = 0; j < lifelines; j++) {
	memset(buffer[j], 0, lifecols);
    }
    
    print_info();
}

void print_info()
{
    int y, x;
    mvwprintw(info, 2, 1, "Total size");
    mvwprintw(info, 3, 1, "  Columns: %d", COLS);
    mvwprintw(info, 4, 1, "  Lines: %d", LINES);
    mvwprintw(info, 5, 1, "Life sizes");
    mvwprintw(info, 6, 1, "  Columns: %d", lifecols);
    mvwprintw(info, 7, 1, "  Lines: %d", lifelines);
    mvwprintw(info, 8, 1, "Array size: %d", CELLCOUNT);
    mvwprintw(info, 9, 1, "Tick size: %d", ticksize);
    mvwprintw(info, 12, 1, "Move with arrow keys");
    mvwprintw(info, 13, 1, "Space to activate/deactivate cell");
    mvwprintw(info, 14, 1, "Enter to tick, [ and ] to adjust tick size");
    mvwprintw(info, 15, 1, "r to randomize");
    mvwprintw(info, 15, 1, "Quit with q");
    getyx(life, y, x);
    wmove(life, y, x);
    wrefresh(info);
    wrefresh(life);
}


void activate(int y, int x, char * matrix)
{
    if (matrix == buffer) {
	BUFLIVEC(y, x);
    }
    else if (matrix == cells) {
	LIVECELL(y, x);
    }
    else {
	perror("Incorrect pointer passed to function activate");
    }
    mvwaddch(life, y, x, LIVE);
}

void deactivate(int y, int x, char * matrix)
{
    if (matrix == buffer) {
	BUFKILLC(y, x);
    }
    else if (matrix == cells) {
	LIVECELL(y, x);
    }
    else {
	perror("Incorrect pointer passed to function deactivate");
    }
    mvwaddch(life, y, x, DEAD);
}
