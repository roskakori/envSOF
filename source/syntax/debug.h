/*
 * debug.h
 *
 * Copyright 1999 Thomas Aglassinger and others, see file "forum.txt"
 */

#ifndef DEBUG
#define DEBUG 1
#endif

#if DEBUG

#define D(x)

#ifdef _DCC
#define bug(x) kputs
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
