/*
 * funcs.c -- Cached Eiffel syntax parser for GoldEd Studio 6.
 *
 * Copyright 1999 Thomas Aglassinger and others, see file "forum.txt"
 */
#define DEBUG 0

#define PARSER_NAME     "Eiffel"
#define PARSER_VERSION  "2.3"
#define PARSER_VERSION_ID 1

#include "defs.h"
#include "debug.h"
#include "language.h"
#include "keyword.h"

/// "Header stuff"

// we extend the ParserHandle structure for our private data

struct MyParserHandle {

   struct ParserHandle ParserHandle;   // embedded parser handle

   struct SyntaxChunk *SyntaxStack;    // parser output

   struct EditConfig *EditConfig;   // buffer data

   struct keyword_info *keyword_info;  // keywords to be recognised

};

#define EMPTY_STACK ((struct SyntaxChunk *)~0)  // empty stack flag

extern char is_normal[256];
extern char is_alpha[256];
extern char is_numeric[256];
extern char is_upper[256];
extern char is_lower[256];

// names of built-in syntax levels

const static UBYTE *levelNames[SYNTAX_MAXIMUM + 1] =
{
   "Standard text",
   "Comment",
   "Manifest string",
   "Keyword",
   "Reserved word",
   "Type name",
   "Constant name",
   // "Routine declaration",
   "Error or flaw",
   NULL
};

///

/// "Prototype"

// library functions

Prototype LibCall struct ParserData     *MountScanner(void);
Prototype LibCall struct ParserHandle   *StartScanner(__A0 struct GlobalConfig *, __A1 struct EditConfig *, __D0 struct SyntaxChunk *, __D1 struct SyntaxSetup *);
Prototype LibCall ULONG                  CloseScanner(__A0 struct ParserHandle *);
Prototype LibCall void                   FlushScanner(__A0 struct ParserHandle *);
Prototype LibCall void                   SetupScanner(__A0 struct GlobalConfig *);
Prototype LibCall struct RefreshRequest *BriefScanner(__A0 struct ParserHandle *, __A1 struct ScannerNotify *);
Prototype LibCall struct SyntaxChunk    *ParseLine   (__A0 struct ParserHandle *, __A1 struct LineNode *, __D0 ULONG);
Prototype LibCall void                   UnparseLines(__A0 struct LineNode *, __D0 ULONG);
Prototype LibCall void                   ParseSection(__A0 struct ParserHandle *, __A1 struct LineNode *, __D0 ULONG);

// private functions

Prototype struct SyntaxChunk            *ParseString (UBYTE *, UWORD, struct ParserHandle *);
Prototype struct SyntaxChunk            *DupStack    (struct SyntaxChunk *);

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

   const static UBYTE *example[] =
   {
      "indexing                       ",
      "  description: \"Say hello.\"    ",
      "                               ",
      "class                          ",
      "   HELLO                       ",
      "                               ",
      "creation {ANY}                 ",
      "   make                        ",
      "                               ",
      "feature {ANY}                  ",
      "                               ",
      "   make is                     ",
      "      do                       ",
      "         print(\"hello%         ",
      "               % world\")       ",
      "         print('%N')           ",
      "      end -- make              ",
      "                               ",
      "   Almost_pi: REAL is 3.1415   ",
      "                               ",
      "   Is_nice: BOOLEAN is True;   ",
      "                               ",
      "   DontUseSuchNames: LOOSER    ",
      "      -- only Java cunts do it ",
      "                               ",
      "end -- class HELLO             ",
      NULL
   };

   setup_char_array();
   create_keyword_list();

   parserData.pd_Release = SCANLIBVERSION;
   parserData.pd_Version = 1;
   parserData.pd_Serial = 0;
   parserData.pd_Info = PARSER_NAME " " PARSER_VERSION;
   parserData.pd_Example = example;
   parserData.pd_Properties = 0;

   // signal editor that we cache syntax information

   parserData.pd_Flags = SCPRF_SYNTAXCACHE;

   return (&parserData);
}

/* ------------------------------- StartScanner --------------------------------
 *
 * Called by the editor after a new text buffer has been created. We allocate a
 * buffer to hold text-specific data. The buffer address is returned as handle.
 *
 */

/* ------------------------------- StartScanner --------------------------------
 *
 * Called by the editor after a new text buffer has been created. We return a
 * parser handle (extended with our private data).
 *
 */

LibCall struct ParserHandle *
StartScanner(__A0 struct GlobalConfig *globalConfigPtr, __A1 struct EditConfig *editConfigPtr, __D0 struct SyntaxChunk *syntaxStack, __D1 struct SyntaxSetup *syntaxSetup)
{
#define TEMPLATE "FILE"
   struct MyParserHandle *handle = NULL;
   UBYTE *settings_argument = syntaxSetup->sp_UserData;  // parser argument string

   ULONG length = 0;            // length of parser argument string

   STRPTR scan_buffer;          // contains copy of parser argument string with "\n"

   static enum {
      ARG_FILE, ARG_MAX
   };
   LONG argument[ARG_MAX] =
   {NULL};
   argument[ARG_FILE] = (LONG) "golded:add-ons/eiffel/syntax/eiffel.keyword";

   // Copy parser arguments to buffer and append "\n"
   if (settings_argument != NULL) {
      length = strlen(settings_argument);
   }
   scan_buffer = AllocVec(length + 2, MEMF_ANY);
   if (scan_buffer != NULL) {
      if (length > 0) {
         strcpy(scan_buffer, settings_argument);
      }
      scan_buffer[length] = '\n';
      scan_buffer[length + 1] = '\0';
   }
   if (scan_buffer != NULL) {
      struct RDArgs *scanner = (struct RDArgs *) AllocDosObject(DOS_RDARGS, NULL);

      if (scanner != NULL) {
         scanner->RDA_Source.CS_Buffer = scan_buffer;
         scanner->RDA_Source.CS_Length = (LONG) strlen(scan_buffer);

         if (ReadArgs(TEMPLATE, argument, scanner)) {

            if (handle = AllocVec(sizeof(struct MyParserHandle), MEMF_PUBLIC | MEMF_CLEAR)) {

               handle->keyword_info = keywords_of((STRPTR) argument[ARG_FILE]);

               if (handle->keyword_info != NULL) {
                  handle->ParserHandle.ph_Levels = SYNTAX_MAXIMUM;
                  handle->ParserHandle.ph_Names = levelNames;
                  handle->ParserHandle.ph_ColorsFG = NULL;
                  handle->ParserHandle.ph_ColorsBG = NULL;
                  handle->SyntaxStack = syntaxStack;
                  handle->EditConfig = editConfigPtr;
               } else {
                  FreeVec(handle);
                  handle = NULL;
               }
            }
            FreeArgs(scanner);
         }
         FreeDosObject(DOS_RDARGS, scanner);
      }
      FreeVec(scan_buffer);
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
CloseScanner(__A0 struct ParserHandle * handle)
{
   D(bug("CloseScanner()\n"));
   if (handle) {

      struct MyParserHandle *my_handle = (struct *MyParserHandle) *handle;

      dispose_keyword_info(my_handle->keyword_info);

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
FlushScanner(__A0 struct ParserHandle *handle)
{
   D(bug("FlushScanner()\n"));
   struct EditConfig *config = ((struct MyParserHandle *) handle)->EditConfig;

   if (config->TextNodes && config->Lines) {
      UnparseLines(config->TextNodes, config->Lines);
   }
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
BriefScanner(__A0 struct ParserHandle *handle, __A1 struct ScannerNotify *notify)
{
   return (NULL);
}

/* --------------------------------- ParseLine ---------------------------------
 *
 * Parse a line, build a syntax description
 *
 */

LibCall struct SyntaxChunk *
ParseLine(__A0 struct ParserHandle *handle, __A1 struct LineNode *lineNode, __D0 ULONG line)
{
   if (IS_FOLD(lineNode))
      return (NULL);

   else if (lineNode->Len) {

      // line not yet parsed ?

      if (lineNode->UserData == NULL) {

         struct SyntaxChunk *syntaxStack = ParseString(lineNode->Text, lineNode->Len, handle);

         if (syntaxStack == EMPTY_STACK)
            lineNode->UserData = EMPTY_STACK;
         else
            lineNode->UserData = DupStack(syntaxStack);
      }
      if (lineNode->UserData == EMPTY_STACK)
         return ((struct SyntaxChunk *) NULL);
      else
         return ((struct SyntaxChunk *) lineNode->UserData);
   } else
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
   while (lines--) {

      // free syntax cache

      if (lineNode->UserData) {

         if (lineNode->UserData != (APTR) EMPTY_STACK)
            FreeVec((APTR) lineNode->UserData);

         lineNode->UserData = NULL;
      }
      // free folded subblock

      if (IS_FOLD(lineNode)) {

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
ParseSection(__A0 struct ParserHandle *handle, __A1 struct LineNode *lineNode, __D0 ULONG lines)
{
#if 0
   while (lines--) {

      // fold headers have to be ignored

      if (IS_FOLD(lineNode) == FALSE) {

         // line not yet parsed ?

         if (lineNode->Len)
            if (lineNode->UserData == NULL)
               lineNode->UserData = DupStack(ParseString(lineNode->Text, lineNode->Len, handle));
      }
      ++lineNode;
   }
#endif
}

///

/// "private"

/* -------------------------------- getNameType ---------------------------- */
UBYTE
getNameType(UBYTE * text, ULONG begin, ULONG end, struct keyword_info *info)
{
#if 1
   UBYTE word[500];
   LONG word_index = begin;

   while (word_index <= end) {
      word[word_index - begin] = text[word_index];
      word_index += 1;
   }
   word[word_index - begin] = 0;

   D(bug("  check word: \""));
   D(bug(word));
   D(bug("\"\n"));
#endif

#if 1
   return get_word_type(text, begin, end + 1, info);
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
ParseString(UBYTE * text, UWORD len, struct ParserHandle *handle)
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

   if (len) {
      struct MyParserHandle *my_handle = (struct MyParserHandle *) handle;
      struct SyntaxChunk *syntaxStack = my_handle->SyntaxStack;
      struct keyword_info *info = my_handle->keyword_info;

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

      if (*text == '%') {

         inString = '"';
         afterPercent = TRUE;
         stringStart = indent;
      }
      for (inComment = FALSE; len >= 1; ++text, ++indent, --len) {

         if (inName) {

            if (!is_normal[*text]) {
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
               nameType = getNameType(lineStart, nameStart, nameEnd, info);

               if (nameType != SYNTAX_TEXT) {
                  ADD_ELEMENT(nameStart, nameEnd, nameType);
               }
               inName = 0;
            }
         } else if (inString) {

            if (afterPercent) {

               // skip escaped character
               afterPercent = FALSE;

            } else if (*text == inString) {

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

            } else if (*text == '%') {

               afterPercent = TRUE;
            }
         } else if ((*text == 34) || (*text == 39)) {

            D(bug("  String start\n"));

            // start of string detected
            inString = *text;
            afterPercent = FALSE;
            stringStart = indent + 1;

         } else if ((len >= 2) && (text[0] == '-') && (text[1] == '-')) {

            // comment until end of line
            ADD_ELEMENT(indent, indent + len - 1, SYNTAX_COMMENT);
            break;
         } else if (is_alpha[*text]) {

            // Start of a new name */
            D(bug("  Name start\n"));

            // start of name detected
            inName = *text;
            afterPercent = FALSE;
            nameStart = indent;
         } else {
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

      if (inString) {

         UBYTE level = SYNTAX_STRING;
         stringEnd = indent + len - 1;

         if (*text != '%') {
            stringEnd += 0;
            level = SYNTAX_BAD;
         }
         ADD_ELEMENT(stringStart, stringEnd, level);
      } else if (inName) {
         UBYTE *buf = "  ";

         buf[0] = text[-1];
         buf[1] = text[0];
         D(bug("last word ends: \""));
         D(bug(buf));
         D(bug("\"\n"));

         if (!is_normal[*text]) {
            D(bug("  (decreased)\n"));
            len -= 1;
         }
         nameEnd = indent + len - 1;
         ADD_ELEMENT(nameStart, nameEnd,
                     getNameType(lineStart, nameStart, nameEnd, info));
      }
      if (element) {

         // check, if last element is "is". If so, first element is
         // a feature declaration
#if 0
         struct SyntaxChunk *firstElement = &(syntaxStack[0]);
         struct SyntaxChunk *lastElement = &(syntaxStack[element - 1]);

         if ((lastElement->sc_Level == SYNTAX_KEYWORD)
             && ((lastElement->sc_End - lastElement->sc_Start) == 2)
            ) {
            firstElement->sc_Level = SYNTAX_DECLARATION;
         }
#endif

         // terminate syntax stack
         ADD_ELEMENT(FALSE, FALSE, FALSE);
         return (syntaxStack);
      } else
         return (EMPTY_STACK);

   } else
      return (EMPTY_STACK);
}

/* --------------------------------- DupStack ----------------------------------
 *
 * Duplicate syntax stack (to be FreeVec'ed). Return NULL in case of failure.
 *
 */

struct SyntaxChunk *
DupStack(struct SyntaxChunk *syntaxStack)
{
   if (syntaxStack && (syntaxStack != EMPTY_STACK)) {

      struct SyntaxChunk *chunk;
      UWORD elements;

      // determine stack size

      for (elements = 0, chunk = syntaxStack; chunk->sc_Level; ++chunk)
         ++elements;

      // create copy of syntax stack (to be attached to a text line by the caller)

      if (elements) {

         ULONG size = (++elements) * sizeof(struct SyntaxChunk);

         chunk = syntaxStack;

         if (syntaxStack = AllocVec(size, MEMF_PUBLIC))
            movmem(chunk, syntaxStack, size);
      } else
         syntaxStack = EMPTY_STACK;
   }
   return (syntaxStack);
}

///
