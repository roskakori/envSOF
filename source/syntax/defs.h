#include <exec/types.h>
#include <exec/ports.h>
#include <exec/semaphores.h>
#include <exec/libraries.h>
#include <dos/dos.h>
#include <exec/memory.h>
#include <clib/exec_protos.h>
#include <clib/alib_protos.h>
#include <string.h>
#include <ctype.h>

#define LibCall    __geta4 __regargs
#define Prototype  extern

#include "golded:developer/include/editor.h"
#include "golded:developer/syntax/include/scanlib.h"
#include "o/lib-protos.h"

extern const char LibName[];
extern const char LibId[];
