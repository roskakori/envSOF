#include "defs.h"

/// "prototypes"

Prototype BOOL InitC(void);
Prototype void ExitC(void);

///
/// "global variables"

struct SignalSemaphore MemorySemaphore = { 0 };
APTR                   MemoryPool      = NULL;

///
/// "init"

/* ----------------------------------- InitC -----------------------------------

 Library startup code (C entry point).

*/

BOOL
InitC(void)
{
    InitSemaphore(&MemorySemaphore);

    if (MemoryPool = AsmCreatePool(MEMF_ANY | MEMF_PUBLIC, 4096, 4096, SysBase))
        return(TRUE);
    else
        return(FALSE);

}

///
/// "exit"

void
ExitC(void)
{
    if (MemoryPool) {

        AsmDeletePool(MemoryPool, SysBase);

        MemoryPool = NULL;
    }
}


///
