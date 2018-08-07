#ifndef LMACROS_H
#define LMACROS_H

#define LIVE (' '|A_REVERSE) // Space + A_REVERSE produces the opaque block which is the default cursor for many terminal emulators
#define DEAD ' '

#define INFOCOLS 35
#define INFOLINES LINES

#define CELLCOUNT (lifecols * lifelines)
#define GRIDSIZE (CELLCOUNT * sizeof(int))

/* positive modulus */
#define MOD(a, b) (((a)+(b)) % (b))

#endif
