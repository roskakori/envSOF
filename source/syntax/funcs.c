/*
 * funcs.c --  Eiffel syntax parser, main functions
 *
 * Copyright 1999-2000 Thomas Aglassinger and others, see file "forum.txt"
 *
 * (based on Dietmar Eilerts `example_generic' syntax parser)
 */

#include "defs.h"
#include "debug.h"

/// "definitions"

// version information
#define PARSER_NAME    "Eiffel"
#define PARSER_VERSION "3.0"

// Maximum length of a word in a name for Eiffel identifier. If
// the word is longer, nothing won't crash, but it will automatically
// be considered a type - without consulting ISpell.
//
// This is for efficiency only.
#define MAXIMUM_WORD_LENGTH 200

#define ISPELL_PORT   "IRexxSpell"  // Name of ISpell ARexx port
#define CHECK_COMMAND "quickcheck " // command used to check one word
#define CHECK_COMMAND_LENGTH 11     // strlen(CHECK_COMMAND)

// maximum attempts to wait for ISpell ARexx port to show up
#define ISPELL_WAIT_ATTEMPTS 5
#define ISPELL_WAIT_TICKS 10

// results for `ispell_quickcheck'
#define SPELLING_OK          0 // word found in dictionary
#define SPELLING_BAD         1 // word not found in dictionary
#define SPELLING_NOT_RUNNING 2 // ISpell not running

// we extend the ParserHandle structure for our private data
struct MyParserHandle {

   struct ParserHandle ParserHandle;   // embedded parser handle
   struct SyntaxChunk *SyntaxStack;    // parser output
   struct EditConfig *EditConfig;   // buffer data
   struct RefreshRequest RefreshRequest;  // display refresh request
   struct Entry *Entries;       // dictionary
   BOOL Seperator[256];             // table: seperator characters
   BOOL Intro[256];             // table: introducing characters
   struct Entry *Index[676];    // hash index (26*26 slots)
   UBYTE *ispell_word; // points into `ispell_command' where parameter starts
   UBYTE check_buffer[MAXIMUM_WORD_LENGTH]; // buffer to store syntax level per letter when spell checking and validating naming conventions on a word
   UBYTE ispell_command[MAXIMUM_WORD_LENGTH + CHECK_COMMAND_LENGTH + 1]; // command sent to ISpell
   BOOL ispell_running; // is ISpell running at all ?
   BOOL check_comment; // check spelling in comments ?
   BOOL check_string; // check spelling in strings ?
   BOOL check_text; // check spelling in normal Eiffel text ?
   BOOL check_naming; // check source for naming violations?
};

// empty stack flag ("line has been parsed and contains no syntax elements")

#define EMPTY_STACK ((struct SyntaxInfo *)~0)

// syntax info structures are attached to lines and used to cache syntax information and information about comments

struct SyntaxInfo {

   UWORD Flags;                 // comment flags (see below)

   struct SyntaxChunk SyntaxChunk;  // array of syntax chunks follow

};

// define basis size of struct SyntaxInfo (without array of chunks)

#define SIZEOF_STRUCT_SYNTAXINFO sizeof(UWORD)

// these flag bits describe the string status of each line

#define STRING_NONE      (1L<<0)    // no string in this line
#define STRING_SIMPLE    (1L<<1)    // there is a string in this line
#define STRING_START     (1L<<2)    // line is start of multi-line string
#define STRING_BODY      (1L<<3)    // line is body  of multi-line string
#define STRING_END       (1L<<4)    // line is end   of multi-line string

// macros to access SyntaxInfo members

#define GET_FLAGS(node)    (((struct SyntaxInfo *)(node)->UserData)->Flags)
#define GET_CHUNK(node)   (&((struct SyntaxInfo *)(node)->UserData)->SyntaxChunk)

// built-in syntax elements understood by parser

enum SyntaxLevels
{
   SYNTAX_STANDARD,
   SYNTAX_COMMENT,
   SYNTAX_COMMENT_IDENTIFIER,
   SYNTAX_STRING,
   SYNTAX_STRING_ESCAPE,
   SYNTAX_STRING_TRAILER,
   SYNTAX_WARNING,
   NAMING_VIOLATION,
   SYNTAX_SPELLING,
   SYNTAX_MAXIMUM
};

// names of built-in syntax levels

UBYTE *LevelNames[] =
{
   "Standard text",
   "Comment",
   "Comment (identifier)",
   "String manifest",
   "String (escape)",
   "String (trailer)",
   "Warning",
   "Naming violation",
   "Spelling mistake",
   NULL
};

// character classes

#define ALL_WHITE_SPACE " \t\n\r"
#define ALL_CRYPTICS    "=<>()[]{}.:,;-+*/\\@^|&#!$?"
#define ALL_ESCAPES     "ABCDFHLNQRSTUV%'\\\"()<>"

static BOOL is_escape[256];  // table: possible escape sequences
static BOOL is_cryptic[256]; // table: non-identifier characters
static BOOL is_letter[256]; // table: letters (international)

// global reply port for communication with ISpell
//
// Note: this port must only be used during Forbid() !
//
// Note: The reason for this ugly global port is that if every parser
//       window allocates an own port, the GoldEd task runs out of
//       available signals.
static struct MsgPort *reply_port = NULL;
static ULONG reply_usage_count = 0;

///
/// "prototypes"

// library functions

Prototype LibCall struct ParserData *MountScanner(void);
Prototype LibCall struct ParserHandle *StartScanner(__A0 struct GlobalConfig *, __A1 struct EditConfig *, __D0 struct SyntaxChunk *, __D1 struct SyntaxSetup *);
Prototype LibCall ULONG CloseScanner(__A0 struct ParserHandle *);
Prototype LibCall void FlushScanner(__A0 struct ParserHandle *);
Prototype LibCall void SetupScanner(__A0 struct GlobalConfig *);
Prototype LibCall struct RefreshRequest *BriefScanner(__A0 struct ParserHandle *, __A1 struct ScannerNotify *);
Prototype LibCall struct SyntaxChunk *ParseLine(__A0 struct ParserHandle *, __A1 struct LineNode *, __D0 ULONG);
Prototype LibCall void UnparseLines(__A0 struct LineNode *, __D0 ULONG);
Prototype LibCall void ParseSection(__A0 struct ParserHandle *, __A1 struct LineNode *, __D0 ULONG);

// private functions

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

#define assert(expression) internal_assert((expression), #expression, __FILE__, __LINE__)
#define implies(a,b) ((a) ? (b) : TRUE)

///
/// "library functions"

/* ------------------------------- MountScanner --------------------------------
 *
 * Called by the editor before first usage of a scanner. Return a description of
 * our abilities.
 *
 */

LibCall struct ParserData *
MountScanner(void)
{
   static UBYTE version[] = "$VER: " PARSER_NAME " " PARSER_VERSION " (" __COMMODORE_DATE__ ")";

   static struct ParserData parserData;

   static UBYTE *example[] =
   {

      "indexing                               ",
      "  description: \"Simple Eiffel example\";",
      "                                       ",
      "class HELLO                            ",
      "                                       ",
      "creation make                          ",
      "                                       ",
      "feature                                ",
      "   make is                             ",
      "         -- Say hello.                 ",
      "      do                               ",
      "         print(\"hello %                ",
      "              %world.%N\");             ",
      "   end                                 ",
      "                                       ",
      "   This: STRING is \"unterminated       ",
      "                                       ",
      "   Unknown_escape: CHARACTER is `%X';  ",
      "                                       ",
      "   _NamingViolation: BOOLEAN;          ",
      "                                       ",
      "   A_tyypo: STRING is \"a tyypo\";       ",
      "      -- Apparently, this is a tyypo.  ",
      "                                       ",
      "end -- class HELLO                     ",

      NULL
   };

   STRPTR rider;
   UWORD i;

   parserData.pd_Release = SCANLIBVERSION;
   parserData.pd_Version = 1;
   parserData.pd_Serial = 0;
   parserData.pd_Info = PARSER_NAME " " PARSER_VERSION;
   parserData.pd_Example = example;

   // signal editor that we cache syntax information, parse contexts and support configuration
   parserData.pd_Flags = SCPRF_SYNTAXCACHE | SCPRF_CONTEXT | SCPRF_GENERIC;

   // enable dictionary
   parserData.pd_Properties = SCPRF_DICTIONARY;

   // build table of characters used as operators etc.
   memset(is_cryptic, FALSE, 256);
   rider = ALL_CRYPTICS ALL_WHITE_SPACE;
   while (*rider) {
      is_cryptic[*rider] = TRUE;
      rider += 1;
   }

   // build table of escape characters
   memset(is_escape, FALSE, 256);
   rider = ALL_ESCAPES;
   while (*rider) {
      is_escape[*rider] = TRUE;
      rider += 1;
   }

   // build table of (international) letters
   //
   // Note: `µ' and `¶' are not included because Greek is not
   //       fully supported in Latin-1.
   //
   // Note: I have no idea if this makes sense. It works for
   //       German and Finish.
   i = 0;
   while (i < 256) {
      is_letter[i] = ((i >= 'a') && (i <= 'z'))
         || ((i >= 'A') && (i <= 'Z'))
         || ((i >= 'À') && (i <= 'ÿ'));
      i += 1;
   }
   is_letter['×'] = FALSE;
   is_letter['÷'] = FALSE;

   return (&parserData);
}

/* ------------------------------- StartScanner --------------------------------
 *
 * Called by the editor after a new text buffer has been created. We return a
 * parser handle (extended with our private data).
 *
 */

LibCall struct ParserHandle *
StartScanner(__A0 struct GlobalConfig *globalConfigPtr, __A1 struct EditConfig *editConfigPtr, __D0 struct SyntaxChunk *syntaxStack, __D1 struct SyntaxSetup *syntaxSetup)
{
   struct MyParserHandle *handle;
   ULONG old_reply_usage_count = reply_usage_count;

   D(init_debug());
   D(bug("StartScanner()\n"));
   if (handle = (struct MyParserHandle *) AllocVecPooled(sizeof(struct MyParserHandle), MEMF_PUBLIC | MEMF_CLEAR)) {

      LONG error_code;

      // build white-seperator table (characters treated as if they were spaces)
      UWORD ascii;
      for (ascii = 0; ascii < 256; ++ascii)
         handle->Seperator[ascii] = !(isalnum(ascii) || (ascii == '_'));

      // prepare spell checking
      strcpy(handle->ispell_command, CHECK_COMMAND);
      handle->ispell_word = handle->ispell_command + CHECK_COMMAND_LENGTH;
      handle->ispell_running = TRUE;

      // parse user arguments
      error_code = parse_arguments(handle, syntaxSetup);
      if (error_code != 0) {
         // TODO: pop-up requester
         D(bug("   argument error: "));
         D(kint(error_code));
      }

      // if this is the first user of the global `reply_port', create it
      Forbid();
      if (reply_usage_count == 0) {
         reply_port = CreateMsgPort();
      }
      reply_usage_count += 1;
      Permit();

      if (reply_port == NULL) {
         D(bug("   cannot create ARexx message port\n"));
         D(bug("   (probably no signal bit left for task)\n"));
         handle->check_comment = FALSE;
         handle->check_string = FALSE;
         handle->check_text = FALSE;
      }

      // show what state of parser
      if (handle->check_comment || handle->check_string || handle->check_text) {
         D(bug("   spell check: "));
         if (handle->check_comment) {
            D(bug("comment "));
         }
         if (handle->check_string) {
            D(bug("string "));
         }
         if (handle->check_text) {
            D(bug("text "));
         }
      } else {
         D(bug("   no spell checking\n"));
      }

      // process dictionary and build array of syntax categories
      if (ExamineDictionary((struct MyParserHandle *) handle, syntaxSetup)) {

         handle->ParserHandle.ph_ColorsFG = NULL;
         handle->ParserHandle.ph_ColorsBG = NULL;
         handle->SyntaxStack = syntaxStack;
         handle->EditConfig = editConfigPtr;

         return ((struct ParserHandle *) handle);
      }
      CloseScanner((struct ParserHandle *) handle);
   }
   return (NULL);
}

/* ------------------------------- CloseScanner --------------------------------
 *
 * Called by the editor if a text buffer is about to be closed. Deallocate buffer
 * specific 'global' data.
 *
 */
LibCall ULONG
CloseScanner(__A0 struct ParserHandle * parserHandle)
{
   D(cleanup_debug());
   if (parserHandle) {
      struct MyParserHandle *handle = (struct MyParserHandle *) parserHandle;

      assert(reply_usage_count > 0);

      // if this is the last user of the global `reply_port', delete it
      Forbid();
      reply_usage_count -= 1;
      if ((reply_usage_count == 0) && (reply_port != NULL)) {
         DeleteMsgPort(reply_port);
         reply_port = NULL;
      }
      Permit();
      FreeVecPooled(parserHandle->ph_Names);
      FreeVecPooled(parserHandle);
   }
   return (0);
}

/* ------------------------------- FlushScanner --------------------------------
 *
 * Called by the editor in low memory situations: we are supposed to free as much
 * memory as possible. We free cached syntax information.
 *
 */

LibCall void
FlushScanner(__A0 struct ParserHandle *parserHandle)
{
   struct EditConfig *config = ((struct MyParserHandle *) parserHandle)->EditConfig;

   if (config->TextNodes && config->Lines)
      UnparseLines(config->TextNodes, config->Lines);
}

/* ------------------------------- SetupScanner --------------------------------
 *
 * Called by the editor if the user wants to change the scanner's configuration.
 * We do not support user configuration (parserData.pd_Flags: SCPRF_CONFIGWIN flag
 * unset).
 *
 */

LibCall void
SetupScanner(__A0 struct GlobalConfig *globalConfigPtr)
{
   ;
}

/* ------------------------------- BriefScanner --------------------------------
 *
 * Called to notify a context scanner if lines have been added, deleted or
 * modified. We are supposed to return an additional display request or NULL
 * (the editor will refresh the damaged region only if we return NULL). Damaged
 * lines have already been unparsed by the editor. This is what we have to do:
 *
 * a) lines have been deleted or modified
 *
 * Check whether the syntax scheme of the remaining lines is still valid (it
 * will be invalid if say a comment start has been deleted but the comment
 * end has not been deleted). Reparse the lines and return a refresh request
 * if not.
 *
 * b) lines have been inserted
 *
 * Check whether the syntax scheme of the new lines affects the existing lines.
 * Reparse all lines starting at the insertion point and return a refresh
 * request if not.
 *
 * c) lines have been (un)folded
 *
 * This scanner assumes that folding doesn't affect syntax highlighting for the
 * sake of simplicity. This isn't necessarily a valid assumption: we will run
 * into touble if the user folds parts of a comment only (e.g. the first line).
 *
 */

LibCall struct RefreshRequest *
BriefScanner(__A0 struct ParserHandle *parserHandle, __A1 struct ScannerNotify *notify)
{
   switch (notify->sn_Class) {

   case SCANNER_NOTIFY_MODIFIED:

      {
         ULONG line = notify->sn_Line;

         if (notify->sn_Removed > notify->sn_Lines) {
            // lines have been deleted
            return (VerifyContext(parserHandle, line, 0));

         } else if (notify->sn_Lines) {
            // lines have been modified or inserted
            return (VerifyContext(parserHandle, line, notify->sn_Lines));

         } else
            return (NULL);
      }

      break;

   default:

      return (NULL);
   }
}

/* --------------------------------- ParseLine ---------------------------------
 *
 * Parse a line, build a syntax description
 *
 */

LibCall struct SyntaxChunk *
ParseLine(__A0 struct ParserHandle *parserHandle, __A1 struct LineNode *lineNode, __D0 ULONG line)
{
   if (IS_FOLD(lineNode)) {

      return (NULL);
   } else if (lineNode->Len) {

      if (lineNode->UserData == EMPTY_STACK) {

         // line parsed already

         return (NULL);
      } else if (lineNode->UserData) {

         // line parsed already

         return (GET_CHUNK(lineNode));
      } else {

         ParseSection(parserHandle, lineNode, 1);

         if (lineNode->UserData == EMPTY_STACK)
            return (NULL);
         else
            return (GET_CHUNK(lineNode));
      }
   } else
      return (NULL);
}

/* -------------------------------- UnparseLines -------------------------------
 *
 * Called by the editor if lines are to be deleted. We are supposed to free
 * private data attached to the lines (cached syntax information).
 *
 */

LibCall void
UnparseLines(__A0 struct LineNode *lineNode, __D0 ULONG lines)
{
   while (lines--) {

      // free syntax cache

      if (lineNode->UserData) {

         if (lineNode->UserData != (APTR) EMPTY_STACK)
            FreeVecPooled((APTR) lineNode->UserData);

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
ParseSection(__A0 struct ParserHandle *parserHandle, __A1 struct LineNode *lineNode, __D0 ULONG lines)
{
   struct LineNode *firstNode;

   if (firstNode = FindContextStart(parserHandle, lineNode))
      ParseContext(parserHandle, firstNode, lines + (lineNode - firstNode));
}

///
/// "private"

/*  ----------------------------- FindContextStart ------------------------------
 *
 * Search backwards until a parsed line or a "context free line" is found, ie. a
 * line which can't possibly be affected by the lines before it as far as syntax
 * parsing is concerned. The first line of a document is a context free line. We
 * assume that the user doesn't use folds inside comments, so fold headers can be
 * considered context-free lines, too (all comments end before the fold). The node
 * returned by this function may be used as starting point for parsing a context.
 *
 */

struct LineNode *
FindContextStart(parserHandle, lineNode)
     struct ParserHandle *parserHandle;
     struct LineNode *lineNode;
{
   ULONG line = lineNode - ((struct MyParserHandle *) parserHandle)->EditConfig->TextNodes;

   while (line--) {

      // found a fold or a parsed line ?

      if (lineNode->UserData || IS_FOLD(lineNode))
         break;

      --lineNode;
   }

   return (lineNode);
}

/* ------------------------------- ParseContext --------------------------------
 *
 * Preparse lines. The first line is expected to be either a context free line
 * (start of file or a fold header) or to be a parsed line.
 *
 */

void
ParseContext(parserHandle, lineNode, lines)
     struct ParserHandle *parserHandle;
     struct LineNode *lineNode;
     ULONG lines;
{
   ULONG mode;

   for (mode = STRING_NONE; lines--; ++lineNode) {

      if (IS_FOLD(lineNode)) {

         mode = STRING_NONE;
      } else {

         if (lineNode->UserData == EMPTY_STACK) {

            mode = STRING_NONE;
         } else if (lineNode->UserData) {

            mode = GET_FLAGS(lineNode);
         } else {

            struct SyntaxInfo *syntaxInfo = ParseContextLine(parserHandle, lineNode, mode);

            if (syntaxInfo == EMPTY_STACK) {

               lineNode->UserData = EMPTY_STACK;

               mode = STRING_NONE;
            } else if (syntaxInfo) {

               lineNode->UserData = (APTR) DupInfo(syntaxInfo);

               mode = syntaxInfo->Flags;
            } else
               mode = STRING_NONE;
         }
      }
   }
}

/* -------------------------------- ParseContextLine ---------------------------
 *
 * Parse a line, build a syntax description. Return EMPTY_STACK if there is
 * nothing to highlight. The status of the last line is passed in by the caller
 *
 */

struct SyntaxInfo *
ParseContextLine(struct ParserHandle *parserHandle, struct LineNode *lineNode, ULONG last)
{
// append one element to syntax stack
#define APPEND_ELEMENT(level,start,end)        \
   {                                           \
      D(bug("      append element "));         \
      D(kint(element));                        \
      D(bug(" from "));                        \
      D(kint(start));                          \
      D(bug(" to "));                          \
      D(kint(end));                            \
      D(bug(" ("));                            \
      if (level < SYNTAX_MAXIMUM) {            \
         if (level == 0) {                     \
            D(bug("END"));                     \
         } else {                              \
            D(bug(LevelNames[level]));         \
         }                                     \
      } else {                                 \
         D(bug("Keyword."));                   \
         D(kint(level));                       \
      }                                        \
      D(bug(")\n"));                           \
      syntaxStack[element].sc_Level = (level); \
      syntaxStack[element].sc_Start = (start); \
      syntaxStack[element].sc_End = (end);     \
      ++element;                               \
   }
// append element consisting of a single character
#define APPEND_ELEMENT_CHAR(level,position) APPEND_ELEMENT((level), (position), (position))
// append naming violation highlighting only one character
#define APPEND_NAMING_CHAR(position) APPEND_ELEMENT(NAMING_VIOLATION, (position), (position))
// change level of existing element
#define CHANGE_ELEMENT_LEVEL(element_id,level)      \
   {                                                \
      D(bug("      change element "));              \
      D(kint(element));                             \
      D(bug(" to ("));                              \
      if ((level) < SYNTAX_MAXIMUM) {               \
         if ((level) == 0) {                        \
            D(bug("END"));                          \
         } else {                                   \
            D(bug(LevelNames[level]));              \
         }                                          \
      }                                             \
      D(bug(")\n"));                                \
      assert((element_id) < element);               \
      assert((level) < SYNTAX_MAXIMUM);             \
      syntaxStack[(element_id)].sc_Level = (level); \
   }

// change one element to syntax stack
#define CHANGE_ELEMENT(element_id,level,start,end) \
   {                                                 \
      D(bug("      change element "));               \
      D(kint((element_id)));                         \
      D(bug(" from "));                              \
      D(kint(start));                                \
      D(bug(" to "));                                \
      D(kint(end));                                  \
      D(bug(" ("));                                  \
      if (level < SYNTAX_MAXIMUM) {                  \
         if (level == 0) {                           \
            D(bug("END"));                           \
         } else {                                    \
            D(bug(LevelNames[level]));               \
         }                                           \
      } else {                                       \
         D(bug("Keyword."));                         \
         D(kint(level));                             \
      }                                              \
      D(bug(")\n"));                                 \
      syntaxStack[(element_id)].sc_Level = (level);  \
      syntaxStack[(element_id)].sc_Start = (start);  \
      syntaxStack[(element_id)].sc_End = (end);      \
   }

// insert (undefined) element at position `at'
#define INSERT_ELEMENT(at)                                       \
   {                                                             \
      int elements_to_move = element - (at) - 1;                 \
      D(bug("      insert element at "));                        \
      D(kint((at)));                                             \
      D(bug("\n"));                                              \
      if (elements_to_move > 0) {                                \
         memmove(&syntaxStack[(at) + 1], &syntaxStack[(at)],     \
                 sizeof(struct SyntaxChunk) * elements_to_move); \
      } else {                                                   \
         assert(elements_to_move == 0);                          \
         D(bug("         (already at last element)\n"));         \
      }                                                          \
      element++;                                                 \
   }


   UWORD len;

   D(bug("ParseContextLine()\n"));
   if (len = lineNode->Len) {

      struct MyParserHandle *handle = (struct MyParserHandle *) parserHandle;
      struct SyntaxInfo *syntaxInfo;
      struct SyntaxChunk *syntaxStack;
      UBYTE *text;
      UBYTE *line_text;
      UWORD indent;
      UWORD element;
      UWORD status;

      UWORD quote_element;      // index in syntaxStack highlighting quote at begin of string

      UBYTE in_string;          // 0=no string, 34=in string, 39=in character

      BOOL in_escape;           // in string, after `%'

      BOOL in_numeric_escape;   // in string, after `%' and after `/'

      UWORD string_start; // position of end of last escape or starting (") or (%)
      UWORD escape_start; // position of last (%)

      BOOL in_comment;
      BOOL in_identifier;     // in comment, after (`)
      UWORD comment_start;    // position of "--" or last end of identifier (')
      UWORD identifier_start; // position after last (`)

      BOOL local_string;
      BOOL *seperator;
      BOOL check_comment = handle->check_comment;
      BOOL check_string = handle->check_string;
      BOOL check_text = handle->check_text;

      syntaxInfo = (struct SyntaxInfo *) ((struct MyParserHandle *) parserHandle)->SyntaxStack;
      syntaxStack = &syntaxInfo->SyntaxChunk;
      element = 0;

      // the white-seperator table is used to find start and end of keywords
      seperator = ((struct MyParserHandle *) parserHandle)->Seperator;

      // just a short-cut
      line_text = lineNode->Text;

      // leading spaces and tabs have to be ignored (we ignore all non-ascii characters)
      text = lineNode->Text;
      for (indent = 0; len && (*text <= 32); ++indent) {
         ++text;
         --len;
      }

      // trailing white seperator has to be ignored
      while (len && (text[len - 1] <= 32))
         --len;

      // initial string status depends on status of previous line
      in_string = FALSE;
      if (last & (STRING_START | STRING_BODY)) {
         // this line is part of a string started earlier
         if (*text == '%') {
            // the first character is a '%'
            in_string = 34;
         }
      }
      if (in_string) {
         status = STRING_BODY;
         string_start = indent + 1;
         quote_element = element;
         APPEND_ELEMENT_CHAR(SYNTAX_STRING_TRAILER, indent);
         ++text;
         ++indent;
         --len;
      } else {
         status = STRING_NONE;
         string_start = FALSE;
      }

      local_string = FALSE;
      in_comment = FALSE;
      in_identifier = FALSE;
      in_escape = FALSE;
      in_numeric_escape = FALSE;

      while (len) {

         UBYTE code = *text;

         assert((in_string == 0) || (in_string == 34) || (in_string == 39));
         assert(implies(in_comment, !in_string));
         assert(implies(in_string, !in_comment));
         assert(implies(in_numeric_escape, in_escape));
         assert(element >= 0);

         if (in_string) {

            if (in_numeric_escape) {

               if (code == '/') {
                  // numeric escape sequence ends
                  // TODO: highlight
                  in_numeric_escape = FALSE;
                  in_escape = FALSE;
               } else {
                  // TODO: check if `code' is numeric digit
               }
            } else if (in_escape) {

               if (code == '/') {
                  // numeric escape sequence starts
                  in_numeric_escape = TRUE;
               } else {
                  // escape sequence ends
                  if (string_start < escape_start) {
                     // highlight previous part of string
                     APPEND_ELEMENT(SYNTAX_STRING, string_start, escape_start - 1);
                  }
                  // highlight escape sequence
                  APPEND_ELEMENT(SYNTAX_STRING_ESCAPE, escape_start, indent);
                  in_escape = FALSE;
                  string_start = indent + 1;
                  // check for unknown escapes
                  if (!is_escape[code]) {
                     syntaxStack[element - 1].sc_Level = SYNTAX_WARNING;
                  }
               }
            } else {

               if (code == '%') {

                  // escape sequence detected
                  in_escape = TRUE;
                  escape_start = indent;
               } else if (code == in_string) {

                  // end of string detected
                  in_string = 0;
                  APPEND_ELEMENT(SYNTAX_STRING, string_start, indent);

                  // end of a local string (starting in the same line) ?
                  if (local_string) {
                     status |= STRING_SIMPLE;
                  } else {
                     status |= STRING_END;
                     status &= ~STRING_BODY;
                  }
               }
            }
         } else if (in_comment) {
            if (in_identifier) {
               if (code == 39) {
                  UWORD identifier_end = indent - 1;
                  in_identifier = FALSE;
                  D(bug("      identifier ends\n"));
                  if (identifier_start < identifier_end) {
                     APPEND_ELEMENT(SYNTAX_COMMENT, comment_start, identifier_start - 1);
                     APPEND_ELEMENT(SYNTAX_COMMENT_IDENTIFIER, identifier_start, identifier_end);
                     comment_start = indent;
                  }
               }
            } else {
               if (code == '`') {
                  D(bug("      identifier starts\n"));
                  in_identifier = TRUE;
                  identifier_start = indent + 1;
               }
            }
         } else if ((code == 34) || (code == 39)) {

            // start of string detected in this line
            in_string = code;
            in_escape = FALSE;
            in_numeric_escape = FALSE;
            string_start = indent + 1;
            quote_element = element;
            APPEND_ELEMENT_CHAR(SYNTAX_STRING, indent);
            local_string = TRUE;

         } else if ((code == '-') && (len > 1) && (text[1] == '-')) {

            // comment detected
            in_comment = TRUE;
            comment_start = indent;

         } else if (!seperator[code]) {

            UBYTE *word;
            UWORD word_length;
            UWORD word_start;

            // determine length of next word
            word = text;
            for (word_length = 0; len; ++text, ++indent, ++word_length, --len)
               if (seperator[*text])
                  break;
            word_start = indent - word_length;

            if (word_length >= 1) {

               // could this be a keyword ?
               BOOL is_keyword = FALSE;
               if (((struct MyParserHandle *) parserHandle)->Intro[*word]) {

                  struct Entry *keyword;

                  if (keyword = IsKeyword((struct MyParserHandle *) parserHandle, word, word_length)) {

                     // keyword detected
                     is_keyword = TRUE;
                     APPEND_ELEMENT((keyword->en_Category & 0xff) + SYNTAX_MAXIMUM,
                                    word_start, indent - 1);
                  }
               }
               if (!is_keyword) {

                  // validate naming conventions
                  //
                  // Note: does not warn about "X_", because this happens
                  //       all the time during typing.

                  // warn about underscore as first character
                  while ((word_length > 0) && (*word == '_')) {
                     APPEND_NAMING_CHAR(word_start);
                     word += 1;
                     word_length -= 1;
                  }

                  // validate rest of word
                  if ((word_length > 1) && (isalpha(*word))) {
                     UBYTE first_letter, second_letter;
                     UBYTE item; // character currently examined
                     BOOL is_class_name;
                     UWORD i;

                     // search for `second_letter';
                     // skip digits and underscores (_)
                     i = 1;
                     first_letter = word[0];
                     second_letter = 0;
                     while ((i <= word_length) && (second_letter == 0)) {
                        item = word[i];
                        if (is_letter[item]) {
                           second_letter = item;
                        }
                        else {
                           // warn about `a__name'
                           if ((item == '_') && (word[i-1] == '_')) {
                              APPEND_NAMING_CHAR(word_start + i);
                           }
                           i = i + 1;
                        }
                     }

                     is_class_name = isupper(first_letter) && isupper(second_letter);
                     while (i < word_length) {
                        item = word[i];
                        if (item == '_') {
                           if (word[i-1] == '_') {
                              APPEND_NAMING_CHAR(word_start + i);
                           }
                        }
                        else if (!isdigit(item)) {
                           if (is_class_name) {
                              if (!isupper(item)) {
                                 APPEND_NAMING_CHAR(word_start + i);
                              }
                           } else {
                              if (!islower(item)) {
                                 APPEND_NAMING_CHAR(word_start + i);
                              }
                           }
                        }
                        i += 1;
                     }
                  }
               }
            }
            // compensate loop-increment
            --text;
            --indent;
            ++len;
         } else if (!is_cryptic[code]) {
            APPEND_ELEMENT_CHAR(SYNTAX_WARNING, indent);
         }

         ++text;
         ++indent;
         --len;
      }

      D(bug("   at end of line\n"));

      // end of line reached with unterminated string ?
      if (in_string) {

         UWORD string_end = indent - 1;

         D(bug("      in_string\n"));

         if (in_numeric_escape) {
            // highlight string as usual,
            // but unterminated numeric escape as error
            UWORD escape_end = string_end;
            string_end = escape_start - 1;
            D(bug("      in_numeric_escape\n"));
            if (string_start < string_end) {
               APPEND_ELEMENT(SYNTAX_STRING, string_start, string_end);
            }
            APPEND_ELEMENT(SYNTAX_WARNING, escape_start, escape_end);

         } else {

            // terminated with `%' to indicate multi-line string ?
            if (in_escape) {

               D(bug("      in_escape\n"));
               string_end -= 1;

               // string starting in this line ?
               if (local_string)
                  status |= STRING_START;
               else
                  status |= STRING_BODY;
            } else {
               // highlight beginning quote as error
               D(bug("      warning: unterminated quote\n"));
               CHANGE_ELEMENT_LEVEL(quote_element, SYNTAX_WARNING);
            }

            if (string_start < string_end) {
               APPEND_ELEMENT(SYNTAX_STRING, string_start, string_end);
            }
            if (in_escape) {
               APPEND_ELEMENT_CHAR(SYNTAX_STRING_TRAILER, string_end + 1);
            }
         }
      } else if (in_comment) {

         // end of comment line
         D(bug("      in_comment\n"));
         if (in_identifier) {
            UWORD comment_end = identifier_start - 2;
            UWORD identifier_end = indent - 1;
            D(bug("      warning: unterminated identifier\n"));
            // highlight comment
            if (comment_start < comment_end) {
               APPEND_ELEMENT(SYNTAX_COMMENT, comment_start, comment_end);
            }
            // highlight unterminated backquote (`) as warning
            APPEND_ELEMENT_CHAR(SYNTAX_WARNING, identifier_start - 1);
            // highlight identifier
            if (identifier_start < identifier_end) {
               APPEND_ELEMENT(SYNTAX_COMMENT_IDENTIFIER, identifier_start, identifier_end);
            }
         } else {
            // simply terminate comment
            APPEND_ELEMENT(SYNTAX_COMMENT, comment_start, indent - 1);
         }
      }

#if 0
      if (check_comment || check_string || check_text) {
         // check spelling of requested elements
         UWORD rider;

         D(bug("   spell checking\n"));

         rider = 0;
         while (rider < element) {
            UBYTE level = syntaxStack[rider].sc_Level;

            if ((check_comment && (level == SYNTAX_COMMENT))
                || (check_string && (level == SYNTAX_STRING)))
            {
               UWORD start = syntaxStack[rider].sc_Start;
               UWORD end = syntaxStack[rider].sc_End;
               UWORD word_start = 0;

               D(bug("      examine element "));
               D(kint(rider));
               D(bug(": ("));
               D(kint(level));
               D(bug(") from "));
               D(kint(start));
               D(bug(" to "));
               D(kint(end));
               D(bug("\n"));

               indent = start;
               while (indent < end) {
                  UBYTE code;

                  code = line_text[indent];
                  if (word_start == 0) {
                     if (is_letter[code]) {
                        word_start = indent;
                     }
                  } else {
                     if (!is_letter[code]) {
                        UWORD word_end = indent - 1;
                        if (!is_spelled_correctly(handle, line_text, word_start, word_end)) {
                           D(bug("      insert typo element\n"));
                           if ((word_start == start) && (word_end == end)) {
                              D(bug("      simply replace old element\n"));
                              CHANGE_ELEMENT_LEVEL(rider, SYNTAX_SPELLING);
                           } else {
                              INSERT_ELEMENT(rider)
                              syntaxStack[rider].sc_End = word_start - 1;
                              CHANGE_ELEMENT(rider + 1, SYNTAX_SPELLING, word_start, word_end);
                              if (end != word_end) {
                                 D(bug("      unspelled text left\n"));
                                 INSERT_ELEMENT(rider+2);
                                 CHANGE_ELEMENT(rider+2, syntaxStack[rider].sc_Level, word_end + 1, end);
                                 rider += 1;
                              }
                           }
                           indent = end - 1; // continue spell check with next element
                        }
                        word_start = 0;
                     }
                  }
                  indent += 1;
               }
            }

            rider += 1;
         }
      }
#endif

      if (element) {

         // terminate syntax stack
         D(bug("   terminate syntax stack\n"));

         APPEND_ELEMENT(0, 0, 0)
            syntaxInfo->Flags = status;

         return (syntaxInfo);
      } else
         return (EMPTY_STACK);
   } else {
      // line is empty
      return (EMPTY_STACK);
   }
}

/* ------------------------------- VerifyContext -------------------------------
 *
 * Ensure that syntax information of a specified range of lines ("damage region")
 * is valid and consistent with the existing text. Return a refresh request if
 * not. The damage area is expected to have been unparsed already. This is what we
 * have to do: we preparse existing lines before the damage area if belonging to
 * the syntax context of the damage area (ie. all lines affecting highlighting of
 * the first line in the damage area). The damage area is parsed, too. Parsed
 * lines after the damage area are reparsed if highlighting is found to be
 * inconsistent with the line(s) before. Reparsing continues until the end of the
 * file respectively until no more inconsistencies are found.
 *
 */

struct RefreshRequest *
VerifyContext(struct ParserHandle *parserHandle, ULONG line, ULONG lines)
{
   struct EditConfig *config;
   struct LineNode *lineNode;
   struct LineNode *lastNode;
   struct SyntaxInfo *syntaxInfo;
   ULONG refreshStart;
   ULONG refresh;
   ULONG last;
   ULONG new;

   config = ((struct MyParserHandle *) parserHandle)->EditConfig;

   lineNode = config->TextNodes + line;
   lastNode = config->TextNodes + line + (lines - 1);

   // preparse from context start until end of damage area

   ParseSection(parserHandle, lineNode, lines);

   // get syntax flags of last line in damage area

   if ((lastNode < config->TextNodes) || IS_FOLD(lastNode) || (lastNode->UserData == EMPTY_STACK) || (lastNode->UserData == NULL))
      last = STRING_NONE;
   else
      last = GET_FLAGS(lastNode);

   // continue parsing until no more inconsistencies are found (until context end)
   refreshStart = (line += lines);
   refresh = FALSE;

   for (lineNode = lastNode + 1; line < config->Lines; ++line, ++lineNode, ++refresh) {

      if (IS_FOLD(lineNode)) {

         // folds terminate parsing (context end)
         break;
      } else if (lineNode->UserData == NULL) {

         // line not yet parsed
         syntaxInfo = ParseContextLine(parserHandle, lineNode, last);
         if (syntaxInfo == EMPTY_STACK) {
            lineNode->UserData = EMPTY_STACK;
         } else if (syntaxInfo)
            lineNode->UserData = DupInfo(syntaxInfo);
      } else {

         // check whether highlighting of this line is consistent with previous line
         if (lineNode->UserData == EMPTY_STACK) {
            new = STRING_NONE;
         } else {
            new = GET_FLAGS(lineNode);
         }

         if (BadSuccessor(last, new)) {
            if (lineNode->UserData != EMPTY_STACK)
               FreeVecPooled(lineNode->UserData);

            if (syntaxInfo = ParseContextLine(parserHandle, lineNode, last)) {
               if (syntaxInfo == EMPTY_STACK)
                  lineNode->UserData = EMPTY_STACK;
               else
                  lineNode->UserData = DupInfo(syntaxInfo);
            } else
               lineNode->UserData = NULL;
         } else
            break;
      }

      if ((lineNode->UserData == EMPTY_STACK) || (lineNode->UserData == NULL) || IS_FOLD(lineNode))
         last = STRING_NONE;
      else
         last = GET_FLAGS(lineNode);
   }

   if (refresh) {

      struct RefreshRequest *refreshRequest = &((struct MyParserHandle *) parserHandle)->RefreshRequest;

      refreshRequest->rr_Line = refreshStart;
      refreshRequest->rr_Lines = refresh;

      return (refreshRequest);
   } else
      return (NULL);
}

/* ------------------------------- BadSuccessor --------------------------------
 *
 * Return TRUE if syntax information of two adjacent lines is inconsistent.
 *
 */

BOOL
BadSuccessor(pred, succ)
     ULONG pred, succ;
{
   if (succ & (STRING_BODY | STRING_END)) {

      // string body/end without start ?
      if ((pred & (STRING_START | STRING_BODY)) == FALSE)
         return (TRUE);

   } else if (pred & STRING_START) {

      // string start without body/end ?
      if ((succ & (STRING_BODY | STRING_END)) == FALSE)
         return (TRUE);

   } else if (pred & STRING_BODY) {

      // string body without end ?
      if ((succ & (STRING_BODY | STRING_END)) == FALSE)
         return (TRUE);
   }
   return (FALSE);
}

/* --------------------------------- DupInfo -----------------------------------
 *
 * Duplicate syntax info (to be FreeVec'ed)
 *
 */

struct SyntaxInfo *
DupInfo(syntaxInfo)
     struct SyntaxInfo *syntaxInfo;
{
   struct SyntaxChunk *chunk;
   struct SyntaxInfo *info;
   ULONG size;
   UWORD elements;

   // determine stack size

   for (elements = 0, chunk = &syntaxInfo->SyntaxChunk; chunk->sc_Level; ++chunk)
      ++elements;

   // create copy of syntax stack (to be attached to a text line by the caller)

   size = SIZEOF_STRUCT_SYNTAXINFO + (++elements) * sizeof(struct SyntaxChunk);

   if (info = (struct SyntaxInfo *) AllocVecPooled(size, MEMF_PUBLIC)) {

      movmem(syntaxInfo, info, size);

      return (info);
   } else
      return (NULL);
}

/* ----------------------------- ExamineDictionary -----------------------------
 *
 * Process dictionary: Build the final array of syntax categories (built-in syntax
 * levels plus dictionary categories). To be FreeVec'ed later.
 *
 */

UBYTE **
ExamineDictionary(parserHandle, setup)
     struct MyParserHandle *parserHandle;
     struct SyntaxSetup *setup;
{
   struct Node *category;
   UWORD categories;

   // count categories

   categories = SYNTAX_MAXIMUM;

   if (setup->sp_Categories) {

      for (category = setup->sp_Categories->lh_Head; category->ln_Succ; category = category->ln_Succ)
         ++categories;
   }
   parserHandle->ParserHandle.ph_Levels = categories;

   // create array of level names

   if (parserHandle->ParserHandle.ph_Names = (UBYTE **) AllocVecPooled((categories + 1) * sizeof(UBYTE **), MEMF_PUBLIC | MEMF_CLEAR)) {

      UBYTE **slot = parserHandle->ParserHandle.ph_Names;

      // built-in levels

      for (categories = 0; categories < SYNTAX_MAXIMUM; ++categories)
         *slot++ = LevelNames[categories];

      // additional levels (dictionary)

      if (setup->sp_Categories) {

         for (category = setup->sp_Categories->lh_Head; category->ln_Succ; category = category->ln_Succ)
            *slot++ = category->ln_Name;
      }
   }
   // build hash index to improve look-up speed

   if (parserHandle->Entries = setup->sp_Dictionary) {

      struct Entry *entry;

      for (entry = (struct Entry *) setup->sp_Dictionary; entry->en_Length; entry = (struct Entry *) (((ULONG) entry->en_Text + entry->en_Length + 2) & ~1)) {

         // remember address of first entry for every possible 1st/2nd character combination

         struct Entry **slot = parserHandle->Index + Hashcode(entry->en_Text, entry->en_Length);

         if (*slot == NULL)
            *slot = entry;

         // remember that the first character of this keyword as possible keyword start

         parserHandle->Intro[*entry->en_Text] = TRUE;
      }
   }
   return (parserHandle->ParserHandle.ph_Names);
}

/* --------------------------------- Hashcode ----------------------------------
 *
 * Return hashcode (0...675) for the specified string (minimum size 1 character).
 *
 */

UWORD
Hashcode(text, len)
     UBYTE *text;
     UWORD len;
{
   register UWORD ascii1, ascii2;

   ascii1 = toupper(*text);

   if (len == 1)
      ascii2 = 0;
   else
      ascii2 = toupper(*++text);

   if (ascii1 < 'A')
      ascii1 = 'A';

   else if (ascii1 > 'Z')
      ascii1 = 'Z';

   if (ascii2 < 'A')
      ascii2 = 'A';

   else if (ascii2 > 'Z')
      ascii2 = 'Z';

   return ((ascii1 - 'A') * 26 + (ascii2 - 'A'));
}

/* -------------------------------- IsKeyword ----------------------------------
 *
 * Check if <text> starts with a known keyword.
 *
 */

struct Entry *
IsKeyword(parserHandle, text, len)
     struct MyParserHandle *parserHandle;
     UBYTE *text;
     UWORD len;
{
   struct Entry *entry;

   // find a reasonable dictionary entry point

   if (entry = parserHandle->Index[Hashcode(text, len)]) {

      UWORD code1, code2;

      code1 = toupper(*text);

      if (len > 1)
         code2 = toupper(*(text + 1));
      else
         code2 = 0;

      // skip to correct letter

      while (entry->en_Length && (toupper(*entry->en_Text) < code1))
         entry = (struct Entry *) (((ULONG) entry->en_Text + entry->en_Length + 2) & ~1);

      // compare to the words in the dictionary

      while (entry->en_Length) {

         register UBYTE *keyword = entry->en_Text;

         // abort search ?

         if (toupper(*keyword) > code1)
            break;

         if (code2)
            if (toupper(*(keyword + 1)) > code2)
               break;

         // keyword found ?

         if (entry->en_Length == len) {

            if (memcmp(text, keyword, len) == 0)
               return (entry);
         }
         // find next (word-aligned) entry

         entry = (struct Entry *) (((ULONG) entry->en_Text + entry->en_Length + 2) & ~1);
      }
   }
   return (NULL);
}


/* -------------------------------- ispell_quickcheck ----------------------
 *
 * send `quickcheck <word>' command to ispell ARexx port and check for
 * result "ok".
 *
 */
UBYTE
ispell_quickcheck(UBYTE * cmd)
{
   UBYTE spelling_result = SPELLING_NOT_RUNNING;
   struct MsgPort *rexx_port;

   D(bug("      sending ispell command: \""));
   D(bug(cmd));
   D(bug("\"\n"));

   assert(reply_port != NULL);

   Forbid();

   rexx_port = FindPort(ISPELL_PORT);
   if (rexx_port == NULL) {

      UWORD wait_count;
      LONG shell_error;

      // invoke ISpell
      Permit();
      shell_error = -1;
      // TODO: start ISpell here
      //       (currently depends on words.api to do it)
      D(bug("      shell error = "));
      D(kint(shell_error));
      D(bug("\n"));
      Forbid();

      if (shell_error == 0) {
         // wait for ISpell
         wait_count = 0;
         while ((rexx_port == NULL) && (wait_count < ISPELL_WAIT_ATTEMPTS)) {

            Permit();
            D(bug("      ...waiting\n"));
            Delay(ISPELL_WAIT_TICKS);
            Forbid();
            rexx_port = FindPort(ISPELL_PORT);
            wait_count += 1;
         }
      } else {
         // TODO: remove this; only there to enable spelling if ISpell was already running before Eiffel mode
         rexx_port = FindPort(ISPELL_PORT);
      }

   }
   if (rexx_port != NULL) {

      struct RexxMsg *rexxMsg;
      struct RexxMsg *answer;

      if (rexxMsg = CreateRexxMsg(reply_port, NULL, NULL)) {

         if (rexxMsg->rm_Args[0] = CreateArgstring(cmd, strlen(cmd))) {

            static ULONG result;

            rexxMsg->rm_Action = RXCOMM | RXFF_RESULT;

            PutMsg(rexx_port, &rexxMsg->rm_Node);

            do {
               WaitPort(reply_port);
               if (answer = (struct RexxMsg *) GetMsg(reply_port))
                  result = answer->rm_Result1;
            } while (!answer);

            Permit();

            if (answer->rm_Result1 == RC_OK) {
               if (answer->rm_Result2) {
                  D(bug("      ispell result=\""));
                  D(bug((STRPTR) answer->rm_Result2));
                  D(bug("\"\n"));
                  if (!strcmp("ok", (STRPTR) answer->rm_Result2)) {
                     spelling_result = SPELLING_OK;
                  } else {
                     assert(!strcmp("bad", (STRPTR) answer->rm_Result2));
                     spelling_result = SPELLING_BAD;
                  }

                  DeleteArgstring((UBYTE *) answer->rm_Result2);
               }
            }
            DeleteArgstring((UBYTE *) ARG0(answer));

            DeleteRexxMsg(answer);

         } else {
            Permit();
            D(bug("*** cannot create argument string"));
         }
      } else {
         Permit();
         D(bug("*** cannot create Rexx message"));
      }
   } else {
      Permit();
      D(bug("      cannot find ISpell ARexx port \"" ISPELL_PORT "\"\n"));
   }

   return (spelling_result);
}

/* ---------------------------- is_spelled_correctly -----------------------
 *
 * check spelling of word `text' from index `start' to `end'.
 *
 * If ISpell is not running, the parser degrades gracefully and disables
 * spell checking for `handle'. In this case, the result is also TRUE.
 *
 */
BOOL
is_spelled_correctly(struct MyParserHandle * handle, UBYTE * text, UWORD start, UWORD end)
{
   UBYTE *command = handle->ispell_command + CHECK_COMMAND_LENGTH;
   UWORD word_length = end - start;
   BOOL result = FALSE;

   if (word_length < MAXIMUM_WORD_LENGTH) {
      if (start != end) {
         UBYTE ispell_result;

         // append word to command buffer
         while (start <= end) {
            *command = text[start];
            start += 1;
            command += 1;
         }
         *command = '\0';

         if (handle->ispell_running) {
            ispell_result = ispell_quickcheck(handle->ispell_command);
            switch (ispell_result) {
            case SPELLING_OK:
               result = TRUE;
               break;
            case SPELLING_BAD:
               result = FALSE;
               break;
            case SPELLING_NOT_RUNNING:
               result = TRUE;
               D(bug("      disabling spell checking\n"));
               handle->ispell_running = FALSE;
               handle->check_comment = FALSE;
               handle->check_string = FALSE;
               handle->check_text = FALSE;
               break;
            default:
               assert(TRUE);
               break;
            }
         } else {
            result = TRUE;
         }
      } else {
         D(bug("      ignoring empty word\n"));
         result = TRUE;
      }
   } else {
      D(bug("*** word too long\n"));
   }
   return result;
}


void internal_assert(int expression, char *expression_text, char *file, int line)
{
   if (!expression) {
      D(bug("*** assertion failed in file \""));
      D(bug(file));
      D(bug("\", line "));
      D(kint(line));
      D(bug(": "));
      D(bug(expression_text));
      D(bug("\n"));
   }
}

/*  ----------------------------- parser_arguments -------------------------
 *
 * Parser arguments given in parser settings dialog.
 *
 * returns an IoErr()-like error code
 *
 */
LONG parse_arguments(struct MyParserHandle *handle, struct SyntaxSetup *setup)
{
#define TEMPLATE "check_comment/S,check_string/S,check_text/S,check_naming/S,language"
   UBYTE *settings_argument = setup->sp_Arguments;  // parser argument string
   ULONG length = 0;            // length of parser argument string
   STRPTR scan_buffer;          // contains copy of parser argument string with "\n"
   LONG error_code = ERROR_NO_FREE_STORE;

   static enum {
      ARG_COMMENT, // spell check comment ?
      ARG_STRING, // spell check strings ?
      ARG_TEXT, // spell check normal source code ?
      ARG_NAMING, // check for naming violations ?
      ARG_LANGUAGE, // TODO: language to use for spell checking
      ARG_MAX
   };
   LONG argument[ARG_MAX] =
   {NULL};

   D(bug("   parsing arguments: "));
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
      D(bug(scan_buffer));
   }

   if (scan_buffer != NULL) {
      struct RDArgs *scanner = (struct RDArgs *) AllocDosObject(DOS_RDARGS, NULL);

      if (scanner != NULL) {
         scanner->RDA_Source.CS_Buffer = scan_buffer;
         scanner->RDA_Source.CS_Length = (LONG) strlen(scan_buffer);

         if (ReadArgs(TEMPLATE, argument, scanner)) {

            handle->check_comment = argument[ARG_COMMENT];
            handle->check_string = argument[ARG_STRING];
            handle->check_text = argument[ARG_TEXT];
            // TODO: language

            FreeArgs(scanner);
         } else {
            D(bug("   ReadArgs() failed\n"));
         }
         FreeDosObject(DOS_RDARGS, scanner);
      }
      error_code = IoErr();
      FreeVec(scan_buffer);
   }
   return (error_code);
}



/* ------------------------------ AllocVecPooled -------------------------------
 *
 * Memory pool based replacement for AllocVec()
 *
 */

APTR
AllocVecPooled(size, flags)
     ULONG size;
     ULONG flags;
{
   ULONG *data;

   size = (size + 3) & ~3;

   ObtainSemaphore(&MemorySemaphore);

   if (data = (ULONG *) AsmAllocPooled(MemoryPool, size + sizeof(ULONG), SysBase)) {

      ReleaseSemaphore(&MemorySemaphore);

      *data++ = size + sizeof(ULONG);

      if (flags & MEMF_CLEAR) {

         register ULONG *memory = data;

         size /= sizeof(ULONG);

         do {

            *memory++ = 0;

         } while (--size);
      }
      return ((APTR) data);
   } else {

      ReleaseSemaphore(&MemorySemaphore);

      return (NULL);
   }
}

/* ------------------------------ AllocVecPooled -------------------------------
 *
 * Memory pool based replacement for FreeVec()
 *
 */

void
FreeVecPooled(memory)
     APTR memory;
{
   if (memory) {

      ULONG *data = (ULONG *) memory;

      ObtainSemaphore(&MemorySemaphore);

      AsmFreePooled(MemoryPool, &data[-1], data[-1], SysBase);

      ReleaseSemaphore(&MemorySemaphore);
   }
}

///
