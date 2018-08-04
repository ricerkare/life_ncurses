#ifndef LIFE_H
#define LIFE_H

#define LIVE (' '|A_REVERSE) // Space + A_REVERSE produces the opaque block which is the default cursor for many terminal emulators
#define DEAD ' '

#define SLEEP 2000000000

#define ISLIVE(y, x) CELL((y), (x)) // 1 if live, 0 if dead

#define HELPCOLS 30
#define HELPROWS LINES

#define CELLCOUNT (lifecols * lifelines)
#define GRIDSIZE (CELLCOUNT * sizeof(int))

#define LIVECELL(y, x) (cells[(y)][(x)] = 1) 
#define KILLCELL(y, x) (cells[(y)][(x)] = 0)
#define BUFLIVEC(y, x) (buffer[(y)][(x)] = 1)
#define BUFKILLC(y, x) (buffer[(y)][(x)] = 0)

#define COPYBUFF memcpy(cells, buffer, SIZE)
#define COPYCELLS memcpy(buffer, cells, SIZE)

#endif
