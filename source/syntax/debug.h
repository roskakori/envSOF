/*
 * debug.h
 *
 * Copyright 1999-2000 Thomas Aglassinger and others, see file "forum.txt"
 */

#ifndef DEBUG
#define DEBUG 0
#endif

#if DEBUG
#define D(x) x

#ifdef _DCC
#define bug(x) kputs(x)
#else /* _DCC */
#define bug(x) printf(x)
#endif /* _DCC */

#else /* DEBUG */

#define D(x)
#define bug(x)

#endif /* DEBUG */

#if DEBUG == 2
#define DD(x) DD((x))
#else
#define DD(x)
#endif /* DEBUG == 2 */

void init_debug(void);
void cleanup_debug(void);
void kint(int number);
void kdump(char *memory, size_t count);


