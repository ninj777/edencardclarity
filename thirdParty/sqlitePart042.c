/************** Begin file tokenize.c ****************************************/
/*
** 2001 September 15
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
*************************************************************************
** An tokenizer for SQL
**
** This file contains C code that splits an SQL input string up into
** individual tokens and sends those tokens one-by-one over to the
** parser for analysis.
*/
/* #include "sqliteInt.h" */
/* #include <stdlib.h> */

/* Character classes for tokenizing
**
** In the sqlite3GetToken() function, a switch() on aiClass[c] is implemented
** using a lookup table, whereas a switch() directly on c uses a binary search.
** The lookup table is much faster.  To maximize speed, and to ensure that
** a lookup table is used, all of the classes need to be small integers and
** all of them need to be used within the switch.
*/
#define CC_X          0    /* The letter 'x', or start of BLOB literal */
#define CC_KYWD0      1    /* First letter of a keyword */
#define CC_KYWD       2    /* Alphabetics or '_'.  Usable in a keyword */
#define CC_DIGIT      3    /* Digits */
#define CC_DOLLAR     4    /* '$' */
#define CC_VARALPHA   5    /* '@', '#', ':'.  Alphabetic SQL variables */
#define CC_VARNUM     6    /* '?'.  Numeric SQL variables */
#define CC_SPACE      7    /* Space characters */
#define CC_QUOTE      8    /* '"', '\'', or '`'.  String literals, quoted ids */
#define CC_QUOTE2     9    /* '['.   [...] style quoted ids */
#define CC_PIPE      10    /* '|'.   Bitwise OR or concatenate */
#define CC_MINUS     11    /* '-'.  Minus or SQL-style comment */
#define CC_LT        12    /* '<'.  Part of < or <= or <> */
#define CC_GT        13    /* '>'.  Part of > or >= */
#define CC_EQ        14    /* '='.  Part of = or == */
#define CC_BANG      15    /* '!'.  Part of != */
#define CC_SLASH     16    /* '/'.  / or c-style comment */
#define CC_LP        17    /* '(' */
#define CC_RP        18    /* ')' */
#define CC_SEMI      19    /* ';' */
#define CC_PLUS      20    /* '+' */
#define CC_STAR      21    /* '*' */
#define CC_PERCENT   22    /* '%' */
#define CC_COMMA     23    /* ',' */
#define CC_AND       24    /* '&' */
#define CC_TILDA     25    /* '~' */
#define CC_DOT       26    /* '.' */
#define CC_ID        27    /* unicode characters usable in IDs */
#define CC_ILLEGAL   28    /* Illegal character */
#define CC_NUL       29    /* 0x00 */
#define CC_BOM       30    /* First byte of UTF8 BOM:  0xEF 0xBB 0xBF */

static const unsigned char aiClass[] = {
#ifdef SQLITE_ASCII
/*         x0  x1  x2  x3  x4  x5  x6  x7  x8  x9  xa  xb  xc  xd  xe  xf */
/* 0x */   29, 28, 28, 28, 28, 28, 28, 28, 28,  7,  7, 28,  7,  7, 28, 28,
/* 1x */   28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
/* 2x */    7, 15,  8,  5,  4, 22, 24,  8, 17, 18, 21, 20, 23, 11, 26, 16,
/* 3x */    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  5, 19, 12, 14, 13,  6,
/* 4x */    5,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
/* 5x */    1,  1,  1,  1,  1,  1,  1,  1,  0,  2,  2,  9, 28, 28, 28,  2,
/* 6x */    8,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
/* 7x */    1,  1,  1,  1,  1,  1,  1,  1,  0,  2,  2, 28, 10, 28, 25, 28,
/* 8x */   27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
/* 9x */   27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
/* Ax */   27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
/* Bx */   27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
/* Cx */   27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
/* Dx */   27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
/* Ex */   27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 30,
/* Fx */   27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27
#endif
#ifdef SQLITE_EBCDIC
/*         x0  x1  x2  x3  x4  x5  x6  x7  x8  x9  xa  xb  xc  xd  xe  xf */
/* 0x */   29, 28, 28, 28, 28,  7, 28, 28, 28, 28, 28, 28,  7,  7, 28, 28,
/* 1x */   28, 28, 28, 28, 28,  7, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
/* 2x */   28, 28, 28, 28, 28,  7, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
/* 3x */   28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
/* 4x */    7, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 26, 12, 17, 20, 10,
/* 5x */   24, 28, 28, 28, 28, 28, 28, 28, 28, 28, 15,  4, 21, 18, 19, 28,
/* 6x */   11, 16, 28, 28, 28, 28, 28, 28, 28, 28, 28, 23, 22,  2, 13,  6,
/* 7x */   28, 28, 28, 28, 28, 28, 28, 28, 28,  8,  5,  5,  5,  8, 14,  8,
/* 8x */   28,  1,  1,  1,  1,  1,  1,  1,  1,  1, 28, 28, 28, 28, 28, 28,
/* 9x */   28,  1,  1,  1,  1,  1,  1,  1,  1,  1, 28, 28, 28, 28, 28, 28,
/* Ax */   28, 25,  1,  1,  1,  1,  1,  0,  2,  2, 28, 28, 28, 28, 28, 28,
/* Bx */   28, 28, 28, 28, 28, 28, 28, 28, 28, 28,  9, 28, 28, 28, 28, 28,
/* Cx */   28,  1,  1,  1,  1,  1,  1,  1,  1,  1, 28, 28, 28, 28, 28, 28,
/* Dx */   28,  1,  1,  1,  1,  1,  1,  1,  1,  1, 28, 28, 28, 28, 28, 28,
/* Ex */   28, 28,  1,  1,  1,  1,  1,  0,  2,  2, 28, 28, 28, 28, 28, 28,
/* Fx */    3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 28, 28, 28, 28, 28, 28,
#endif
};

/*
** The charMap() macro maps alphabetic characters (only) into their
** lower-case ASCII equivalent.  On ASCII machines, this is just
** an upper-to-lower case map.  On EBCDIC machines we also need
** to adjust the encoding.  The mapping is only valid for alphabetics
** which are the only characters for which this feature is used.
**
** Used by keywordhash.h
*/
#ifdef SQLITE_ASCII
# define charMap(X) sqlite3UpperToLower[(unsigned char)X]
#endif
#ifdef SQLITE_EBCDIC
# define charMap(X) ebcdicToAscii[(unsigned char)X]
const unsigned char ebcdicToAscii[] = {
/* 0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F */
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  /* 0x */
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  /* 1x */
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  /* 2x */
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  /* 3x */
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  /* 4x */
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  /* 5x */
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 95,  0,  0,  /* 6x */
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  /* 7x */
   0, 97, 98, 99,100,101,102,103,104,105,  0,  0,  0,  0,  0,  0,  /* 8x */
   0,106,107,108,109,110,111,112,113,114,  0,  0,  0,  0,  0,  0,  /* 9x */
   0,  0,115,116,117,118,119,120,121,122,  0,  0,  0,  0,  0,  0,  /* Ax */
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  /* Bx */
   0, 97, 98, 99,100,101,102,103,104,105,  0,  0,  0,  0,  0,  0,  /* Cx */
   0,106,107,108,109,110,111,112,113,114,  0,  0,  0,  0,  0,  0,  /* Dx */
   0,  0,115,116,117,118,119,120,121,122,  0,  0,  0,  0,  0,  0,  /* Ex */
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  /* Fx */
};
#endif

/*
** The sqlite3KeywordCode function looks up an identifier to determine if
** it is a keyword.  If it is a keyword, the token code of that keyword is
** returned.  If the input is not a keyword, TK_ID is returned.
**
** The implementation of this routine was generated by a program,
** mkkeywordhash.c, located in the tool subdirectory of the distribution.
** The output of the mkkeywordhash.c program is written into a file
** named keywordhash.h and then included into this source file by
** the #include below.
*/
/************** Include keywordhash.h in the middle of tokenize.c ************/
/************** Begin file keywordhash.h *************************************/
/***** This file contains automatically generated code ******
**
** The code in this file has been automatically generated by
**
**   sqlite/tool/mkkeywordhash.c
**
** The code in this file implements a function that determines whether
** or not a given identifier is really an SQL keyword.  The same thing
** might be implemented more directly using a hand-written hash table.
** But by using this automatically generated code, the size of the code
** is substantially reduced.  This is important for embedded applications
** on platforms with limited memory.
*/
/* Hash score: 231 */
/* zKWText[] encodes 1007 bytes of keyword text in 667 bytes */
/*   REINDEXEDESCAPEACHECKEYBEFOREIGNOREGEXPLAINSTEADDATABASELECT       */
/*   ABLEFTHENDEFERRABLELSEXCLUDELETEMPORARYISNULLSAVEPOINTERSECT       */
/*   IESNOTNULLIKEXCEPTRANSACTIONATURALTERAISEXCLUSIVEXISTS             */
/*   CONSTRAINTOFFSETRIGGERANGENERATEDETACHAVINGLOBEGINNEREFERENCES     */
/*   UNIQUERYWITHOUTERELEASEATTACHBETWEENOTHINGROUPSCASCADEFAULT        */
/*   CASECOLLATECREATECURRENT_DATEIMMEDIATEJOINSERTMATCHPLANALYZE       */
/*   PRAGMATERIALIZEDEFERREDISTINCTUPDATEVALUESVIRTUALWAYSWHENWHERE     */
/*   CURSIVEABORTAFTERENAMEANDROPARTITIONAUTOINCREMENTCASTCOLUMN        */
/*   COMMITCONFLICTCROSSCURRENT_TIMESTAMPRECEDINGFAILASTFILTER          */
/*   EPLACEFIRSTFOLLOWINGFROMFULLIMITIFORDERESTRICTOTHERSOVER           */
/*   ETURNINGRIGHTROLLBACKROWSUNBOUNDEDUNIONUSINGVACUUMVIEWINDOWBY      */
/*   INITIALLYPRIMARY                                                   */
static const char zKWText[666] = {
  'R','E','I','N','D','E','X','E','D','E','S','C','A','P','E','A','C','H',
  'E','C','K','E','Y','B','E','F','O','R','E','I','G','N','O','R','E','G',
  'E','X','P','L','A','I','N','S','T','E','A','D','D','A','T','A','B','A',
  'S','E','L','E','C','T','A','B','L','E','F','T','H','E','N','D','E','F',
  'E','R','R','A','B','L','E','L','S','E','X','C','L','U','D','E','L','E',
  'T','E','M','P','O','R','A','R','Y','I','S','N','U','L','L','S','A','V',
  'E','P','O','I','N','T','E','R','S','E','C','T','I','E','S','N','O','T',
  'N','U','L','L','I','K','E','X','C','E','P','T','R','A','N','S','A','C',
  'T','I','O','N','A','T','U','R','A','L','T','E','R','A','I','S','E','X',
  'C','L','U','S','I','V','E','X','I','S','T','S','C','O','N','S','T','R',
  'A','I','N','T','O','F','F','S','E','T','R','I','G','G','E','R','A','N',
  'G','E','N','E','R','A','T','E','D','E','T','A','C','H','A','V','I','N',
  'G','L','O','B','E','G','I','N','N','E','R','E','F','E','R','E','N','C',
  'E','S','U','N','I','Q','U','E','R','Y','W','I','T','H','O','U','T','E',
  'R','E','L','E','A','S','E','A','T','T','A','C','H','B','E','T','W','E',
  'E','N','O','T','H','I','N','G','R','O','U','P','S','C','A','S','C','A',
  'D','E','F','A','U','L','T','C','A','S','E','C','O','L','L','A','T','E',
  'C','R','E','A','T','E','C','U','R','R','E','N','T','_','D','A','T','E',
  'I','M','M','E','D','I','A','T','E','J','O','I','N','S','E','R','T','M',
  'A','T','C','H','P','L','A','N','A','L','Y','Z','E','P','R','A','G','M',
  'A','T','E','R','I','A','L','I','Z','E','D','E','F','E','R','R','E','D',
  'I','S','T','I','N','C','T','U','P','D','A','T','E','V','A','L','U','E',
  'S','V','I','R','T','U','A','L','W','A','Y','S','W','H','E','N','W','H',
  'E','R','E','C','U','R','S','I','V','E','A','B','O','R','T','A','F','T',
  'E','R','E','N','A','M','E','A','N','D','R','O','P','A','R','T','I','T',
  'I','O','N','A','U','T','O','I','N','C','R','E','M','E','N','T','C','A',
  'S','T','C','O','L','U','M','N','C','O','M','M','I','T','C','O','N','F',
  'L','I','C','T','C','R','O','S','S','C','U','R','R','E','N','T','_','T',
  'I','M','E','S','T','A','M','P','R','E','C','E','D','I','N','G','F','A',
  'I','L','A','S','T','F','I','L','T','E','R','E','P','L','A','C','E','F',
  'I','R','S','T','F','O','L','L','O','W','I','N','G','F','R','O','M','F',
  'U','L','L','I','M','I','T','I','F','O','R','D','E','R','E','S','T','R',
  'I','C','T','O','T','H','E','R','S','O','V','E','R','E','T','U','R','N',
  'I','N','G','R','I','G','H','T','R','O','L','L','B','A','C','K','R','O',
  'W','S','U','N','B','O','U','N','D','E','D','U','N','I','O','N','U','S',
  'I','N','G','V','A','C','U','U','M','V','I','E','W','I','N','D','O','W',
  'B','Y','I','N','I','T','I','A','L','L','Y','P','R','I','M','A','R','Y',
};
/* aKWHash[i] is the hash value for the i-th keyword */
static const unsigned char aKWHash[127] = {
    84,  92, 134,  82, 105,  29,   0,   0,  94,   0,  85,  72,   0,
    53,  35,  86,  15,   0,  42,  97,  54,  89, 135,  19,   0,   0,
   140,   0,  40, 129,   0,  22, 107,   0,   9,   0,   0, 123,  80,
     0,  78,   6,   0,  65, 103, 147,   0, 136, 115,   0,   0,  48,
     0,  90,  24,   0,  17,   0,  27,  70,  23,  26,   5,  60, 142,
   110, 122,   0,  73,  91,  71, 145,  61, 120,  74,   0,  49,   0,
    11,  41,   0, 113,   0,   0,   0, 109,  10, 111, 116, 125,  14,
    50, 124,   0, 100,   0,  18, 121, 144,  56, 130, 139,  88,  83,
    37,  30, 126,   0,   0, 108,  51, 131, 128,   0,  34,   0,   0,
   132,   0,  98,  38,  39,   0,  20,  45, 117,  93,
};
/* aKWNext[] forms the hash collision chain.  If aKWHash[i]==0
** then the i-th keyword has no more hash collisions.  Otherwise,
** the next keyword with the same hash is aKWHash[i]-1. */
static const unsigned char aKWNext[148] = {0,
     0,   0,   0,   0,   4,   0,  43,   0,   0, 106, 114,   0,   0,
     0,   2,   0,   0, 143,   0,   0,   0,  13,   0,   0,   0,   0,
   141,   0,   0, 119,  52,   0,   0, 137,  12,   0,   0,  62,   0,
   138,   0, 133,   0,   0,  36,   0,   0,  28,  77,   0,   0,   0,
     0,  59,   0,  47,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     0,  69,   0,   0,   0,   0,   0, 146,   3,   0,  58,   0,   1,
    75,   0,   0,   0,  31,   0,   0,   0,   0,   0, 127,   0, 104,
     0,  64,  66,  63,   0,   0,   0,   0,   0,  46,   0,  16,   8,
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  81, 101,   0,
   112,  21,   7,  67,   0,  79,  96, 118,   0,   0,  68,   0,   0,
    99,  44,   0,  55,   0,  76,   0,  95,  32,  33,  57,  25,   0,
   102,   0,   0,  87,
};
/* aKWLen[i] is the length (in bytes) of the i-th keyword */
static const unsigned char aKWLen[148] = {0,
     7,   7,   5,   4,   6,   4,   5,   3,   6,   7,   3,   6,   6,
     7,   7,   3,   8,   2,   6,   5,   4,   4,   3,  10,   4,   7,
     6,   9,   4,   2,   6,   5,   9,   9,   4,   7,   3,   2,   4,
     4,   6,  11,   6,   2,   7,   5,   5,   9,   6,  10,   4,   6,
     2,   3,   7,   5,   9,   6,   6,   4,   5,   5,  10,   6,   5,
     7,   4,   5,   7,   6,   7,   7,   6,   5,   7,   3,   7,   4,
     7,   6,  12,   9,   4,   6,   5,   4,   7,   6,  12,   8,   8,
     2,   6,   6,   7,   6,   4,   5,   9,   5,   5,   6,   3,   4,
     9,  13,   2,   2,   4,   6,   6,   8,   5,  17,  12,   7,   9,
     4,   4,   6,   7,   5,   9,   4,   4,   5,   2,   5,   8,   6,
     4,   9,   5,   8,   4,   3,   9,   5,   5,   6,   4,   6,   2,
     2,   9,   3,   7,
};
/* aKWOffset[i] is the index into zKWText[] of the start of
** the text for the i-th keyword. */
static const unsigned short int aKWOffset[148] = {0,
     0,   2,   2,   8,   9,  14,  16,  20,  23,  25,  25,  29,  33,
    36,  41,  46,  48,  53,  54,  59,  62,  65,  67,  69,  78,  81,
    86,  90,  90,  94,  99, 101, 105, 111, 119, 123, 123, 123, 126,
   129, 132, 137, 142, 146, 147, 152, 156, 160, 168, 174, 181, 184,
   184, 187, 189, 195, 198, 206, 211, 216, 219, 222, 226, 236, 239,
   244, 244, 248, 252, 259, 265, 271, 277, 277, 283, 284, 288, 295,
   299, 306, 312, 324, 333, 335, 341, 346, 348, 355, 359, 370, 377,
   378, 385, 391, 397, 402, 408, 412, 415, 424, 429, 433, 439, 441,
   444, 453, 455, 457, 466, 470, 476, 482, 490, 495, 495, 495, 511,
   520, 523, 527, 532, 539, 544, 553, 557, 560, 565, 567, 571, 579,
   585, 588, 597, 602, 610, 610, 614, 623, 628, 633, 639, 642, 645,
   648, 650, 655, 659,
};
/* aKWCode[i] is the parser symbol code for the i-th keyword */
static const unsigned char aKWCode[148] = {0,
  TK_REINDEX,    TK_INDEXED,    TK_INDEX,      TK_DESC,       TK_ESCAPE,
  TK_EACH,       TK_CHECK,      TK_KEY,        TK_BEFORE,     TK_FOREIGN,
  TK_FOR,        TK_IGNORE,     TK_LIKE_KW,    TK_EXPLAIN,    TK_INSTEAD,
  TK_ADD,        TK_DATABASE,   TK_AS,         TK_SELECT,     TK_TABLE,
  TK_JOIN_KW,    TK_THEN,       TK_END,        TK_DEFERRABLE, TK_ELSE,
  TK_EXCLUDE,    TK_DELETE,     TK_TEMP,       TK_TEMP,       TK_OR,
  TK_ISNULL,     TK_NULLS,      TK_SAVEPOINT,  TK_INTERSECT,  TK_TIES,
  TK_NOTNULL,    TK_NOT,        TK_NO,         TK_NULL,       TK_LIKE_KW,
  TK_EXCEPT,     TK_TRANSACTION,TK_ACTION,     TK_ON,         TK_JOIN_KW,
  TK_ALTER,      TK_RAISE,      TK_EXCLUSIVE,  TK_EXISTS,     TK_CONSTRAINT,
  TK_INTO,       TK_OFFSET,     TK_OF,         TK_SET,        TK_TRIGGER,
  TK_RANGE,      TK_GENERATED,  TK_DETACH,     TK_HAVING,     TK_LIKE_KW,
  TK_BEGIN,      TK_JOIN_KW,    TK_REFERENCES, TK_UNIQUE,     TK_QUERY,
  TK_WITHOUT,    TK_WITH,       TK_JOIN_KW,    TK_RELEASE,    TK_ATTACH,
  TK_BETWEEN,    TK_NOTHING,    TK_GROUPS,     TK_GROUP,      TK_CASCADE,
  TK_ASC,        TK_DEFAULT,    TK_CASE,       TK_COLLATE,    TK_CREATE,
  TK_CTIME_KW,   TK_IMMEDIATE,  TK_JOIN,       TK_INSERT,     TK_MATCH,
  TK_PLAN,       TK_ANALYZE,    TK_PRAGMA,     TK_MATERIALIZED, TK_DEFERRED,
  TK_DISTINCT,   TK_IS,         TK_UPDATE,     TK_VALUES,     TK_VIRTUAL,
  TK_ALWAYS,     TK_WHEN,       TK_WHERE,      TK_RECURSIVE,  TK_ABORT,
  TK_AFTER,      TK_RENAME,     TK_AND,        TK_DROP,       TK_PARTITION,
  TK_AUTOINCR,   TK_TO,         TK_IN,         TK_CAST,       TK_COLUMNKW,
  TK_COMMIT,     TK_CONFLICT,   TK_JOIN_KW,    TK_CTIME_KW,   TK_CTIME_KW,
  TK_CURRENT,    TK_PRECEDING,  TK_FAIL,       TK_LAST,       TK_FILTER,
  TK_REPLACE,    TK_FIRST,      TK_FOLLOWING,  TK_FROM,       TK_JOIN_KW,
  TK_LIMIT,      TK_IF,         TK_ORDER,      TK_RESTRICT,   TK_OTHERS,
  TK_OVER,       TK_RETURNING,  TK_JOIN_KW,    TK_ROLLBACK,   TK_ROWS,
  TK_ROW,        TK_UNBOUNDED,  TK_UNION,      TK_USING,      TK_VACUUM,
  TK_VIEW,       TK_WINDOW,     TK_DO,         TK_BY,         TK_INITIALLY,
  TK_ALL,        TK_PRIMARY,
};
/* Hash table decoded:
**   0: INSERT
**   1: IS
**   2: ROLLBACK TRIGGER
**   3: IMMEDIATE
**   4: PARTITION
**   5: TEMP
**   6:
**   7:
**   8: VALUES WITHOUT
**   9:
**  10: MATCH
**  11: NOTHING
**  12:
**  13: OF
**  14: TIES IGNORE
**  15: PLAN
**  16: INSTEAD INDEXED
**  17:
**  18: TRANSACTION RIGHT
**  19: WHEN
**  20: SET HAVING
**  21: MATERIALIZED IF
**  22: ROWS
**  23: SELECT
**  24:
**  25:
**  26: VACUUM SAVEPOINT
**  27:
**  28: LIKE UNION VIRTUAL REFERENCES
**  29: RESTRICT
**  30:
**  31: THEN REGEXP
**  32: TO
**  33:
**  34: BEFORE
**  35:
**  36:
**  37: FOLLOWING COLLATE CASCADE
**  38: CREATE
**  39:
**  40: CASE REINDEX
**  41: EACH
**  42:
**  43: QUERY
**  44: AND ADD
**  45: PRIMARY ANALYZE
**  46:
**  47: ROW ASC DETACH
**  48: CURRENT_TIME CURRENT_DATE
**  49:
**  50:
**  51: EXCLUSIVE TEMPORARY
**  52:
**  53: DEFERRED
**  54: DEFERRABLE
**  55:
**  56: DATABASE
**  57:
**  58: DELETE VIEW GENERATED
**  59: ATTACH
**  60: END
**  61: EXCLUDE
**  62: ESCAPE DESC
**  63: GLOB
**  64: WINDOW ELSE
**  65: COLUMN
**  66: FIRST
**  67:
**  68: GROUPS ALL
**  69: DISTINCT DROP KEY
**  70: BETWEEN
**  71: INITIALLY
**  72: BEGIN
**  73: FILTER CHECK ACTION
**  74: GROUP INDEX
**  75:
**  76: EXISTS DEFAULT
**  77:
**  78: FOR CURRENT_TIMESTAMP
**  79: EXCEPT
**  80:
**  81: CROSS
**  82:
**  83:
**  84:
**  85: CAST
**  86: FOREIGN AUTOINCREMENT
**  87: COMMIT
**  88: CURRENT AFTER ALTER
**  89: FULL FAIL CONFLICT
**  90: EXPLAIN
**  91: CONSTRAINT
**  92: FROM ALWAYS
**  93:
**  94: ABORT
**  95:
**  96: AS DO
**  97: REPLACE WITH RELEASE
**  98: BY RENAME
**  99: RANGE RAISE
** 100: OTHERS
** 101: USING NULLS
** 102: PRAGMA
** 103: JOIN ISNULL OFFSET
** 104: NOT
** 105: OR LAST LEFT
** 106: LIMIT
** 107:
** 108:
** 109: IN
** 110: INTO
** 111: OVER RECURSIVE
** 112: ORDER OUTER
** 113:
** 114: INTERSECT UNBOUNDED
** 115:
** 116:
** 117: RETURNING ON
** 118:
** 119: WHERE
** 120: NO INNER
** 121: NULL
** 122:
** 123: TABLE
** 124: NATURAL NOTNULL
** 125: PRECEDING
** 126: UPDATE UNIQUE
*/
/* Check to see if z[0..n-1] is a keyword. If it is, write the
** parser symbol code for that keyword into *pType.  Always
** return the integer n (the length of the token). */
static i64 keywordCode(const char *z, i64 n, int *pType){
  i64 i, j;
  const char *zKW;
  assert( n>=2 );
  i = ((charMap(z[0])*4) ^ (charMap(z[n-1])*3) ^ n*1) % 127;
  for(i=(int)aKWHash[i]; i>0; i=aKWNext[i]){
    if( aKWLen[i]!=n ) continue;
    zKW = &zKWText[aKWOffset[i]];
#ifdef SQLITE_ASCII
    if( (z[0]&~0x20)!=zKW[0] ) continue;
    if( (z[1]&~0x20)!=zKW[1] ) continue;
    j = 2;
    while( j<n && (z[j]&~0x20)==zKW[j] ){ j++; }
#endif
#ifdef SQLITE_EBCDIC
    if( toupper(z[0])!=zKW[0] ) continue;
    if( toupper(z[1])!=zKW[1] ) continue;
    j = 2;
    while( j<n && toupper(z[j])==zKW[j] ){ j++; }
#endif
    if( j<n ) continue;
    testcase( i==1 ); /* REINDEX */
    testcase( i==2 ); /* INDEXED */
    testcase( i==3 ); /* INDEX */
    testcase( i==4 ); /* DESC */
    testcase( i==5 ); /* ESCAPE */
    testcase( i==6 ); /* EACH */
    testcase( i==7 ); /* CHECK */
    testcase( i==8 ); /* KEY */
    testcase( i==9 ); /* BEFORE */
    testcase( i==10 ); /* FOREIGN */
    testcase( i==11 ); /* FOR */
    testcase( i==12 ); /* IGNORE */
    testcase( i==13 ); /* REGEXP */
    testcase( i==14 ); /* EXPLAIN */
    testcase( i==15 ); /* INSTEAD */
    testcase( i==16 ); /* ADD */
    testcase( i==17 ); /* DATABASE */
    testcase( i==18 ); /* AS */
    testcase( i==19 ); /* SELECT */
    testcase( i==20 ); /* TABLE */
    testcase( i==21 ); /* LEFT */
    testcase( i==22 ); /* THEN */
    testcase( i==23 ); /* END */
    testcase( i==24 ); /* DEFERRABLE */
    testcase( i==25 ); /* ELSE */
    testcase( i==26 ); /* EXCLUDE */
    testcase( i==27 ); /* DELETE */
    testcase( i==28 ); /* TEMPORARY */
    testcase( i==29 ); /* TEMP */
    testcase( i==30 ); /* OR */
    testcase( i==31 ); /* ISNULL */
    testcase( i==32 ); /* NULLS */
    testcase( i==33 ); /* SAVEPOINT */
    testcase( i==34 ); /* INTERSECT */
    testcase( i==35 ); /* TIES */
    testcase( i==36 ); /* NOTNULL */
    testcase( i==37 ); /* NOT */
    testcase( i==38 ); /* NO */
    testcase( i==39 ); /* NULL */
    testcase( i==40 ); /* LIKE */
    testcase( i==41 ); /* EXCEPT */
    testcase( i==42 ); /* TRANSACTION */
    testcase( i==43 ); /* ACTION */
    testcase( i==44 ); /* ON */
    testcase( i==45 ); /* NATURAL */
    testcase( i==46 ); /* ALTER */
    testcase( i==47 ); /* RAISE */
    testcase( i==48 ); /* EXCLUSIVE */
    testcase( i==49 ); /* EXISTS */
    testcase( i==50 ); /* CONSTRAINT */
    testcase( i==51 ); /* INTO */
    testcase( i==52 ); /* OFFSET */
    testcase( i==53 ); /* OF */
    testcase( i==54 ); /* SET */
    testcase( i==55 ); /* TRIGGER */
    testcase( i==56 ); /* RANGE */
    testcase( i==57 ); /* GENERATED */
    testcase( i==58 ); /* DETACH */
    testcase( i==59 ); /* HAVING */
    testcase( i==60 ); /* GLOB */
    testcase( i==61 ); /* BEGIN */
    testcase( i==62 ); /* INNER */
    testcase( i==63 ); /* REFERENCES */
    testcase( i==64 ); /* UNIQUE */
    testcase( i==65 ); /* QUERY */
    testcase( i==66 ); /* WITHOUT */
    testcase( i==67 ); /* WITH */
    testcase( i==68 ); /* OUTER */
    testcase( i==69 ); /* RELEASE */
    testcase( i==70 ); /* ATTACH */
    testcase( i==71 ); /* BETWEEN */
    testcase( i==72 ); /* NOTHING */
    testcase( i==73 ); /* GROUPS */
    testcase( i==74 ); /* GROUP */
    testcase( i==75 ); /* CASCADE */
    testcase( i==76 ); /* ASC */
    testcase( i==77 ); /* DEFAULT */
    testcase( i==78 ); /* CASE */
    testcase( i==79 ); /* COLLATE */
    testcase( i==80 ); /* CREATE */
    testcase( i==81 ); /* CURRENT_DATE */
    testcase( i==82 ); /* IMMEDIATE */
    testcase( i==83 ); /* JOIN */
    testcase( i==84 ); /* INSERT */
    testcase( i==85 ); /* MATCH */
    testcase( i==86 ); /* PLAN */
    testcase( i==87 ); /* ANALYZE */
    testcase( i==88 ); /* PRAGMA */
    testcase( i==89 ); /* MATERIALIZED */
    testcase( i==90 ); /* DEFERRED */
    testcase( i==91 ); /* DISTINCT */
    testcase( i==92 ); /* IS */
    testcase( i==93 ); /* UPDATE */
    testcase( i==94 ); /* VALUES */
    testcase( i==95 ); /* VIRTUAL */
    testcase( i==96 ); /* ALWAYS */
    testcase( i==97 ); /* WHEN */
    testcase( i==98 ); /* WHERE */
    testcase( i==99 ); /* RECURSIVE */
    testcase( i==100 ); /* ABORT */
    testcase( i==101 ); /* AFTER */
    testcase( i==102 ); /* RENAME */
    testcase( i==103 ); /* AND */
    testcase( i==104 ); /* DROP */
    testcase( i==105 ); /* PARTITION */
    testcase( i==106 ); /* AUTOINCREMENT */
    testcase( i==107 ); /* TO */
    testcase( i==108 ); /* IN */
    testcase( i==109 ); /* CAST */
    testcase( i==110 ); /* COLUMN */
    testcase( i==111 ); /* COMMIT */
    testcase( i==112 ); /* CONFLICT */
    testcase( i==113 ); /* CROSS */
    testcase( i==114 ); /* CURRENT_TIMESTAMP */
    testcase( i==115 ); /* CURRENT_TIME */
    testcase( i==116 ); /* CURRENT */
    testcase( i==117 ); /* PRECEDING */
    testcase( i==118 ); /* FAIL */
    testcase( i==119 ); /* LAST */
    testcase( i==120 ); /* FILTER */
    testcase( i==121 ); /* REPLACE */
    testcase( i==122 ); /* FIRST */
    testcase( i==123 ); /* FOLLOWING */
    testcase( i==124 ); /* FROM */
    testcase( i==125 ); /* FULL */
    testcase( i==126 ); /* LIMIT */
    testcase( i==127 ); /* IF */
    testcase( i==128 ); /* ORDER */
    testcase( i==129 ); /* RESTRICT */
    testcase( i==130 ); /* OTHERS */
    testcase( i==131 ); /* OVER */
    testcase( i==132 ); /* RETURNING */
    testcase( i==133 ); /* RIGHT */
    testcase( i==134 ); /* ROLLBACK */
    testcase( i==135 ); /* ROWS */
    testcase( i==136 ); /* ROW */
    testcase( i==137 ); /* UNBOUNDED */
    testcase( i==138 ); /* UNION */
    testcase( i==139 ); /* USING */
    testcase( i==140 ); /* VACUUM */
    testcase( i==141 ); /* VIEW */
    testcase( i==142 ); /* WINDOW */
    testcase( i==143 ); /* DO */
    testcase( i==144 ); /* BY */
    testcase( i==145 ); /* INITIALLY */
    testcase( i==146 ); /* ALL */
    testcase( i==147 ); /* PRIMARY */
    *pType = aKWCode[i];
    break;
  }
  return n;
}
SQLITE_PRIVATE int sqlite3KeywordCode(const unsigned char *z, int n){
  int id = TK_ID;
  if( n>=2 ) keywordCode((char*)z, n, &id);
  return id;
}
#define SQLITE_N_KEYWORD 147
SQLITE_API int sqlite3_keyword_name(int i,const char **pzName,int *pnName){
  if( i<0 || i>=SQLITE_N_KEYWORD ) return SQLITE_ERROR;
  i++;
  *pzName = zKWText + aKWOffset[i];
  *pnName = aKWLen[i];
  return SQLITE_OK;
}
SQLITE_API int sqlite3_keyword_count(void){ return SQLITE_N_KEYWORD; }
SQLITE_API int sqlite3_keyword_check(const char *zName, int nName){
  return TK_ID!=sqlite3KeywordCode((const u8*)zName, nName);
}

/************** End of keywordhash.h *****************************************/
/************** Continuing where we left off in tokenize.c *******************/


/*
** If X is a character that can be used in an identifier then
** IdChar(X) will be true.  Otherwise it is false.
**
** For ASCII, any character with the high-order bit set is
** allowed in an identifier.  For 7-bit characters,
** sqlite3IsIdChar[X] must be 1.
**
** For EBCDIC, the rules are more complex but have the same
** end result.
**
** Ticket #1066.  the SQL standard does not allow '$' in the
** middle of identifiers.  But many SQL implementations do.
** SQLite will allow '$' in identifiers for compatibility.
** But the feature is undocumented.
*/
#ifdef SQLITE_ASCII
#define IdChar(C)  ((sqlite3CtypeMap[(unsigned char)C]&0x46)!=0)
#endif
#ifdef SQLITE_EBCDIC
SQLITE_PRIVATE const char sqlite3IsEbcdicIdChar[] = {
/* x0 x1 x2 x3 x4 x5 x6 x7 x8 x9 xA xB xC xD xE xF */
    0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,  /* 4x */
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 0,  /* 5x */
    0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0,  /* 6x */
    0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,  /* 7x */
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0,  /* 8x */
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 1, 0,  /* 9x */
    1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0,  /* Ax */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* Bx */
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1,  /* Cx */
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1,  /* Dx */
    0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1,  /* Ex */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0,  /* Fx */
};
#define IdChar(C)  (((c=C)>=0x42 && sqlite3IsEbcdicIdChar[c-0x40]))
#endif

/* Make the IdChar function accessible from ctime.c and alter.c */
SQLITE_PRIVATE int sqlite3IsIdChar(u8 c){ return IdChar(c); }

#ifndef SQLITE_OMIT_WINDOWFUNC
/*
** Return the id of the next token in string (*pz). Before returning, set
** (*pz) to point to the byte following the parsed token.
*/
static int getToken(const unsigned char **pz){
  const unsigned char *z = *pz;
  int t;                          /* Token type to return */
  do {
    z += sqlite3GetToken(z, &t);
  }while( t==TK_SPACE || t==TK_COMMENT );
  if( t==TK_ID
   || t==TK_STRING
   || t==TK_JOIN_KW
   || t==TK_WINDOW
   || t==TK_OVER
   || sqlite3ParserFallback(t)==TK_ID
  ){
    t = TK_ID;
  }
  *pz = z;
  return t;
}

/*
** The following three functions are called immediately after the tokenizer
** reads the keywords WINDOW, OVER and FILTER, respectively, to determine
** whether the token should be treated as a keyword or an SQL identifier.
** This cannot be handled by the usual lemon %fallback method, due to
** the ambiguity in some constructions. e.g.
**
**   SELECT sum(x) OVER ...
**
** In the above, "OVER" might be a keyword, or it might be an alias for the
** sum(x) expression. If a "%fallback ID OVER" directive were added to
** grammar, then SQLite would always treat "OVER" as an alias, making it
** impossible to call a window-function without a FILTER clause.
**
** WINDOW is treated as a keyword if:
**
**   * the following token is an identifier, or a keyword that can fallback
**     to being an identifier, and
**   * the token after than one is TK_AS.
**
** OVER is a keyword if:
**
**   * the previous token was TK_RP, and
**   * the next token is either TK_LP or an identifier.
**
** FILTER is a keyword if:
**
**   * the previous token was TK_RP, and
**   * the next token is TK_LP.
*/
static int analyzeWindowKeyword(const unsigned char *z){
  int t;
  t = getToken(&z);
  if( t!=TK_ID ) return TK_ID;
  t = getToken(&z);
  if( t!=TK_AS ) return TK_ID;
  return TK_WINDOW;
}
static int analyzeOverKeyword(const unsigned char *z, int lastToken){
  if( lastToken==TK_RP ){
    int t = getToken(&z);
    if( t==TK_LP || t==TK_ID ) return TK_OVER;
  }
  return TK_ID;
}
static int analyzeFilterKeyword(const unsigned char *z, int lastToken){
  if( lastToken==TK_RP && getToken(&z)==TK_LP ){
    return TK_FILTER;
  }
  return TK_ID;
}
#endif /* SQLITE_OMIT_WINDOWFUNC */

/*
** Return the length (in bytes) of the token that begins at z[0].
** Store the token type in *tokenType before returning.
*/
SQLITE_PRIVATE i64 sqlite3GetToken(const unsigned char *z, int *tokenType){
  i64 i;
  int c;
  switch( aiClass[*z] ){  /* Switch on the character-class of the first byte
                          ** of the token. See the comment on the CC_ defines
                          ** above. */
    case CC_SPACE: {
      testcase( z[0]==' ' );
      testcase( z[0]=='\t' );
      testcase( z[0]=='\n' );
      testcase( z[0]=='\f' );
      testcase( z[0]=='\r' );
      for(i=1; sqlite3Isspace(z[i]); i++){}
      *tokenType = TK_SPACE;
      return i;
    }
    case CC_MINUS: {
      if( z[1]=='-' ){
        for(i=2; (c=z[i])!=0 && c!='\n'; i++){}
        *tokenType = TK_COMMENT;
        return i;
      }else if( z[1]=='>' ){
        *tokenType = TK_PTR;
        return 2 + (z[2]=='>');
      }
      *tokenType = TK_MINUS;
      return 1;
    }
    case CC_LP: {
      *tokenType = TK_LP;
      return 1;
    }
    case CC_RP: {
      *tokenType = TK_RP;
      return 1;
    }
    case CC_SEMI: {
      *tokenType = TK_SEMI;
      return 1;
    }
    case CC_PLUS: {
      *tokenType = TK_PLUS;
      return 1;
    }
    case CC_STAR: {
      *tokenType = TK_STAR;
      return 1;
    }
    case CC_SLASH: {
      if( z[1]!='*' || z[2]==0 ){
        *tokenType = TK_SLASH;
        return 1;
      }
      for(i=3, c=z[2]; (c!='*' || z[i]!='/') && (c=z[i])!=0; i++){}
      if( c ) i++;
      *tokenType = TK_COMMENT;
      return i;
    }
    case CC_PERCENT: {
      *tokenType = TK_REM;
      return 1;
    }
    case CC_EQ: {
      *tokenType = TK_EQ;
      return 1 + (z[1]=='=');
    }
    case CC_LT: {
      if( (c=z[1])=='=' ){
        *tokenType = TK_LE;
        return 2;
      }else if( c=='>' ){
        *tokenType = TK_NE;
        return 2;
      }else if( c=='<' ){
        *tokenType = TK_LSHIFT;
        return 2;
      }else{
        *tokenType = TK_LT;
        return 1;
      }
    }
    case CC_GT: {
      if( (c=z[1])=='=' ){
        *tokenType = TK_GE;
        return 2;
      }else if( c=='>' ){
        *tokenType = TK_RSHIFT;
        return 2;
      }else{
        *tokenType = TK_GT;
        return 1;
      }
    }
    case CC_BANG: {
      if( z[1]!='=' ){
        *tokenType = TK_ILLEGAL;
        return 1;
      }else{
        *tokenType = TK_NE;
        return 2;
      }
    }
    case CC_PIPE: {
      if( z[1]!='|' ){
        *tokenType = TK_BITOR;
        return 1;
      }else{
        *tokenType = TK_CONCAT;
        return 2;
      }
    }
    case CC_COMMA: {
      *tokenType = TK_COMMA;
      return 1;
    }
    case CC_AND: {
      *tokenType = TK_BITAND;
      return 1;
    }
    case CC_TILDA: {
      *tokenType = TK_BITNOT;
      return 1;
    }
    case CC_QUOTE: {
      int delim = z[0];
      testcase( delim=='`' );
      testcase( delim=='\'' );
      testcase( delim=='"' );
      for(i=1; (c=z[i])!=0; i++){
        if( c==delim ){
          if( z[i+1]==delim ){
            i++;
          }else{
            break;
          }
        }
      }
      if( c=='\'' ){
        *tokenType = TK_STRING;
        return i+1;
      }else if( c!=0 ){
        *tokenType = TK_ID;
        return i+1;
      }else{
        *tokenType = TK_ILLEGAL;
        return i;
      }
    }
    case CC_DOT: {
#ifndef SQLITE_OMIT_FLOATING_POINT
      if( !sqlite3Isdigit(z[1]) )
#endif
      {
        *tokenType = TK_DOT;
        return 1;
      }
      /* If the next character is a digit, this is a floating point
      ** number that begins with ".".  Fall thru into the next case */
      /* no break */ deliberate_fall_through
    }
    case CC_DIGIT: {
      testcase( z[0]=='0' );  testcase( z[0]=='1' );  testcase( z[0]=='2' );
      testcase( z[0]=='3' );  testcase( z[0]=='4' );  testcase( z[0]=='5' );
      testcase( z[0]=='6' );  testcase( z[0]=='7' );  testcase( z[0]=='8' );
      testcase( z[0]=='9' );  testcase( z[0]=='.' );
      *tokenType = TK_INTEGER;
#ifndef SQLITE_OMIT_HEX_INTEGER
      if( z[0]=='0' && (z[1]=='x' || z[1]=='X') && sqlite3Isxdigit(z[2]) ){
        for(i=3; 1; i++){
          if( sqlite3Isxdigit(z[i])==0 ){
            if( z[i]==SQLITE_DIGIT_SEPARATOR ){
              *tokenType = TK_QNUMBER;
            }else{
              break;
            }
          }
        }
      }else
#endif
        {
        for(i=0; 1; i++){
          if( sqlite3Isdigit(z[i])==0 ){
            if( z[i]==SQLITE_DIGIT_SEPARATOR ){
              *tokenType = TK_QNUMBER;
            }else{
              break;
            }
          }
        }
#ifndef SQLITE_OMIT_FLOATING_POINT
        if( z[i]=='.' ){
          if( *tokenType==TK_INTEGER ) *tokenType = TK_FLOAT;
          for(i++; 1; i++){
            if( sqlite3Isdigit(z[i])==0 ){
              if( z[i]==SQLITE_DIGIT_SEPARATOR ){
                *tokenType = TK_QNUMBER;
              }else{
                break;
              }
            }
          }
        }
        if( (z[i]=='e' || z[i]=='E') &&
             ( sqlite3Isdigit(z[i+1])
              || ((z[i+1]=='+' || z[i+1]=='-') && sqlite3Isdigit(z[i+2]))
             )
        ){
          if( *tokenType==TK_INTEGER ) *tokenType = TK_FLOAT;
          for(i+=2; 1; i++){
            if( sqlite3Isdigit(z[i])==0 ){
              if( z[i]==SQLITE_DIGIT_SEPARATOR ){
                *tokenType = TK_QNUMBER;
              }else{
                break;
              }
            }
          }
        }
#endif
      }
      while( IdChar(z[i]) ){
        *tokenType = TK_ILLEGAL;
        i++;
      }
      return i;
    }
    case CC_QUOTE2: {
      for(i=1, c=z[0]; c!=']' && (c=z[i])!=0; i++){}
      *tokenType = c==']' ? TK_ID : TK_ILLEGAL;
      return i;
    }
    case CC_VARNUM: {
      *tokenType = TK_VARIABLE;
      for(i=1; sqlite3Isdigit(z[i]); i++){}
      return i;
    }
    case CC_DOLLAR:
    case CC_VARALPHA: {
      i64 n = 0;
      testcase( z[0]=='$' );  testcase( z[0]=='@' );
      testcase( z[0]==':' );  testcase( z[0]=='#' );
      *tokenType = TK_VARIABLE;
      for(i=1; (c=z[i])!=0; i++){
        if( IdChar(c) ){
          n++;
#ifndef SQLITE_OMIT_TCL_VARIABLE
        }else if( c=='(' && n>0 ){
          do{
            i++;
          }while( (c=z[i])!=0 && !sqlite3Isspace(c) && c!=')' );
          if( c==')' ){
            i++;
          }else{
            *tokenType = TK_ILLEGAL;
          }
          break;
        }else if( c==':' && z[i+1]==':' ){
          i++;
#endif
        }else{
          break;
        }
      }
      if( n==0 ) *tokenType = TK_ILLEGAL;
      return i;
    }
    case CC_KYWD0: {
      if( aiClass[z[1]]>CC_KYWD ){ i = 1;  break; }
      for(i=2; aiClass[z[i]]<=CC_KYWD; i++){}
      if( IdChar(z[i]) ){
        /* This token started out using characters that can appear in keywords,
        ** but z[i] is a character not allowed within keywords, so this must
        ** be an identifier instead */
        i++;
        break;
      }
      *tokenType = TK_ID;
      return keywordCode((char*)z, i, tokenType);
    }
    case CC_X: {
#ifndef SQLITE_OMIT_BLOB_LITERAL
      testcase( z[0]=='x' ); testcase( z[0]=='X' );
      if( z[1]=='\'' ){
        *tokenType = TK_BLOB;
        for(i=2; sqlite3Isxdigit(z[i]); i++){}
        if( z[i]!='\'' || i%2 ){
          *tokenType = TK_ILLEGAL;
          while( z[i] && z[i]!='\'' ){ i++; }
        }
        if( z[i] ) i++;
        return i;
      }
#endif
      /* If it is not a BLOB literal, then it must be an ID, since no
      ** SQL keywords start with the letter 'x'.  Fall through */
      /* no break */ deliberate_fall_through
    }
    case CC_KYWD:
    case CC_ID: {
      i = 1;
      break;
    }
    case CC_BOM: {
      if( z[1]==0xbb && z[2]==0xbf ){
        *tokenType = TK_SPACE;
        return 3;
      }
      i = 1;
      break;
    }
    case CC_NUL: {
      *tokenType = TK_ILLEGAL;
      return 0;
    }
    default: {
      *tokenType = TK_ILLEGAL;
      return 1;
    }
  }
  while( IdChar(z[i]) ){ i++; }
  *tokenType = TK_ID;
  return i;
}

/*
** Run the parser on the given SQL string.
*/
SQLITE_PRIVATE int sqlite3RunParser(Parse *pParse, const char *zSql){
  int nErr = 0;                   /* Number of errors encountered */
  void *pEngine;                  /* The LEMON-generated LALR(1) parser */
  i64 n = 0;                      /* Length of the next token token */
  int tokenType;                  /* type of the next token */
  int lastTokenParsed = -1;       /* type of the previous token */
  sqlite3 *db = pParse->db;       /* The database connection */
  i64 mxSqlLen;                   /* Max length of an SQL string */
  Parse *pParentParse = 0;        /* Outer parse context, if any */
#ifdef sqlite3Parser_ENGINEALWAYSONSTACK
  yyParser sEngine;    /* Space to hold the Lemon-generated Parser object */
#endif
  VVA_ONLY( u8 startedWithOom = db->mallocFailed );

  assert( zSql!=0 );
  mxSqlLen = db->aLimit[SQLITE_LIMIT_SQL_LENGTH];
  if( db->nVdbeActive==0 ){
    AtomicStore(&db->u1.isInterrupted, 0);
  }
  pParse->rc = SQLITE_OK;
  pParse->zTail = zSql;
#ifdef SQLITE_DEBUG
  if( db->flags & SQLITE_ParserTrace ){
    printf("parser: [[[%s]]]\n", zSql);
    sqlite3ParserTrace(stdout, "parser: ");
  }else{
    sqlite3ParserTrace(0, 0);
  }
#endif
#ifdef sqlite3Parser_ENGINEALWAYSONSTACK
  pEngine = &sEngine;
  sqlite3ParserInit(pEngine, pParse);
#else
  pEngine = sqlite3ParserAlloc(sqlite3Malloc, pParse);
  if( pEngine==0 ){
    sqlite3OomFault(db);
    return SQLITE_NOMEM_BKPT;
  }
#endif
  assert( pParse->pNewTable==0 );
  assert( pParse->pNewTrigger==0 );
  assert( pParse->nVar==0 );
  assert( pParse->pVList==0 );
  pParentParse = db->pParse;
  db->pParse = pParse;
  while( 1 ){
    n = sqlite3GetToken((u8*)zSql, &tokenType);
    mxSqlLen -= n;
    if( mxSqlLen<0 ){
      pParse->rc = SQLITE_TOOBIG;
      pParse->nErr++;
      break;
    }
#ifndef SQLITE_OMIT_WINDOWFUNC
    if( tokenType>=TK_WINDOW ){
      assert( tokenType==TK_SPACE || tokenType==TK_OVER || tokenType==TK_FILTER
           || tokenType==TK_ILLEGAL || tokenType==TK_WINDOW
           || tokenType==TK_QNUMBER || tokenType==TK_COMMENT
      );
#else
    if( tokenType>=TK_SPACE ){
      assert( tokenType==TK_SPACE || tokenType==TK_ILLEGAL
           || tokenType==TK_QNUMBER || tokenType==TK_COMMENT
      );
#endif /* SQLITE_OMIT_WINDOWFUNC */
      if( AtomicLoad(&db->u1.isInterrupted) ){
        pParse->rc = SQLITE_INTERRUPT;
        pParse->nErr++;
        break;
      }
      if( tokenType==TK_SPACE ){
        zSql += n;
        continue;
      }
      if( zSql[0]==0 ){
        /* Upon reaching the end of input, call the parser two more times
        ** with tokens TK_SEMI and 0, in that order. */
        if( lastTokenParsed==TK_SEMI ){
          tokenType = 0;
        }else if( lastTokenParsed==0 ){
          break;
        }else{
          tokenType = TK_SEMI;
        }
        n = 0;
#ifndef SQLITE_OMIT_WINDOWFUNC
      }else if( tokenType==TK_WINDOW ){
        assert( n==6 );
        tokenType = analyzeWindowKeyword((const u8*)&zSql[6]);
      }else if( tokenType==TK_OVER ){
        assert( n==4 );
        tokenType = analyzeOverKeyword((const u8*)&zSql[4], lastTokenParsed);
      }else if( tokenType==TK_FILTER ){
        assert( n==6 );
        tokenType = analyzeFilterKeyword((const u8*)&zSql[6], lastTokenParsed);
#endif /* SQLITE_OMIT_WINDOWFUNC */
      }else if( tokenType==TK_COMMENT
             && (db->init.busy || (db->flags & SQLITE_Comments)!=0)
      ){
        /* Ignore SQL comments if either (1) we are reparsing the schema or
        ** (2) SQLITE_DBCONFIG_ENABLE_COMMENTS is turned on (the default). */
        zSql += n;
        continue;
      }else if( tokenType!=TK_QNUMBER ){
        Token x;
        x.z = zSql;
        x.n = (u32)n;
        sqlite3ErrorMsg(pParse, "unrecognized token: \"%T\"", &x);
        break;
      }
    }
    pParse->sLastToken.z = zSql;
    pParse->sLastToken.n = (u32)n;
    sqlite3Parser(pEngine, tokenType, pParse->sLastToken);
    lastTokenParsed = tokenType;
    zSql += n;
    assert( db->mallocFailed==0 || pParse->rc!=SQLITE_OK || startedWithOom );
    if( pParse->rc!=SQLITE_OK ) break;
  }
  assert( nErr==0 );
#ifdef YYTRACKMAXSTACKDEPTH
  sqlite3_mutex_enter(sqlite3MallocMutex());
  sqlite3StatusHighwater(SQLITE_STATUS_PARSER_STACK,
      sqlite3ParserStackPeak(pEngine)
  );
  sqlite3_mutex_leave(sqlite3MallocMutex());
#endif /* YYDEBUG */
#ifdef sqlite3Parser_ENGINEALWAYSONSTACK
  sqlite3ParserFinalize(pEngine);
#else
  sqlite3ParserFree(pEngine, sqlite3_free);
#endif
  if( db->mallocFailed ){
    pParse->rc = SQLITE_NOMEM_BKPT;
  }
  if( pParse->zErrMsg || (pParse->rc!=SQLITE_OK && pParse->rc!=SQLITE_DONE) ){
    if( pParse->zErrMsg==0 ){
      pParse->zErrMsg = sqlite3DbStrDup(db, sqlite3ErrStr(pParse->rc));
    }
    if( (pParse->prepFlags & SQLITE_PREPARE_DONT_LOG)==0 ){
      sqlite3_log(pParse->rc, "%s in \"%s\"", pParse->zErrMsg, pParse->zTail);
    }
    nErr++;
  }
  pParse->zTail = zSql;
#ifndef SQLITE_OMIT_VIRTUALTABLE
  sqlite3_free(pParse->apVtabLock);
#endif

  if( pParse->pNewTable && !IN_SPECIAL_PARSE ){
    /* If the pParse->declareVtab flag is set, do not delete any table
    ** structure built up in pParse->pNewTable. The calling code (see vtab.c)
    ** will take responsibility for freeing the Table structure.
    */
    sqlite3DeleteTable(db, pParse->pNewTable);
  }
  if( pParse->pNewTrigger && !IN_RENAME_OBJECT ){
    sqlite3DeleteTrigger(db, pParse->pNewTrigger);
  }
  if( pParse->pVList ) sqlite3DbNNFreeNN(db, pParse->pVList);
  db->pParse = pParentParse;
  assert( nErr==0 || pParse->rc!=SQLITE_OK );
  return nErr;
}


#ifdef SQLITE_ENABLE_NORMALIZE
/*
** Insert a single space character into pStr if the current string
** ends with an identifier
*/
static void addSpaceSeparator(sqlite3_str *pStr){
  if( pStr->nChar && sqlite3IsIdChar(pStr->zText[pStr->nChar-1]) ){
    sqlite3_str_append(pStr, " ", 1);
  }
}

/*
** Compute a normalization of the SQL given by zSql[0..nSql-1].  Return
** the normalization in space obtained from sqlite3DbMalloc().  Or return
** NULL if anything goes wrong or if zSql is NULL.
*/
SQLITE_PRIVATE char *sqlite3Normalize(
  Vdbe *pVdbe,       /* VM being reprepared */
  const char *zSql   /* The original SQL string */
){
  sqlite3 *db;       /* The database connection */
  int i;             /* Next unread byte of zSql[] */
  i64 n;             /* length of current token */
  int tokenType;     /* type of current token */
  int prevType = 0;  /* Previous non-whitespace token */
  int nParen;        /* Number of nested levels of parentheses */
  int iStartIN;      /* Start of RHS of IN operator in z[] */
  int nParenAtIN;    /* Value of nParent at start of RHS of IN operator */
  u32 j;             /* Bytes of normalized SQL generated so far */
  sqlite3_str *pStr; /* The normalized SQL string under construction */

  db = sqlite3VdbeDb(pVdbe);
  tokenType = -1;
  nParen = iStartIN = nParenAtIN = 0;
  pStr = sqlite3_str_new(db);
  assert( pStr!=0 );  /* sqlite3_str_new() never returns NULL */
  for(i=0; zSql[i] && pStr->accError==0; i+=n){
    if( tokenType!=TK_SPACE ){
      prevType = tokenType;
    }
    n = sqlite3GetToken((unsigned char*)zSql+i, &tokenType);
    if( NEVER(n<=0) ) break;
    switch( tokenType ){
      case TK_COMMENT:
      case TK_SPACE: {
        break;
      }
      case TK_NULL: {
        if( prevType==TK_IS || prevType==TK_NOT ){
          sqlite3_str_append(pStr, " NULL", 5);
          break;
        }
        /* no break */ deliberate_fall_through
      }
      case TK_STRING:
      case TK_INTEGER:
      case TK_FLOAT:
      case TK_VARIABLE:
      case TK_BLOB: {
        sqlite3_str_append(pStr, "?", 1);
        break;
      }
      case TK_LP: {
        nParen++;
        if( prevType==TK_IN ){
          iStartIN = pStr->nChar;
          nParenAtIN = nParen;
        }
        sqlite3_str_append(pStr, "(", 1);
        break;
      }
      case TK_RP: {
        if( iStartIN>0 && nParen==nParenAtIN ){
          assert( pStr->nChar>=(u32)iStartIN );
          pStr->nChar = iStartIN+1;
          sqlite3_str_append(pStr, "?,?,?", 5);
          iStartIN = 0;
        }
        nParen--;
        sqlite3_str_append(pStr, ")", 1);
        break;
      }
      case TK_ID: {
        iStartIN = 0;
        j = pStr->nChar;
        if( sqlite3Isquote(zSql[i]) ){
          char *zId = sqlite3DbStrNDup(db, zSql+i, n);
          int nId;
          int eType = 0;
          if( zId==0 ) break;
          sqlite3Dequote(zId);
          if( zSql[i]=='"' && sqlite3VdbeUsesDoubleQuotedString(pVdbe, zId) ){
            sqlite3_str_append(pStr, "?", 1);
            sqlite3DbFree(db, zId);
            break;
          }
          nId = sqlite3Strlen30(zId);
          if( sqlite3GetToken((u8*)zId, &eType)==nId && eType==TK_ID ){
            addSpaceSeparator(pStr);
            sqlite3_str_append(pStr, zId, nId);
          }else{
            sqlite3_str_appendf(pStr, "\"%w\"", zId);
          }
          sqlite3DbFree(db, zId);
        }else{
          addSpaceSeparator(pStr);
          sqlite3_str_append(pStr, zSql+i, n);
        }
        while( j<pStr->nChar ){
          pStr->zText[j] = sqlite3Tolower(pStr->zText[j]);
          j++;
        }
        break;
      }
      case TK_SELECT: {
        iStartIN = 0;
        /* no break */ deliberate_fall_through
      }
      default: {
        if( sqlite3IsIdChar(zSql[i]) ) addSpaceSeparator(pStr);
        j = pStr->nChar;
        sqlite3_str_append(pStr, zSql+i, n);
        while( j<pStr->nChar ){
          pStr->zText[j] = sqlite3Toupper(pStr->zText[j]);
          j++;
        }
        break;
      }
    }
  }
  if( tokenType!=TK_SEMI ) sqlite3_str_append(pStr, ";", 1);
  return sqlite3_str_finish(pStr);
}
#endif /* SQLITE_ENABLE_NORMALIZE */

/************** End of tokenize.c ********************************************/
/************** Begin file complete.c ****************************************/
/*
** 2001 September 15
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
*************************************************************************
** An tokenizer for SQL
**
** This file contains C code that implements the sqlite3_complete() API.
** This code used to be part of the tokenizer.c source file.  But by
** separating it out, the code will be automatically omitted from
** static links that do not use it.
*/
/* #include "sqliteInt.h" */
#ifndef SQLITE_OMIT_COMPLETE

/*
** This is defined in tokenize.c.  We just have to import the definition.
*/
#ifndef SQLITE_AMALGAMATION
#ifdef SQLITE_ASCII
#define IdChar(C)  ((sqlite3CtypeMap[(unsigned char)C]&0x46)!=0)
#endif
#ifdef SQLITE_EBCDIC
SQLITE_PRIVATE const char sqlite3IsEbcdicIdChar[];
#define IdChar(C)  (((c=C)>=0x42 && sqlite3IsEbcdicIdChar[c-0x40]))
#endif
#endif /* SQLITE_AMALGAMATION */


/*
** Token types used by the sqlite3_complete() routine.  See the header
** comments on that procedure for additional information.
*/
#define tkSEMI    0
#define tkWS      1
#define tkOTHER   2
#ifndef SQLITE_OMIT_TRIGGER
#define tkEXPLAIN 3
#define tkCREATE  4
#define tkTEMP    5
#define tkTRIGGER 6
#define tkEND     7
#endif

/*
** Return zero if the given SQL string is complete - if all comments,
** string and blob literals, and quoted identifiers have been closed and
** if the entire string ends with ";" and possibly with ";END;" if the
** string is a CREATE TRIGGER statement.  A non-zero return indicates
** that the string is empty or incomplete.  Bits of the return value
** indicate what is missing and is needed to close out the statement.
**
** Special handling is require for CREATE TRIGGER statements.
** Whenever the CREATE TRIGGER keywords are seen, the statement
** must end with ";END;".
**
** Let the return code be a value R.  R is split up into various
** subfields, at byte boundaries:
**
**    R = 0xwwwwwwww00xxyyzz
**
** In other words, zz is the least significant byte, yy is the next
** most significant byte, xx is the third byte, wwwwwwww is the upper
** four bytes of the return value.
**
**   zz == SQLITE_OK       Input is complete
**   zz == SQLITE_ERROR    Input is incomplete
**   zz == SQLITE_EMPTY    Input contains only whitespace
**   zz == SQLITE_MISUSE   Input is a NULL pointer
**   zz != 0               New values for zz may be added in the future
**
**   yy == 0x01            Need a semicolon at the end
**   yy == 0x02            Need "END" and a semicolon
**   yy == 0x03            Need semicolon, "END", and semicolon
**   yy != 0               New values for yy may be added in the future
**
**   xx == '\''            Incomplete string or blob literal
**   xx == '"'             Incomplete quoted identifier
**   xx == '`'             Incompelte MySQL-style quoted identifier
**   xx == ']'             Incomplete SQLServer-style quoted identifer
**   xx == '-'             Incomplete SQL-style comment
**   xx == '/'             Incomplete C-style comment
**   xx != 0               New values of xx may be added in the future
**
**   wwwwwwww              Interpret as a signed integer, the number
**                         of unmatched "(".  Negative means there are
**                         more ")" and "(".
**
**   ((R>>24)&0xff)!=0     New uses for the 4th byte may be added
**                         in the future
**
** This implementation uses a state machine with 8 states:
**
**   (0) NOTHING   We have not yet seen a non-whitespace character.
**
**   (1) START     At the beginning or end of an SQL statement.  This routine
**                 returns 1 if it ends in the START state and 0 if it ends
**                 in any other state.
**
**   (2) NORMAL    We are in the middle of statement which ends with a single
**                 semicolon.
**
**   (3) EXPLAIN   The keyword EXPLAIN has been seen at the beginning of
**                 a statement.
**
**   (4) CREATE    The keyword CREATE has been seen at the beginning of a
**                 statement, possibly preceded by EXPLAIN and/or followed by
**                 TEMP or TEMPORARY
**
**   (5) TRIGGER   We are in the middle of a trigger definition that must be
**                 ended by a semicolon, the keyword END, and another semicolon.
**
**   (6) SEMI      We've seen the first semicolon in the ";END;" that occurs at
**                 the end of a trigger definition.
**
**   (7) END       We've seen the ";END" of the ";END;" that occurs at the end
**                 of a trigger definition.
**
** Transitions between states above are determined by tokens extracted
** from the input.  The following tokens are significant:
**
**   (0) tkSEMI      A semicolon.
**   (1) tkWS        Whitespace.
**   (2) tkOTHER     Any other SQL token.
**   (3) tkEXPLAIN   The "explain" keyword.
**   (4) tkCREATE    The "create" keyword.
**   (5) tkTEMP      The "temp" or "temporary" keyword.
**   (6) tkTRIGGER   The "trigger" keyword.
**   (7) tkEND       The "end" keyword.
**
** Whitespace never causes a state transition and is always ignored.
** This means that a SQL string of all whitespace is invalid.
**
** If we compile with SQLITE_OMIT_TRIGGER, all of the computation needed
** to recognize the end of a trigger can be omitted.  All we have to do
** is look for a semicolon that is not part of an string or comment.
*/
SQLITE_API sqlite3_int64 sqlite3_incomplete(const char *zSql){
  u8 state = 0;   /* Current state, using numbers defined in header comment */
  u8 token;       /* Value of the next token */
  u8 pending = 0; /* unmatched structure character */
  int nParen = 0; /* Nested parentheses */

#ifndef SQLITE_OMIT_TRIGGER
  /* A complex statement machine used to detect the end of a CREATE TRIGGER
  ** statement.  This is the normal case.
  */
  static const u8 trans[8][8] = {
                     /* Token:                                                */
     /* State:       **  SEMI  WS  OTHER  EXPLAIN  CREATE  TEMP  TRIGGER  END */
     /* 0 NOTHING: */ {    1,  0,     2,       3,      4,    2,       2,   2, },
     /* 1   START: */ {    1,  1,     2,       3,      4,    2,       2,   2, },
     /* 2  NORMAL: */ {    1,  2,     2,       2,      2,    2,       2,   2, },
     /* 3 EXPLAIN: */ {    1,  3,     3,       2,      4,    2,       2,   2, },
     /* 4  CREATE: */ {    1,  4,     2,       2,      2,    4,       5,   2, },
     /* 5 TRIGGER: */ {    6,  5,     5,       5,      5,    5,       5,   5, },
     /* 6    SEMI: */ {    6,  6,     5,       5,      5,    5,       5,   7, },
     /* 7     END: */ {    1,  7,     5,       5,      5,    5,       5,   5, },
  };
#else
  /* If triggers are not supported by this compile then the statement machine
  ** used to detect the end of a statement is much simpler
  */
  static const u8 trans[3][3] = {
                     /* Token:           */
     /* State:       **  SEMI  WS  OTHER */
     /* 0 NOTHING: */ {    1,  0,     2, },
     /* 1   START: */ {    1,  1,     2, },
     /* 2  NORMAL: */ {    1,  2,     2, },
  };
#endif /* SQLITE_OMIT_TRIGGER */
  /* Mapping state number to yy value for the return */
  static const u8 statemap[8] = {
     /* 0 NOTHING */ 0,
     /* 1 START   */ 0,
     /* 2 NORMAL  */ 1,
     /* 3 EXPLAIN */ 1,
     /* 4 CREATE  */ 1,
     /* 5 TRIGGER */ 3,
     /* 6 SEMI    */ 2,
     /* 7 END     */ 1,
  };

#ifdef SQLITE_ENABLE_API_ARMOR
  if( zSql==0 ){
    return SQLITE_MISUSE_BKPT;
  }
#endif

  while( *zSql ){
    switch( *zSql ){
      case ';': {  /* A semicolon */
        token = tkSEMI;
        break;
      }
      case ' ':
      case '\r':
      case '\t':
      case '\n':
      case '\f': {  /* White space is ignored */
        token = tkWS;
        break;
      }
      case '/': {   /* C-style comments */
        if( zSql[1]!='*' ){
          token = tkOTHER;
          break;
        }
        zSql += 2;
        while( zSql[0] && (zSql[0]!='*' || zSql[1]!='/') ){ zSql++; }
        if( zSql[0]==0 ){
          pending = '/';
          goto incomplete_finish;
        }
        zSql++;
        token = tkWS;
        break;
      }
      case '-': {   /* SQL-style comments from "--" to end of line */
        if( zSql[1]!='-' ){
          token = tkOTHER;
          break;
        }
        while( *zSql && *zSql!='\n' ){ zSql++; }
        if( *zSql==0 ){
          pending = '-';
          goto incomplete_finish;
        }
        token = tkWS;
        break;
      }
      case '[': {   /* Microsoft-style identifiers in [...] */
        zSql++;
        while( *zSql && *zSql!=']' ){ zSql++; }
        if( *zSql==0 ){
          pending = ']';
          goto incomplete_finish;
        }
        token = tkOTHER;
        break;
      }
      case '`':     /* Grave-accent quoted symbols used by MySQL */
      case '"':     /* single- and double-quoted strings */
      case '\'': {
        int c = *zSql;
        zSql++;
        while( *zSql && *zSql!=c ){ zSql++; }
        if( *zSql==0 ){
          pending = c;
          goto incomplete_finish;
        }
        token = tkOTHER;
        break;
      }
      case '(': {
        nParen++;
        token = tkOTHER;
        break;
      }
      case ')': {
        nParen--;
        token = tkOTHER;
        break;
      }
      default: {
#ifdef SQLITE_EBCDIC
        unsigned char c;
#endif
        if( IdChar((u8)*zSql) ){
          /* Keywords and unquoted identifiers */
          int nId;
          for(nId=1; IdChar(zSql[nId]); nId++){}
#ifdef SQLITE_OMIT_TRIGGER
          token = tkOTHER;
#else
          switch( *zSql ){
            case 'c': case 'C': {
              if( nId==6 && sqlite3StrNICmp(zSql, "create", 6)==0 ){
                token = tkCREATE;
              }else{
                token = tkOTHER;
              }
              break;
            }
            case 't': case 'T': {
              if( nId==7 && sqlite3StrNICmp(zSql, "trigger", 7)==0 ){
                token = tkTRIGGER;
              }else if( nId==4 && sqlite3StrNICmp(zSql, "temp", 4)==0 ){
                token = tkTEMP;
              }else if( nId==9 && sqlite3StrNICmp(zSql, "temporary", 9)==0 ){
                token = tkTEMP;
              }else{
                token = tkOTHER;
              }
              break;
            }
            case 'e':  case 'E': {
              if( nId==3 && sqlite3StrNICmp(zSql, "end", 3)==0 ){
                token = tkEND;
              }else
#ifndef SQLITE_OMIT_EXPLAIN
              if( nId==7 && sqlite3StrNICmp(zSql, "explain", 7)==0 ){
                token = tkEXPLAIN;
              }else
#endif
              {
                token = tkOTHER;
              }
              break;
            }
            default: {
              token = tkOTHER;
              break;
            }
          }
#endif /* SQLITE_OMIT_TRIGGER */
          zSql += nId-1;
        }else{
          /* Operators and special symbols */
          token = tkOTHER;
        }
        break;
      }
    }
    state = trans[state][token];
    zSql++;
  }
incomplete_finish:
  if( state==0 && pending==0 ) return SQLITE_EMPTY;
  if( state==1 ) nParen = 0;
  return (i64)((((u64)nParen)<<32) |
               ((u64)pending<<16) |
               ((u64)statemap[state]<<8) |
               (state>1 || pending!=0));
}
SQLITE_API int sqlite3_complete(const char *zSql){
  return sqlite3_incomplete(zSql)==0;
}

#ifndef SQLITE_OMIT_UTF16
/*
** This routine is the same as the sqlite3_complete() routine described
** above, except that the parameter is required to be UTF-16 encoded, not
** UTF-8.
*/
SQLITE_API int sqlite3_complete16(const void *zSql){
  sqlite3_value *pVal;
  char const *zSql8;
  int rc;

#ifndef SQLITE_OMIT_AUTOINIT
  rc = sqlite3_initialize();
  if( rc ) return rc;
#endif
  pVal = sqlite3ValueNew(0);
  sqlite3ValueSetStr(pVal, -1, zSql, SQLITE_UTF16NATIVE, SQLITE_STATIC);
  zSql8 = sqlite3ValueText(pVal, SQLITE_UTF8);
  if( zSql8 ){
    rc = sqlite3_incomplete(zSql8)==0;
  }else{
    rc = SQLITE_NOMEM_BKPT;
  }
  sqlite3ValueFree(pVal);
  return rc & 0xff;
}
#endif /* SQLITE_OMIT_UTF16 */
#endif /* SQLITE_OMIT_COMPLETE */

/************** End of complete.c ********************************************/
