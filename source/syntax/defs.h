#include <exec/types.h>
#include <exec/ports.h>
#include <exec/semaphores.h>
#include <exec/libraries.h>
#include <dos/dos.h>
#include <exec/memory.h>
#include <rexx/errors.h>
#include <rexx/storage.h>
#include <clib/exec_protos.h>
#include <clib/dos_protos.h>
#include <clib/alib_protos.h>
#include <clib/rexxsyslib_protos.h>
#include <string.h>
#include <ctype.h>

#define LibCall    __geta4 __regargs
#define Prototype  extern

/* editor-related includes */

#include "golded:developer/include/editor.h"
#include "golded:developer/syntax/include/scanlib.h"
#include "golded:developer/registry/include/registry.h"

/* prototypes pools.lib */

Prototype void AsmDeletePool (__A0 APTR,  __A6 struct Library *);
Prototype APTR AsmAllocPooled(__A0 APTR,  __D0 ULONG, __A6 struct Library *);
Prototype APTR AsmCreatePool (__D0 ULONG, __D1 ULONG, __D2 ULONG, __A6 struct Library *);
Prototype void AsmFreePooled (__A0 APTR,  __A1 APTR,  __D0 ULONG, __A6 struct Library *);

#include "o/lib-protos.h"

extern const char LibName[];
extern const char LibId[];

extern struct Library *SysBase;
extern struct DosLibrary *DOSBase;

/* globals */

extern struct SignalSemaphore MemorySemaphore;
extern APTR                   MemoryPool;
