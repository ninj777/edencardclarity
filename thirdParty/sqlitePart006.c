/************** Begin file printf.c ******************************************/
/*
** The "printf" code that follows dates from the 1980's.  It is in
** the public domain.
**
**************************************************************************
**
** This file contains code for a set of "printf"-like routines.  These
** routines format strings much like the printf() from the standard C
** library, though the implementation here has enhancements to support
** SQLite.
*/
/* #include "sqliteInt.h" */

/*
** Conversion types fall into various categories as defined by the
** following enumeration.
*/
#define etRADIX       0 /* non-decimal integer types.  %x %o */
#define etFLOAT       1 /* Floating point.  %f */
#define etEXP         2 /* Exponentional notation. %e and %E */
#define etGENERIC     3 /* Floating or exponential, depending on exponent. %g */
#define etSIZE        4 /* Return number of characters processed so far. %n */
#define etSTRING      5 /* Strings. %s */
#define etDYNSTRING   6 /* Dynamically allocated strings. %z */
#define etPERCENT     7 /* Percent symbol. %% */
#define etCHARX       8 /* Characters. %c */
/* The rest are extensions, not normally found in printf() */
#define etESCAPE_q    9  /* Strings with '\'' doubled.  %q */
#define etESCAPE_Q    10 /* Strings with '\'' doubled and enclosed in '',
                            NULL pointers replaced by SQL NULL.  %Q */
#define etTOKEN       11 /* a pointer to a Token structure */
#define etSRCITEM     12 /* a pointer to a SrcItem */
#define etPOINTER     13 /* The %p conversion */
#define etESCAPE_w    14 /* %w -> Strings with '\"' doubled */
#define etORDINAL     15 /* %r -> 1st, 2nd, 3rd, 4th, etc.  English only */
#define etDECIMAL     16 /* %d or %u, but not %x, %o */
#define etESCAPE_j    17 /* %j -> JSON string literal w/o "..." */
#define etESCAPE_J    18 /* %J -> JSON string literal with "..." */

#define etINVALID     19 /* Any unrecognized conversion type */


/*
** An "etByte" is an 8-bit unsigned value.
*/
typedef unsigned char etByte;

/*
** Each builtin conversion character (ex: the 'd' in "%d") is described
** by an instance of the following structure
*/
typedef struct et_info {   /* Information about each format field */
  char fmttype;            /* The format field code letter */
  etByte base;             /* The base for radix conversion */
  etByte flags;            /* One or more of FLAG_ constants below */
  etByte type;             /* Conversion paradigm */
  etByte charset;          /* Offset into aDigits[] of the digits string */
  etByte prefix;           /* Offset into aPrefix[] of the prefix string */
  char iNxt;               /* Next with same hash, or 0 for end of chain */
} et_info;

/*
** Allowed values for et_info.flags
*/
#define FLAG_SIGNED    1     /* True if the value to convert is signed */
#define FLAG_STRING    4     /* Allow infinite precision */

/*
** The table is searched by hash.  In the case of %C where C is the character
** and that character has ASCII value j, then the hash is j%25.
**
** The order of the entries in fmtinfo[] and the hash chain was entered
** manually, but based on the output of the following TCL script:
*/
#if 0  /*****  Beginning of script ******/
foreach c {d s g z q Q w c o u x X f e E G i n % p T S r J j} {
  scan $c %c x
  set n($c) $x
}
set mx [llength [array names n]]
puts "count: $mx"

set mx 25
puts "*********** mx=$mx ************"
for {set r 0} {$r<$mx} {incr r} {
  puts -nonewline [format %2d: $r]
  foreach c [array names n] {
    if {($n($c))%$mx==$r} {puts -nonewline " $c"}
  }
  puts ""
}
#endif /***** End of script ********/

static const char aDigits[] = "0123456789ABCDEF0123456789abcdef";
static const char aHex[]    = "0123456789abcdef";
static const char aPrefix[] = "-x0\000X0";
static const et_info fmtinfo[25] = {
  /*  0 */  {  'd', 10, 1, etDECIMAL,    0,  0,  0 },
  /*  1 */  {  'e',  0, 1, etEXP,        30, 0,  0 },
  /*  2 */  {  'f',  0, 1, etFLOAT,      0,  0,  0 },
  /*  3 */  {  'g',  0, 1, etGENERIC,    30, 0,  0 },
  /*  4 */  {  'j',  0, 0, etESCAPE_j,   0,  0,  0 },  /* Hash: 6 */
  /*  5 */  {  'i', 10, 1, etDECIMAL,    0,  0,  0 },
  /*  6 */  {  'Q',  0, 4, etESCAPE_Q,   0,  0,  4 },
  /*  7 */  {  'p', 16, 0, etPOINTER,    0,  1,  0 },  /* Hash: 12 */
  /*  8 */  {  'S',  0, 0, etSRCITEM,    0,  0,  0 },
  /*  9 */  {  'T',  0, 0, etTOKEN,      0,  0,  0 },
  /* 10 */  {  'n',  0, 0, etSIZE,       0,  0,  0 },
  /* 11 */  {  'o',  8, 0, etRADIX,      0,  2,  0 },
  /* 12 */  {  '%',  0, 0, etPERCENT,    0,  0,  7 },
  /* 13 */  {  'q',  0, 4, etESCAPE_q,   0,  0, 16 },
  /* 14 */  {  'r', 10, 1, etORDINAL,    0,  0,  0 },
  /* 15 */  {  's',  0, 4, etSTRING,     0,  0,  0 },
  /* 16 */  {  'X', 16, 0, etRADIX,      0,  4,  0 },  /* Hash: 13 */
  /* 17 */  {  'u', 10, 0, etDECIMAL,    0,  0,  0 },
  /* 18 */  {  'w',  0, 4, etESCAPE_w,   0,  0,  0 },  /* Hash: 19 */
  /* 19 */  {  'E',  0, 1, etEXP,        14, 0, 18 },
  /* 20 */  {  'x', 16, 0, etRADIX,      16, 1,  0 },
  /* 21 */  {  'G',  0, 1, etGENERIC,    14, 0,  0 },
  /* 22 */  {  'z',  0, 4, etDYNSTRING,  0,  0,  0 },
  /* 23 */  {  'J',  0, 0, etESCAPE_J,   0,  0,  0 },  /* Hash: 24 */
  /* 24 */  {  'c',  0, 0, etCHARX,      0,  0, 23 }
};

/* Additional Notes:
**
**    %S    Takes a pointer to SrcItem.  Shows name or database.name
**    %!S   Like %S but prefer the zName over the zAlias
*/

/*
** Set the StrAccum object to an error mode.
*/
SQLITE_PRIVATE void sqlite3StrAccumSetError(StrAccum *p, u8 eError){
  assert( eError==SQLITE_NOMEM || eError==SQLITE_TOOBIG );
  p->accError = eError;
  if( p->mxAlloc ) sqlite3_str_reset(p);
  if( eError==SQLITE_TOOBIG ) sqlite3ErrorToParser(p->db, eError);
}

/*
** Extra argument values from a PrintfArguments object
*/
static sqlite3_int64 getIntArg(PrintfArguments *p){
  if( p->nArg<=p->nUsed ) return 0;
  return sqlite3_value_int64(p->apArg[p->nUsed++]);
}
static double getDoubleArg(PrintfArguments *p){
  if( p->nArg<=p->nUsed ) return 0.0;
  return sqlite3_value_double(p->apArg[p->nUsed++]);
}
static char *getTextArg(PrintfArguments *p){
  if( p->nArg<=p->nUsed ) return 0;
  return (char*)sqlite3_value_text(p->apArg[p->nUsed++]);
}

/*
** Allocate memory for a temporary buffer needed for printf rendering.
**
** If the requested size of the temp buffer is larger than the size
** of the output buffer in pAccum, then cause an SQLITE_TOOBIG error.
** Do the size check before the memory allocation to prevent rogue
** SQL from requesting large allocations using the precision or width
** field of the printf() function.
*/
static char *printfTempBuf(sqlite3_str *pAccum, sqlite3_int64 n){
  char *z;
  if( pAccum->accError ) return 0;
  if( n>pAccum->nAlloc && n>pAccum->mxAlloc ){
    sqlite3StrAccumSetError(pAccum, SQLITE_TOOBIG);
    return 0;
  }
  z = sqlite3_malloc(n);
  if( z==0 ){
    sqlite3StrAccumSetError(pAccum, SQLITE_NOMEM);
  }
  return z;
}

/*
** On machines with a small stack size, you can redefine the
** SQLITE_PRINT_BUF_SIZE to be something smaller, if desired.
*/
#ifndef SQLITE_PRINT_BUF_SIZE
# define SQLITE_PRINT_BUF_SIZE 70
#endif
#define etBUFSIZE SQLITE_PRINT_BUF_SIZE  /* Size of the output buffer */

/*
** Hard limit on the precision of floating-point conversions to
** the minimum of SQLITE_PRINTF_PRECISION_LIMIT and 100 million.
*/
#ifdef SQLITE_PRINTF_PRECISION_LIMIT
# if SQLITE_PRINTF_PRECISION_LIMIT<100000000
#   define SQLITE_FP_PRECISION_LIMIT SQLITE_PRINTF_PRECISION_LIMIT
# else
#   define SQLITE_FP_PRECISION_LIMIT 100000000
# endif
#else
# define SQLITE_FP_PRECISION_LIMIT 100000000
#endif

/* Forward reference */
static void sqlite3StrAppend64(sqlite3_str *p, const char *z, i64 N);
static void sqlite3StrAppendchar64(sqlite3_str *p, i64 N, char c);

/*
** Render a string given by "fmt" into the StrAccum object.
*/
SQLITE_API void sqlite3_str_vappendf(
  sqlite3_str *pAccum,       /* Accumulate results here */
  const char *fmt,           /* Format string */
  va_list ap                 /* arguments */
){
  int c;                     /* Next character in the format string */
  char *bufpt;               /* Pointer to the conversion buffer */
  i64 precision;             /* Precision of the current field */
  i64 length;                /* Length of the field */
  int idx;                   /* A general purpose loop counter */
  i64 width;                 /* Width of the current field */
  etByte flag_leftjustify;   /* True if "-" flag is present */
  etByte flag_prefix;        /* '+' or ' ' or 0 for prefix */
  etByte flag_alternateform; /* True if "#" flag is present */
  etByte flag_altform2;      /* True if "!" flag is present */
  etByte flag_zeropad;       /* True if field width constant starts with zero */
  etByte flag_long;          /* 1 for the "l" flag, 2 for "ll", 0 by default */
  etByte done;               /* Loop termination flag */
  etByte cThousand;          /* Thousands separator for %d and %u */
  etByte xtype = etINVALID;  /* Conversion paradigm */
  u8 bArgList;               /* True for SQLITE_PRINTF_SQLFUNC */
  char prefix;               /* Prefix character.  "+" or "-" or " " or '\0'. */
  sqlite_uint64 longvalue;   /* Value for integer types */
  double realvalue;          /* Value for real types */
  const et_info *infop;      /* Pointer to the appropriate info structure */
  char *zOut;                /* Rendering buffer */
  int nOut;                  /* Size of the rendering buffer */
  char *zExtra = 0;          /* Malloced memory used by some conversion */
  int exp, e2;               /* exponent of real numbers */
  etByte flag_dp;            /* True if decimal point should be shown */
  etByte flag_rtz;           /* True if trailing zeros should be removed */

  PrintfArguments *pArgList = 0; /* Arguments for SQLITE_PRINTF_SQLFUNC */
  char buf[etBUFSIZE];       /* Conversion buffer */

  /* pAccum never starts out with an empty buffer that was obtained from
  ** malloc().  This precondition is required by the mprintf("%z...")
  ** optimization. */
  assert( pAccum->nChar>0 || (pAccum->printfFlags&SQLITE_PRINTF_MALLOCED)==0 );

  bufpt = 0;
  if( (pAccum->printfFlags & SQLITE_PRINTF_SQLFUNC)!=0 ){
    pArgList = va_arg(ap, PrintfArguments*);
    bArgList = 1;
  }else{
    bArgList = 0;
  }
  for(; (c=(*fmt))!=0; ++fmt){
    if( c!='%' ){
      bufpt = (char *)fmt;
#if HAVE_STRCHRNUL
      fmt = strchrnul(fmt, '%');
#else
      fmt = strchr(fmt, '%');
      if( fmt==0 ){
        fmt = bufpt + strlen(bufpt);
      }
#endif
      sqlite3StrAppend64(pAccum, bufpt, fmt - bufpt);
      if( *fmt==0 ) break;
    }
    if( (c=(*++fmt))==0 ){
      sqlite3_str_append(pAccum, "%", 1);
      break;
    }
    /* Find out what flags are present */
    flag_leftjustify = flag_prefix = cThousand =
     flag_alternateform = flag_altform2 = flag_zeropad = 0;
    done = 0;
    width = 0;
    flag_long = 0;
    precision = -1;
    do{
      switch( c ){
        case '-':   flag_leftjustify = 1;     break;
        case '+':   flag_prefix = '+';        break;
        case ' ':   flag_prefix = ' ';        break;
        case '#':   flag_alternateform = 1;   break;
        case '!':   flag_altform2 = 1;        break;
        case '0':   flag_zeropad = 1;         break;
        case ',':   cThousand = ',';          break;
        default:    done = 1;                 break;
        case 'l': {
          flag_long = 1;
          c = *++fmt;
          if( c=='l' ){
            c = *++fmt;
            flag_long = 2;
          }
          done = 1;
          break;
        }
        case '1': case '2': case '3': case '4': case '5':
        case '6': case '7': case '8': case '9': {
          unsigned wx = c - '0';
          while( (c = *++fmt)>='0' && c<='9' ){
            wx = wx*10 + c - '0';
          }
          testcase( wx>0x7fffffff );
          width = wx & 0x7fffffff;
#ifdef SQLITE_PRINTF_PRECISION_LIMIT
          if( width>SQLITE_PRINTF_PRECISION_LIMIT ){
            width = SQLITE_PRINTF_PRECISION_LIMIT;
          }
#endif
          if( c!='.' && c!='l' ){
            done = 1;
          }else{
            fmt--;
          }
          break;
        }
        case '*': {
          if( bArgList ){
            width = (int)getIntArg(pArgList);
          }else{
            width = va_arg(ap,int);
          }
          if( width<0 ){
            flag_leftjustify = 1;
            width = width >= -2147483647 ? -width : 0;
          }
#ifdef SQLITE_PRINTF_PRECISION_LIMIT
          if( width>SQLITE_PRINTF_PRECISION_LIMIT ){
            width = SQLITE_PRINTF_PRECISION_LIMIT;
          }
#endif
          if( (c = fmt[1])!='.' && c!='l' ){
            c = *++fmt;
            done = 1;
          }
          break;
        }
        case '.': {
          c = *++fmt;
          if( c=='*' ){
            if( bArgList ){
              precision = (int)getIntArg(pArgList);
            }else{
              precision = va_arg(ap,int);
            }
            if( precision<0 ){
              precision = precision >= -2147483647 ? -precision : -1;
            }
            c = *++fmt;
          }else{
            unsigned px = 0;
            while( c>='0' && c<='9' ){
              px = px*10 + c - '0';
              c = *++fmt;
            }
            testcase( px>0x7fffffff );
            precision = px & 0x7fffffff;
          }
#ifdef SQLITE_PRINTF_PRECISION_LIMIT
          if( precision>SQLITE_PRINTF_PRECISION_LIMIT ){
            precision = SQLITE_PRINTF_PRECISION_LIMIT;
          }
#endif
          if( c=='l' ){
            --fmt;
          }else{
            done = 1;
          }
          break;
        }
      }
    }while( !done && (c=(*++fmt))!=0 );

    /* Fetch the info entry for the field */
#ifdef SQLITE_EBCDIC
    /* The hash table only works for ASCII.  For EBCDIC, we need to do
    ** a linear search of the table */
    infop = &fmtinfo[0];
    xtype = etINVALID;
    for(idx=0; idx<ArraySize(fmtinfo); idx++){
      if( c==fmtinfo[idx].fmttype ){
        infop = &fmtinfo[idx];
        xtype = infop->type;
        break;
      }
    }
#else
    /* Fast hash-table lookup */
    assert( ArraySize(fmtinfo)==25 );
    idx = ((unsigned)c) % 25;
    if( fmtinfo[idx].fmttype==c
     || fmtinfo[idx = fmtinfo[idx].iNxt].fmttype==c
    ){
      infop = &fmtinfo[idx];
      xtype = infop->type;
    }else{
      infop = &fmtinfo[0];
      xtype = etINVALID;
    }
#endif

    /*
    ** At this point, variables are initialized as follows:
    **
    **   flag_alternateform          TRUE if a '#' is present.
    **   flag_altform2               TRUE if a '!' is present.
    **   flag_prefix                 '+' or ' ' or zero
    **   flag_leftjustify            TRUE if a '-' is present or if the
    **                               field width was negative.
    **   flag_zeropad                TRUE if the width began with 0.
    **   flag_long                   1 for "l", 2 for "ll"
    **   width                       The specified field width.  This is
    **                               always non-negative.  Zero is the default.
    **   precision                   The specified precision.  The default
    **                               is -1.
    **   xtype                       The class of the conversion.
    **   infop                       Pointer to the appropriate info struct.
    */
    assert( width>=0 );
    assert( precision>=(-1) );
    switch( xtype ){
      case etPOINTER:
        flag_long = sizeof(char*)==sizeof(i64) ? 2 :
                     sizeof(char*)==sizeof(long int) ? 1 : 0;
        /* no break */ deliberate_fall_through
      case etORDINAL:
      case etRADIX:
        cThousand = 0;
        /* no break */ deliberate_fall_through
      case etDECIMAL:
        if( infop->flags & FLAG_SIGNED ){
          i64 v;
          if( bArgList ){
            v = getIntArg(pArgList);
          }else if( flag_long ){
            if( flag_long==2 ){
              v = va_arg(ap,i64) ;
            }else{
              v = va_arg(ap,long int);
            }
          }else{
            v = va_arg(ap,int);
          }
          if( v<0 ){
            testcase( v==SMALLEST_INT64 );
            testcase( v==(-1) );
            longvalue = ~v;
            longvalue++;
            prefix = '-';
          }else{
            longvalue = v;
            prefix = flag_prefix;
          }
        }else{
          if( bArgList ){
            longvalue = (u64)getIntArg(pArgList);
          }else if( flag_long ){
            if( flag_long==2 ){
              longvalue = va_arg(ap,u64);
            }else{
              longvalue = va_arg(ap,unsigned long int);
            }
          }else{
            longvalue = va_arg(ap,unsigned int);
          }
          prefix = 0;
        }

#if WHERETRACE_ENABLED
        if( xtype==etPOINTER && sqlite3WhereTrace & 0x100000 ) longvalue = 0;
#endif
#if TREETRACE_ENABLED
        if( xtype==etPOINTER && sqlite3TreeTrace & 0x100000 ) longvalue = 0;
#endif

        if( longvalue==0 ) flag_alternateform = 0;
        if( flag_zeropad && precision<width-(prefix!=0) ){
          precision = width-(prefix!=0);
        }
        if( precision<etBUFSIZE-10-etBUFSIZE/3 ){
          nOut = etBUFSIZE;
          zOut = buf;
        }else{
          u64 n;
          n = (u64)precision + 10;
          if( cThousand ) n += precision/3;
          zOut = zExtra = printfTempBuf(pAccum, n);
          if( zOut==0 ) return;
          nOut = (int)n;
        }
        bufpt = &zOut[nOut-1];
        if( xtype==etORDINAL ){
          static const char zOrd[] = "thstndrd";
          int x = (int)(longvalue % 10);
          if( x>=4 || (longvalue/10)%10==1 ){
            x = 0;
          }
          *(--bufpt) = zOrd[x*2+1];
          *(--bufpt) = zOrd[x*2];
        }
        {
          const char *cset = &aDigits[infop->charset];
          u8 base = infop->base;
          do{                                           /* Convert to ascii */
            *(--bufpt) = cset[longvalue%base];
            longvalue = longvalue/base;
          }while( longvalue>0 );
        }
        length = &zOut[nOut-1] - bufpt;
        if( precision>length ){                         /* zero pad */
          i64 nn = precision-length;
          bufpt -= nn;
          memset(bufpt,'0',nn);
          length = precision;
        }
        if( cThousand ){
          i64 nn = (length - 1)/3;  /* Number of "," to insert */
          i64 ix = (length - 1)%3 + 1;
          int ii;
          bufpt -= nn;
          for(ii=0; nn>0; ii++){
            bufpt[ii] = bufpt[ii+nn];
            ix--;
            if( ix==0 ){
              bufpt[++ii] = cThousand;
              nn--;
              ix = 3;
            }
          }
        }
        if( prefix ) *(--bufpt) = prefix;               /* Add sign */
        if( flag_alternateform && infop->prefix ){      /* Add "0" or "0x" */
          const char *pre;
          char x;
          pre = &aPrefix[infop->prefix];
          for(; (x=(*pre))!=0; pre++) *(--bufpt) = x;
        }
        length = &zOut[nOut-1] - bufpt;
        break;
      case etFLOAT:
      case etEXP:
      case etGENERIC: {
        FpDecode s;
        int iRound;
        int j;
        i64 szBufNeeded;       /* Size needed to hold the output */

        if( bArgList ){
          realvalue = getDoubleArg(pArgList);
        }else{
          realvalue = va_arg(ap,double);
        }
        if( precision<0 ) precision = 6;         /* Set default precision */
        if( precision>SQLITE_FP_PRECISION_LIMIT ){
          precision = SQLITE_FP_PRECISION_LIMIT;
        }
        if( xtype==etFLOAT ){
          iRound = -precision;
        }else if( xtype==etGENERIC ){
          if( precision==0 ) precision = 1;
          iRound = precision;
        }else{
          iRound = precision+1;
        }
        sqlite3FpDecode(&s, realvalue, iRound, flag_altform2 ? 20 : 16);
        if( s.isSpecial ){
          if( s.isSpecial==2 ){
            if( flag_zeropad ){
              bufpt = "null";
              length = 4;
            }else{
              bufpt = "NaN";
              length = 3;
            }
            break;
          }else if( flag_zeropad ){
            s.z[0] = '9';
            s.iDP = 1000;
            s.n = 1;
          }else{
            memcpy(buf, "-Inf", 5);
            bufpt = buf;
            if( s.sign=='-' ){
              /* no-op */
            }else if( flag_prefix ){
              buf[0] = flag_prefix;
            }else{
              bufpt++;
            }
            length = strlen(bufpt);
            break;
          }
        }
        if( s.sign=='-' ){
          if( flag_alternateform
           && !flag_prefix
           && xtype==etFLOAT
           && s.iDP<=iRound
          ){
            /* Suppress the minus sign if all of the following are true:
            **   *  The value displayed is zero
            **   *  The '#' flag is used
            **   *  The '+' flag is not used, and
            **   *  The format is %f
            */
            prefix = 0;
          }else{
            prefix = '-';
          }
        }else{
          prefix = flag_prefix;
        }

        exp = s.iDP-1;

        /*
        ** If the field type is etGENERIC, then convert to either etEXP
        ** or etFLOAT, as appropriate.
        */
        if( xtype==etGENERIC ){
          assert( precision>0 );
          precision--;
          flag_rtz = !flag_alternateform;
          if( exp<-4 || exp>precision ){
            xtype = etEXP;
          }else{
            precision = precision - exp;
            xtype = etFLOAT;
          }
        }else{
          flag_rtz = flag_altform2;
        }
        if( xtype==etEXP ){
          e2 = 0;
        }else{
          e2 = s.iDP - 1;
        }

        szBufNeeded = MAX(e2,0)+(i64)precision+(i64)width+10;
        if( cThousand && e2>0 ) szBufNeeded += (e2+2)/3;
        if( szBufNeeded + pAccum->nChar >= pAccum->nAlloc ){
          if( pAccum->mxAlloc==0 && pAccum->accError==0 ){
            /* Unable to allocate space in pAccum, perhaps because it
            ** is coming from sqlite3_snprintf() or similar.  We'll have
            ** to render into temporary space and the memcpy() it over. */
            bufpt = sqlite3_malloc64(szBufNeeded);
            if( bufpt==0 ){
              sqlite3StrAccumSetError(pAccum, SQLITE_NOMEM);
              return;
            }
            zExtra = bufpt;
          }else if( sqlite3StrAccumEnlarge(pAccum, szBufNeeded)<szBufNeeded ){
            width = length = 0;
            break;
          }else{
            bufpt = pAccum->zText + pAccum->nChar;
          }
        }else{
          bufpt = pAccum->zText + pAccum->nChar;
        }
        zOut = bufpt;

        flag_dp = (precision>0 ?1:0) | flag_alternateform | flag_altform2;
        /* The sign in front of the number */
        if( prefix ){
          *(bufpt++) = prefix;
        }
        /* Digits prior to the decimal point */
        j = 0;
        assert( s.n>0 );
        if( e2<0 ){
          *(bufpt++) = '0';
        }else if( cThousand ){
          for(; e2>=0; e2--){
            *(bufpt++) = j<s.n ? s.z[j++] : '0';
            if( (e2%3)==0 && e2>1 ) *(bufpt++) = ',';
          }
        }else{
          j = e2+1;
          if( j>s.n ) j = s.n;
          memcpy(bufpt, s.z, j);
          bufpt += j;
          e2 -= j;
          if( e2>=0 ){
            memset(bufpt, '0', e2+1);
            bufpt += e2+1;
            e2 = -1;
          }
        }
        /* The decimal point */
        if( flag_dp ){
          *(bufpt++) = '.';
        }
        /* "0" digits after the decimal point but before the first
        ** significant digit of the number */
        if( e2<(-1) && precision>0 ){
          int nn = -1-e2;
          if( nn>precision ) nn = precision;
          memset(bufpt, '0', nn);
          bufpt += nn;
          precision -= nn;
        }
        /* Significant digits after the decimal point */
        if( precision>0 ){
          int nn = s.n - j;
          if( NEVER(nn>precision) ) nn = precision;
          if( nn>0 ){
            memcpy(bufpt, s.z+j, nn);
            bufpt += nn;
            precision -= nn;
          }
          if( precision>0 && !flag_rtz ){
            memset(bufpt, '0', precision);
            bufpt += precision;
          }
        }
        /* Remove trailing zeros and the "." if no digits follow the "." */
        if( flag_rtz && flag_dp ){
          while( bufpt[-1]=='0' ) *(--bufpt) = 0;
          assert( bufpt>zOut );
          if( bufpt[-1]=='.' ){
            if( flag_altform2 ){
              *(bufpt++) = '0';
            }else{
              *(--bufpt) = 0;
            }
          }
        }
        /* Add the "eNNN" suffix */
        if( xtype==etEXP ){
          exp = s.iDP - 1;
          *(bufpt++) = aDigits[infop->charset];
          if( exp<0 ){
            *(bufpt++) = '-'; exp = -exp;
          }else{
            *(bufpt++) = '+';
          }
          if( exp>=100 ){
            *(bufpt++) = (char)((exp/100)+'0');        /* 100's digit */
            exp %= 100;
          }
          *(bufpt++) = (char)(exp/10+'0');             /* 10's digit */
          *(bufpt++) = (char)(exp%10+'0');             /* 1's digit */
        }

        length = bufpt - zOut;
        assert( length <= szBufNeeded );
        if( length<width ){
          i64 nPad = width - length;
          if( flag_leftjustify ){
            memset(bufpt, ' ', nPad);
          }else if( !flag_zeropad ){
            memmove(zOut+nPad, zOut, length);
            memset(zOut, ' ', nPad);
          }else{
            int adj = prefix!=0;
            memmove(zOut+nPad+adj, zOut+adj, length-adj);
            memset(zOut+adj, '0', nPad);
          }
          length = width;
        }

        if( zExtra==0 ){
          /* The result is being rendered directory into pAccum.  This
          ** is the common and fast case */
          assert( pAccum->nChar + length < SMXV(pAccum->nChar) );
          pAccum->nChar += length;
          zOut[length] = 0;
          continue;
        }else{
          /* We were unable to render directly into pAccum because we
          ** couldn't allocate sufficient memory.  We need to memcpy()
          ** the rendering (or some prefix thereof) into the output
          ** buffer. */
          bufpt[0] = 0;
          bufpt = zExtra;
          break;
        }
      }
      case etSIZE:
        if( !bArgList ){
          *(va_arg(ap,int*)) = pAccum->nChar;
        }
        length = width = 0;
        break;
      case etPERCENT:
        buf[0] = '%';
        bufpt = buf;
        length = 1;
        break;
      case etCHARX:
        if( bArgList ){
          bufpt = getTextArg(pArgList);
          length = 1;
          if( bufpt ){
            buf[0] = c = *(bufpt++);
            if( (c&0xc0)==0xc0 ){
              while( length<4 && (bufpt[0]&0xc0)==0x80 ){
                buf[length++] = *(bufpt++);
              }
            }
          }else{
            buf[0] = 0;
          }
        }else{
          unsigned int ch = va_arg(ap,unsigned int);
          length = sqlite3AppendOneUtf8Character(buf, ch);
        }
        if( precision>1 ){
          i64 nPrior = 1;
          width -= precision-1;
          if( width>1 && !flag_leftjustify ){
            sqlite3StrAppendchar64(pAccum, width-1, ' ');
            width = 0;
          }
          sqlite3StrAppend64(pAccum, buf, length);
          precision--;
          while( precision > 1 ){
            i64 nCopyBytes;
            if( nPrior > precision-1 ) nPrior = precision - 1;
            nCopyBytes = length*nPrior;
            if( sqlite3StrAccumEnlargeIfNeeded(pAccum, nCopyBytes) ){
              break;
            }
            sqlite3_str_append(pAccum,
                 &pAccum->zText[pAccum->nChar-nCopyBytes], nCopyBytes);
            precision -= nPrior;
            nPrior *= 2;
          }
        }
        bufpt = buf;
        flag_altform2 = 1;
        goto adjust_width_for_utf8;
      case etSTRING:
      case etDYNSTRING:
        if( bArgList ){
          bufpt = getTextArg(pArgList);
          xtype = etSTRING;
        }else{
          bufpt = va_arg(ap,char*);
        }
        if( bufpt==0 ){
          bufpt = "";
        }else if( xtype==etDYNSTRING ){
          if( pAccum->nChar==0
           && pAccum->mxAlloc
           && width==0
           && precision<0
           && pAccum->accError==0
          ){
            /* Special optimization for sqlite3_mprintf("%z..."):
            ** Extend an existing memory allocation rather than creating
            ** a new one. */
            assert( (pAccum->printfFlags&SQLITE_PRINTF_MALLOCED)==0 );
            pAccum->zText = bufpt;
            pAccum->nAlloc = sqlite3DbMallocSize(pAccum->db, bufpt);
            pAccum->nChar = 0x7fffffff & (int)strlen(bufpt);
            pAccum->printfFlags |= SQLITE_PRINTF_MALLOCED;
            length = 0;
            break;
          }
          zExtra = bufpt;
        }
        if( precision>=0 ){
          if( flag_altform2 ){
            /* Set length to the number of bytes needed in order to display
            ** precision characters */
            unsigned char *z = (unsigned char*)bufpt;
            while( precision-- > 0 && z[0] ){
              SQLITE_SKIP_UTF8(z);
            }
            length = z - (unsigned char*)bufpt;
          }else{
            for(length=0; length<precision && bufpt[length]; length++){}
          }
        }else{
          length = strlen(bufpt);
        }
      adjust_width_for_utf8:
        if( flag_altform2 && width>0 ){
          /* Adjust width to account for extra bytes in UTF-8 characters */
          i64 ii = length - 1;
          while( ii>=0 ) if( (bufpt[ii--] & 0xc0)==0x80 ) width++;
        }
        break;
      case etESCAPE_j:           /* %j: JSON string literal w/o "..." */
      case etESCAPE_J: {         /* %J: Generate a JSON string literal */
        char *escarg;
        i64 i, j, px, iStart;
        unsigned char ch;

        if( bArgList ){
          escarg = getTextArg(pArgList);
        }else{
          escarg = va_arg(ap,char*);
        }
        iStart = sqlite3_str_length(pAccum);
        if( escarg==0 ){
          if( xtype==etESCAPE_J ) sqlite3_str_append(pAccum, "null", 4);
        }else{
          if( xtype==etESCAPE_J ) sqlite3_str_append(pAccum, "\"", 1);
          px = precision;
          testcase( px==0 );
          if( px<0 ){
            px = 0x7fffffff;
          }else if( flag_altform2 ){
            /* Convert precision from code-points to bytes */
            for(i=0; i<px && escarg[i]; i++){
              if( (escarg[i]&0xc0)==0x80 ) px++;
            }
            if( i==px ){
              while( (escarg[px]&0xc0)==0x80 ) px++;
            }
          }
          for(i=j=0; i<px; i++){
            if( (ch = ((u8*)escarg)[i])<=0x1f || ch=='"' || ch=='\\' ){
              if( j<i ) sqlite3StrAppend64(pAccum, &escarg[j], i-j);
              j = i+1;
              if( ch==0 ) break;
              sqlite3_str_appendchar(pAccum, 1, '\\');
              if( ch>0x1f ){
                sqlite3_str_appendchar(pAccum, 1, ch);
              }else if( ((1u<<ch)&0x3700)!=0 ){
                ch = "btn?fr"[ch-8];
                sqlite3_str_appendchar(pAccum, 1, ch);
              }else{
                sqlite3_str_append(pAccum, "u00", 3);
                sqlite3_str_appendchar(pAccum, 1, aHex[ch>>4]);
                sqlite3_str_appendchar(pAccum, 1, aHex[ch&0xf]);
              }
            }
          }
          if( j<i ) sqlite3StrAppend64(pAccum, &escarg[j], i-j);
          if( xtype==etESCAPE_J ) sqlite3_str_append(pAccum, "\"", 1);
        }
        if( width>0 && sqlite3_str_errcode(pAccum)==SQLITE_OK ){
          sqlite3_int64 n = sqlite3_str_length(pAccum) - iStart;
          sqlite3_int64 len = n;
          char *zz;
          if( flag_altform2 && n>0 ){
            zz = sqlite3_str_value(pAccum);
            for(i=iStart; zz[i]; i++){
              if( (zz[i]&0xc0)==0x80 ) len--;
            }
          }
          if( width>len ){
            sqlite3_int64 sp = width-len;
            assert( sp>0 && sp<0x7fffffff );
            sqlite3StrAppendchar64(pAccum, (int)sp, ' ');
            if( !flag_leftjustify
             && n>0
             && sqlite3_str_errcode(pAccum)==0
            ){
              zz = sqlite3_str_value(pAccum);
              zz += iStart;
              memmove(zz+sp, zz, n);
              memset(zz, ' ', sp);
            }
          }
        }
        continue;
      }
      case etESCAPE_q:          /* %q: Escape ' characters */
      case etESCAPE_Q:          /* %Q: Escape ' and enclose in '...' */
      case etESCAPE_w: {        /* %w: Escape " characters */
        i64 i, j, k, n;
        int needQuote = 0;
        char ch;
        char *escarg;
        char q;

        if( bArgList ){
          escarg = getTextArg(pArgList);
        }else{
          escarg = va_arg(ap,char*);
        }
        if( escarg==0 ){
          escarg = (xtype==etESCAPE_Q ? "NULL" : "(NULL)");
        }else if( xtype==etESCAPE_Q ){
          needQuote = 1;
        }
        if( xtype==etESCAPE_w ){
          q = '"';
          flag_alternateform = 0;
        }else{
          q = '\'';
        }
        /* For %q, %Q, and %w, the precision is the number of bytes (or
        ** characters if the ! flags is present) to use from the input.
        ** Because of the extra quoting characters inserted, the number
        ** of output characters may be larger than the precision.
        */
        k = precision;
        for(i=n=0; k!=0 && (ch=escarg[i])!=0; i++, k--){
          if( ch==q )  n++;
          if( flag_altform2 && (ch&0xc0)==0xc0 ){
            while( (escarg[i+1]&0xc0)==0x80 ){ i++; }
          }
        }
        if( flag_alternateform ){
          /* For %#q, do unistr()-style backslash escapes for
          ** all control characters, and for backslash itself.
          ** For %#Q, do the same but only if there is at least
          ** one control character. */
          i64 nBack = 0;
          i64 nCtrl = 0;
          for(k=0; k<i; k++){
            if( escarg[k]=='\\' ){
              nBack++;
            }else if( ((u8*)escarg)[k]<=0x1f ){
              nCtrl++;
            }
          }
          if( nCtrl || xtype==etESCAPE_q ){
            n += nBack + 5*nCtrl;
            if( xtype==etESCAPE_Q ){
              n += 10;
              needQuote = 2;
            }
          }else{
            flag_alternateform = 0;
          }
        }
        n += i + 3;
        if( n>etBUFSIZE ){
          bufpt = zExtra = printfTempBuf(pAccum, n);
          if( bufpt==0 ) return;
        }else{
          bufpt = buf;
        }
        j = 0;
        if( needQuote ){
          if( needQuote==2 ){
            memcpy(&bufpt[j], "unistr('", 8);
            j += 8;
          }else{
            bufpt[j++] = '\'';
          }
        }
        k = i;
        if( flag_alternateform ){
          for(i=0; i<k; i++){
            bufpt[j++] = ch = escarg[i];
            if( ch==q ){
              bufpt[j++] = ch;
            }else if( ch=='\\' ){
              bufpt[j++] = '\\';
            }else if( ((unsigned char)ch)<=0x1f ){
              bufpt[j-1] = '\\';
              bufpt[j++] = 'u';
              bufpt[j++] = '0';
              bufpt[j++] = '0';
              bufpt[j++] = ch>=0x10 ? '1' : '0';
              bufpt[j++] = aHex[ch&0xf];
            }
          }
        }else{
          for(i=0; i<k; i++){
            bufpt[j++] = ch = escarg[i];
            if( ch==q ) bufpt[j++] = ch;
          }
        }
        if( needQuote ){
          bufpt[j++] = '\'';
          if( needQuote==2 ) bufpt[j++] = ')';
        }
        bufpt[j] = 0;
        length = j;
        goto adjust_width_for_utf8;
      }
      case etTOKEN: {
        if( (pAccum->printfFlags & SQLITE_PRINTF_INTERNAL)==0 ) return;
        if( flag_alternateform ){
          /* %#T means an Expr pointer that uses Expr.u.zToken */
          Expr *pExpr = va_arg(ap,Expr*);
          if( ALWAYS(pExpr) && ALWAYS(!ExprHasProperty(pExpr,EP_IntValue)) ){
            sqlite3_str_appendall(pAccum, (const char*)pExpr->u.zToken);
            sqlite3RecordErrorOffsetOfExpr(pAccum->db, pExpr);
          }
        }else{
          /* %T means a Token pointer */
          Token *pToken = va_arg(ap, Token*);
          assert( bArgList==0 );
          if( pToken && pToken->n ){
            sqlite3_str_append(pAccum, (const char*)pToken->z, pToken->n);
            sqlite3RecordErrorByteOffset(pAccum->db, pToken->z);
          }
        }
        length = width = 0;
        break;
      }
      case etSRCITEM: {
        SrcItem *pItem;
        if( (pAccum->printfFlags & SQLITE_PRINTF_INTERNAL)==0 ) return;
        pItem = va_arg(ap, SrcItem*);
        assert( bArgList==0 );
        if( pItem->zAlias && !flag_altform2 ){
          sqlite3_str_appendall(pAccum, pItem->zAlias);
        }else if( pItem->zName ){
          if( pItem->fg.fixedSchema==0
           && pItem->fg.isSubquery==0
           && pItem->u4.zDatabase!=0
          ){
            sqlite3_str_appendall(pAccum, pItem->u4.zDatabase);
            sqlite3_str_append(pAccum, ".", 1);
          }
          sqlite3_str_appendall(pAccum, pItem->zName);
        }else if( pItem->zAlias ){
          sqlite3_str_appendall(pAccum, pItem->zAlias);
        }else if( ALWAYS(pItem->fg.isSubquery) ){/* Because of tag-20240424-1 */
          Select *pSel = pItem->u4.pSubq->pSelect;
          assert( pSel!=0 );
          if( pSel->selFlags & SF_NestedFrom ){
            sqlite3_str_appendf(pAccum, "(join-%u)", pSel->selId);
          }else if( pSel->selFlags & SF_MultiValue ){
            assert( !pItem->fg.isTabFunc && !pItem->fg.isIndexedBy );
            sqlite3_str_appendf(pAccum, "%u-ROW VALUES CLAUSE",
                                pItem->u1.nRow);
          }else{
            sqlite3_str_appendf(pAccum, "(subquery-%u)", pSel->selId);
          }
        }
        length = width = 0;
        break;
      }
      default: {
        assert( xtype==etINVALID );
        return;
      }
    }/* End switch over the format type */
    /*
    ** The text of the conversion is pointed to by "bufpt" and is
    ** "length" characters long.  The field width is "width".  Do
    ** the output.  Both length and width are in bytes, not characters,
    ** at this point.  If the "!" flag was present on string conversions
    ** indicating that width and precision should be expressed in characters,
    ** then the values have been translated prior to reaching this point.
    */
    width -= length;
    if( width>0 ){
      if( !flag_leftjustify ) sqlite3StrAppendchar64(pAccum, width, ' ');
      sqlite3StrAppend64(pAccum, bufpt, length);
      if( flag_leftjustify ) sqlite3StrAppendchar64(pAccum, width, ' ');
    }else{
      sqlite3StrAppend64(pAccum, bufpt, length);
    }

    if( zExtra ){
      sqlite3DbFree(pAccum->db, zExtra);
      zExtra = 0;
    }
  }/* End for loop over the format string */
} /* End of function */


/*
** The z string points to the first character of a token that is
** associated with an error.  If db does not already have an error
** byte offset recorded, try to compute the error byte offset for
** z and set the error byte offset in db.
*/
SQLITE_PRIVATE void sqlite3RecordErrorByteOffset(sqlite3 *db, const char *z){
  const Parse *pParse;
  const char *zText;
  const char *zEnd;
  assert( z!=0 );
  if( NEVER(db==0) ) return;
  if( db->errByteOffset!=(-2) ) return;
  pParse = db->pParse;
  if( NEVER(pParse==0) ) return;
  zText =pParse->zTail;
  if( NEVER(zText==0) ) return;
  zEnd = &zText[strlen(zText)];
  if( SQLITE_WITHIN(z,zText,zEnd) ){
    db->errByteOffset = (int)(z-zText);
  }
}

/*
** If pExpr has a byte offset for the start of a token, record that as
** as the error offset.
*/
SQLITE_PRIVATE void sqlite3RecordErrorOffsetOfExpr(sqlite3 *db, const Expr *pExpr){
  while( pExpr ){
    if( ExprHasProperty(pExpr, EP_Reduced|EP_TokenOnly) ) return;
    if( ExprHasProperty(pExpr,EP_OuterON|EP_InnerON) || pExpr->w.iOfst<=0 ){
      pExpr = pExpr->pLeft;
    }else{
      break;
    }
  }
  if( pExpr==0 ) return;
  if( ExprHasProperty(pExpr, EP_FromDDL) ) return;
  db->errByteOffset = pExpr->w.iOfst;
}

/*
** Enlarge the memory allocation on a StrAccum object so that it is
** able to accept at least N more bytes of text.
**
** Return the number of bytes of text that StrAccum is able to accept
** after the attempted enlargement.  The value returned might be zero.
*/
SQLITE_PRIVATE int sqlite3StrAccumEnlarge(StrAccum *p, i64 N){
  char *zNew;
  assert( p->nChar+N >= p->nAlloc ); /* Only called if really needed */
  if( p->accError ){
    testcase(p->accError==SQLITE_TOOBIG);
    testcase(p->accError==SQLITE_NOMEM);
    return 0;
  }
  if( p->mxAlloc==0 ){
    sqlite3StrAccumSetError(p, SQLITE_TOOBIG);
    return p->nAlloc - p->nChar - 1;
  }else{
    char *zOld = isMalloced(p) ? p->zText : 0;
    i64 szNew = p->nChar + N + 1;
    if( szNew+p->nChar<=p->mxAlloc ){
      /* Force exponential buffer size growth as long as it does not overflow,
      ** to avoid having to call this routine too often */
      szNew += p->nChar;
    }
    if( szNew > p->mxAlloc ){
      sqlite3_str_reset(p);
      sqlite3StrAccumSetError(p, SQLITE_TOOBIG);
      return 0;
    }else{
      p->nAlloc = (int)szNew;
    }
    if( p->db ){
      zNew = sqlite3DbRealloc(p->db, zOld, p->nAlloc);
    }else{
      zNew = sqlite3Realloc(zOld, p->nAlloc);
    }
    if( zNew ){
      assert( p->zText!=0 || p->nChar==0 );
      if( !isMalloced(p) && p->nChar>0 ) memcpy(zNew, p->zText, p->nChar);
      p->zText = zNew;
      p->nAlloc = sqlite3DbMallocSize(p->db, zNew);
      p->printfFlags |= SQLITE_PRINTF_MALLOCED;
    }else{
      sqlite3_str_reset(p);
      sqlite3StrAccumSetError(p, SQLITE_NOMEM);
      return 0;
    }
  }
  assert( N>=0 && N<=0x7fffffff );
  return (int)N;
}

SQLITE_PRIVATE int sqlite3StrAccumEnlargeIfNeeded(StrAccum *p, i64 N){
  if( N + p->nChar >= p->nAlloc ){
    sqlite3StrAccumEnlarge(p, N);
  }
  return p->accError;
}

/*
** Append N copies of character c to the given string buffer.
*/
SQLITE_API void sqlite3_str_appendchar(sqlite3_str *p, int N, char c){
  testcase( p->nChar + (i64)N > 0x7fffffff );
  if( p->nChar+(i64)N >= p->nAlloc && (N = sqlite3StrAccumEnlarge(p, N))<=0 ){
    return;
  }
  while( (N--)>0 ) p->zText[p->nChar++] = c;
}
static void sqlite3StrAppendchar64(sqlite3_str *p, i64 N, char c){
  testcase( p->nChar + N > 0x7fffffff );
  if( p->nChar+N >= p->nAlloc && (N = sqlite3StrAccumEnlarge(p, N))<=0 ){
    return;
  }
  while( (N--)>0 ) p->zText[p->nChar++] = c;
}

/*
** The StrAccum "p" is not large enough to accept N new bytes of z[].
** So enlarge if first, then do the append.
**
** This is a helper routine to sqlite3_str_append() that does special-case
** work (enlarging the buffer) using tail recursion, so that the
** sqlite3_str_append() routine can use fast calling semantics.
*/
static void SQLITE_NOINLINE enlargeAndAppend(StrAccum *p, const char *z, int N){
  N = sqlite3StrAccumEnlarge(p, N);
  assert( z!=0 || N==0 );
  if( N>0 ){
    memcpy(&p->zText[p->nChar], z, N);
    p->nChar += N;
  }
}

/*
** Append N bytes of text from z to the StrAccum object.  Increase the
** size of the memory allocation for StrAccum if necessary.
*/
SQLITE_API void sqlite3_str_append(sqlite3_str *p, const char *z, int N){
  assert( z!=0 || N==0 );
  assert( p->zText!=0 || p->nChar==0 || p->accError );
  assert( N>=0 );
  assert( p->accError==0 || p->nAlloc==0 || p->mxAlloc==0 );
  if( p->nChar+N >= p->nAlloc ){
    enlargeAndAppend(p,z,N);
  }else if( N ){
    assert( p->zText );
    p->nChar += N;
    memcpy(&p->zText[p->nChar-N], z, N);
  }
}
static void sqlite3StrAppend64(sqlite3_str *p, const char *z, i64 N){
  assert( z!=0 || N==0 );
  assert( p->zText!=0 || p->nChar==0 || p->accError );
  assert( N>=0 );
  assert( p->accError==0 || p->nAlloc==0 || p->mxAlloc==0 );
  if( p->nChar+N >= (i64)p->nAlloc ){
    enlargeAndAppend(p,z,N);
  }else if( N ){
    assert( p->zText );
    p->nChar += N;
    memcpy(&p->zText[p->nChar-N], z, N);
  }
}


/*
** Append the complete text of zero-terminated string z[] to the p string.
*/
SQLITE_API void sqlite3_str_appendall(sqlite3_str *p, const char *z){
  sqlite3_str_append(p, z, sqlite3Strlen30(z));
}


/*
** Finish off a string by making sure it is zero-terminated.
** Return a pointer to the resulting string.  Return a NULL
** pointer if any kind of error was encountered.
*/
static SQLITE_NOINLINE char *strAccumFinishRealloc(StrAccum *p){
  char *zText;
  assert( p->mxAlloc>0 && !isMalloced(p) );
  zText = sqlite3DbMallocRaw(p->db, 1+(u64)p->nChar );
  if( zText ){
    memcpy(zText, p->zText, p->nChar+1);
    p->printfFlags |= SQLITE_PRINTF_MALLOCED;
  }else{
    sqlite3StrAccumSetError(p, SQLITE_NOMEM);
  }
  p->zText = zText;
  return zText;
}
SQLITE_PRIVATE char *sqlite3StrAccumFinish(StrAccum *p){
  if( p->zText ){
    p->zText[p->nChar] = 0;
    if( p->mxAlloc>0 && !isMalloced(p) ){
      return strAccumFinishRealloc(p);
    }
  }
  return p->zText;
}

/* Finalize a string created using sqlite3_str_new().
*/
SQLITE_API char *sqlite3_str_finish(sqlite3_str *p){
  char *z;
  if( p!=0 && p!=(sqlite3_str*)&sqlite3OomStr ){
    z = sqlite3StrAccumFinish(p);
    sqlite3_free(p);
  }else{
    z = 0;
  }
  return z;
}

/* Return any error code associated with p */
SQLITE_API int sqlite3_str_errcode(sqlite3_str *p){
  return p ? p->accError : SQLITE_NOMEM;
}

/* Return the current length of p in bytes */
SQLITE_API int sqlite3_str_length(sqlite3_str *p){
  return p ? p->nChar : 0;
}

/* Truncate the text of the string to be no more than N bytes. */
SQLITE_API void sqlite3_str_truncate(sqlite3_str *p, int N){
  if( p!=0 && N>=0 && (u32)N<p->nChar ){
    p->nChar = N;
    p->zText[p->nChar] = 0;
  }
}

/* Return the current value for p */
SQLITE_API char *sqlite3_str_value(sqlite3_str *p){
  if( p==0 || p->nChar==0 ) return 0;
  p->zText[p->nChar] = 0;
  return p->zText;
}

/*
** Reset an StrAccum string.  Reclaim all malloced memory.
*/
SQLITE_API void sqlite3_str_reset(StrAccum *p){
  if( isMalloced(p) ){
    sqlite3DbFree(p->db, p->zText);
    p->printfFlags &= ~SQLITE_PRINTF_MALLOCED;
  }else if( p==(sqlite3_str*)&sqlite3OomStr ){
    return;
  }
  p->nAlloc = 0;
  p->nChar = 0;
  p->zText = 0;
}

/*
** Destroy a dynamically allocate sqlite3_str object and all
** of its content, all in one call.
*/
SQLITE_API void sqlite3_str_free(sqlite3_str *p){
  if( p!=0 && p!=(sqlite3_str*)&sqlite3OomStr ){
    sqlite3_str_reset(p);
    sqlite3_free(p);
  }
}

/*
** Initialize a string accumulator.
**
** p:     The accumulator to be initialized.
** db:    Pointer to a database connection.  May be NULL.  Lookaside
**        memory is used if not NULL. db->mallocFailed is set appropriately
**        when not NULL.
** zBase: An initial buffer.  May be NULL in which case the initial buffer
**        is malloced.
** n:     Size of zBase in bytes.  If total space requirements never exceed
**        n then no memory allocations ever occur.
** mx:    Maximum number of bytes to accumulate.  If mx==0 then no memory
**        allocations will ever occur.
*/
SQLITE_PRIVATE void sqlite3StrAccumInit(StrAccum *p, sqlite3 *db, char *zBase, int n, int mx){
  p->zText = zBase;
  p->db = db;
  p->nAlloc = n;
  p->mxAlloc = mx;
  p->nChar = 0;
  p->accError = 0;
  p->printfFlags = 0;
}

/* Allocate and initialize a new dynamic string object */
SQLITE_API sqlite3_str *sqlite3_str_new(sqlite3 *db){
  sqlite3_str *p = sqlite3_malloc64(sizeof(*p));
  if( p ){
    sqlite3StrAccumInit(p, 0, 0, 0,
            db ? db->aLimit[SQLITE_LIMIT_LENGTH] : SQLITE_MAX_LENGTH);
  }else{
    p = (sqlite3_str*)&sqlite3OomStr;
  }
  return p;
}

/*
** Print into memory obtained from sqliteMalloc().  Use the internal
** %-conversion extensions.
*/
SQLITE_PRIVATE char *sqlite3VMPrintf(sqlite3 *db, const char *zFormat, va_list ap){
  char *z;
  char zBase[SQLITE_PRINT_BUF_SIZE];
  StrAccum acc;
  assert( db!=0 );
  sqlite3StrAccumInit(&acc, db, zBase, sizeof(zBase),
                      db->aLimit[SQLITE_LIMIT_LENGTH]);
  acc.printfFlags = SQLITE_PRINTF_INTERNAL;
  sqlite3_str_vappendf(&acc, zFormat, ap);
  z = sqlite3StrAccumFinish(&acc);
  if( acc.accError==SQLITE_NOMEM ){
    sqlite3OomFault(db);
  }
  return z;
}

/*
** Print into memory obtained from sqliteMalloc().  Use the internal
** %-conversion extensions.
*/
SQLITE_PRIVATE char *sqlite3MPrintf(sqlite3 *db, const char *zFormat, ...){
  va_list ap;
  char *z;
  va_start(ap, zFormat);
  z = sqlite3VMPrintf(db, zFormat, ap);
  va_end(ap);
  return z;
}

/*
** Print into memory obtained from sqlite3_malloc().  Omit the internal
** %-conversion extensions.
*/
SQLITE_API char *sqlite3_vmprintf(const char *zFormat, va_list ap){
  char *z;
  char zBase[SQLITE_PRINT_BUF_SIZE];
  StrAccum acc;

#ifdef SQLITE_ENABLE_API_ARMOR
  if( zFormat==0 ){
    (void)SQLITE_MISUSE_BKPT;
    return 0;
  }
#endif
#ifndef SQLITE_OMIT_AUTOINIT
  if( sqlite3_initialize() ) return 0;
#endif
  sqlite3StrAccumInit(&acc, 0, zBase, sizeof(zBase), SQLITE_MAX_LENGTH);
  sqlite3_str_vappendf(&acc, zFormat, ap);
  z = sqlite3StrAccumFinish(&acc);
  return z;
}

/*
** Print into memory obtained from sqlite3_malloc()().  Omit the internal
** %-conversion extensions.
*/
SQLITE_API char *sqlite3_mprintf(const char *zFormat, ...){
  va_list ap;
  char *z;
#ifndef SQLITE_OMIT_AUTOINIT
  if( sqlite3_initialize() ) return 0;
#endif
  va_start(ap, zFormat);
  z = sqlite3_vmprintf(zFormat, ap);
  va_end(ap);
  return z;
}

/*
** sqlite3_snprintf() works like snprintf() except that it ignores the
** current locale settings.  This is important for SQLite because we
** are not able to use a "," as the decimal point in place of "." as
** specified by some locales.
**
** Oops:  The first two arguments of sqlite3_snprintf() are backwards
** from the snprintf() standard.  Unfortunately, it is too late to change
** this without breaking compatibility, so we just have to live with the
** mistake.
**
** sqlite3_vsnprintf() is the varargs version.
*/
SQLITE_API char *sqlite3_vsnprintf(int n, char *zBuf, const char *zFormat, va_list ap){
  StrAccum acc;
  if( n<=0 ) return zBuf;
#ifdef SQLITE_ENABLE_API_ARMOR
  if( zBuf==0 || zFormat==0 ) {
    (void)SQLITE_MISUSE_BKPT;
    if( zBuf ) zBuf[0] = 0;
    return zBuf;
  }
#endif
  sqlite3StrAccumInit(&acc, 0, zBuf, n, 0);
  sqlite3_str_vappendf(&acc, zFormat, ap);
  zBuf[acc.nChar] = 0;
  return zBuf;
}
SQLITE_API char *sqlite3_snprintf(int n, char *zBuf, const char *zFormat, ...){
  StrAccum acc;
  va_list ap;
  if( n<=0 ) return zBuf;
#ifdef SQLITE_ENABLE_API_ARMOR
  if( zBuf==0 || zFormat==0 ) {
    (void)SQLITE_MISUSE_BKPT;
    if( zBuf ) zBuf[0] = 0;
    return zBuf;
  }
#endif
  sqlite3StrAccumInit(&acc, 0, zBuf, n, 0);
  va_start(ap,zFormat);
  sqlite3_str_vappendf(&acc, zFormat, ap);
  va_end(ap);
  zBuf[acc.nChar] = 0;
  return zBuf;
}

/* Maximum size of an sqlite3_log() message. */
#if defined(SQLITE_MAX_LOG_MESSAGE)
  /* Leave the definition as supplied */
#elif SQLITE_PRINT_BUF_SIZE*10>10000
# define SQLITE_MAX_LOG_MESSAGE 10000
#else
# define SQLITE_MAX_LOG_MESSAGE (SQLITE_PRINT_BUF_SIZE*10)
#endif

/*
** This is the routine that actually formats the sqlite3_log() message.
** We house it in a separate routine from sqlite3_log() to avoid using
** stack space on small-stack systems when logging is disabled.
**
** sqlite3_log() must render into a static buffer.  It cannot dynamically
** allocate memory because it might be called while the memory allocator
** mutex is held.
**
** sqlite3_str_vappendf() might ask for *temporary* memory allocations for
** certain format characters (%q) or for very large precisions or widths.
** Care must be taken that any sqlite3_log() calls that occur while the
** memory mutex is held do not use these mechanisms.
*/
static void renderLogMsg(int iErrCode, const char *zFormat, va_list ap){
  StrAccum acc;                          /* String accumulator */
  char zMsg[SQLITE_MAX_LOG_MESSAGE];     /* Complete log message */

  sqlite3StrAccumInit(&acc, 0, zMsg, sizeof(zMsg), 0);
  sqlite3_str_vappendf(&acc, zFormat, ap);
  sqlite3GlobalConfig.xLog(sqlite3GlobalConfig.pLogArg, iErrCode,
                           sqlite3StrAccumFinish(&acc));
}

/*
** Format and write a message to the log if logging is enabled.
*/
SQLITE_API void sqlite3_log(int iErrCode, const char *zFormat, ...){
  va_list ap;                             /* Vararg list */
  if( sqlite3GlobalConfig.xLog ){
    va_start(ap, zFormat);
    renderLogMsg(iErrCode, zFormat, ap);
    va_end(ap);
  }
}

#if defined(SQLITE_DEBUG) || defined(SQLITE_HAVE_OS_TRACE)
/*
** A version of printf() that understands %lld.  Used for debugging.
** The printf() built into some versions of windows does not understand %lld
** and segfaults if you give it a long long int.
*/
SQLITE_PRIVATE void sqlite3DebugPrintf(const char *zFormat, ...){
  va_list ap;
  StrAccum acc;
  char zBuf[SQLITE_PRINT_BUF_SIZE*10];
  sqlite3StrAccumInit(&acc, 0, zBuf, sizeof(zBuf), 0);
  va_start(ap,zFormat);
  sqlite3_str_vappendf(&acc, zFormat, ap);
  va_end(ap);
  sqlite3StrAccumFinish(&acc);
#ifdef SQLITE_OS_TRACE_PROC
  {
    extern void SQLITE_OS_TRACE_PROC(const char *zBuf, int nBuf);
    SQLITE_OS_TRACE_PROC(zBuf, sizeof(zBuf));
  }
#else
  fprintf(stdout,"%s", zBuf);
  fflush(stdout);
#endif
}
#endif


/*
** variable-argument wrapper around sqlite3_str_vappendf(). The bFlags argument
** can contain the bit SQLITE_PRINTF_INTERNAL enable internal formats.
*/
SQLITE_API void sqlite3_str_appendf(StrAccum *p, const char *zFormat, ...){
  va_list ap;
  va_start(ap,zFormat);
  sqlite3_str_vappendf(p, zFormat, ap);
  va_end(ap);
}


/*****************************************************************************
** Reference counted string/blob storage
*****************************************************************************/

/*
** Increase the reference count of the string by one.
**
** The input parameter is returned.
*/
SQLITE_PRIVATE char *sqlite3RCStrRef(char *z){
  RCStr *p = (RCStr*)z;
  assert( p!=0 );
  p--;
  p->nRCRef++;
  return z;
}

/*
** Decrease the reference count by one.  Free the string when the
** reference count reaches zero.
*/
SQLITE_PRIVATE void sqlite3RCStrUnref(void *z){
  RCStr *p = (RCStr*)z;
  assert( p!=0 );
  p--;
  assert( p->nRCRef>0 );
  if( p->nRCRef>=2 ){
    p->nRCRef--;
  }else{
    sqlite3_free(p);
  }
}

/*
** Create a new string that is capable of holding N bytes of text, not counting
** the zero byte at the end.  The string is uninitialized.
**
** The reference count is initially 1.  Call sqlite3RCStrUnref() to free the
** newly allocated string.
**
** This routine returns 0 on an OOM.
*/
SQLITE_PRIVATE char *sqlite3RCStrNew(u64 N){
  RCStr *p = sqlite3_malloc64( N + sizeof(*p) + 1 );
  if( p==0 ) return 0;
  p->nRCRef = 1;
  return (char*)&p[1];
}

/*
** Change the size of the string so that it is able to hold N bytes.
** The string might be reallocated, so return the new allocation.
*/
SQLITE_PRIVATE char *sqlite3RCStrResize(char *z, u64 N){
  RCStr *p = (RCStr*)z;
  RCStr *pNew;
  assert( p!=0 );
  p--;
  assert( p->nRCRef==1 );
  pNew = sqlite3_realloc64(p, N+sizeof(RCStr)+1);
  if( pNew==0 ){
    sqlite3_free(p);
    return 0;
  }else{
    return (char*)&pNew[1];
  }
}

/************** End of printf.c **********************************************/
/************** Begin file treeview.c ****************************************/
/*
** 2015-06-08
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
*************************************************************************
**
** This file contains C code to implement the TreeView debugging routines.
** These routines print a parse tree to standard output for debugging and
** analysis.
**
** The interfaces in this file is only available when compiling
** with SQLITE_DEBUG.
*/
/* #include "sqliteInt.h" */
#ifdef SQLITE_DEBUG

/*
** Add a new subitem to the tree.  The moreToFollow flag indicates that this
** is not the last item in the tree.
*/
static void sqlite3TreeViewPush(TreeView **pp, u8 moreToFollow){
  TreeView *p = *pp;
  if( p==0 ){
    *pp = p = sqlite3_malloc64( sizeof(*p) );
    if( p==0 ) return;
    memset(p, 0, sizeof(*p));
  }else{
    p->iLevel++;
  }
  assert( moreToFollow==0 || moreToFollow==1 );
  if( p->iLevel<(int)sizeof(p->bLine) ) p->bLine[p->iLevel] = moreToFollow;
}

/*
** Finished with one layer of the tree
*/
static void sqlite3TreeViewPop(TreeView **pp){
  TreeView *p = *pp;
  if( p==0 ) return;
  p->iLevel--;
  if( p->iLevel<0 ){
    sqlite3_free(p);
    *pp = 0;
  }
}

/*
** Generate a single line of output for the tree, with a prefix that contains
** all the appropriate tree lines
*/
SQLITE_PRIVATE void sqlite3TreeViewLine(TreeView *p, const char *zFormat, ...){
  va_list ap;
  int i;
  StrAccum acc;
  char zBuf[1000];
  sqlite3StrAccumInit(&acc, 0, zBuf, sizeof(zBuf), 0);
  if( p ){
    for(i=0; i<p->iLevel && i<(int)sizeof(p->bLine)-1; i++){
      sqlite3_str_append(&acc, p->bLine[i] ? "|   " : "    ", 4);
    }
    sqlite3_str_append(&acc, p->bLine[i] ? "|-- " : "'-- ", 4);
  }
  if( zFormat!=0 ){
    va_start(ap, zFormat);
    sqlite3_str_vappendf(&acc, zFormat, ap);
    va_end(ap);
    assert( acc.nChar>0 || acc.accError );
    sqlite3_str_append(&acc, "\n", 1);
  }
  sqlite3StrAccumFinish(&acc);
  fprintf(stdout,"%s", zBuf);
  fflush(stdout);
}

/*
** Shorthand for starting a new tree item that consists of a single label
*/
static void sqlite3TreeViewItem(TreeView *p, const char *zLabel,u8 moreFollows){
  sqlite3TreeViewPush(&p, moreFollows);
  sqlite3TreeViewLine(p, "%s", zLabel);
}

/*
** Show a list of Column objects in tree format.
*/
SQLITE_PRIVATE void sqlite3TreeViewColumnList(
  TreeView *pView,
  const Column *aCol,
  int nCol,
  u8 moreToFollow
){
  int i;
  sqlite3TreeViewPush(&pView, moreToFollow);
  sqlite3TreeViewLine(pView, "COLUMNS");
  for(i=0; i<nCol; i++){
    u16 flg = aCol[i].colFlags;
    int colMoreToFollow = i<(nCol - 1);
    sqlite3TreeViewPush(&pView, colMoreToFollow);
    sqlite3TreeViewLine(pView, 0);
    printf(" %s", aCol[i].zCnName);
    switch( aCol[i].eCType ){
      case COLTYPE_ANY:      printf(" ANY");        break;
      case COLTYPE_BLOB:     printf(" BLOB");       break;
      case COLTYPE_INT:      printf(" INT");        break;
      case COLTYPE_INTEGER:  printf(" INTEGER");    break;
      case COLTYPE_REAL:     printf(" REAL");       break;
      case COLTYPE_TEXT:     printf(" TEXT");       break;
      case COLTYPE_CUSTOM: {
        if( flg & COLFLAG_HASTYPE ){
          const char *z = aCol[i].zCnName;
          z += strlen(z)+1;
          printf(" X-%s", z);
          break;
        }
      }
    }
    if( flg & COLFLAG_PRIMKEY ) printf(" PRIMARY KEY");
    if( flg & COLFLAG_HIDDEN ) printf(" HIDDEN");
#ifdef COLFLAG_NOEXPAND
    if( flg & COLFLAG_NOEXPAND ) printf(" NO-EXPAND");
#endif
    if( flg ) printf(" flags=%04x", flg);
    printf("\n");
    fflush(stdout);
    sqlite3TreeViewPop(&pView);
  }
  sqlite3TreeViewPop(&pView);
}

/*
** Generate a human-readable description of a WITH clause.
*/
SQLITE_PRIVATE void sqlite3TreeViewWith(TreeView *pView, const With *pWith, u8 moreToFollow){
  int i;
  if( pWith==0 ) return;
  if( pWith->nCte==0 ) return;
  if( pWith->pOuter ){
    sqlite3TreeViewLine(pView, "WITH (0x%p, pOuter=0x%p)",pWith,pWith->pOuter);
  }else{
    sqlite3TreeViewLine(pView, "WITH (0x%p)", pWith);
  }
  if( pWith->nCte>0 ){
    sqlite3TreeViewPush(&pView, moreToFollow);
    for(i=0; i<pWith->nCte; i++){
      StrAccum x;
      char zLine[1000];
      const struct Cte *pCte = &pWith->a[i];
      sqlite3StrAccumInit(&x, 0, zLine, sizeof(zLine), 0);
      sqlite3_str_appendf(&x, "%s", pCte->zName);
      if( pCte->pCols && pCte->pCols->nExpr>0 ){
        char cSep = '(';
        int j;
        for(j=0; j<pCte->pCols->nExpr; j++){
          sqlite3_str_appendf(&x, "%c%s", cSep, pCte->pCols->a[j].zEName);
          cSep = ',';
        }
        sqlite3_str_appendf(&x, ")");
      }
      if( pCte->eM10d!=M10d_Any ){
        sqlite3_str_appendf(&x, " %sMATERIALIZED",
           pCte->eM10d==M10d_No ? "NOT " : "");
      }
      if( pCte->pUse ){
        sqlite3_str_appendf(&x, " (pUse=0x%p, nUse=%d)", pCte->pUse,
                 pCte->pUse->nUse);
      }
      sqlite3StrAccumFinish(&x);
      sqlite3TreeViewItem(pView, zLine, i<pWith->nCte-1);
      sqlite3TreeViewSelect(pView, pCte->pSelect, 0);
      sqlite3TreeViewPop(&pView);
    }
    sqlite3TreeViewPop(&pView);
  }
}

/*
** Generate a human-readable description of a SrcList object.
*/
SQLITE_PRIVATE void sqlite3TreeViewSrcList(TreeView *pView, const SrcList *pSrc){
  int i;
  if( pSrc==0 ) return;
  for(i=0; i<pSrc->nSrc; i++){
    const SrcItem *pItem = &pSrc->a[i];
    StrAccum x;
    int n = 0;
    char zLine[1000];
    sqlite3StrAccumInit(&x, 0, zLine, sizeof(zLine), 0);
    x.printfFlags |= SQLITE_PRINTF_INTERNAL;
    sqlite3_str_appendf(&x, "{%d:*} %!S", pItem->iCursor, pItem);
    if( pItem->pSTab ){
      sqlite3_str_appendf(&x, " tab=%Q nCol=%d ptr=%p used=%llx%s",
           pItem->pSTab->zName, pItem->pSTab->nCol, pItem->pSTab,
           pItem->colUsed,
           pItem->fg.rowidUsed ? "+rowid" : "");
    }
    if( (pItem->fg.jointype & (JT_LEFT|JT_RIGHT))==(JT_LEFT|JT_RIGHT) ){
      sqlite3_str_appendf(&x, " FULL-OUTER-JOIN");
    }else if( pItem->fg.jointype & JT_LEFT ){
      sqlite3_str_appendf(&x, " LEFT-JOIN");
    }else if( pItem->fg.jointype & JT_RIGHT ){
      sqlite3_str_appendf(&x, " RIGHT-JOIN");
    }else if( pItem->fg.jointype & JT_CROSS ){
      sqlite3_str_appendf(&x, " CROSS-JOIN");
    }
    if( pItem->fg.jointype & JT_LTORJ ){
      sqlite3_str_appendf(&x, " LTORJ");
    }
    if( pItem->fg.fromDDL ){
      sqlite3_str_appendf(&x, " DDL");
    }
    if( pItem->fg.isCte ){
      static const char *aMat[] = {",MAT", "", ",NO-MAT"};
      sqlite3_str_appendf(&x, " CteUse=%d%s",
                          pItem->u2.pCteUse->nUse,
                          aMat[pItem->u2.pCteUse->eM10d]);
    }
    if( pItem->fg.isOn || (pItem->fg.isUsing==0 && pItem->u3.pOn!=0) ){
      sqlite3_str_appendf(&x, " isOn");
    }
    if( pItem->fg.isTabFunc )      sqlite3_str_appendf(&x, " isTabFunc");
    if( pItem->fg.isCorrelated )   sqlite3_str_appendf(&x, " isCorrelated");
    if( pItem->fg.isMaterialized ) sqlite3_str_appendf(&x, " isMaterialized");
    if( pItem->fg.viaCoroutine )   sqlite3_str_appendf(&x, " viaCoroutine");
    if( pItem->fg.notCte )         sqlite3_str_appendf(&x, " notCte");
    if( pItem->fg.isNestedFrom )   sqlite3_str_appendf(&x, " isNestedFrom");
    if( pItem->fg.fixedSchema )    sqlite3_str_appendf(&x, " fixedSchema");
    if( pItem->fg.hadSchema )      sqlite3_str_appendf(&x, " hadSchema");
    if( pItem->fg.isSubquery )     sqlite3_str_appendf(&x, " isSubquery");

    sqlite3StrAccumFinish(&x);
    sqlite3TreeViewItem(pView, zLine, i<pSrc->nSrc-1);
    n = 0;
    if( pItem->fg.isSubquery ) n++;
    if( pItem->fg.isTabFunc ) n++;
    if( pItem->fg.isUsing || pItem->u3.pOn!=0 ) n++;
    if( pItem->fg.isUsing ){
      sqlite3TreeViewIdList(pView, pItem->u3.pUsing, (--n)>0, "USING");
    }else if( pItem->u3.pOn!=0 ){
      sqlite3TreeViewItem(pView, "ON", (--n)>0);
      sqlite3TreeViewExpr(pView, pItem->u3.pOn, 0);
      sqlite3TreeViewPop(&pView);
    }
    if( pItem->fg.isSubquery ){
      assert( n==1 );
      if( pItem->pSTab ){
        Table *pTab = pItem->pSTab;
        sqlite3TreeViewColumnList(pView, pTab->aCol, pTab->nCol, 1);
      }
      assert( (int)pItem->fg.isNestedFrom == IsNestedFrom(pItem) );
      sqlite3TreeViewSelect(pView, pItem->u4.pSubq->pSelect, 0);
    }
    if( pItem->fg.isTabFunc ){
      sqlite3TreeViewExprList(pView, pItem->u1.pFuncArg, 0, "func-args:");
    }
    sqlite3TreeViewPop(&pView);
  }
}

/*
** Generate a human-readable description of a Select object.
*/
SQLITE_PRIVATE void sqlite3TreeViewSelect(TreeView *pView, const Select *p, u8 moreToFollow){
  int n = 0;
  int cnt = 0;
  if( p==0 ){
    sqlite3TreeViewLine(pView, "nil-SELECT");
    return;
  }
  sqlite3TreeViewPush(&pView, moreToFollow);
  if( p->pWith ){
    sqlite3TreeViewWith(pView, p->pWith, 1);
    cnt = 1;
    sqlite3TreeViewPush(&pView, 1);
  }
  do{
    if( p->selFlags & SF_WhereBegin ){
      sqlite3TreeViewLine(pView, "sqlite3WhereBegin()");
    }else{
      sqlite3TreeViewLine(pView,
        "SELECT%s%s (%u/%p) selFlags=0x%x nSelectRow=%d",
        ((p->selFlags & SF_Distinct) ? " DISTINCT" : ""),
        ((p->selFlags & SF_Aggregate) ? " agg_flag" : ""),
        p->selId, p, p->selFlags,
        (int)p->nSelectRow
      );
    }
    if( cnt++ ) sqlite3TreeViewPop(&pView);
    if( p->pPrior ){
      n = 1000;
    }else{
      n = 0;
      if( p->pSrc && p->pSrc->nSrc && p->pSrc->nAlloc ) n++;
      if( p->pWhere ) n++;
      if( p->pGroupBy ) n++;
      if( p->pHaving ) n++;
      if( p->pOrderBy ) n++;
      if( p->pLimit ) n++;
#ifndef SQLITE_OMIT_WINDOWFUNC
      if( p->pWin ) n++;
      if( p->pWinDefn ) n++;
#endif
    }
    if( p->pEList ){
      sqlite3TreeViewExprList(pView, p->pEList, n>0, "result-set");
    }
    n--;
#ifndef SQLITE_OMIT_WINDOWFUNC
    if( p->pWin ){
      Window *pX;
      sqlite3TreeViewPush(&pView, (n--)>0);
      sqlite3TreeViewLine(pView, "window-functions");
      for(pX=p->pWin; pX; pX=pX->pNextWin){
        sqlite3TreeViewWinFunc(pView, pX, pX->pNextWin!=0);
      }
      sqlite3TreeViewPop(&pView);
    }
#endif
    if( p->pSrc && p->pSrc->nSrc && p->pSrc->nAlloc ){
      sqlite3TreeViewPush(&pView, (n--)>0);
      sqlite3TreeViewLine(pView, "FROM");
      sqlite3TreeViewSrcList(pView, p->pSrc);
      sqlite3TreeViewPop(&pView);
    }
    if( p->pWhere ){
      sqlite3TreeViewItem(pView, "WHERE", (n--)>0);
      sqlite3TreeViewExpr(pView, p->pWhere, 0);
      sqlite3TreeViewPop(&pView);
    }
    if( p->pGroupBy ){
      sqlite3TreeViewExprList(pView, p->pGroupBy, (n--)>0, "GROUPBY");
    }
    if( p->pHaving ){
      sqlite3TreeViewItem(pView, "HAVING", (n--)>0);
      sqlite3TreeViewExpr(pView, p->pHaving, 0);
      sqlite3TreeViewPop(&pView);
    }
#ifndef SQLITE_OMIT_WINDOWFUNC
    if( p->pWinDefn ){
      Window *pX;
      sqlite3TreeViewItem(pView, "WINDOW", (n--)>0);
      for(pX=p->pWinDefn; pX; pX=pX->pNextWin){
        sqlite3TreeViewWindow(pView, pX, pX->pNextWin!=0);
      }
      sqlite3TreeViewPop(&pView);
    }
#endif
    if( p->pOrderBy ){
      sqlite3TreeViewExprList(pView, p->pOrderBy, (n--)>0, "ORDERBY");
    }
    if( p->pLimit ){
      sqlite3TreeViewItem(pView, "LIMIT", (n--)>0);
      sqlite3TreeViewExpr(pView, p->pLimit->pLeft, p->pLimit->pRight!=0);
      if( p->pLimit->pRight ){
        sqlite3TreeViewItem(pView, "OFFSET", 0);
        sqlite3TreeViewExpr(pView, p->pLimit->pRight, 0);
        sqlite3TreeViewPop(&pView);
      }
      sqlite3TreeViewPop(&pView);
    }
    if( p->pPrior ){
      const char *zOp = "UNION";
      switch( p->op ){
        case TK_ALL:         zOp = "UNION ALL";  break;
        case TK_INTERSECT:   zOp = "INTERSECT";  break;
        case TK_EXCEPT:      zOp = "EXCEPT";     break;
      }
      sqlite3TreeViewItem(pView, zOp, 1);
    }
    p = p->pPrior;
  }while( p!=0 );
  sqlite3TreeViewPop(&pView);
}

#ifndef SQLITE_OMIT_WINDOWFUNC
/*
** Generate a description of starting or stopping bounds
*/
SQLITE_PRIVATE void sqlite3TreeViewBound(
  TreeView *pView,        /* View context */
  u8 eBound,              /* UNBOUNDED, CURRENT, PRECEDING, FOLLOWING */
  Expr *pExpr,            /* Value for PRECEDING or FOLLOWING */
  u8 moreToFollow         /* True if more to follow */
){
  switch( eBound ){
    case TK_UNBOUNDED: {
      sqlite3TreeViewItem(pView, "UNBOUNDED", moreToFollow);
      sqlite3TreeViewPop(&pView);
      break;
    }
    case TK_CURRENT: {
      sqlite3TreeViewItem(pView, "CURRENT", moreToFollow);
      sqlite3TreeViewPop(&pView);
      break;
    }
    case TK_PRECEDING: {
      sqlite3TreeViewItem(pView, "PRECEDING", moreToFollow);
      sqlite3TreeViewExpr(pView, pExpr, 0);
      sqlite3TreeViewPop(&pView);
      break;
    }
    case TK_FOLLOWING: {
      sqlite3TreeViewItem(pView, "FOLLOWING", moreToFollow);
      sqlite3TreeViewExpr(pView, pExpr, 0);
      sqlite3TreeViewPop(&pView);
      break;
    }
  }
}
#endif /* SQLITE_OMIT_WINDOWFUNC */

#ifndef SQLITE_OMIT_WINDOWFUNC
/*
** Generate a human-readable explanation for a Window object
*/
SQLITE_PRIVATE void sqlite3TreeViewWindow(TreeView *pView, const Window *pWin, u8 more){
  int nElement = 0;
  if( pWin==0 ) return;
  if( pWin->pFilter ){
    sqlite3TreeViewItem(pView, "FILTER", 1);
    sqlite3TreeViewExpr(pView, pWin->pFilter, 0);
    sqlite3TreeViewPop(&pView);
    if( pWin->eFrmType==TK_FILTER ) return;
  }
  sqlite3TreeViewPush(&pView, more);
  if( pWin->zName ){
    sqlite3TreeViewLine(pView, "OVER %s (%p)", pWin->zName, pWin);
  }else{
    sqlite3TreeViewLine(pView, "OVER (%p)", pWin);
  }
  if( pWin->zBase )    nElement++;
  if( pWin->pOrderBy ) nElement++;
  if( pWin->eFrmType!=0 && pWin->eFrmType!=TK_FILTER ) nElement++;
  if( pWin->eExclude ) nElement++;
  if( pWin->zBase ){
    sqlite3TreeViewPush(&pView, (--nElement)>0);
    sqlite3TreeViewLine(pView, "window: %s", pWin->zBase);
    sqlite3TreeViewPop(&pView);
  }
  if( pWin->pPartition ){
    sqlite3TreeViewExprList(pView, pWin->pPartition, nElement>0,"PARTITION-BY");
  }
  if( pWin->pOrderBy ){
    sqlite3TreeViewExprList(pView, pWin->pOrderBy, (--nElement)>0, "ORDER-BY");
  }
  if( pWin->eFrmType!=0 && pWin->eFrmType!=TK_FILTER ){
    char zBuf[30];
    const char *zFrmType = "ROWS";
    if( pWin->eFrmType==TK_RANGE ) zFrmType = "RANGE";
    if( pWin->eFrmType==TK_GROUPS ) zFrmType = "GROUPS";
    sqlite3_snprintf(sizeof(zBuf),zBuf,"%s%s",zFrmType,
        pWin->bImplicitFrame ? " (implied)" : "");
    sqlite3TreeViewItem(pView, zBuf, (--nElement)>0);
    sqlite3TreeViewBound(pView, pWin->eStart, pWin->pStart, 1);
    sqlite3TreeViewBound(pView, pWin->eEnd, pWin->pEnd, 0);
    sqlite3TreeViewPop(&pView);
  }
  if( pWin->eExclude ){
    char zBuf[30];
    const char *zExclude;
    switch( pWin->eExclude ){
      case TK_NO:      zExclude = "NO OTHERS";   break;
      case TK_CURRENT: zExclude = "CURRENT ROW"; break;
      case TK_GROUP:   zExclude = "GROUP";       break;
      case TK_TIES:    zExclude = "TIES";        break;
      default:
        sqlite3_snprintf(sizeof(zBuf),zBuf,"invalid(%d)", pWin->eExclude);
        zExclude = zBuf;
        break;
    }
    sqlite3TreeViewPush(&pView, 0);
    sqlite3TreeViewLine(pView, "EXCLUDE %s", zExclude);
    sqlite3TreeViewPop(&pView);
  }
  sqlite3TreeViewPop(&pView);
}
#endif /* SQLITE_OMIT_WINDOWFUNC */

#ifndef SQLITE_OMIT_WINDOWFUNC
/*
** Generate a human-readable explanation for a Window Function object
*/
SQLITE_PRIVATE void sqlite3TreeViewWinFunc(TreeView *pView, const Window *pWin, u8 more){
  if( pWin==0 ) return;
  sqlite3TreeViewPush(&pView, more);
  sqlite3TreeViewLine(pView, "WINFUNC %s(%d)",
                       pWin->pWFunc->zName, pWin->pWFunc->nArg);
  sqlite3TreeViewWindow(pView, pWin, 0);
  sqlite3TreeViewPop(&pView);
}
#endif /* SQLITE_OMIT_WINDOWFUNC */

/*
** Generate a human-readable explanation of an expression tree.
*/
SQLITE_PRIVATE void sqlite3TreeViewExpr(TreeView *pView, const Expr *pExpr, u8 moreToFollow){
  const char *zBinOp = 0;   /* Binary operator */
  const char *zUniOp = 0;   /* Unary operator */
  char zFlgs[200];
  sqlite3TreeViewPush(&pView, moreToFollow);
  if( pExpr==0 ){
    sqlite3TreeViewLine(pView, "nil");
    sqlite3TreeViewPop(&pView);
    return;
  }
  if( pExpr->flags || pExpr->affExpr || pExpr->vvaFlags || pExpr->pAggInfo ){
    StrAccum x;
    sqlite3StrAccumInit(&x, 0, zFlgs, sizeof(zFlgs), 0);
    sqlite3_str_appendf(&x, " fg.af=%x.%c",
      pExpr->flags, pExpr->affExpr ? pExpr->affExpr : 'n');
    if( ExprHasProperty(pExpr, EP_OuterON) ){
      sqlite3_str_appendf(&x, " outer.iJoin=%d", pExpr->w.iJoin);
    }
    if( ExprHasProperty(pExpr, EP_InnerON) ){
      sqlite3_str_appendf(&x, " inner.iJoin=%d", pExpr->w.iJoin);
    }
    if( ExprHasProperty(pExpr, EP_FromDDL) ){
      sqlite3_str_appendf(&x, " DDL");
    }
    if( ExprHasVVAProperty(pExpr, EP_Immutable) ){
      sqlite3_str_appendf(&x, " IMMUTABLE");
    }
    if( pExpr->pAggInfo!=0 ){
      sqlite3_str_appendf(&x, " agg-column[%d]", pExpr->iAgg);
    }
    sqlite3StrAccumFinish(&x);
  }else{
    zFlgs[0] = 0;
  }
  switch( pExpr->op ){
    case TK_AGG_COLUMN: {
      sqlite3TreeViewLine(pView, "AGG{%d:%d}%s",
            pExpr->iTable, pExpr->iColumn, zFlgs);
      break;
    }
    case TK_COLUMN: {
      if( pExpr->iTable<0 ){
        /* This only happens when coding check constraints */
        char zOp2[16];
        if( pExpr->op2 ){
          sqlite3_snprintf(sizeof(zOp2),zOp2," op2=0x%02x",pExpr->op2);
        }else{
          zOp2[0] = 0;
        }
        sqlite3TreeViewLine(pView, "COLUMN(%d)%s%s",
                                    pExpr->iColumn, zFlgs, zOp2);
      }else{
        assert( ExprUseYTab(pExpr) );
        sqlite3TreeViewLine(pView, "{%d:%d} pTab=%p%s",
                        pExpr->iTable, pExpr->iColumn,
                        pExpr->y.pTab, zFlgs);
      }
      if( ExprHasProperty(pExpr, EP_FixedCol) ){
        sqlite3TreeViewExpr(pView, pExpr->pLeft, 0);
      }
      break;
    }
    case TK_INTEGER: {
      if( pExpr->flags & EP_IntValue ){
        sqlite3TreeViewLine(pView, "%d", pExpr->u.iValue);
      }else{
        sqlite3TreeViewLine(pView, "%s", pExpr->u.zToken);
      }
      break;
    }
#ifndef SQLITE_OMIT_FLOATING_POINT
    case TK_FLOAT: {
      assert( !ExprHasProperty(pExpr, EP_IntValue) );
      sqlite3TreeViewLine(pView,"%s", pExpr->u.zToken);
      break;
    }
#endif
    case TK_STRING: {
      assert( !ExprHasProperty(pExpr, EP_IntValue) );
      sqlite3TreeViewLine(pView,"%Q", pExpr->u.zToken);
      break;
    }
    case TK_NULL: {
      sqlite3TreeViewLine(pView,"NULL");
      break;
    }
    case TK_TRUEFALSE: {
      sqlite3TreeViewLine(pView,"%s%s",
         sqlite3ExprTruthValue(pExpr) ? "TRUE" : "FALSE", zFlgs);
      break;
    }
#ifndef SQLITE_OMIT_BLOB_LITERAL
    case TK_BLOB: {
      assert( !ExprHasProperty(pExpr, EP_IntValue) );
      sqlite3TreeViewLine(pView,"%s", pExpr->u.zToken);
      break;
    }
#endif
    case TK_VARIABLE: {
      assert( !ExprHasProperty(pExpr, EP_IntValue) );
      sqlite3TreeViewLine(pView,"VARIABLE(%s,%d)",
                          pExpr->u.zToken, pExpr->iColumn);
      break;
    }
    case TK_REGISTER: {
      sqlite3TreeViewLine(pView,"REGISTER(%d)", pExpr->iTable);
      break;
    }
    case TK_ID: {
      assert( !ExprHasProperty(pExpr, EP_IntValue) );
      sqlite3TreeViewLine(pView,"ID \"%w\"", pExpr->u.zToken);
      break;
    }
    case TK_ASTERISK: {
      sqlite3TreeViewLine(pView,"ASKTERISK");
      break;
    }
#ifndef SQLITE_OMIT_CAST
    case TK_CAST: {
      /* Expressions of the form:   CAST(pLeft AS token) */
      assert( !ExprHasProperty(pExpr, EP_IntValue) );
      sqlite3TreeViewLine(pView,"CAST %Q", pExpr->u.zToken);
      sqlite3TreeViewExpr(pView, pExpr->pLeft, 0);
      break;
    }
#endif /* SQLITE_OMIT_CAST */
    case TK_LT:      zBinOp = "LT";     break;
    case TK_LE:      zBinOp = "LE";     break;
    case TK_GT:      zBinOp = "GT";     break;
    case TK_GE:      zBinOp = "GE";     break;
    case TK_NE:      zBinOp = "NE";     break;
    case TK_EQ:      zBinOp = "EQ";     break;
    case TK_IS:      zBinOp = "IS";     break;
    case TK_ISNOT:   zBinOp = "ISNOT";  break;
    case TK_AND:     zBinOp = "AND";    break;
    case TK_OR:      zBinOp = "OR";     break;
    case TK_PLUS:    zBinOp = "ADD";    break;
    case TK_STAR:    zBinOp = "MUL";    break;
    case TK_MINUS:   zBinOp = "SUB";    break;
    case TK_REM:     zBinOp = "REM";    break;
    case TK_BITAND:  zBinOp = "BITAND"; break;
    case TK_BITOR:   zBinOp = "BITOR";  break;
    case TK_SLASH:   zBinOp = "DIV";    break;
    case TK_LSHIFT:  zBinOp = "LSHIFT"; break;
    case TK_RSHIFT:  zBinOp = "RSHIFT"; break;
    case TK_CONCAT:  zBinOp = "CONCAT"; break;
    case TK_DOT:     zBinOp = "DOT";    break;
    case TK_LIMIT:   zBinOp = "LIMIT";  break;

    case TK_UMINUS:  zUniOp = "UMINUS"; break;
    case TK_UPLUS:   zUniOp = "UPLUS";  break;
    case TK_BITNOT:  zUniOp = "BITNOT"; break;
    case TK_NOT:     zUniOp = "NOT";    break;
    case TK_ISNULL:  zUniOp = "ISNULL"; break;
    case TK_NOTNULL: zUniOp = "NOTNULL"; break;

    case TK_TRUTH: {
      int x;
      const char *azOp[] = {
         "IS-FALSE", "IS-TRUE", "IS-NOT-FALSE", "IS-NOT-TRUE"
      };
      assert( pExpr->op2==TK_IS || pExpr->op2==TK_ISNOT );
      assert( pExpr->pRight );
      assert( sqlite3ExprSkipCollateAndLikely(pExpr->pRight)->op
                  == TK_TRUEFALSE );
      x = (pExpr->op2==TK_ISNOT)*2 + sqlite3ExprTruthValue(pExpr->pRight);
      zUniOp = azOp[x];
      break;
    }

    case TK_SPAN: {
      assert( !ExprHasProperty(pExpr, EP_IntValue) );
      sqlite3TreeViewLine(pView, "SPAN %Q", pExpr->u.zToken);
      sqlite3TreeViewExpr(pView, pExpr->pLeft, 0);
      break;
    }

    case TK_COLLATE: {
      /* COLLATE operators without the EP_Collate flag are intended to
      ** emulate collation associated with a table column.  These show
      ** up in the treeview output as "SOFT-COLLATE".  Explicit COLLATE
      ** operators that appear in the original SQL always have the
      ** EP_Collate bit set and appear in treeview output as just "COLLATE" */
      assert( !ExprHasProperty(pExpr, EP_IntValue) );
      sqlite3TreeViewLine(pView, "%sCOLLATE %Q%s",
        !ExprHasProperty(pExpr, EP_Collate) ? "SOFT-" : "",
        pExpr->u.zToken, zFlgs);
      sqlite3TreeViewExpr(pView, pExpr->pLeft, 0);
      break;
    }

    case TK_AGG_FUNCTION:
    case TK_FUNCTION: {
      ExprList *pFarg;       /* List of function arguments */
      Window *pWin;
      if( ExprHasProperty(pExpr, EP_TokenOnly) ){
        pFarg = 0;
        pWin = 0;
      }else{
        assert( ExprUseXList(pExpr) );
        pFarg = pExpr->x.pList;
#ifndef SQLITE_OMIT_WINDOWFUNC
        pWin = IsWindowFunc(pExpr) ? pExpr->y.pWin : 0;
#else
        pWin = 0;
#endif
      }
      assert( !ExprHasProperty(pExpr, EP_IntValue) );
      if( pExpr->op==TK_AGG_FUNCTION ){
        sqlite3TreeViewLine(pView, "AGG_FUNCTION%d %Q%s agg=%d[%d]/%p",
                             pExpr->op2, pExpr->u.zToken, zFlgs,
                             pExpr->pAggInfo ? pExpr->pAggInfo->selId : 0,
                             pExpr->iAgg, pExpr->pAggInfo);
      }else if( pExpr->op2!=0 ){
        const char *zOp2;
        char zBuf[8];
        sqlite3_snprintf(sizeof(zBuf),zBuf,"0x%02x",pExpr->op2);
        zOp2 = zBuf;
        if( pExpr->op2==NC_IsCheck ) zOp2 = "NC_IsCheck";
        if( pExpr->op2==NC_IdxExpr ) zOp2 = "NC_IdxExpr";
        if( pExpr->op2==NC_PartIdx ) zOp2 = "NC_PartIdx";
        if( pExpr->op2==NC_GenCol ) zOp2 = "NC_GenCol";
        sqlite3TreeViewLine(pView, "FUNCTION %Q%s op2=%s",
                            pExpr->u.zToken, zFlgs, zOp2);
      }else{
        sqlite3TreeViewLine(pView, "FUNCTION %Q%s", pExpr->u.zToken, zFlgs);
      }
      if( pFarg ){
        sqlite3TreeViewExprList(pView, pFarg, pWin!=0 || pExpr->pLeft, 0);
        if( pExpr->pLeft ){
          Expr *pOB = pExpr->pLeft;
          assert( pOB->op==TK_ORDER );
          assert( ExprUseXList(pOB) );
          sqlite3TreeViewExprList(pView, pOB->x.pList, pWin!=0, "ORDERBY");
        }
      }
#ifndef SQLITE_OMIT_WINDOWFUNC
      if( pWin ){
        sqlite3TreeViewWindow(pView, pWin, 0);
      }
#endif
      break;
    }
    case TK_ORDER: {
      sqlite3TreeViewExprList(pView, pExpr->x.pList, 0, "ORDERBY");
      break;
    }
#ifndef SQLITE_OMIT_SUBQUERY
    case TK_EXISTS: {
      assert( ExprUseXSelect(pExpr) );
      sqlite3TreeViewLine(pView, "EXISTS-expr flags=0x%x", pExpr->flags);
      sqlite3TreeViewSelect(pView, pExpr->x.pSelect, 0);
      break;
    }
    case TK_SELECT: {
      assert( ExprUseXSelect(pExpr) );
      sqlite3TreeViewLine(pView, "subquery-expr flags=0x%x", pExpr->flags);
      sqlite3TreeViewSelect(pView, pExpr->x.pSelect, 0);
      break;
    }
    case TK_IN: {
      sqlite3_str *pStr = sqlite3_str_new(0);
      char *z;
      sqlite3_str_appendf(pStr, "IN flags=0x%x", pExpr->flags);
      if( pExpr->iTable ) sqlite3_str_appendf(pStr, " iTable=%d",pExpr->iTable);
      if( ExprHasProperty(pExpr, EP_Subrtn) ){
        sqlite3_str_appendf(pStr, " subrtn(%d,%d)",
            pExpr->y.sub.regReturn, pExpr->y.sub.iAddr);
      }
      z = sqlite3_str_finish(pStr);
      sqlite3TreeViewLine(pView, z);
      sqlite3_free(z);
      sqlite3TreeViewExpr(pView, pExpr->pLeft, 1);
      if( ExprUseXSelect(pExpr) ){
        sqlite3TreeViewSelect(pView, pExpr->x.pSelect, 0);
      }else{
        sqlite3TreeViewExprList(pView, pExpr->x.pList, 0, 0);
      }
      break;
    }
#endif /* SQLITE_OMIT_SUBQUERY */

    /*
    **    x BETWEEN y AND z
    **
    ** This is equivalent to
    **
    **    x>=y AND x<=z
    **
    ** X is stored in pExpr->pLeft.
    ** Y is stored in pExpr->pList->a[0].pExpr.
    ** Z is stored in pExpr->pList->a[1].pExpr.
    */
    case TK_BETWEEN: {
      const Expr *pX, *pY, *pZ;
      pX = pExpr->pLeft;
      assert( ExprUseXList(pExpr) );
      assert( pExpr->x.pList->nExpr==2 );
      pY = pExpr->x.pList->a[0].pExpr;
      pZ = pExpr->x.pList->a[1].pExpr;
      sqlite3TreeViewLine(pView, "BETWEEN%s", zFlgs);
      sqlite3TreeViewExpr(pView, pX, 1);
      sqlite3TreeViewExpr(pView, pY, 1);
      sqlite3TreeViewExpr(pView, pZ, 0);
      break;
    }
    case TK_TRIGGER: {
      /* If the opcode is TK_TRIGGER, then the expression is a reference
      ** to a column in the new.* or old.* pseudo-tables available to
      ** trigger programs. In this case Expr.iTable is set to 1 for the
      ** new.* pseudo-table, or 0 for the old.* pseudo-table. Expr.iColumn
      ** is set to the column of the pseudo-table to read, or to -1 to
      ** read the rowid field.
      */
      sqlite3TreeViewLine(pView, "%s(%d)",
          pExpr->iTable ? "NEW" : "OLD", pExpr->iColumn);
      break;
    }
    case TK_CASE: {
      sqlite3TreeViewLine(pView, "CASE");
      sqlite3TreeViewExpr(pView, pExpr->pLeft, 1);
      assert( ExprUseXList(pExpr) );
      sqlite3TreeViewExprList(pView, pExpr->x.pList, 0, 0);
      break;
    }
#ifndef SQLITE_OMIT_TRIGGER
    case TK_RAISE: {
      const char *zType = "unk";
      switch( pExpr->affExpr ){
        case OE_Rollback:   zType = "rollback";  break;
        case OE_Abort:      zType = "abort";     break;
        case OE_Fail:       zType = "fail";      break;
        case OE_Ignore:     zType = "ignore";    break;
      }
      assert( !ExprHasProperty(pExpr, EP_IntValue) );
      sqlite3TreeViewLine(pView, "RAISE %s", zType);
      sqlite3TreeViewExpr(pView, pExpr->pLeft, 0);
      break;
    }
#endif
    case TK_MATCH: {
      sqlite3TreeViewLine(pView, "MATCH {%d:%d}%s",
                          pExpr->iTable, pExpr->iColumn, zFlgs);
      sqlite3TreeViewExpr(pView, pExpr->pRight, 0);
      break;
    }
    case TK_VECTOR: {
      char *z = sqlite3_mprintf("VECTOR%s",zFlgs);
      assert( ExprUseXList(pExpr) );
      sqlite3TreeViewBareExprList(pView, pExpr->x.pList, z);
      sqlite3_free(z);
      break;
    }
    case TK_SELECT_COLUMN: {
      sqlite3TreeViewLine(pView, "SELECT-COLUMN %d of [0..%d]%s",
              pExpr->iColumn, pExpr->iTable-1,
              pExpr->pRight==pExpr->pLeft ? " (SELECT-owner)" : "");
      assert( ExprUseXSelect(pExpr->pLeft) );
      sqlite3TreeViewSelect(pView, pExpr->pLeft->x.pSelect, 0);
      break;
    }
    case TK_IF_NULL_ROW: {
      sqlite3TreeViewLine(pView, "IF-NULL-ROW %d", pExpr->iTable);
      sqlite3TreeViewExpr(pView, pExpr->pLeft, 0);
      break;
    }
    case TK_ERROR: {
      Expr tmp;
      sqlite3TreeViewLine(pView, "ERROR");
      tmp = *pExpr;
      tmp.op = pExpr->op2;
      sqlite3TreeViewExpr(pView, &tmp, 0);
      break;
    }
    case TK_ROW: {
      if( pExpr->iColumn<=0 ){
        sqlite3TreeViewLine(pView, "First FROM table rowid");
      }else{
        sqlite3TreeViewLine(pView, "First FROM table column %d",
            pExpr->iColumn-1);
      }
      break;
    }
    default: {
      sqlite3TreeViewLine(pView, "op=%d", pExpr->op);
      break;
    }
  }
  if( zBinOp ){
    sqlite3TreeViewLine(pView, "%s%s", zBinOp, zFlgs);
    sqlite3TreeViewExpr(pView, pExpr->pLeft, 1);
    sqlite3TreeViewExpr(pView, pExpr->pRight, 0);
  }else if( zUniOp ){
    sqlite3TreeViewLine(pView, "%s%s", zUniOp, zFlgs);
   sqlite3TreeViewExpr(pView, pExpr->pLeft, 0);
  }
  sqlite3TreeViewPop(&pView);
}


/*
** Generate a human-readable explanation of an expression list.
*/
SQLITE_PRIVATE void sqlite3TreeViewBareExprList(
  TreeView *pView,
  const ExprList *pList,
  const char *zLabel
){
  if( zLabel==0 || zLabel[0]==0 ) zLabel = "LIST";
  if( pList==0 ){
    sqlite3TreeViewLine(pView, "%s (empty)", zLabel);
  }else{
    int i;
    sqlite3TreeViewLine(pView, "%s", zLabel);
    for(i=0; i<pList->nExpr; i++){
      int j = pList->a[i].u.x.iOrderByCol;
      u8 sortFlags = pList->a[i].fg.sortFlags;
      char *zName = pList->a[i].zEName;
      int moreToFollow = i<pList->nExpr - 1;
      if( j || zName || sortFlags ){
        sqlite3TreeViewPush(&pView, moreToFollow);
        moreToFollow = 0;
        sqlite3TreeViewLine(pView, 0);
        if( zName ){
          switch( pList->a[i].fg.eEName ){
            default:
              fprintf(stdout, "AS %s ", zName);
              break;
            case ENAME_TAB:
              fprintf(stdout, "TABLE-ALIAS-NAME(\"%s\") ", zName);
              if( pList->a[i].fg.bUsed ) fprintf(stdout, "(used) ");
              if( pList->a[i].fg.bUsingTerm ) fprintf(stdout, "(USING-term) ");
              if( pList->a[i].fg.bNoExpand ) fprintf(stdout, "(NoExpand) ");
              break;
            case ENAME_SPAN:
              fprintf(stdout, "SPAN(\"%s\") ", zName);
              break;
          }
        }
        if( j ){
          fprintf(stdout, "iOrderByCol=%d ", j);
        }
        if( sortFlags & KEYINFO_ORDER_DESC ){
          fprintf(stdout, "DESC ");
        }else if( sortFlags & KEYINFO_ORDER_BIGNULL ){
          fprintf(stdout, "NULLS-LAST");
        }
        fprintf(stdout, "\n");
        fflush(stdout);
      }
      sqlite3TreeViewExpr(pView, pList->a[i].pExpr, moreToFollow);
      if( j || zName || sortFlags ){
        sqlite3TreeViewPop(&pView);
      }
    }
  }
}
SQLITE_PRIVATE void sqlite3TreeViewExprList(
  TreeView *pView,
  const ExprList *pList,
  u8 moreToFollow,
  const char *zLabel
){
  sqlite3TreeViewPush(&pView, moreToFollow);
  sqlite3TreeViewBareExprList(pView, pList, zLabel);
  sqlite3TreeViewPop(&pView);
}

/*
** Generate a human-readable explanation of an id-list.
*/
SQLITE_PRIVATE void sqlite3TreeViewBareIdList(
  TreeView *pView,
  const IdList *pList,
  const char *zLabel
){
  if( zLabel==0 || zLabel[0]==0 ) zLabel = "LIST";
  if( pList==0 ){
    sqlite3TreeViewLine(pView, "%s (empty)", zLabel);
  }else{
    int i;
    sqlite3TreeViewLine(pView, "%s", zLabel);
    for(i=0; i<pList->nId; i++){
      char *zName = pList->a[i].zName;
      int moreToFollow = i<pList->nId - 1;
      if( zName==0 ) zName = "(null)";
      sqlite3TreeViewPush(&pView, moreToFollow);
      sqlite3TreeViewLine(pView, 0);
      fprintf(stdout, "%s\n", zName);
      sqlite3TreeViewPop(&pView);
    }
  }
}
SQLITE_PRIVATE void sqlite3TreeViewIdList(
  TreeView *pView,
  const IdList *pList,
  u8 moreToFollow,
  const char *zLabel
){
  sqlite3TreeViewPush(&pView, moreToFollow);
  sqlite3TreeViewBareIdList(pView, pList, zLabel);
  sqlite3TreeViewPop(&pView);
}

/*
** Generate a human-readable explanation of a list of Upsert objects
*/
SQLITE_PRIVATE void sqlite3TreeViewUpsert(
  TreeView *pView,
  const Upsert *pUpsert,
  u8 moreToFollow
){
  if( pUpsert==0 ) return;
  sqlite3TreeViewPush(&pView, moreToFollow);
  while( pUpsert ){
    int n;
    sqlite3TreeViewPush(&pView, pUpsert->pNextUpsert!=0 || moreToFollow);
    sqlite3TreeViewLine(pView, "ON CONFLICT DO %s",
         pUpsert->isDoUpdate ? "UPDATE" : "NOTHING");
    n = (pUpsert->pUpsertSet!=0) + (pUpsert->pUpsertWhere!=0);
    sqlite3TreeViewExprList(pView, pUpsert->pUpsertTarget, (n--)>0, "TARGET");
    sqlite3TreeViewExprList(pView, pUpsert->pUpsertSet, (n--)>0, "SET");
    if( pUpsert->pUpsertWhere ){
      sqlite3TreeViewItem(pView, "WHERE", (n--)>0);
      sqlite3TreeViewExpr(pView, pUpsert->pUpsertWhere, 0);
      sqlite3TreeViewPop(&pView);
    }
    sqlite3TreeViewPop(&pView);
    pUpsert = pUpsert->pNextUpsert;
  }
  sqlite3TreeViewPop(&pView);
}

#if TREETRACE_ENABLED
/*
** Generate a human-readable diagram of the data structure that go
** into generating an DELETE statement.
*/
SQLITE_PRIVATE void sqlite3TreeViewDelete(
  const With *pWith,
  const SrcList *pTabList,
  const Expr *pWhere,
  const ExprList *pOrderBy,
  const Expr *pLimit,
  const Trigger *pTrigger
){
  int n = 0;
  TreeView *pView = 0;
  sqlite3TreeViewPush(&pView, 0);
  sqlite3TreeViewLine(pView, "DELETE");
  if( pWith ) n++;
  if( pTabList ) n++;
  if( pWhere ) n++;
  if( pOrderBy ) n++;
  if( pLimit ) n++;
  if( pTrigger ) n++;
  if( pWith ){
    sqlite3TreeViewPush(&pView, (--n)>0);
    sqlite3TreeViewWith(pView, pWith, 0);
    sqlite3TreeViewPop(&pView);
  }
  if( pTabList ){
    sqlite3TreeViewPush(&pView, (--n)>0);
    sqlite3TreeViewLine(pView, "FROM");
    sqlite3TreeViewSrcList(pView, pTabList);
    sqlite3TreeViewPop(&pView);
  }
  if( pWhere ){
    sqlite3TreeViewPush(&pView, (--n)>0);
    sqlite3TreeViewLine(pView, "WHERE");
    sqlite3TreeViewExpr(pView, pWhere, 0);
    sqlite3TreeViewPop(&pView);
  }
  if( pOrderBy ){
    sqlite3TreeViewExprList(pView, pOrderBy, (--n)>0, "ORDER-BY");
  }
  if( pLimit ){
    sqlite3TreeViewPush(&pView, (--n)>0);
    sqlite3TreeViewLine(pView, "LIMIT");
    sqlite3TreeViewExpr(pView, pLimit, 0);
    sqlite3TreeViewPop(&pView);
  }
  if( pTrigger ){
    sqlite3TreeViewTrigger(pView, pTrigger, (--n)>0, 1);
  }
  sqlite3TreeViewPop(&pView);
}
#endif /* TREETRACE_ENABLED */

#if TREETRACE_ENABLED
/*
** Generate a human-readable diagram of the data structure that go
** into generating an INSERT statement.
*/
SQLITE_PRIVATE void sqlite3TreeViewInsert(
  const With *pWith,
  const SrcList *pTabList,
  const IdList *pColumnList,
  const Select *pSelect,
  const ExprList *pExprList,
  int onError,
  const Upsert *pUpsert,
  const Trigger *pTrigger
){
  TreeView *pView = 0;
  int n = 0;
  const char *zLabel = "INSERT";
  switch( onError ){
    case OE_Replace:  zLabel = "REPLACE";             break;
    case OE_Ignore:   zLabel = "INSERT OR IGNORE";    break;
    case OE_Rollback: zLabel = "INSERT OR ROLLBACK";  break;
    case OE_Abort:    zLabel = "INSERT OR ABORT";     break;
    case OE_Fail:     zLabel = "INSERT OR FAIL";      break;
  }
  sqlite3TreeViewPush(&pView, 0);
  sqlite3TreeViewLine(pView, zLabel);
  if( pWith ) n++;
  if( pTabList ) n++;
  if( pColumnList ) n++;
  if( pSelect ) n++;
  if( pExprList ) n++;
  if( pUpsert ) n++;
  if( pTrigger ) n++;
  if( pWith ){
    sqlite3TreeViewPush(&pView, (--n)>0);
    sqlite3TreeViewWith(pView, pWith, 0);
    sqlite3TreeViewPop(&pView);
  }
  if( pTabList ){
    sqlite3TreeViewPush(&pView, (--n)>0);
    sqlite3TreeViewLine(pView, "INTO");
    sqlite3TreeViewSrcList(pView, pTabList);
    sqlite3TreeViewPop(&pView);
  }
  if( pColumnList ){
    sqlite3TreeViewIdList(pView, pColumnList, (--n)>0, "COLUMNS");
  }
  if( pSelect ){
    sqlite3TreeViewPush(&pView, (--n)>0);
    sqlite3TreeViewLine(pView, "DATA-SOURCE");
    sqlite3TreeViewSelect(pView, pSelect, 0);
    sqlite3TreeViewPop(&pView);
  }
  if( pExprList ){
    sqlite3TreeViewExprList(pView, pExprList, (--n)>0, "VALUES");
  }
  if( pUpsert ){
    sqlite3TreeViewPush(&pView, (--n)>0);
    sqlite3TreeViewLine(pView, "UPSERT");
    sqlite3TreeViewUpsert(pView, pUpsert, 0);
    sqlite3TreeViewPop(&pView);
  }
  if( pTrigger ){
    sqlite3TreeViewTrigger(pView, pTrigger, (--n)>0, 1);
  }
  sqlite3TreeViewPop(&pView);
}
#endif /* TREETRACE_ENABLED */

#if TREETRACE_ENABLED
/*
** Generate a human-readable diagram of the data structure that go
** into generating an UPDATE statement.
*/
SQLITE_PRIVATE void sqlite3TreeViewUpdate(
  const With *pWith,
  const SrcList *pTabList,
  const ExprList *pChanges,
  const Expr *pWhere,
  int onError,
  const ExprList *pOrderBy,
  const Expr *pLimit,
  const Upsert *pUpsert,
  const Trigger *pTrigger
){
  int n = 0;
  TreeView *pView = 0;
  const char *zLabel = "UPDATE";
  switch( onError ){
    case OE_Replace:  zLabel = "UPDATE OR REPLACE";   break;
    case OE_Ignore:   zLabel = "UPDATE OR IGNORE";    break;
    case OE_Rollback: zLabel = "UPDATE OR ROLLBACK";  break;
    case OE_Abort:    zLabel = "UPDATE OR ABORT";     break;
    case OE_Fail:     zLabel = "UPDATE OR FAIL";      break;
  }
  sqlite3TreeViewPush(&pView, 0);
  sqlite3TreeViewLine(pView, zLabel);
  if( pWith ) n++;
  if( pTabList ) n++;
  if( pChanges ) n++;
  if( pWhere ) n++;
  if( pOrderBy ) n++;
  if( pLimit ) n++;
  if( pUpsert ) n++;
  if( pTrigger ) n++;
  if( pWith ){
    sqlite3TreeViewPush(&pView, (--n)>0);
    sqlite3TreeViewWith(pView, pWith, 0);
    sqlite3TreeViewPop(&pView);
  }
  if( pTabList ){
    sqlite3TreeViewPush(&pView, (--n)>0);
    sqlite3TreeViewLine(pView, "FROM");
    sqlite3TreeViewSrcList(pView, pTabList);
    sqlite3TreeViewPop(&pView);
  }
  if( pChanges ){
    sqlite3TreeViewExprList(pView, pChanges, (--n)>0, "SET");
  }
  if( pWhere ){
    sqlite3TreeViewPush(&pView, (--n)>0);
    sqlite3TreeViewLine(pView, "WHERE");
    sqlite3TreeViewExpr(pView, pWhere, 0);
    sqlite3TreeViewPop(&pView);
  }
  if( pOrderBy ){
    sqlite3TreeViewExprList(pView, pOrderBy, (--n)>0, "ORDER-BY");
  }
  if( pLimit ){
    sqlite3TreeViewPush(&pView, (--n)>0);
    sqlite3TreeViewLine(pView, "LIMIT");
    sqlite3TreeViewExpr(pView, pLimit, 0);
    sqlite3TreeViewPop(&pView);
  }
  if( pUpsert ){
    sqlite3TreeViewPush(&pView, (--n)>0);
    sqlite3TreeViewLine(pView, "UPSERT");
    sqlite3TreeViewUpsert(pView, pUpsert, 0);
    sqlite3TreeViewPop(&pView);
  }
  if( pTrigger ){
    sqlite3TreeViewTrigger(pView, pTrigger, (--n)>0, 1);
  }
  sqlite3TreeViewPop(&pView);
}
#endif /* TREETRACE_ENABLED */

#ifndef SQLITE_OMIT_TRIGGER
/*
** Show a human-readable graph of a TriggerStep
*/
SQLITE_PRIVATE void sqlite3TreeViewTriggerStep(
  TreeView *pView,
  const TriggerStep *pStep,
  u8 moreToFollow,
  u8 showFullList
){
  int cnt = 0;
  if( pStep==0 ) return;
  sqlite3TreeViewPush(&pView,
      moreToFollow || (showFullList && pStep->pNext!=0));
  do{
    if( cnt++ && pStep->pNext==0 ){
      sqlite3TreeViewPop(&pView);
      sqlite3TreeViewPush(&pView, 0);
    }
    sqlite3TreeViewLine(pView, "%s", pStep->zSpan ? pStep->zSpan : "RETURNING");
  }while( showFullList && (pStep = pStep->pNext)!=0 );
  sqlite3TreeViewPop(&pView);
}

/*
** Show a human-readable graph of a Trigger
*/
SQLITE_PRIVATE void sqlite3TreeViewTrigger(
  TreeView *pView,
  const Trigger *pTrigger,
  u8 moreToFollow,
  u8 showFullList
){
  int cnt = 0;
  if( pTrigger==0 ) return;
  sqlite3TreeViewPush(&pView,
     moreToFollow || (showFullList && pTrigger->pNext!=0));
  do{
    if( cnt++ && pTrigger->pNext==0 ){
      sqlite3TreeViewPop(&pView);
      sqlite3TreeViewPush(&pView, 0);
    }
    sqlite3TreeViewLine(pView, "TRIGGER %s", pTrigger->zName);
    sqlite3TreeViewPush(&pView, 0);
    sqlite3TreeViewTriggerStep(pView, pTrigger->step_list, 0, 1);
    sqlite3TreeViewPop(&pView);
  }while( showFullList && (pTrigger = pTrigger->pNext)!=0 );
  sqlite3TreeViewPop(&pView);
}
#endif /* SQLITE_OMIT_TRIGGER */


/*
** These simplified versions of the tree-view routines omit unnecessary
** parameters.  These variants are intended to be used from a symbolic
** debugger, such as "gdb", during interactive debugging sessions.
**
** This routines are given external linkage so that they will always be
** accessible to the debugging, and to avoid warnings about unused
** functions.  But these routines only exist in debugging builds, so they
** do not contaminate the interface.
**
** See Also:
**
**     sqlite3ShowWhereTerm() in where.c
*/
SQLITE_PRIVATE void sqlite3ShowExpr(const Expr *p){ sqlite3TreeViewExpr(0,p,0); }
SQLITE_PRIVATE void sqlite3ShowExprList(const ExprList *p){ sqlite3TreeViewExprList(0,p,0,0);}
SQLITE_PRIVATE void sqlite3ShowIdList(const IdList *p){ sqlite3TreeViewIdList(0,p,0,0); }
SQLITE_PRIVATE void sqlite3ShowSrcList(const SrcList *p){
  TreeView *pView = 0;
  sqlite3TreeViewPush(&pView, 0);
  sqlite3TreeViewLine(pView, "SRCLIST");
  sqlite3TreeViewSrcList(pView,p);
  sqlite3TreeViewPop(&pView);
}
SQLITE_PRIVATE void sqlite3ShowSelect(const Select *p){ sqlite3TreeViewSelect(0,p,0); }
SQLITE_PRIVATE void sqlite3ShowWith(const With *p){ sqlite3TreeViewWith(0,p,0); }
SQLITE_PRIVATE void sqlite3ShowUpsert(const Upsert *p){ sqlite3TreeViewUpsert(0,p,0); }
#ifndef SQLITE_OMIT_TRIGGER
SQLITE_PRIVATE void sqlite3ShowTriggerStep(const TriggerStep *p){
  sqlite3TreeViewTriggerStep(0,p,0,0);
}
SQLITE_PRIVATE void sqlite3ShowTriggerStepList(const TriggerStep *p){
  sqlite3TreeViewTriggerStep(0,p,0,1);
}
SQLITE_PRIVATE void sqlite3ShowTrigger(const Trigger *p){ sqlite3TreeViewTrigger(0,p,0,0); }
SQLITE_PRIVATE void sqlite3ShowTriggerList(const Trigger *p){ sqlite3TreeViewTrigger(0,p,0,1);}
#endif
#ifndef SQLITE_OMIT_WINDOWFUNC
SQLITE_PRIVATE void sqlite3ShowWindow(const Window *p){ sqlite3TreeViewWindow(0,p,0); }
SQLITE_PRIVATE void sqlite3ShowWinFunc(const Window *p){ sqlite3TreeViewWinFunc(0,p,0); }
#endif

#endif /* SQLITE_DEBUG */

/************** End of treeview.c ********************************************/
/************** Begin file random.c ******************************************/
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
** This file contains code to implement a pseudo-random number
** generator (PRNG) for SQLite.
**
** Random numbers are used by some of the database backends in order
** to generate random integer keys for tables or random filenames.
*/
/* #include "sqliteInt.h" */


/* All threads share a single random number generator.
** This structure is the current state of the generator.
*/
static SQLITE_WSD struct sqlite3PrngType {
  u32 s[16];                 /* 64 bytes of chacha20 state */
  u8 out[64];                /* Output bytes */
  u8 n;                      /* Output bytes remaining */
} sqlite3Prng;


/* The RFC-7539 ChaCha20 block function
*/
#define ROTL(a,b) (((a) << (b)) | ((a) >> (32 - (b))))
#define QR(a, b, c, d) ( \
    a += b, d ^= a, d = ROTL(d,16), \
    c += d, b ^= c, b = ROTL(b,12), \
    a += b, d ^= a, d = ROTL(d, 8), \
    c += d, b ^= c, b = ROTL(b, 7))
static void chacha_block(u32 *out, const u32 *in){
  int i;
  u32 x[16];
  memcpy(x, in, 64);
  for(i=0; i<10; i++){
    QR(x[0], x[4], x[ 8], x[12]);
    QR(x[1], x[5], x[ 9], x[13]);
    QR(x[2], x[6], x[10], x[14]);
    QR(x[3], x[7], x[11], x[15]);
    QR(x[0], x[5], x[10], x[15]);
    QR(x[1], x[6], x[11], x[12]);
    QR(x[2], x[7], x[ 8], x[13]);
    QR(x[3], x[4], x[ 9], x[14]);
  }
  for(i=0; i<16; i++) out[i] = x[i]+in[i];
}

/*
** Return N random bytes.
*/
SQLITE_API void sqlite3_randomness(int N, void *pBuf){
  unsigned char *zBuf = pBuf;

  /* The "wsdPrng" macro will resolve to the pseudo-random number generator
  ** state vector.  If writable static data is unsupported on the target,
  ** we have to locate the state vector at run-time.  In the more common
  ** case where writable static data is supported, wsdPrng can refer directly
  ** to the "sqlite3Prng" state vector declared above.
  */
#ifdef SQLITE_OMIT_WSD
  struct sqlite3PrngType *p = &GLOBAL(struct sqlite3PrngType, sqlite3Prng);
# define wsdPrng p[0]
#else
# define wsdPrng sqlite3Prng
#endif

#if SQLITE_THREADSAFE
  sqlite3_mutex *mutex;
#endif

#ifndef SQLITE_OMIT_AUTOINIT
  if( sqlite3_initialize() ) return;
#endif

#if SQLITE_THREADSAFE
  mutex = sqlite3MutexAlloc(SQLITE_MUTEX_STATIC_PRNG);
#endif

  sqlite3_mutex_enter(mutex);
  if( N<=0 || pBuf==0 ){
    wsdPrng.s[0] = 0;
    sqlite3_mutex_leave(mutex);
    return;
  }

  /* Initialize the state of the random number generator once,
  ** the first time this routine is called.
  */
  if( wsdPrng.s[0]==0 ){
    sqlite3_vfs *pVfs = sqlite3_vfs_find(0);
    static const u32 chacha20_init[] = {
      0x61707865, 0x3320646e, 0x79622d32, 0x6b206574
    };
    memcpy(&wsdPrng.s[0], chacha20_init, 16);
    if( NEVER(pVfs==0) ){
      memset(&wsdPrng.s[4], 0, 44);
    }else{
      sqlite3OsRandomness(pVfs, 44, (char*)&wsdPrng.s[4]);
    }
    wsdPrng.s[15] = wsdPrng.s[12];
    wsdPrng.s[12] = 0;
    wsdPrng.n = 0;
  }

  assert( N>0 );
  while( 1 /* exit by break */ ){
    if( N<=wsdPrng.n ){
      memcpy(zBuf, &wsdPrng.out[wsdPrng.n-N], N);
      wsdPrng.n -= N;
      break;
    }
    if( wsdPrng.n>0 ){
      memcpy(zBuf, wsdPrng.out, wsdPrng.n);
      N -= wsdPrng.n;
      zBuf += wsdPrng.n;
    }
    wsdPrng.s[12]++;
    chacha_block((u32*)wsdPrng.out, wsdPrng.s);
    wsdPrng.n = 64;
  }
  sqlite3_mutex_leave(mutex);
}

#ifndef SQLITE_UNTESTABLE
/*
** For testing purposes, we sometimes want to preserve the state of
** PRNG and restore the PRNG to its saved state at a later time, or
** to reset the PRNG to its initial state.  These routines accomplish
** those tasks.
**
** The sqlite3_test_control() interface calls these routines to
** control the PRNG.
*/
static SQLITE_WSD struct sqlite3PrngType sqlite3SavedPrng;
SQLITE_PRIVATE void sqlite3PrngSaveState(void){
  memcpy(
    &GLOBAL(struct sqlite3PrngType, sqlite3SavedPrng),
    &GLOBAL(struct sqlite3PrngType, sqlite3Prng),
    sizeof(sqlite3Prng)
  );
}
SQLITE_PRIVATE void sqlite3PrngRestoreState(void){
  memcpy(
    &GLOBAL(struct sqlite3PrngType, sqlite3Prng),
    &GLOBAL(struct sqlite3PrngType, sqlite3SavedPrng),
    sizeof(sqlite3Prng)
  );
}
#endif /* SQLITE_UNTESTABLE */

/************** End of random.c **********************************************/
/************** Begin file threads.c *****************************************/
/*
** 2012 July 21
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
******************************************************************************
**
** This file presents a simple cross-platform threading interface for
** use internally by SQLite.
**
** A "thread" can be created using sqlite3ThreadCreate().  This thread
** runs independently of its creator until it is joined using
** sqlite3ThreadJoin(), at which point it terminates.
**
** Threads do not have to be real.  It could be that the work of the
** "thread" is done by the main thread at either the sqlite3ThreadCreate()
** or sqlite3ThreadJoin() call.  This is, in fact, what happens in
** single threaded systems.  Nothing in SQLite requires multiple threads.
** This interface exists so that applications that want to take advantage
** of multiple cores can do so, while also allowing applications to stay
** single-threaded if desired.
*/
/* #include "sqliteInt.h" */
#if SQLITE_OS_WIN
/* #  include "os_win.h" */
#endif

#if SQLITE_MAX_WORKER_THREADS>0

/********************************* Unix Pthreads ****************************/
#if SQLITE_OS_UNIX && defined(SQLITE_MUTEX_PTHREADS) && SQLITE_THREADSAFE>0

#define SQLITE_THREADS_IMPLEMENTED 1  /* Prevent the single-thread code below */
/* #include <pthread.h> */

/* A running thread */
struct SQLiteThread {
  pthread_t tid;                 /* Thread ID */
  int done;                      /* Set to true when thread finishes */
  void *pOut;                    /* Result returned by the thread */
  void *(*xTask)(void*);         /* The thread routine */
  void *pIn;                     /* Argument to the thread */
};

/* Create a new thread */
SQLITE_PRIVATE int sqlite3ThreadCreate(
  SQLiteThread **ppThread,  /* OUT: Write the thread object here */
  void *(*xTask)(void*),    /* Routine to run in a separate thread */
  void *pIn                 /* Argument passed into xTask() */
){
  SQLiteThread *p;
  int rc;

  assert( ppThread!=0 );
  assert( xTask!=0 );
  /* This routine is never used in single-threaded mode */
  assert( sqlite3GlobalConfig.bCoreMutex!=0 );

  *ppThread = 0;
  p = sqlite3Malloc(sizeof(*p));
  if( p==0 ) return SQLITE_NOMEM_BKPT;
  memset(p, 0, sizeof(*p));
  p->xTask = xTask;
  p->pIn = pIn;
  /* If the SQLITE_TESTCTRL_FAULT_INSTALL callback is registered to a
  ** function that returns SQLITE_ERROR when passed the argument 200, that
  ** forces worker threads to run sequentially and deterministically
  ** for testing purposes. */
  if( sqlite3FaultSim(200) ){
    rc = 1;
  }else{
    rc = pthread_create(&p->tid, 0, xTask, pIn);
  }
  if( rc ){
    p->done = 1;
    p->pOut = xTask(pIn);
  }
  *ppThread = p;
  return SQLITE_OK;
}

/* Get the results of the thread */
SQLITE_PRIVATE int sqlite3ThreadJoin(SQLiteThread *p, void **ppOut){
  int rc;

  assert( ppOut!=0 );
  if( NEVER(p==0) ) return SQLITE_NOMEM_BKPT;
  if( p->done ){
    *ppOut = p->pOut;
    rc = SQLITE_OK;
  }else{
    rc = pthread_join(p->tid, ppOut) ? SQLITE_ERROR : SQLITE_OK;
  }
  sqlite3_free(p);
  return rc;
}

#endif /* SQLITE_OS_UNIX && defined(SQLITE_MUTEX_PTHREADS) */
/******************************** End Unix Pthreads *************************/


/********************************* Win32 Threads ****************************/
#if SQLITE_OS_WIN_THREADS

#define SQLITE_THREADS_IMPLEMENTED 1  /* Prevent the single-thread code below */
#include <process.h>

/* A running thread */
struct SQLiteThread {
  void *tid;               /* The thread handle */
  unsigned id;             /* The thread identifier */
  void *(*xTask)(void*);   /* The routine to run as a thread */
  void *pIn;               /* Argument to xTask */
  void *pResult;           /* Result of xTask */
};

/* Thread procedure Win32 compatibility shim */
static unsigned __stdcall sqlite3ThreadProc(
  void *pArg  /* IN: Pointer to the SQLiteThread structure */
){
  SQLiteThread *p = (SQLiteThread *)pArg;

  assert( p!=0 );
#if 0
  /*
  ** This assert appears to trigger spuriously on certain
  ** versions of Windows, possibly due to _beginthreadex()
  ** and/or CreateThread() not fully setting their thread
  ** ID parameter before starting the thread.
  */
  assert( p->id==GetCurrentThreadId() );
#endif
  assert( p->xTask!=0 );
  p->pResult = p->xTask(p->pIn);

  _endthreadex(0);
  return 0; /* NOT REACHED */
}

/* Create a new thread */
SQLITE_PRIVATE int sqlite3ThreadCreate(
  SQLiteThread **ppThread,  /* OUT: Write the thread object here */
  void *(*xTask)(void*),    /* Routine to run in a separate thread */
  void *pIn                 /* Argument passed into xTask() */
){
  SQLiteThread *p;

  assert( ppThread!=0 );
  assert( xTask!=0 );
  *ppThread = 0;
  p = sqlite3Malloc(sizeof(*p));
  if( p==0 ) return SQLITE_NOMEM_BKPT;
  /* If the SQLITE_TESTCTRL_FAULT_INSTALL callback is registered to a
  ** function that returns SQLITE_ERROR when passed the argument 200, that
  ** forces worker threads to run sequentially and deterministically
  ** (via the sqlite3FaultSim() term of the conditional) for testing
  ** purposes. */
  if( sqlite3GlobalConfig.bCoreMutex==0 || sqlite3FaultSim(200) ){
    memset(p, 0, sizeof(*p));
  }else{
    p->xTask = xTask;
    p->pIn = pIn;
    p->tid = (void*)_beginthreadex(0, 0, sqlite3ThreadProc, p, 0, &p->id);
    if( p->tid==0 ){
      memset(p, 0, sizeof(*p));
    }
  }
  if( p->xTask==0 ){
    p->id = GetCurrentThreadId();
    p->pResult = xTask(pIn);
  }
  *ppThread = p;
  return SQLITE_OK;
}

SQLITE_PRIVATE DWORD sqlite3Win32Wait(HANDLE hObject); /* os_win.c */

/* Get the results of the thread */
SQLITE_PRIVATE int sqlite3ThreadJoin(SQLiteThread *p, void **ppOut){
  DWORD rc;
  BOOL bRc;

  assert( ppOut!=0 );
  if( NEVER(p==0) ) return SQLITE_NOMEM_BKPT;
  if( p->xTask==0 ){
    /* assert( p->id==GetCurrentThreadId() ); */
    rc = WAIT_OBJECT_0;
    assert( p->tid==0 );
  }else{
    assert( p->id!=0 && p->id!=GetCurrentThreadId() );
    rc = sqlite3Win32Wait((HANDLE)p->tid);
    assert( rc!=WAIT_IO_COMPLETION );
    bRc = CloseHandle((HANDLE)p->tid);
    (void)bRc;  /* Prevent warning when assert() is a no-op */
    assert( bRc );
  }
  if( rc==WAIT_OBJECT_0 ) *ppOut = p->pResult;
  sqlite3_free(p);
  return (rc==WAIT_OBJECT_0) ? SQLITE_OK : SQLITE_ERROR;
}

#endif /* SQLITE_OS_WIN_THREADS */
/******************************** End Win32 Threads *************************/


/********************************* Single-Threaded **************************/
#ifndef SQLITE_THREADS_IMPLEMENTED
/*
** This implementation does not actually create a new thread.  It does the
** work of the thread in the main thread, when either the thread is created
** or when it is joined
*/

/* A running thread */
struct SQLiteThread {
  void *(*xTask)(void*);   /* The routine to run as a thread */
  void *pIn;               /* Argument to xTask */
  void *pResult;           /* Result of xTask */
};

/* Create a new thread */
SQLITE_PRIVATE int sqlite3ThreadCreate(
  SQLiteThread **ppThread,  /* OUT: Write the thread object here */
  void *(*xTask)(void*),    /* Routine to run in a separate thread */
  void *pIn                 /* Argument passed into xTask() */
){
  SQLiteThread *p;

  assert( ppThread!=0 );
  assert( xTask!=0 );
  *ppThread = 0;
  p = sqlite3Malloc(sizeof(*p));
  if( p==0 ) return SQLITE_NOMEM_BKPT;
  if( (SQLITE_PTR_TO_INT(p)/17)&1 ){
    p->xTask = xTask;
    p->pIn = pIn;
  }else{
    p->xTask = 0;
    p->pResult = xTask(pIn);
  }
  *ppThread = p;
  return SQLITE_OK;
}

/* Get the results of the thread */
SQLITE_PRIVATE int sqlite3ThreadJoin(SQLiteThread *p, void **ppOut){

  assert( ppOut!=0 );
  if( NEVER(p==0) ) return SQLITE_NOMEM_BKPT;
  if( p->xTask ){
    *ppOut = p->xTask(p->pIn);
  }else{
    *ppOut = p->pResult;
  }
  sqlite3_free(p);

#if defined(SQLITE_TEST)
  {
    void *pTstAlloc = sqlite3Malloc(10);
    if (!pTstAlloc) return SQLITE_NOMEM_BKPT;
    sqlite3_free(pTstAlloc);
  }
#endif

  return SQLITE_OK;
}

#endif /* !defined(SQLITE_THREADS_IMPLEMENTED) */
/****************************** End Single-Threaded *************************/
#endif /* SQLITE_MAX_WORKER_THREADS>0 */

/************** End of threads.c *********************************************/
