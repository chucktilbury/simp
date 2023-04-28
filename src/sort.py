#!/usr/bin/env python3

# This is a quick and dirty sort program to sort the list of key words.
# I was not getting expected results from the Linux sort program. Longer
# words are sorted before a similar shorter word and strcmp() says that
# the longer word should be after the shorter word. File names are hard
# coded from simplicity.
import sys

infile = sys.argv[1]

#infile = "keywords.txt"
outfile = "keywords.h"

header = '''
#ifndef _KEYWORD_LIST_H
#define _KEYWORD_LIST_H
// Auto generated file. Do not modify.

static struct _token_list_ {
    TokenType type;
    const char* str;
} token_list [] = {
'''

footer = '''
    {-1, NULL}
};
// static const int num_keywords =
//                    (sizeof(keyword_list)/sizeof(struct _keywords_))-1;

#define NUM_TOK_LST (sizeof(token_list)/sizeof(struct _token_list_))-1
#endif /* _KEYWORD_LIST_H */

'''

lst = []
with open(infile, "r") as ifp:
    lst = ifp.readlines()

lst.sort()
with open(outfile, "w") as ofp:
    ofp.write(header)
    for item in lst:
        ofp.write("    "+item)
    ofp.write(footer)
