/*
 *
 * eiffel.parser -- Cached Eiffel syntax parser for GoldEd.
 *
 * Copyright 1999 Thomas Aglassinger
 *
 */
#define DEBUG 0

#define PARSER_NAME     "Eiffel"
#define PARSER_VERSION  "2.1"
#define PARSER_VERSION_ID 1

#include "defs.h"
#include "language.h"

/// "Header stuff"

// Buffer handles are allocated for each text buffer to keep track of ressources:

struct BufferHandle
{

	struct EditConfig *bh_EditConfig;	// pointer to text data

	struct GlobalConfig *bh_GlobalConfig;	// editor configuration

	struct SyntaxChunk *bh_SyntaxStack;		// parser output

	struct RefreshRequest bh_RefreshRequest;	// display refresh request

};

#define EMPTY_STACK ((struct SyntaxChunk *)~0)	// empty stack flag

extern char is_normal[256];
extern char is_alpha[256];
extern char is_numeric[256];
extern char is_upper[256];
extern char is_lower[256];

///
/// "Prototype"

// library functions

Prototype LibCall struct ParserData *MountScanner(void);
Prototype LibCall ULONG StartScanner(__A0 struct GlobalConfig *, __A1 struct EditConfig *, __D0 struct SyntaxChunk *);
Prototype LibCall ULONG CloseScanner(__D0 ULONG);
Prototype LibCall void FlushScanner(__D0 ULONG);
Prototype LibCall void SetupScanner(__A0 struct GlobalConfig *);
Prototype LibCall struct RefreshRequest *BriefScanner(__D0 ULONG, __A0 struct ScannerNotify *);
Prototype LibCall struct SyntaxChunk *ParseLine(__D0 ULONG, __A0 struct LineNode *, __D1 ULONG);
Prototype LibCall void UnparseLines(__A0 struct LineNode *, __D0 ULONG);
Prototype LibCall void ParseSection(__D0 ULONG, __A0 struct LineNode *, __D1 ULONG);

// private functions

Prototype struct SyntaxChunk *ParseString(UBYTE *, UWORD, ULONG);
Prototype struct SyntaxChunk *DupStack(struct SyntaxChunk *);

///

/// "Debugging stuff"
#if DEBUG
#define D(x) x
#define bug kputs

ULONG debugFH = NULL;
#include <clib/dos_protos.h>

#define DEBUG 0

// print debugging message; automatically opens a console
// NOTE: The console is never closed, thus keeping a lock and resources.
//       This is ugly, but I don't know how to use kprintf() in DICE.
//       Nevertheless you can close the console window by clicking its
//       close gadget.
void kputs(char *text)
{
	if (debugFH == NULL)
	{
		debugFH = Open("con:9999//200/700/Debug/AUTO/INACTIVE", 1006);
	}

	Write(debugFH, text, strlen(text));
}

// display integer in debugging console
void kint(int number)
{
	char *digits3 = "1234";
	digits3[0] = (number / 1000) % 10 + '0';
	digits3[1] = (number / 100) % 10 + '0';
	digits3[2] = (number / 10) % 10 + '0';
	digits3[3] = (number / 1) % 10 + '0';

	kputs(digits3);
}

#else
#define D(x)
#define bug
#endif

///
/// "Library functions"

/* ------------------------------- MountScanner --------------------------------
 *
 * Called by the editor before first usage of a scanner. Return a description of
 * our abilities.
 *
 */

LibCall struct ParserData *
  MountScanner()
{
	static UBYTE version[] = "$VER: " PARSER_NAME " " PARSER_VERSION " (" __COMMODORE_DATE__ ")";

	static struct ParserData parserData;

	// syntax elements understood by parser

	static UBYTE *levelNames[] =
	{

		"Standard text",
		"Comment",
		"Manifest string",
		"Keyword",
		"Reserved word",
		"Type name",
		"Constant name",
	/*"Routine declaration", */
		"Error or flaw",
	/*"Other name", *//* "Other name" is useless because of "Standard text" */
		NULL
	};

	static UBYTE *example[] =
	{
		"-- Simple Eiffel example.      ",
		"class                          ",
		"   HELLO                       ",
		"creation {ANY}                 ",
		"   make                        ",
		"feature {ANY}                  ",
		"   make is                     ",
		"      do                       ",
		"         print(\"hello%         ",
		"               % world\");      ",
		"         print('%N');          ",
		"      end; -- make             ",
		"   DontUseSuchNames: INTEGER;  ",
		"      -- only Java cunts do it ",
		"   Is_nice: BOOLEAN is True;   ",
		"end -- class HELLO             ",
		NULL
	};

	// color suggestions

	static ULONG levelColors[] =
	{

		MAKE_RGB4(0, 0, 0),		  /* SYNTAX_TEXT */
		MAKE_RGB4(0, 0, 0),		  /* SYNTAX_COMMENT */
		MAKE_RGB4(0, 0, 0),		  /* SYNTAX_STRING */
		MAKE_RGB4(15, 15, 15),	  /* SYNTAX_KEYWORD */
		MAKE_RGB4(0, 0, 0),		  /* SYNTAX_RESERVED_WORD */
		MAKE_RGB4(0, 0, 0),		  /* SYNTAX_TYPE */
		MAKE_RGB4(0, 0, 0),		  /* SYNTAX_CONSTANT */
		MAKE_RGB4(0, 0, 0),		  /* SYNTAX_DECLARATION */
		MAKE_RGB4(0, 0, 0)		  /* SYNTAX_BAD */
	};

	setup_char_array();

	parserData.pd_Release = SCANLIBVERSION;
	parserData.pd_Version = PARSER_VERSION_ID;
	parserData.pd_Serial = 0;
	parserData.pd_Info = PARSER_NAME " " PARSER_VERSION;
	parserData.pd_Levels = (sizeof(levelNames) / sizeof(UBYTE *)) - 1;
	parserData.pd_Names = levelNames;
	parserData.pd_Colors = levelColors;
	parserData.pd_Flags = SCPRF_SYNTAXCACHE;
	parserData.pd_Example = example;

	return (&parserData);
}

/* ------------------------------- StartScanner --------------------------------
 *
 * Called by the editor after a new text buffer has been created. We allocate a
 * buffer to hold text-specific data. The buffer address is returned as handle.
 *
 */

LibCall ULONG
  StartScanner(__A0 struct GlobalConfig * globalConfigPtr, __A1 struct EditConfig * editConfigPtr, __D0 struct SyntaxChunk * syntaxStack)
{
	struct BufferHandle *handle;

	if (handle = AllocVec(sizeof(struct BufferHandle), MEMF_PUBLIC | MEMF_CLEAR))
	{

		handle->bh_GlobalConfig = globalConfigPtr;
		handle->bh_EditConfig = editConfigPtr;
		handle->bh_SyntaxStack = syntaxStack;
	}

	return ((ULONG) handle);
}

/* ------------------------------- CloseScanner --------------------------------
 *
 * Called by the editor if a text buffer is about to be closed. Deallocate buffer
 * specific 'global' data.
 *
 */

LibCall ULONG
  CloseScanner(__D0 ULONG scanID)
{
	if (scanID)
	{

		struct BufferHandle *handle = (struct BufferHandle *) scanID;

		FreeVec(handle);
	}

	return (0);
}

/* ------------------------------- FlushScanner --------------------------------
 *
 * Called by the editor in low memory situations: we are supposed to free as much
 * memory as possible.
 *
 */

LibCall void
  FlushScanner(__D0 ULONG scanID)
{
	struct BufferHandle *handle;
	struct EditConfig *config;
	struct LineNode *lineNode;

	handle = (struct BufferHandle *) scanID;
	config = handle->bh_EditConfig;

	if (lineNode = config->TextNodes)
		UnparseLines(lineNode, config->Lines);
}

/* ------------------------------- SetupScanner --------------------------------
 *
 * Called by the editor if the user wants to change the scanner's configuration.
 * We do not support user configuration (parserData.pd_Flags: SCPRF_CONFIGWIN flag
 * unset).
 *
 */

LibCall void
  SetupScanner(__A0 globalConfigPtr)
{
	;
}

/* ------------------------------- BriefScanner --------------------------------
 *
 * Called to notify a context scanner if lines have been added, deleted or
 * modified. We aren't a context scanner (parserData.pd_Flags: SCPRF_CONTEXT
 * flag unset), so we won't ever have to request additional display requests:
 * the editor's built-in refresh of damage regions is sufficient.
 *
 */

LibCall struct RefreshRequest *
  BriefScanner(__D0 ULONG scanID, __A0 struct ScannerNotify *notify)
{
	return (NULL);
}

/* --------------------------------- ParseLine ---------------------------------
 *
 * Parse a line, build a syntax description
 *
 */

LibCall struct SyntaxChunk *
  ParseLine(__D0 ULONG scanID, __A0 struct LineNode *lineNode, __D1 ULONG line)
{
	if (IS_FOLD(lineNode))

		return (NULL);

	else if (lineNode->Len)
	{

		// line not yet parsed ?

		if (lineNode->UserData == NULL)
		{

			struct SyntaxChunk *syntaxStack = ParseString(lineNode->Text, lineNode->Len, scanID);

			if (syntaxStack == EMPTY_STACK)
				lineNode->UserData = EMPTY_STACK;
			else
				lineNode->UserData = DupStack(syntaxStack);
		}

		if (lineNode->UserData == EMPTY_STACK)
			return ((struct SyntaxChunk *) NULL);
		else
			return ((struct SyntaxChunk *) lineNode->UserData);
	}
	else
		return (NULL);
}

/* -------------------------------- UnparseLines -------------------------------
 *
 * Called by the editor if lines are to be deleted. We are supposed to free
 * private data attached to the lines.
 *
 */

LibCall void
  UnparseLines(__A0 struct LineNode *lineNode, __D0 ULONG lines)
{
	while (lines--)
	{

		// free syntax cache

		if (lineNode->UserData)
		{

			if (lineNode->UserData != (APTR) EMPTY_STACK)
				FreeVec((APTR) lineNode->UserData);

			lineNode->UserData = NULL;
		}

		// free folded subblock

		if (IS_FOLD(lineNode))
		{

			struct Fold *fold = (struct Fold *) lineNode->SpecialInfo;

			UnparseLines(fold->TextNodes, fold->Lines);
		}

		++lineNode;
	}
}

/* -------------------------------- ParseSection -------------------------------
 *
 * Called by the editor if lines are to be displayed. The scanner is encouraged to
 * preparse the lines.
 *
 */

LibCall void
  ParseSection(__D0 ULONG scanID, __A0 struct LineNode *lineNode, __D1 ULONG lines)
{
	while (lines--)
	{

		// fold headers have to be ignored

		if (IS_FOLD(lineNode) == FALSE)
		{

			// line not yet parsed ?

			if (lineNode->Len)
				if (lineNode->UserData == NULL)
					lineNode->UserData = DupStack(ParseString(lineNode->Text, lineNode->Len, scanID));
		}

		++lineNode;
	}
}

///
/// "private"

/* -------------------------------- getNameType ---------------------------- */
UBYTE
getNameType(UBYTE * text, ULONG begin, ULONG end)
{
#if 1
	UBYTE word[500];
	LONG word_index = begin;

	while (word_index <= end)
	{
		word[word_index - begin] = text[word_index];
		word_index += 1;
	}
	word[word_index - begin] = 0;

	D(bug("  check word: \""));
	D(bug(word));
	D(bug("\"\n"));
#endif

#if 1
	return get_word_type(text, begin, end + 1);
#else
	return SYNTAX_KEYWORD;
#endif
}

/* -------------------------------- ParseString --------------------------------
 *
 * Parse a string, build a syntax description. Return EMPTY_STACK in case there
 * is nothing to highlight.
 *
 */

struct SyntaxChunk *
  ParseString(UBYTE * text, UWORD len, ULONG scanID)
{
#define ADD_ELEMENT(start,end,level)           \
{                                              \
    syntaxStack[element].sc_Start = (start);   \
    syntaxStack[element].sc_End   = (end);     \
    syntaxStack[element].sc_Level = (level);   \
    element += 1;                              \
    D(bug("    elem: "));                      \
    D(kint(start));                            \
    D(bug(" "));                               \
    D(kint(end));                              \
    D(bug(" "));                               \
    D(kint(level));                            \
    D(bug("\n"));                              \
}

	UBYTE *lineStart = text;
	UWORD lenStart = len;

	if (len)
	{

		struct SyntaxChunk *syntaxStack = ((struct BufferHandle *) scanID)->bh_SyntaxStack;

		UWORD element, indent;
		UWORD stringStart, stringEnd;
		UWORD nameStart, nameEnd;
		// if in string/character, this is either (") or ('), otherwise 0
		UBYTE inString = 0;
		// if in name, this is the first character of the name, otherwise 0
		UBYTE inName = 0;
		BOOL inComment = FALSE;
		BOOL afterPercent = FALSE;

		// leading and trailing spaces have to be ignored

		for (indent = 0; (len && (*text <= 32)); --len, ++indent)
			++text;

		while (len && (text[len - 1] <= 32))
			--len;

		// no syntax elements found so far

		element = 0;

		if (*text == '%')
		{

			inString = '"';
			afterPercent = TRUE;
			stringStart = indent;
		}

		for (inComment = FALSE; len >= 1; ++text, ++indent, --len)
		{

			if (inName)
			{

				if (!is_normal[*text])
				{
					int nameType;
					UBYTE *ends_with = " ";
					ends_with[0] = *text;

					// end of new name
					D(bug("  Name ends with \""));
					D(bug(ends_with));
					D(bug("\"\n"));

					// go back one char so it is parsed again
					text -= 1;
					indent -= 1;
					len += 1;
					ends_with[0] = *text;
					D(bug("  Now at \""));
					D(bug(ends_with));
					D(bug("\"\n"));

					nameEnd = indent;
					nameType = getNameType(lineStart, nameStart, nameEnd);

					if (nameType != SYNTAX_TEXT)
					{
						ADD_ELEMENT(nameStart, nameEnd, nameType);
					}
					inName = 0;
				}
			}
			else if (inString)
			{

				if (afterPercent)
				{

					// skip escaped character
					afterPercent = FALSE;

				}
				else if (*text == inString)
				{

					// end of string detected

					D(bug("  String end: "));

					stringEnd = indent - 1;
					ADD_ELEMENT(stringStart, stringEnd, SYNTAX_STRING);
					inString = 0;
					afterPercent = FALSE;

					D(kint(stringStart));
					D(bug(" - "));
					D(kint(stringEnd));
					D(bug("\n"));

				}
				else if (*text == '%')
				{

					afterPercent = TRUE;
				}
			}
			else if ((*text == 34) || (*text == 39))
			{

				D(bug("  String start\n"));

				// start of string detected
				inString = *text;
				afterPercent = FALSE;
				stringStart = indent + 1;

			}
			else if ((len >= 2) && (text[0] == '-') && (text[1] == '-'))
			{

				// comment until end of line
				ADD_ELEMENT(indent, indent + len - 1, SYNTAX_COMMENT);
				break;
			}
			else if (is_alpha[*text])
			{

				// Start of a new name */
				D(bug("  Name start\n"));

				// start of name detected
				inName = *text;
				afterPercent = FALSE;
				nameStart = indent;
			}
			else
			{
				UBYTE *ends_with = " ";
				ends_with[0] = *text;

				// end of new name
				D(bug("ignore \""));
				D(bug(ends_with));
				D(bug("\"\n"));
			}
		}

		// go back one char so the last char of the line is addressed
		text -= 1;
		indent -= 1;
		len += 1;

		// unterminated string or name? highlight rest of line

		if (inString)
		{

			UBYTE level = SYNTAX_STRING;
			stringEnd = indent + len - 1;

			if (*text != '%')
			{
				stringEnd += 0;
				level = SYNTAX_BAD;
			}

			ADD_ELEMENT(stringStart, stringEnd, level);
		}
		else if (inName)
		{
			UBYTE *buf = "  ";

			buf[0] = text[-1];
			buf[1] = text[0];
			D(bug("last word ends: \""));
			D(bug(buf));
			D(bug("\"\n"));

			if (!is_normal[*text])
			{
				D(bug("  (decreased)\n"));
				len -= 1;
			}

			nameEnd = indent + len - 1;
			ADD_ELEMENT(nameStart, nameEnd,
							getNameType(lineStart, nameStart, nameEnd));
		}

		if (element)
		{

			// check, if last element is "is". If so, first element is
			// a feature declaration
#if 0
			struct SyntaxChunk *firstElement = &(syntaxStack[0]);
			struct SyntaxChunk *lastElement = &(syntaxStack[element - 1]);

			if ((lastElement->sc_Level == SYNTAX_KEYWORD)
				 && ((lastElement->sc_End - lastElement->sc_Start) == 2)
				)
			{
				firstElement->sc_Level = SYNTAX_DECLARATION;
			}
#endif

			// terminate syntax stack
			ADD_ELEMENT(FALSE, FALSE, FALSE);
			return (syntaxStack);
		}
		else
			return (EMPTY_STACK);

	}
	else
		return (EMPTY_STACK);
}

/* --------------------------------- DupStack ----------------------------------
 *
 * Duplicate syntax stack (to be FreeVec'ed). Return NULL in case of failure.
 *
 */

struct SyntaxChunk *
  DupStack(syntaxStack)

	  struct SyntaxChunk *syntaxStack;
{
	if (syntaxStack && (syntaxStack != EMPTY_STACK))
	{

		struct SyntaxChunk *chunk;
		UWORD elements;

		// determine stack size

		for (elements = 0, chunk = syntaxStack; chunk->sc_Level; ++chunk)
			++elements;

		// create copy of syntax stack (to be attached to a text line by the caller)

		if (elements)
		{

			ULONG size = (++elements) * sizeof(struct SyntaxChunk);

			chunk = syntaxStack;

			if (syntaxStack = AllocVec(size, MEMF_PUBLIC))
				movmem(chunk, syntaxStack, size);
		}
		else
			syntaxStack = EMPTY_STACK;
	}

	return (syntaxStack);
}

///
