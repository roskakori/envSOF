
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

/* funcs.c              */

Prototype LibCall struct ParserData     *MountScanner(void);
Prototype LibCall struct ParserHandle   *StartScanner(__A0 struct GlobalConfig *, __A1 struct EditConfig *, __D0 struct SyntaxChunk *, __D1 struct SyntaxSetup *);
Prototype LibCall ULONG                  CloseScanner(__A0 struct ParserHandle *);
Prototype LibCall void                   FlushScanner(__A0 struct ParserHandle *);
Prototype LibCall void                   SetupScanner(__A0 struct GlobalConfig *);
Prototype LibCall struct RefreshRequest *BriefScanner(__A0 struct ParserHandle *, __A1 struct ScannerNotify *);
Prototype LibCall struct SyntaxChunk    *ParseLine   (__A0 struct ParserHandle *, __A1 struct LineNode *, __D0 ULONG);
Prototype LibCall void                   UnparseLines(__A0 struct LineNode *, __D0 ULONG);
Prototype LibCall void                   ParseSection(__A0 struct ParserHandle *, __A1 struct LineNode *, __D0 ULONG);
Prototype struct SyntaxChunk            *ParseString (UBYTE *, UWORD, struct ParserHandle *);
Prototype struct SyntaxChunk            *DupStack    (struct SyntaxChunk *);

/* language.c           */

Prototype void setup_char_array(void);
Prototype int fstrncmp(const char *s1, const char *s2, ULONG n);
Prototype int is_reserved_word(const char *word, ULONG word_start, ULONG word_end, struct keyword_info *info);
Prototype int is_reserved_name(const char *name, ULONG name_start, ULONG name_end, struct keyword_info *info);
Prototype int get_word_type(const char *text, ULONG word_start, ULONG word_end, struct keyword_info *info);
Prototype char *get_word_type_text(int word_type);

/* keyword.c            */

Prototype VOID create_keyword_list(VOID);
Prototype VOID dispose_keyword_info(struct keyword_info *info);
Prototype struct keyword_info *create_keyword_info(STRPTR name);
Prototype VOID dispose_keyword_list(VOID);
Prototype BOOL read_keyword_info(struct keyword_info *info, ULONG *line);
Prototype struct keyword_info *keywords_of(char *filename);

/* debug.c              */

Prototype void kputs(char *text);
Prototype void kint(int number);
