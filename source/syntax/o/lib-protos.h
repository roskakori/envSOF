
/* MACHINE GENERATED */


/* tag.a                */

Prototype ALibExpunge(), ALibClose(), ALibOpen(), ALibReserved();

/* init.c               */

Prototype BOOL InitC(void);
Prototype void ExitC(void);

/* lib.c                */

Prototype LibCall struct Library *LibInit   (__D0 BPTR);
Prototype LibCall struct Library *LibOpen   (__D0 long, __A0 struct Library *);
Prototype LibCall long            LibClose  (__A0 struct Library *);
Prototype LibCall long            LibExpunge(__A0 struct Library *);

/* debug.c              */

Prototype void kputs(char *text);
Prototype void kint(int number);
Prototype void kputptr(void *pointer);

/* funcs.c              */

Prototype LibCall struct ParserData *MountScanner(void);
Prototype LibCall struct ParserHandle *StartScanner(__A0 struct GlobalConfig *, __A1 struct EditConfig *, __D0 struct SyntaxChunk *, __D1 struct SyntaxSetup *);
Prototype LibCall ULONG CloseScanner(__A0 struct ParserHandle *);
Prototype LibCall void FlushScanner(__A0 struct ParserHandle *);
Prototype LibCall void SetupScanner(__A0 struct GlobalConfig *);
Prototype LibCall struct RefreshRequest *BriefScanner(__A0 struct ParserHandle *, __A1 struct ScannerNotify *);
Prototype LibCall struct SyntaxChunk *ParseLine(__A0 struct ParserHandle *, __A1 struct LineNode *, __D0 ULONG);
Prototype LibCall void UnparseLines(__A0 struct LineNode *, __D0 ULONG);
Prototype LibCall void ParseSection(__A0 struct ParserHandle *, __A1 struct LineNode *, __D0 ULONG);
Prototype APTR AllocVecPooled(ULONG, ULONG);
Prototype BOOL BadSuccessor(ULONG, ULONG);
Prototype struct SyntaxInfo *DupInfo(struct SyntaxInfo *);
Prototype UBYTE **ExamineDictionary(struct MyParserHandle *, struct SyntaxSetup *);
Prototype struct LineNode *FindContextStart(struct ParserHandle *, struct LineNode *);
Prototype void FreeVecPooled(APTR);
Prototype UWORD Hashcode(UBYTE *, UWORD);
Prototype struct Entry *IsKeyword(struct MyParserHandle *, UBYTE *, UWORD);
Prototype void ParseContext(struct ParserHandle *, struct LineNode *, ULONG);
Prototype struct SyntaxInfo *ParseContextLine(struct ParserHandle *, struct LineNode *, ULONG);
Prototype struct RefreshRequest *VerifyContext(struct ParserHandle *, ULONG, ULONG);
Prototype LONG parse_arguments(struct MyParserHandle *handle, struct SyntaxSetup *setup);
Prototype BOOL is_spelled_correctly(struct MyParserHandle * handle, UBYTE * text, UWORD start, UWORD end);
Prototype void internal_assert(int expression, char *expression_text, char *file, int line);
