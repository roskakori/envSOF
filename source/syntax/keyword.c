/*
 * keyword.c -- Handling of reserved word lists.
 *
 * Copyright 1999 Thomas Aglassinger and others, see file "forum.txt"
 */

#include "defs.h"
#include "debug.h"
#include "keyword.h"

#include <intuition/intuition.h>
#include <clib/intuition_protos.h>

#define MAXIMUM_KEYWORD_COUNT  100
#define MAXIMUM_KEYWORD_LENGTH 100

Prototype VOID create_keyword_list(VOID);
Prototype VOID dispose_keyword_info(struct keyword_info *info);
Prototype struct keyword_info *create_keyword_info(STRPTR name);
Prototype VOID dispose_keyword_list(VOID);
Prototype BOOL read_keyword_info(struct keyword_info *info, ULONG *line);
Prototype struct keyword_info *keywords_of(char *filename);



// Master list of holding all keyword info's (protected with a semaphore)
static struct keyword_list_s keyword_list;

/* Clone string contents using AllocVec() */
static char *clone_string(char *original)
{
   char *clone = AllocVec((ULONG) strlen(original)+1, MEMF_ANY);
   if (clone != NULL) {
      strcpy(clone, original);
   } else {
      SetIoErr(ERROR_NO_FREE_STORE);
   }
   return clone;
}

/* Initialize the one and only master list */
VOID create_keyword_list(VOID)
{
   InitSemaphore(&(keyword_list.semaphore));
   NewList(&keyword_list.list);
}

/* Dispose a keyword_info. Perform the following steps:
 * - decrese usage counter; if last user gone, also:
 * - unlink info from master list
 * - release all memory allocated by info
 */
VOID dispose_keyword_info(struct keyword_info *info)
{
   ObtainSemaphore(&keyword_list.semaphore);

   info->usage_count -= 1;
   if (info->usage_count == 0) {

      ULONG index = 0;

      D(bug("remove "));
      D(bug(info->node.ln_Name));
      D(bug("\n"));

      Remove((struct Node *) info);

      while (index < info->word_count) {
         FreeVec(info->word[index]);
         index += 1;
      }

      if (info->node.ln_Name != NULL) {
         FreeVec(info->node.ln_Name);
      }
      if (info->word != NULL) {
         FreeVec(info->word);
      }
      FreeVec(info);
   }
   ReleaseSemaphore(&keyword_list.semaphore);
}

/* Create a new info that know nothing but its name.
 * All other fields are set to default */
struct keyword_info *create_keyword_info(STRPTR name)
{
   struct keyword_info *info = AllocVec(sizeof(struct keyword_info), MEMF_CLEAR);

   if (info != NULL) {
      info->node.ln_Name = clone_string(name);
      if (info->node.ln_Name == NULL) {
         dispose_keyword_info(info);
         info = NULL;
      } else {
         AddTail(&keyword_list.list, (struct Node*) info);
      }

   }
   return (info);
}

/* dispose all information about all keywords;
 * this is never called by the scanner because there is no global
 * cleanup. However, this does not cause any memory leaks or the
 * like because all information is released if usage_count goes
 * to 0 in dispose_keyword_info. */
VOID dispose_keyword_list(VOID)
{
   struct Node *node = keyword_list.list.lh_Head;
   struct Node *next_node;

   while (node->ln_Succ) {
      next_node = node->ln_Succ;
      dispose_keyword_info((struct keyword_info *) node);
      node = next_node;
   }
}

/* Read keywords from a file named like info. The info has to empty,
 * which is only the case if it was created by create_keyword_info(). */
BOOL read_keyword_info(struct keyword_info *info, ULONG *line)
{
   static UBYTE buffer[MAXIMUM_KEYWORD_LENGTH];
   BOOL ok = FALSE;
   STRPTR previous_word = NULL; /* to ensure sort order */

   *line = 0;

   // TODO: don't use maximum size info
   info->word_count = 0;
   info->word = AllocVec(MAXIMUM_KEYWORD_COUNT * sizeof(char *), MEMF_ANY);
   if (info->word != NULL) {
      BPTR file = Open(info->node.ln_Name, MODE_OLDFILE);
      if (file != NULL) {
         ok = TRUE;
         ;
         while (ok && (FGets(file, buffer, MAXIMUM_KEYWORD_LENGTH) != NULL)) {
            ULONG buffer_length = strlen(buffer);
            *line = *line + 1;
            if ((buffer_length > 1) && (buffer[0] != '-')) {
               // Add new keyword to list
               STRPTR last_ch = &(buffer[buffer_length - 1]);
               // Strip trailing linefeed
               if (*last_ch == '\n') {
                  *last_ch = '\0';
               }
               if (info->word_count < MAXIMUM_KEYWORD_COUNT) {
                  info->word[info->word_count] = clone_string(buffer);
                  ok = (info->word[info->word_count] != NULL);
                  if (ok) {
                     // ensure that input is sorted
                     if ((previous_word == NULL)
                         || (strcmp(previous_word, buffer) <= 0)
                        ) {
                        previous_word = info->word[info->word_count];
                     } else {
                        SetIoErr(ERROR_INVALID_COMPONENT_NAME);
                        ok = FALSE;
                     }
                     info->word_count += 1;

#if 0
                     D(kint((int) *line));
                     D(bug("."));
                     D(kint((int) info->word_count));
                     D(bug(" = "));
                     D(bug(info->word[info->word_count]));
                     D(bug("\n"));
#endif
                  }
               } else {
                  SetIoErr(ERROR_OBJECT_TOO_LARGE);
                  ok = FALSE;
               }
            }
         }
         Close(file);
      }
   } else {
      SetIoErr(ERROR_NO_FREE_STORE);
   }

   return ok;
}

/* Return pointer to keyword info stored in `filename'. If it is
 * not in the master list allready, read it automatically. */
struct keyword_info *keywords_of(char *filename)
{
#define MAXIMUM_FAULT_LENGTH 300

   static char fault_buffer[MAXIMUM_FAULT_LENGTH];
   ULONG error_line;

   struct keyword_info *words = NULL;
   struct Node *node;
   BOOL ok = FALSE;

   ObtainSemaphore(&keyword_list.semaphore);

   node = FindName(&keyword_list.list, filename);
   if (node != NULL) {
      words = (struct keyword_info *) node;
      ok = TRUE;
   } else {
      // Read keywords from disk
      words = create_keyword_info(filename);
      if (words != NULL) {
         ok = read_keyword_info(words, &error_line);
         if (!ok) {
            dispose_keyword_info(words);
         }
      }
   }

   if (!ok) {
      /* View error message in requester */
      struct EasyStruct error_requester = {
         sizeof(struct EasyStruct),
         0,
         "Syntax Parser Problem",
         "Cannot read keywords for syntax parser.\n\n"
         "file: \"%s\"\nline: %ld%s",
         "Continue"
      };
      ULONG argument[3];

      if (IoErr() != 0) {
         Fault(IoErr(), "\ncause", fault_buffer, MAXIMUM_FAULT_LENGTH);
      } else {
         fault_buffer[0] = '\0';
      }

      argument[0] = (ULONG) filename;
      argument[1] = error_line;
      argument[2] = (ULONG) fault_buffer;

      EasyRequestArgs(NULL, &error_requester, NULL, argument);

      words = NULL;
   }

   ReleaseSemaphore(&keyword_list.semaphore);

   return words;
}

