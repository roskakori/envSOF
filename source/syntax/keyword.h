/*
 * keyword.h
 *
 * Copyright 1999 Thomas Aglassinger and others, see file "forum.txt"
 */

/* Handling of reserved word lists */
#define MAXIMUM_KEYWORD_COUNT  100
#define MAXIMUM_KEYWORD_LENGTH 100

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/nodes.h>
#include <exec/semaphores.h>

// Master list of holding all keyword list nodes
struct keyword_list_s {
   struct SignalSemaphore semaphore;
   struct List list;
};

struct keyword_info {
   struct Node node;
   ULONG usage_count;
   ULONG word_count;
   STRPTR *word;
};

