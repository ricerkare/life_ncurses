/* Loosely based on MasseR's Curses-game-of-life (https://github.com/MasseR/Curses-game-of-life).
 *
 * NOTA BENE: any adjustments to the configuration (for instance, speed of 
 * animation) must be made when the animation is off.
 */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <locale.h>
#include <time.h>

#define LIVE (' '|A_REVERSE) 	// Space + A_REVERSE produces the solid block which is the default cursor for many terminal emulators
#define DEAD ' '		// Blank for dead cell

#define INFOCOLS 35
#define INFOLINES LINES

/* positive modulus */
#define MOD(a, b) ((((a) % (b)) + (b)) % (b))

/* grid will contain the cell information when the animation is paused, and will feed the buffer, whose cells are then displayed */

int **grid;
int **buffer;

static int lifecols = 0, lifelines = 0;

static WINDOW * info = NULL;
static WINDOW * life = NULL;

/* Default wait time is 100 000 000 nanoseconds (0.1 second); i.e., roughly, 10 fps */
static struct timespec wait;
static double fwait = 100e+6;

void display_cells(int **matrix, int y_len, int x_len);
void copy_cells(int **matrix0, int **matrix1, int y_len, int x_len);
void randomize_grid();
void clear_grid();
void tick();
void print_info(); 
void init();
void init_ncurses();
void finish(); 
int count_neighbors(int **matrix, int y, int x);

WINDOW *createwin(int height, int width, int begy, int begx);

int main()
{
    int ch, y, x;
    init();
    while ((ch = getch()) != 'q' && ch != 'Q') {
	getyx(life, y, x);
	switch(ch) {
	case 'r':
	case 'R':
	    randomize_grid();
	    break;
	case 't':
	case 'T':
	    tick();
	    break;
	case 'c':
	case 'C':
	    clear_grid(grid, lifelines, lifecols);
	    break;
	case ' ':
	    if (!grid[y][x]) {
		grid[y][x] = 1;
	    }
	    else {
		grid[y][x] = 0;
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
		
	    /* Wait time increments/decrements by 50 000 000 nanoseconds.
	     * 81 fps (roughly) is the maximum speed and 1 (roughly) fps the infimum.
	     * So as to not risk making this wildly inaccurate, we have one variable
	     * to control the increments/decrements, and then cast to long for 'wait'.
	     * floor and ceil are used to round; for example, if fwait is ~12499999.999
	     * then checking if fwait > 12.5e+6 will not be useful
	     */
	case ']':
	    if (ceil(fwait) > floor(12.5e+6)) {
		fwait = 1e+9 * fwait / (1e+9 + fwait);
		wait.tv_nsec = (long) fwait;
	    }
	    print_info();
	    break;
	case '[':
	    if (floor(fwait) < ceil(0.5e+9)) {
		fwait = 1e+9 * fwait / (1e+9 - fwait);
		wait.tv_nsec = (fwait < 1e+9) ? (long) fwait : 999999999;
	    }
	    print_info();
	    break;
	case 10:
	    /* nodelay (called in init_ncurses) is essential 
	     * for this to work as a non-blocking check. */
	    do {
		tick();
		display_cells(grid, lifelines, lifecols);
		wmove(life, y, x);
		wrefresh(life);
		nanosleep(&wait, NULL);
	    }
	    while ((ch = getch()) != 10);
	    break;
	}
	
	display_cells(grid, lifelines, lifecols);
	wmove(life, y, x);
	wrefresh(life);
    }
    finish();

    return 0;
}

void display_cells(int **matrix, int y_len, int x_len)
{
    for (int j = 0; j < y_len; j++) {
	for (int i = 0; i < x_len; i++) {
	    mvwaddch(life, j, i, (matrix[j][i]) ? LIVE : DEAD);
	}
    }
}

void copy_cells(int ** matrix0, int ** matrix1, int y_len, int x_len)
{
    for (int j = 0; j < y_len; j++) {
	memcpy(matrix0[j], matrix1[j], x_len * sizeof(int));
    }

}

void randomize_grid()
{
    for (int j = 0; j < lifelines; j++) {
	for (int i = 0; i < lifecols; i++) {
	    int r = rand();
	    if (r % 8 >= 4) {
		grid[j][i] = 1;
	    }
	    else {
		grid[j][i] = 0;
	    }
	}
    }
}

int count_neighbors(int **matrix, int y, int x)
{
    return matrix[MOD(y-1, lifelines)][MOD(x-1, lifecols)]
	+ matrix[MOD(y-1, lifelines)][MOD(x, lifecols)]
	+ matrix[MOD(y-1, lifelines)][MOD(x+1, lifecols)]
	+ matrix[MOD(y, lifelines)][MOD(x-1, lifecols)]
	+ matrix[MOD(y, lifelines)][MOD(x+1, lifecols)]
	+ matrix[MOD(y+1, lifelines)][MOD(x-1, lifecols)]
	+ matrix[MOD(y+1, lifelines)][MOD(x, lifecols)]
	+ matrix[MOD(y+1, lifelines)][MOD(x+1, lifecols)];	     
}

void tick()
{
    /*
     * The Rules.
     * For every cell, if n is the neighbor count, then
     *     cell live and n = 2 or 3 implies cell lives
     *     cell live and (n < 2 or n > 3) implies cell dies
     *     cell dead and n = 3 implies cell is revived
     *     cell dead and n =/= 3 implies cell stays dead
     */

    copy_cells(buffer, grid, lifelines, lifecols);
    for (int j = 0; j < lifelines; j++) {
	for (int i = 0; i < lifecols; i++) {
	    int n = count_neighbors(grid, j, i);
	    if (n < 2 || n > 3) {
		buffer[j][i] = 0;
	    }
	    else if (n == 3) {
		buffer[j][i] = 1;
	    }
	}
    }
    copy_cells(grid, buffer, lifelines, lifecols);
    wrefresh(life);
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
    wmove(life, 1, 1);

    wrefresh(info);
    wrefresh(life);
}

void init()
{
    srand(time(NULL));
    wait.tv_sec = 0;
    wait.tv_nsec = (long) fwait;
    init_ncurses();
    setlocale(LC_ALL, "");

    /* We fill grid and buffer with zeros. */

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
	grid[j] = (*grid + lifecols*j);
	memset(grid[j], 0, lifecols);
	buffer[j] = (*buffer + lifecols*j);
	memset(buffer[j], 0, lifecols);
    }
    
    print_info();
}

void print_info()
{
    int y, x;
    mvwprintw(info, 1, 1, "Grid Dimensions");
    mvwprintw(info, 2, 1, "  Columns: %d", lifecols);
    mvwprintw(info, 3, 1, "  Lines: %d", lifelines);
    double speed = (wait.tv_sec == 0) ? (1.0 / (wait.tv_nsec / 1e+9)) : 1.0;
    mvwprintw(info, 4, 1, "Animation speed: %.2f fps ", speed);
    mvwprintw(info, 6, 1, "Move with arrow keys");
    mvwprintw(info, 7, 1, "Space to activate/deactivate cell");
    mvwprintw(info, 8, 1, "Enter to start animation");
    mvwprintw(info, 9, 1, "T to tick");
    mvwprintw(info, 10, 1, "[ and ] to adjust animation speed");
    mvwprintw(info, 11, 1, "r to randomize");
    mvwprintw(info, 12, 1, "Quit with q");
    getyx(life, y, x);
    wmove(life, y, x);
    wrefresh(info);
    wrefresh(life);
}

void clear_grid(int **matrix, int y_len, int x_len)
{
    for (int j = 0; j < y_len; j++) {
	for (int i = 0; i < x_len; i++) {
	    matrix[j][i] = 0;
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
