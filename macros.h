#ifndef LMACROS_H
#define LMACROS_H

#define LIVE (' '|A_REVERSE) // Space + A_REVERSE produces the opaque block which is the default cursor for many terminal emulators
#define DEAD ' '

#define ISLIVE(y, x) grid[(y)][(x)] // 1 if live, 0 if dead

#define INFOCOLS 30
#define INFOLINES LINES

#define CELLCOUNT (lifecols * lifelines)
#define GRIDSIZE (CELLCOUNT * sizeof(int))

/* Create live/dead cells */
#define LIVECELL(y, x) (grid[(y)][(x)] = 1) 
#define KILLCELL(y, x) (grid[(y)][(x)] = 0)
#define BUFLIVEC(y, x) (buffer[(y)][(x)] = 1)
#define BUFKILLC(y, x) (buffer[(y)][(x)] = 0)

#endif
