/* Based on 
 *
 * TODO.
 * write copycells
 * 
 * Allow for real-time adjustments to terminal screen dimensions
 * (Possibly at some point) Use two adjacent terminal cells per game of life cell 
 * for ``aesthetic'' purposes
 *
 * NOTA BENE: any adjustments to the configuration (for instance, speed of 
 * animation) must be made when the animation is off.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <locale.h>
#include <time.h>
#include "macros.h"

/* grid will contain the cell information when the animation is paused, and will feed the buffer, whose cells are then displayed */

int ** grid;
int ** buffer;

static int lifecols = 0, lifelines = 0;

static WINDOW * info = NULL;
static WINDOW * life = NULL;

/* 100 000 000 nanoseconds (0.1 second); i.e., roughly, 10 fps */
static struct timespec animation_sleep;

void copy_cells(int ** matrix0, int ** matrix1);
void activate(int y, int x, int ** matrix);
void deactivate(int y, int x, int ** matrix);
void randomize_grid();
void clear_grid();
void tick();
void print_info(); //TODO
void init();
void init_ncurses();
void finish(); //TODO
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
	else if (ch == 't' || ch == 'T') {
	    tick();
	}
	else if (ch == 'c' || ch == 'C') {
	    clear_grid();
	}
	else {
	    switch(ch) {
	    case ' ':
		if (!ISLIVE(y, x)) {
		    activate(y, x, grid);
		}
		else {
		    deactivate(y, x, grid);
		}
		break;
	    case KEY_UP:
		y = (y - 1) % lifelines;
		break;
	    case KEY_DOWN:
		y = (y + 1) % lifelines;
		break;
	    case KEY_LEFT:
		x = (x - 1) % lifecols;
		break;
	    case KEY_RIGHT:
		x = (x + 1) % lifecols;
		break;
		/*  Wait time increments/decrements by 50 000 000 nanoseconds; 20 fps (roughly) is the maximum speed and 1 (roughly) fps the minimum. */
	    case ']':
		if (animation_sleep.tv_sec == 0
		    && animation_sleep.tv_nsec > (long) 50e+6 
		    && animation_sleep.tv_nsec <= (long) 950e+6) { 
		    animation_sleep.tv_nsec -= (long) 50e+6;
		}
		else if (animation_sleep.tv_sec == 1
			 && animation_sleep.tv_nsec == 0) {
		    animation_sleep.tv_sec = 0;
		    animation_sleep.tv_nsec = (long) 950e+6;
		}
		print_info();
		break;
	    case '[':
		if (animation_sleep.tv_sec == 0
		    && animation_sleep.tv_nsec < (long) 950e+6) {
		    animation_sleep.tv_nsec += (long) 50e+6;
		}
		else if (animation_sleep.tv_sec == 0
			 && animation_sleep.tv_nsec == 950e+6) {
		    animation_sleep.tv_sec = 1;
		    animation_sleep.tv_nsec = 0;
		}
		print_info();
		break;
	    case 10:
		do {
		    tick(animation_sleep);
		}
		/* nodelay (called in init_ncurses) is essential 
		 * for this to work as a non-blocking check. */
		while ((ch = getch()) != 10);
		break;
	    }
	}
	wmove(life, y, x);

	wrefresh(life);
    }
    finish();

    return 0;
}

void copy_cells(int ** matrix0, int ** matrix1) {
    if ((matrix0 == grid && matrix1 == buffer) || (matrix0 == buffer && matrix1 == grid)) {
	for (int j = 0; j < lifelines; j++) {
	    memcpy(matrix0[j], matrix1[j], lifecols * sizeof(int));
	}
    }
    else {
	perror("wrong arguments to copy_cells");
	exit(1);
    }
}

int count_neighbors(int x, int y)
{
    /* While this does take two more steps than is strictly necessary,
     * writing out the eight operands in the return statement would be
     * both cumbersome to write and rather hideous to read.
     */
    int n = 0;
    for (int j = -1; j <= 1; j++) {
	for (int i = -1; i <= 1; i++) {
	    n += ISLIVE((y + j) % lifelines, (x + i) % lifecols); 
	}
    }
    n -= ISLIVE(y, x);
    return n;
}

void randomize_grid()
{
    int r = rand();
    for (int j = 0; j < lifelines; j++) {
	for (int i = 0; i < lifecols; i++) {
	    if (r % 8 >= 4) {
		activate(j, i, grid);
	    }
	    else {
		deactivate(j, i, grid);
	    }
	}
    }
}

void tick(struct timespec sleep)
{
    /* We alter buffer according to the Game of Life rules, based on the
     * states of grid; display the buffer; then copy buffer to grid.
     * While this is a little backwards (the buffer, intuitively, should
     * be copied to cells and then the cells displayed), it makes use of
     * one set of for loops rather than two, so, ya know . . .
     */

    /* 
     * The Rules.
     * For every cell, if n is the neighbor count, then
     *     cell live and n = 2 or 3 implies cell lives
     *     cell live and (n < 2 or n > 3) implies cell dies
     *     cell dead and n = 3 implies cell is revived
     *     cell dead and n =/= 3 implies cell stays dead
     *
     * Idea for future: optimize the tick by displaying only cells from 
     * buffer which differ from grid; then displaying the rest from grid;
     * then copying differing cells from buffer to grid.
     */
    
    copy_cells(buffer, grid);
    for (int y = 0; y < lifelines; y++) {
	for (int x = 0; x < lifecols; x++) {
//	    if (buffer[y][x] != grid[y][x]) { //This check gets rid of redundant calculations
	    int n = count_neighbors(y, x);
	    if (n < 2 || n > 3) {
		deactivate(y, x, buffer);
	    }
	    else if (n == 3) {
		activate(y, x, buffer);
	    }
	}
    }
    copy_cells(grid, buffer);
    wrefresh(life);
    nanosleep(&sleep, NULL);
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
    nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);
    refresh();
    
    lifecols = COLS - INFOCOLS - 1;
    lifelines = LINES;

    /* life and info are one cell apart */

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
    srand(time(NULL));
    animation_sleep.tv_sec = 0;
    animation_sleep.tv_nsec = (long) 100e+6;
    init_ncurses();
    setlocale(LC_ALL, "");

    /*We fill grid and buffer with zeros. */

    grid = (int **)malloc(lifelines * sizeof(int *));
    if (grid == NULL) {
	exit(1);
    }
    buffer = (int **)malloc(lifelines * sizeof(int *));
    if (buffer == NULL) {
	exit(1);
    }
    grid[0] = (int *)malloc(lifecols * lifelines * sizeof(int));
    buffer[0] = (int *)malloc(lifecols * lifelines * sizeof(int));
    for (int j = 0; j < lifelines; j++) {
	grid[j] = (*grid + lifelines*j);
	memset(grid[j], 0, lifecols);
	buffer[j] = (*buffer + lifelines*j);
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

/* activate and deactivate make a cell live or dead (respectively) AND display that cell.
 * This is a little awkward; it might be more intuitive to have one central function to 
 * display grid or buffer after they have been modified, but this works just as well.
 */

void activate(int y, int x, int ** matrix)
{
    if (matrix == buffer) {
	BUFLIVEC(y, x);
    }
    else if (matrix == grid) {
	LIVECELL(y, x);
    }
    else {
	perror("Incorrect pointer passed to function activate");
    }
    mvwaddch(life, y, x, LIVE);
}

void deactivate(int y, int x, int ** matrix)
{
    if (matrix == buffer) {
	BUFKILLC(y, x);
    }
    else if (matrix == grid) {
	KILLCELL(y, x);
    }
    else {
	perror("Incorrect pointer passed to function deactivate");
    }
    mvwaddch(life, y, x, DEAD);
}

void clear_grid()
{
    for (int j = 0; j < lifelines; j++) {
	for (int i = 0; i < lifecols; i++) {
	    KILLCELL(j, i);
	    deactivate(j, i, buffer);
	}
    }
}

void finish()
{
    free(grid);
    free(buffer);
    delwin(info);
    delwin(life);
    endwin();
}
