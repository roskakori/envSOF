
/* MACHINE GENERATED */


/* tag.a                */

Prototype ALibExpunge(), ALibClose(), ALibOpen(), ALibReserved();

/* init.c               */

Prototype void InitC(void);

/* lib.c                */

Prototype LibCall struct Library *LibInit   (__D0 BPTR);
Prototype LibCall struct Library *LibOpen   (__D0 long, __A0 struct Library *);
Prototype LibCall long            LibClose  (__A0 struct Library *);
Prototype LibCall long            LibExpunge(__A0 struct Library *);

/* funcs.c              */

Prototype LibCall struct ParserData     *MountScanner(void);
Prototype LibCall ULONG                  StartScanner(__A0 struct GlobalConfig *, __A1 struct EditConfig *, __D0 struct SyntaxChunk *);
Prototype LibCall ULONG                  CloseScanner(__D0 ULONG);
Prototype LibCall void                   FlushScanner(__D0 ULONG);
Prototype LibCall void                   SetupScanner(__A0 struct GlobalConfig  *);
Prototype LibCall struct RefreshRequest *BriefScanner(__D0 ULONG, __A0 struct ScannerNotify *);
Prototype LibCall struct SyntaxChunk    *ParseLine   (__D0 ULONG, __A0 struct LineNode *, __D1 ULONG);
Prototype LibCall void                   UnparseLines(__A0 struct LineNode *, __D0 ULONG);
Prototype LibCall void                   ParseSection(__D0 ULONG, __A0 struct LineNode *, __D1 ULONG);
Prototype struct SyntaxChunk *ParseString(UBYTE *, UWORD, ULONG);
Prototype struct SyntaxChunk *DupStack(struct SyntaxChunk *);

/* language.c           */

