/*
 * defs.h
 *
 * Copyright 1999 Thomas Aglassinger and others, see file "forum.txt"
 */

#include <exec/types.h>
#include <exec/ports.h>
#include <exec/semaphores.h>
#include <exec/libraries.h>
#include <dos/dos.h>
#include <dos/rdargs.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <exec/nodes.h>

#include <clib/exec_protos.h>
#include <clib/alib_protos.h>
#include <clib/dos_protos.h>

#include <string.h>
#include <ctype.h>

#ifdef _DCC
#define LibCall    __geta4 __regargs
#define Prototype  extern

#else

#define LibCall
#define Prototype

#define __A0
#define __A1
#define __A2
#define __A3
#define __A4
#define __A5
#define __A6
#define __A7

#define __D0
#define __D1
#define __D2
#define __D3
#define __D4
#define __D5
#define __D6
#define __D7

#endif /* _DCC */


#include "golded:developer/include/editor.h"
#include "golded:developer/syntax/include/scanlib.h"
#include "o/lib-protos.h"

extern const char LibName[];
extern const char LibId[];
