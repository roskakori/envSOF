/*
 *  LIB.C
 *
 *  Basic Library Resource Handling
 *
 *  NOTE: all data declarations should be initialized since we skip
 *        normal C startup code (unless initial value is don't care)
 *
 *  WARNING: arguments are passed in certain registers from the assembly
 *        tag file, matched to how they are declared below.  Do not change
 *        the argument declarations!
 */

#include "defs.h"

Prototype LibCall struct Library *LibInit   (__D0 BPTR);
Prototype LibCall struct Library *LibOpen   (__D0 long, __A0 struct Library *);
Prototype LibCall long            LibClose  (__A0 struct Library *);
Prototype LibCall long            LibExpunge(__A0 struct Library *);

struct Library *LibBase = NULL;
struct Library *SysBase = NULL;
struct Library *DOSBase = NULL;
struct Library *RexxSysBase = NULL;

BPTR SegList  = 0;

/*
 *    The Initialization routine is given only a seglist pointer.  Since
 *    we are NOT AUTOINIT we must construct and add the library ourselves
 *    and return either NULL or the library pointer.  Exec has Forbid()
 *    for us during the call.
 *
 *    We use an extended library structure to allow identification as a
 *    GoldED syntax scanner.
 */

LibCall struct Library *
LibInit(__D0 BPTR segment)
{
   struct Library *lib;

   static const long Vectors[] =
   {

      (long) ALibOpen,
      (long) ALibClose,
      (long) ALibExpunge,
      (long) ALibReserved,

      (long) MountScanner,
      (long) StartScanner,
      (long) CloseScanner,
      (long) FlushScanner,
      (long) SetupScanner,
      (long) BriefScanner,
      (long) ParseLine,
      (long) UnparseLines,
      (long) ParseSection,
      -1
   };

   SysBase = *(struct Library **) 4;

   if (DOSBase = OpenLibrary("dos.library", 0)) {
      if (RexxSysBase = OpenLibrary("rexxsyslib.library", 0)) {

         if (LibBase = lib = MakeLibrary((APTR) Vectors, NULL, NULL, sizeof(struct ParserBase), (BPTR) NULL)) {

            lib->lib_Node.ln_Type = NT_LIBRARY;
            lib->lib_Node.ln_Name = LibName;
            lib->lib_Flags = LIBF_CHANGED | LIBF_SUMUSED;
            lib->lib_Version = 38;
            lib->lib_Revision = 0;
            lib->lib_IdString = (APTR) LibId;

            ((struct ParserBase *) lib)->Magic = PARSER_MAGIC;

            SegList = segment;

            AddLibrary(lib);

            if (InitC())
               return (lib);

            Remove(&lib->lib_Node);

            FreeMem((char *) lib - lib->lib_NegSize, lib->lib_NegSize + lib->lib_PosSize);
         }
         CloseLibrary(RexxSysBase);
         RexxSysBase = NULL;
      }
      CloseLibrary(DOSBase);
      DOSBase = NULL;
   }
   return (NULL);
}

/*
 *    Open is given the library pointer and the version request.  Either
 *    return the library pointer or NULL.  Remove the DELAYED-EXPUNGE flag.
 *    Exec has Forbid() for us during the call.
 */

LibCall struct Library *
LibOpen(__D0 long version, __A0 struct Library *lib)
{
    ++lib->lib_OpenCnt;

    lib->lib_Flags &= ~LIBF_DELEXP;

    return(lib);
}

/*
 *    Close is given the library pointer and the version request.  Be sure
 *    not to decrement the open count if already zero.  If the open count
 *    is or becomes zero AND there is a LIBF_DELEXP, we expunge the library
 *    and return the seglist.  Otherwise we return NULL.
 *
 *    Note that this routine never sets LIBF_DELEXP on its own.
 *
 *    Exec has Forbid() for us during the call.
 */

LibCall long
LibClose(__A0 struct Library *lib)
{
    if (lib->lib_OpenCnt && --lib->lib_OpenCnt)
        return(NULL);

    if (lib->lib_Flags & LIBF_DELEXP)
        return(LibExpunge(lib));

    return(NULL);
}

/*
 *    We expunge the library and return the Seglist ONLY if the open count
 *    is zero.  If the open count is not zero we set the DELAYED-EXPUNGE
 *    flag and return NULL.
 *
 *    Exec has Forbid() for us during the call.  NOTE ALSO that Expunge
 *    might be called from the memory allocator and thus we CANNOT DO A
 *    Wait() or otherwise take a long time to complete (straight from RKM).
 *
 *    Apparently RemLibrary(lib) calls our expunge routine and would
 *    therefore freeze if we called it ourselves.  As far as I can tell
 *    from RKM, LibExpunge(lib) must remove the library itself as shown
 *    below.
 */

LibCall long
LibExpunge(__A0 struct Library *lib)
{
   if (lib->lib_OpenCnt) {

      lib->lib_Flags |= LIBF_DELEXP;
      return (NULL);
   }
   ExitC();

   Remove(&lib->lib_Node);

   FreeMem((char *) lib - lib->lib_NegSize, lib->lib_NegSize + lib->lib_PosSize);

   if (DOSBase) {
      CloseLibrary(DOSBase);
      DOSBase = NULL;
   }
   if (RexxSysBase) {
      CloseLibrary(RexxSysBase);
      RexxSysBase = NULL;
   }
   return ((long) SegList);
}

