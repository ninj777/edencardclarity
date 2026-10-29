/************** Begin file utf.c *********************************************/
/*
** 2004 April 13
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
*************************************************************************
** This file contains routines used to translate between UTF-8,
** UTF-16, UTF-16BE, and UTF-16LE.
**
** Notes on UTF-8:
**
**   Byte-0    Byte-1    Byte-2    Byte-3    Value
**  0xxxxxxx                                 00000000 00000000 0xxxxxxx
**  110yyyyy  10xxxxxx                       00000000 00000yyy yyxxxxxx
**  1110zzzz  10yyyyyy  10xxxxxx             00000000 zzzzyyyy yyxxxxxx
**  11110uuu  10uuzzzz  10yyyyyy  10xxxxxx   000uuuuu zzzzyyyy yyxxxxxx
**
**
** Notes on UTF-16:  (with wwww+1==uuuuu)
**
**      Word-0               Word-1          Value
**  110110ww wwzzzzyy   110111yy yyxxxxxx    000uuuuu zzzzyyyy yyxxxxxx
**  zzzzyyyy yyxxxxxx                        00000000 zzzzyyyy yyxxxxxx
**
**
** BOM or Byte Order Mark:
**     0xff 0xfe   little-endian utf-16 follows
**     0xfe 0xff   big-endian utf-16 follows
**
*/
/* #include "sqliteInt.h" */
/* #include <assert.h> */
/* #include "vdbeInt.h" */

#if !defined(SQLITE_AMALGAMATION) && SQLITE_BYTEORDER==0
/*
** The following constant value is used by the SQLITE_BIGENDIAN and
** SQLITE_LITTLEENDIAN macros.
*/
SQLITE_PRIVATE const int sqlite3one = 1;
#endif /* SQLITE_AMALGAMATION && SQLITE_BYTEORDER==0 */

/*
** This lookup table is used to help decode the first byte of
** a multi-byte UTF8 character.
*/
static const unsigned char sqlite3Utf8Trans1[] = {
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
  0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
  0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
  0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
  0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
  0x00, 0x01, 0x02, 0x03, 0x00, 0x01, 0x00, 0x00,
};


#define WRITE_UTF8(zOut, c) {                          \
  if( c<0x00080 ){                                     \
    *zOut++ = (u8)(c&0xFF);                            \
  }                                                    \
  else if( c<0x00800 ){                                \
    *zOut++ = 0xC0 + (u8)((c>>6)&0x1F);                \
    *zOut++ = 0x80 + (u8)(c & 0x3F);                   \
  }                                                    \
  else if( c<0x10000 ){                                \
    *zOut++ = 0xE0 + (u8)((c>>12)&0x0F);               \
    *zOut++ = 0x80 + (u8)((c>>6) & 0x3F);              \
    *zOut++ = 0x80 + (u8)(c & 0x3F);                   \
  }else{                                               \
    *zOut++ = 0xF0 + (u8)((c>>18) & 0x07);             \
    *zOut++ = 0x80 + (u8)((c>>12) & 0x3F);             \
    *zOut++ = 0x80 + (u8)((c>>6) & 0x3F);              \
    *zOut++ = 0x80 + (u8)(c & 0x3F);                   \
  }                                                    \
}

#define WRITE_UTF16LE(zOut, c) {                                    \
  if( c<=0xFFFF ){                                                  \
    *zOut++ = (u8)(c&0x00FF);                                       \
    *zOut++ = (u8)((c>>8)&0x00FF);                                  \
  }else{                                                            \
    *zOut++ = (u8)(((c>>10)&0x003F) + (((c-0x10000)>>10)&0x00C0));  \
    *zOut++ = (u8)(0x00D8 + (((c-0x10000)>>18)&0x03));              \
    *zOut++ = (u8)(c&0x00FF);                                       \
    *zOut++ = (u8)(0x00DC + ((c>>8)&0x03));                         \
  }                                                                 \
}

#define WRITE_UTF16BE(zOut, c) {                                    \
  if( c<=0xFFFF ){                                                  \
    *zOut++ = (u8)((c>>8)&0x00FF);                                  \
    *zOut++ = (u8)(c&0x00FF);                                       \
  }else{                                                            \
    *zOut++ = (u8)(0x00D8 + (((c-0x10000)>>18)&0x03));              \
    *zOut++ = (u8)(((c>>10)&0x003F) + (((c-0x10000)>>10)&0x00C0));  \
    *zOut++ = (u8)(0x00DC + ((c>>8)&0x03));                         \
    *zOut++ = (u8)(c&0x00FF);                                       \
  }                                                                 \
}

/*
** Write a single UTF8 character whose value is v into the
** buffer starting at zOut.  zOut must be sized to hold at
** least four bytes.  Return the number of bytes needed
** to encode the new character.
*/
SQLITE_PRIVATE int sqlite3AppendOneUtf8Character(char *zOut, u32 v){
  if( v<0x00080 ){
    zOut[0] = (u8)(v & 0xff);
    return 1;
  }
  if( v<0x00800 ){
    zOut[0] = 0xc0 + (u8)((v>>6) & 0x1f);
    zOut[1] = 0x80 + (u8)(v & 0x3f);
    return 2;
  }
  if( v<0x10000 ){
    zOut[0] = 0xe0 + (u8)((v>>12) & 0x0f);
    zOut[1] = 0x80 + (u8)((v>>6) & 0x3f);
    zOut[2] = 0x80 + (u8)(v & 0x3f);
    return 3;
  }
  zOut[0] = 0xf0 + (u8)((v>>18) & 0x07);
  zOut[1] = 0x80 + (u8)((v>>12) & 0x3f);
  zOut[2] = 0x80 + (u8)((v>>6) & 0x3f);
  zOut[3] = 0x80 + (u8)(v & 0x3f);
  return 4;
}

/*
** Translate a single UTF-8 character.  Return the unicode value.
**
** During translation, assume that the byte that zTerm points
** is a 0x00.
**
** Write a pointer to the next unread byte back into *pzNext.
**
** Notes On Invalid UTF-8:
**
**  *  This routine never allows a 7-bit character (0x00 through 0x7f) to
**     be encoded as a multi-byte character.  Any multi-byte character that
**     attempts to encode a value between 0x00 and 0x7f is rendered as 0xfffd.
**
**  *  This routine never allows a UTF16 surrogate value to be encoded.
**     If a multi-byte character attempts to encode a value between
**     0xd800 and 0xe000 then it is rendered as 0xfffd.
**
**  *  Bytes in the range of 0x80 through 0xbf which occur as the first
**     byte of a character are interpreted as single-byte characters
**     and rendered as themselves even though they are technically
**     invalid characters.
**
**  *  This routine accepts over-length UTF8 encodings
**     for unicode values 0x80 and greater.  It does not change over-length
**     encodings to 0xfffd as some systems recommend.
*/
#define READ_UTF8(zIn, zTerm, c)                           \
  c = *(zIn++);                                            \
  if( c>=0xc0 ){                                           \
    c = sqlite3Utf8Trans1[c-0xc0];                         \
    while( zIn<zTerm && (*zIn & 0xc0)==0x80 ){             \
      c = (c<<6) + (0x3f & *(zIn++));                      \
    }                                                      \
    if( c<0x80 || (c&0xFFFFF800)==0xD800 ){  c = 0xFFFD; } \
  }
SQLITE_PRIVATE u32 sqlite3Utf8Read(
  const unsigned char **pz    /* Pointer to string from which to read char */
){
  unsigned int c;

  /* Same as READ_UTF8() above but without the zTerm parameter.
  ** For this routine, we assume the UTF8 string is always zero-terminated.
  */
  c = *((*pz)++);
  if( c>=0xc0 ){
    c = sqlite3Utf8Trans1[c-0xc0];
    while( (*(*pz) & 0xc0)==0x80 ){
      c = (c<<6) + (0x3f & *((*pz)++));
    }
    if( c<0x80 || (c&0xFFFFF800)==0xD800 ){  c = 0xFFFD; }
  }
  return c;
}

/*
** Read a single UTF8 character out of buffer z[], but reading no
** more than n characters from the buffer.  z[] is not zero-terminated.
**
** Return the number of bytes used to construct the character.
**
** Invalid UTF8 might generate a strange result.  No effort is made
** to detect invalid UTF8.
**
** At most 4 bytes will be read out of z[].  The return value will always
** be between 1 and 4.
*/
SQLITE_PRIVATE int sqlite3Utf8ReadLimited(
  const u8 *z,
  int n,
  u32 *piOut
){
  u32 c;
  int i = 1;
  assert( n>0 );
  c = z[0];
  if( c>=0xc0 ){
    c = sqlite3Utf8Trans1[c-0xc0];
    if( n>4 ) n = 4;
    while( i<n && (z[i] & 0xc0)==0x80 ){
      c = (c<<6) + (0x3f & z[i]);
      i++;
    }
  }
  *piOut = c;
  return i;
}


/*
** If the TRANSLATE_TRACE macro is defined, the value of each Mem is
** printed on stderr on the way into and out of sqlite3VdbeMemTranslate().
*/
/* #define TRANSLATE_TRACE 1 */

#ifndef SQLITE_OMIT_UTF16
/*
** This routine transforms the internal text encoding used by pMem to
** desiredEnc. It is an error if the string is already of the desired
** encoding, or if *pMem does not contain a string value.
*/
SQLITE_PRIVATE SQLITE_NOINLINE int sqlite3VdbeMemTranslate(Mem *pMem, u8 desiredEnc){
  sqlite3_int64 len;          /* Maximum length of output string in bytes */
  unsigned char *zOut;        /* Output buffer */
  unsigned char *zIn;         /* Input iterator */
  unsigned char *zTerm;       /* End of input */
  unsigned char *z;           /* Output iterator */
  unsigned int c;

  assert( pMem->db==0 || sqlite3_mutex_held(pMem->db->mutex) );
  assert( pMem->flags&MEM_Str );
  assert( pMem->enc!=desiredEnc );
  assert( pMem->enc!=0 );
  assert( pMem->n>=0 );

#if defined(TRANSLATE_TRACE) && defined(SQLITE_DEBUG)
  {
    StrAccum acc;
    char zBuf[1000];
    sqlite3StrAccumInit(&acc, 0, zBuf, sizeof(zBuf), 0);
    sqlite3VdbeMemPrettyPrint(pMem, &acc);
    fprintf(stderr, "INPUT:  %s\n", sqlite3StrAccumFinish(&acc));
  }
#endif

  /* If the translation is between UTF-16 little and big endian, then
  ** all that is required is to swap the byte order. This case is handled
  ** differently from the others.
  */
  if( pMem->enc!=SQLITE_UTF8 && desiredEnc!=SQLITE_UTF8 ){
    u8 temp;
    int rc;
    rc = sqlite3VdbeMemMakeWriteable(pMem);
    if( rc!=SQLITE_OK ){
      assert( rc==SQLITE_NOMEM );
      return SQLITE_NOMEM_BKPT;
    }
    zIn = (u8*)pMem->z;
    zTerm = &zIn[pMem->n&~1];
    while( zIn<zTerm ){
      temp = *zIn;
      *zIn = *(zIn+1);
      zIn++;
      *zIn++ = temp;
    }
    pMem->enc = desiredEnc;
    goto translate_out;
  }

  /* Set len to the maximum number of bytes required in the output buffer. */
  if( desiredEnc==SQLITE_UTF8 ){
    /* When converting from UTF-16, the maximum growth results from
    ** translating a 2-byte character to a 4-byte UTF-8 character.
    ** A single byte is required for the output string
    ** nul-terminator.
    */
    pMem->n &= ~1;
    len = 2 * (sqlite3_int64)pMem->n + 1;
  }else{
    /* When converting from UTF-8 to UTF-16 the maximum growth is caused
    ** when a 1-byte UTF-8 character is translated into a 2-byte UTF-16
    ** character. Two bytes are required in the output buffer for the
    ** nul-terminator.
    */
    len = 2 * (sqlite3_int64)pMem->n + 2;
  }

  /* Set zIn to point at the start of the input buffer and zTerm to point 1
  ** byte past the end.
  **
  ** Variable zOut is set to point at the output buffer, space obtained
  ** from sqlite3_malloc().
  */
  zIn = (u8*)pMem->z;
  zTerm = &zIn[pMem->n];
  zOut = sqlite3DbMallocRaw(pMem->db, len);
  if( !zOut ){
    return SQLITE_NOMEM_BKPT;
  }
  z = zOut;

  if( pMem->enc==SQLITE_UTF8 ){
    if( desiredEnc==SQLITE_UTF16LE ){
      /* UTF-8 -> UTF-16 Little-endian */
      while( zIn<zTerm ){
        READ_UTF8(zIn, zTerm, c);
        WRITE_UTF16LE(z, c);
      }
    }else{
      assert( desiredEnc==SQLITE_UTF16BE );
      /* UTF-8 -> UTF-16 Big-endian */
      while( zIn<zTerm ){
        READ_UTF8(zIn, zTerm, c);
        WRITE_UTF16BE(z, c);
      }
    }
    pMem->n = (int)(z - zOut);
    *z++ = 0;
  }else{
    assert( desiredEnc==SQLITE_UTF8 );
    if( pMem->enc==SQLITE_UTF16LE ){
      /* UTF-16 Little-endian -> UTF-8 */
      while( zIn<zTerm ){
        c = *(zIn++);
        c += (*(zIn++))<<8;
        if( c>=0xd800 && c<0xe000 ){
#ifdef SQLITE_REPLACE_INVALID_UTF
          if( c>=0xdc00 || zIn>=zTerm ){
            c = 0xfffd;
          }else{
            int c2 = *(zIn++);
            c2 += (*(zIn++))<<8;
            if( c2<0xdc00 || c2>=0xe000 ){
              zIn -= 2;
              c = 0xfffd;
            }else{
              c = ((c&0x3ff)<<10) + (c2&0x3ff) + 0x10000;
            }
          }
#else
          if( zIn<zTerm ){
            int c2 = (*zIn++);
            c2 += ((*zIn++)<<8);
            c = (c2&0x03FF) + ((c&0x003F)<<10) + (((c&0x03C0)+0x0040)<<10);
          }
#endif
        }
        WRITE_UTF8(z, c);
      }
    }else{
      /* UTF-16 Big-endian -> UTF-8 */
      while( zIn<zTerm ){
        c = (*(zIn++))<<8;
        c += *(zIn++);
        if( c>=0xd800 && c<0xe000 ){
#ifdef SQLITE_REPLACE_INVALID_UTF
          if( c>=0xdc00 || zIn>=zTerm ){
            c = 0xfffd;
          }else{
            int c2 = (*(zIn++))<<8;
            c2 += *(zIn++);
            if( c2<0xdc00 || c2>=0xe000 ){
              zIn -= 2;
              c = 0xfffd;
            }else{
              c = ((c&0x3ff)<<10) + (c2&0x3ff) + 0x10000;
            }
          }
#else
          if( zIn<zTerm ){
            int c2 = ((*zIn++)<<8);
            c2 += (*zIn++);
            c = (c2&0x03FF) + ((c&0x003F)<<10) + (((c&0x03C0)+0x0040)<<10);
          }
#endif
        }
        WRITE_UTF8(z, c);
      }
    }
    pMem->n = (int)(z - zOut);
  }
  *z = 0;
  assert( (pMem->n+(desiredEnc==SQLITE_UTF8?1:2))<=len );

  c = MEM_Str|MEM_Term|(pMem->flags&(MEM_AffMask|MEM_Subtype));
  sqlite3VdbeMemRelease(pMem);
  pMem->flags = c;
  pMem->enc = desiredEnc;
  pMem->z = (char*)zOut;
  pMem->zMalloc = pMem->z;
  pMem->szMalloc = sqlite3DbMallocSize(pMem->db, pMem->z);

translate_out:
#if defined(TRANSLATE_TRACE) && defined(SQLITE_DEBUG)
  {
    StrAccum acc;
    char zBuf[1000];
    sqlite3StrAccumInit(&acc, 0, zBuf, sizeof(zBuf), 0);
    sqlite3VdbeMemPrettyPrint(pMem, &acc);
    fprintf(stderr, "OUTPUT: %s\n", sqlite3StrAccumFinish(&acc));
  }
#endif
  return SQLITE_OK;
}
#endif /* SQLITE_OMIT_UTF16 */

#ifndef SQLITE_OMIT_UTF16
/*
** This routine checks for a byte-order mark at the beginning of the
** UTF-16 string stored in *pMem. If one is present, it is removed and
** the encoding of the Mem adjusted. This routine does not do any
** byte-swapping, it just sets Mem.enc appropriately.
**
** The allocation (static, dynamic etc.) and encoding of the Mem may be
** changed by this function.
*/
SQLITE_PRIVATE int sqlite3VdbeMemHandleBom(Mem *pMem){
  int rc = SQLITE_OK;
  u8 bom = 0;

  assert( pMem->n>=0 );
  if( pMem->n>1 ){
    u8 b1 = *(u8 *)pMem->z;
    u8 b2 = *(((u8 *)pMem->z) + 1);
    if( b1==0xFE && b2==0xFF ){
      bom = SQLITE_UTF16BE;
    }
    if( b1==0xFF && b2==0xFE ){
      bom = SQLITE_UTF16LE;
    }
  }

  if( bom ){
    rc = sqlite3VdbeMemMakeWriteable(pMem);
    if( rc==SQLITE_OK ){
      pMem->n -= 2;
      memmove(pMem->z, &pMem->z[2], pMem->n);
      pMem->z[pMem->n] = '\0';
      pMem->z[pMem->n+1] = '\0';
      pMem->flags |= MEM_Term;
      pMem->enc = bom;
    }
  }
  return rc;
}
#endif /* SQLITE_OMIT_UTF16 */

/*
** pZ is a UTF-8 encoded unicode string. If nByte is less than zero,
** return the number of unicode characters in pZ up to (but not including)
** the first 0x00 byte. If nByte is not less than zero, return the
** number of unicode characters in the first nByte of pZ (or up to
** the first 0x00, whichever comes first).
*/
SQLITE_PRIVATE int sqlite3Utf8CharLen(const char *zIn, int nByte){
  int r = 0;
  const u8 *z = (const u8*)zIn;
  const u8 *zTerm;
  if( nByte>=0 ){
    zTerm = &z[nByte];
  }else{
    zTerm = (const u8*)(-1);
  }
  assert( z<=zTerm );
  while( *z!=0 && z<zTerm ){
    SQLITE_SKIP_UTF8(z);
    r++;
  }
  return r;
}

/* This test function is not currently used by the automated test-suite.
** Hence it is only available in debug builds.
*/
#if defined(SQLITE_TEST) && defined(SQLITE_DEBUG)
/*
** Translate UTF-8 to UTF-8.
**
** This has the effect of making sure that the string is well-formed
** UTF-8.  Miscoded characters are removed.
**
** The translation is done in-place and aborted if the output
** overruns the input.
*/
SQLITE_PRIVATE int sqlite3Utf8To8(unsigned char *zIn){
  unsigned char *zOut = zIn;
  unsigned char *zStart = zIn;
  u32 c;

  while( zIn[0] && zOut<=zIn ){
    c = sqlite3Utf8Read((const u8**)&zIn);
    if( c!=0xfffd ){
      WRITE_UTF8(zOut, c);
    }
  }
  *zOut = 0;
  return (int)(zOut - zStart);
}
#endif

#ifndef SQLITE_OMIT_UTF16
/*
** Convert a UTF-16 string in the native encoding into a UTF-8 string.
** Memory to hold the UTF-8 string is obtained from sqlite3_malloc and must
** be freed by the calling function.
**
** NULL is returned if there is an allocation error.
*/
SQLITE_PRIVATE char *sqlite3Utf16to8(sqlite3 *db, const void *z, int nByte, u8 enc){
  Mem m;
  memset(&m, 0, sizeof(m));
  m.db = db;
  sqlite3VdbeMemSetStr(&m, z, nByte, enc, SQLITE_STATIC);
  sqlite3VdbeChangeEncoding(&m, SQLITE_UTF8);
  if( db->mallocFailed ){
    sqlite3VdbeMemRelease(&m);
    m.z = 0;
  }
  assert( (m.flags & MEM_Term)!=0 || db->mallocFailed );
  assert( (m.flags & MEM_Str)!=0 || db->mallocFailed );
  assert( m.z || db->mallocFailed );
  return m.z;
}

/*
** zIn is a UTF-16 encoded unicode string at least nByte bytes long.
** Return the number of bytes in the first nChar unicode characters
** in pZ.  nChar must be non-negative.  Surrogate pairs count as a single
** character.
*/
SQLITE_PRIVATE int sqlite3Utf16ByteLen(const void *zIn, int nByte, int nChar){
  int c;
  unsigned char const *z = zIn;
  unsigned char const *zEnd = &z[nByte-1];
  int n = 0;

  if( SQLITE_UTF16NATIVE==SQLITE_UTF16LE ) z++;
  while( n<nChar && z<=zEnd ){
    c = z[0];
    z += 2;
    if( c>=0xd8 && c<0xdc && z<=zEnd && z[0]>=0xdc && z[0]<0xe0 ) z += 2;
    n++;
  }
  return (int)(z-(unsigned char const *)zIn)
              - (SQLITE_UTF16NATIVE==SQLITE_UTF16LE);
}

#if defined(SQLITE_TEST)
/*
** This routine is called from the TCL test function "translate_selftest".
** It checks that the primitives for serializing and deserializing
** characters in each encoding are inverses of each other.
*/
SQLITE_PRIVATE void sqlite3UtfSelfTest(void){
  unsigned int i, t;
  unsigned char zBuf[20];
  unsigned char *z;
  int n;
  unsigned int c;

  for(i=0; i<0x00110000; i++){
    z = zBuf;
    WRITE_UTF8(z, i);
    n = (int)(z-zBuf);
    assert( n>0 && n<=4 );
    z[0] = 0;
    z = zBuf;
    c = sqlite3Utf8Read((const u8**)&z);
    t = i;
    if( i>=0xD800 && i<=0xDFFF ) t = 0xFFFD;
    assert( c==t );
    assert( (z-zBuf)==n );
  }
}
#endif /* SQLITE_TEST */
#endif /* SQLITE_OMIT_UTF16 */

/************** End of utf.c *************************************************/
/************** Begin file util.c ********************************************/
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
** Utility functions used throughout sqlite.
**
** This file contains functions for allocating memory, comparing
** strings, and stuff like that.
**
*/
/* #include "sqliteInt.h" */
/* #include <stdarg.h> */
#ifndef SQLITE_OMIT_FLOATING_POINT
#include <math.h>

/* Work around a bug in older Microsoft compilers
** Forum post 2026-04-10T06:33:11z */
#if !defined(INFINITY) && defined(_MSC_VER)
# define INFINITY HUGE_VAL
#endif

#endif /* SQLITE_OMIT_FLOATING_POINT */

/*
** Calls to sqlite3FaultSim() are used to simulate a failure during testing,
** or to bypass normal error detection during testing in order to let
** execute proceed further downstream.
**
** In deployment, sqlite3FaultSim() *always* return SQLITE_OK (0).  The
** sqlite3FaultSim() function only returns non-zero during testing.
**
** During testing, if the test harness has set a fault-sim callback using
** a call to sqlite3_test_control(SQLITE_TESTCTRL_FAULT_INSTALL), then
** each call to sqlite3FaultSim() is relayed to that application-supplied
** callback and the integer return value form the application-supplied
** callback is returned by sqlite3FaultSim().
**
** The integer argument to sqlite3FaultSim() is a code to identify which
** sqlite3FaultSim() instance is being invoked. Each call to sqlite3FaultSim()
** should have a unique code.  To prevent legacy testing applications from
** breaking, the codes should not be changed or reused.
*/
#ifndef SQLITE_UNTESTABLE
SQLITE_PRIVATE int sqlite3FaultSim(int iTest){
  int (*xCallback)(int) = sqlite3GlobalConfig.xTestCallback;
  return xCallback ? xCallback(iTest) : SQLITE_OK;
}
#endif

#ifndef SQLITE_OMIT_FLOATING_POINT
/*
** Return true if the floating point value is Not a Number (NaN).
**
** Use the math library isnan() function if compiled with SQLITE_HAVE_ISNAN.
** Otherwise, we have our own implementation that works on most systems.
*/
SQLITE_PRIVATE int sqlite3IsNaN(double x){
  int rc;   /* The value return */
#if !SQLITE_HAVE_ISNAN && !HAVE_ISNAN
  u64 y;
  memcpy(&y,&x,sizeof(y));
  rc = IsNaN(y);
#else
  rc = isnan(x);
#endif /* HAVE_ISNAN */
  testcase( rc );
  return rc;
}
#endif /* SQLITE_OMIT_FLOATING_POINT */

#ifndef SQLITE_OMIT_FLOATING_POINT
/*
** Return true if the floating point value is NaN or +Inf or -Inf.
*/
SQLITE_PRIVATE int sqlite3IsOverflow(double x){
  int rc;   /* The value return */
  u64 y;
  memcpy(&y,&x,sizeof(y));
  rc = IsOvfl(y);
  return rc;
}
#endif /* SQLITE_OMIT_FLOATING_POINT */

/*
** Compute a string length that is limited to what can be stored in
** lower 30 bits of a 32-bit signed integer.
**
** The value returned will never be negative.  Nor will it ever be greater
** than the actual length of the string.  For very long strings (greater
** than 1GiB) the value returned might be less than the true string length.
*/
SQLITE_PRIVATE int sqlite3Strlen30(const char *z){
  if( z==0 ) return 0;
  return 0x3fffffff & (int)strlen(z);
}

/*
** Return the declared type of a column.  Or return zDflt if the column
** has no declared type.
**
** The column type is an extra string stored after the zero-terminator on
** the column name if and only if the COLFLAG_HASTYPE flag is set.
*/
SQLITE_PRIVATE char *sqlite3ColumnType(Column *pCol, char *zDflt){
  if( pCol->colFlags & COLFLAG_HASTYPE ){
    return pCol->zCnName + strlen(pCol->zCnName) + 1;
  }else if( pCol->eCType ){
    assert( pCol->eCType<=SQLITE_N_STDTYPE );
    return (char*)sqlite3StdType[pCol->eCType-1];
  }else{
    return zDflt;
  }
}

/*
** Helper function for sqlite3Error() - called rarely.  Broken out into
** a separate routine to avoid unnecessary register saves on entry to
** sqlite3Error().
*/
static SQLITE_NOINLINE void  sqlite3ErrorFinish(sqlite3 *db, int err_code){
  if( db->pErr ) sqlite3ValueSetNull(db->pErr);
  sqlite3SystemError(db, err_code);
}

/*
** Set the current error code to err_code and clear any prior error message.
** Also set iSysErrno (by calling sqlite3System) if the err_code indicates
** that would be appropriate.
*/
SQLITE_PRIVATE void sqlite3Error(sqlite3 *db, int err_code){
  assert( db!=0 );
  db->errCode = err_code;
  if( err_code || db->pErr ){
    sqlite3ErrorFinish(db, err_code);
  }else{
    db->errByteOffset = -1;
  }
}

/*
** The equivalent of sqlite3Error(db, SQLITE_OK).  Clear the error state
** and error message.
*/
SQLITE_PRIVATE void sqlite3ErrorClear(sqlite3 *db){
  assert( db!=0 );
  db->errCode = SQLITE_OK;
  db->errByteOffset = -1;
  if( db->pErr ) sqlite3ValueSetNull(db->pErr);
}

/*
** Load the sqlite3.iSysErrno field if that is an appropriate thing
** to do based on the SQLite error code in rc.
*/
SQLITE_PRIVATE void sqlite3SystemError(sqlite3 *db, int rc){
  if( rc==SQLITE_IOERR_NOMEM ) return;
#if defined(SQLITE_USE_SEH) && !defined(SQLITE_OMIT_WAL)
  if( rc==SQLITE_IOERR_IN_PAGE ){
    int ii;
    int iErr;
    sqlite3BtreeEnterAll(db);
    for(ii=0; ii<db->nDb; ii++){
      if( db->aDb[ii].pBt ){
        iErr = sqlite3PagerWalSystemErrno(sqlite3BtreePager(db->aDb[ii].pBt));
        if( iErr ){
          db->iSysErrno = iErr;
        }
      }
    }
    sqlite3BtreeLeaveAll(db);
    return;
  }
#endif
  rc &= 0xff;
  if( rc==SQLITE_CANTOPEN || rc==SQLITE_IOERR ){
    db->iSysErrno = sqlite3OsGetLastError(db->pVfs);
  }
}

/*
** Set the most recent error code and error string for the sqlite
** handle "db". The error code is set to "err_code".
**
** If it is not NULL, string zFormat specifies the format of the
** error string.  zFormat and any string tokens that follow it are
** assumed to be encoded in UTF-8.
**
** To clear the most recent error for sqlite handle "db", sqlite3Error
** should be called with err_code set to SQLITE_OK and zFormat set
** to NULL.
*/
SQLITE_PRIVATE void sqlite3ErrorWithMsg(sqlite3 *db, int err_code, const char *zFormat, ...){
  assert( db!=0 );
  db->errCode = err_code;
  sqlite3SystemError(db, err_code);
  if( zFormat==0 ){
    sqlite3Error(db, err_code);
  }else if( db->pErr || (db->pErr = sqlite3ValueNew(db))!=0 ){
    char *z;
    va_list ap;
    va_start(ap, zFormat);
    z = sqlite3VMPrintf(db, zFormat, ap);
    va_end(ap);
    sqlite3ValueSetStr(db->pErr, -1, z, SQLITE_UTF8, SQLITE_DYNAMIC);
  }
}

/*
** Check for interrupts and invoke progress callback.
*/
SQLITE_PRIVATE void sqlite3ProgressCheck(Parse *p){
  sqlite3 *db = p->db;
  if( AtomicLoad(&db->u1.isInterrupted) ){
    p->nErr++;
    p->rc = SQLITE_INTERRUPT;
  }
#ifndef SQLITE_OMIT_PROGRESS_CALLBACK
  if( db->xProgress ){
    if( p->rc==SQLITE_INTERRUPT ){
      p->nProgressSteps = 0;
    }else if( (++p->nProgressSteps)>=db->nProgressOps ){
      if( db->xProgress(db->pProgressArg) ){
        p->nErr++;
        p->rc = SQLITE_INTERRUPT;
      }
      p->nProgressSteps = 0;
    }
  }
#endif
}

/*
** Add an error message to pParse->zErrMsg and increment pParse->nErr.
**
** This function should be used to report any error that occurs while
** compiling an SQL statement (i.e. within sqlite3_prepare()). The
** last thing the sqlite3_prepare() function does is copy the error
** stored by this function into the database handle using sqlite3Error().
** Functions sqlite3Error() or sqlite3ErrorWithMsg() should be used
** during statement execution (sqlite3_step() etc.).
*/
SQLITE_PRIVATE void sqlite3ErrorMsg(Parse *pParse, const char *zFormat, ...){
  char *zMsg;
  va_list ap;
  sqlite3 *db = pParse->db;
  assert( db!=0 );
  assert( db->pParse==pParse || db->pParse->pToplevel==pParse );
  db->errByteOffset = -2;
  va_start(ap, zFormat);
  zMsg = sqlite3VMPrintf(db, zFormat, ap);
  va_end(ap);
  if( db->errByteOffset<-1 ) db->errByteOffset = -1;
  if( db->suppressErr ){
    sqlite3DbFree(db, zMsg);
    if( db->mallocFailed ){
      pParse->nErr++;
      pParse->rc = SQLITE_NOMEM;
    }
  }else{
    pParse->nErr++;
    sqlite3DbFree(db, pParse->zErrMsg);
    pParse->zErrMsg = zMsg;
    pParse->rc = SQLITE_ERROR;
    pParse->pWith = 0;
  }
}

/*
** If database connection db is currently parsing SQL, then transfer
** error code errCode to that parser if the parser has not already
** encountered some other kind of error.
*/
SQLITE_PRIVATE int sqlite3ErrorToParser(sqlite3 *db, int errCode){
  Parse *pParse;
  if( db==0 || (pParse = db->pParse)==0 ) return errCode;
  pParse->rc = errCode;
  pParse->nErr++;
  return errCode;
}

/*
** Convert an SQL-style quoted string into a normal string by removing
** the quote characters.  The conversion is done in-place.  If the
** input does not begin with a quote character, then this routine
** is a no-op.
**
** The input string must be zero-terminated.  A new zero-terminator
** is added to the dequoted string.
**
** The return value is -1 if no dequoting occurs or the length of the
** dequoted string, exclusive of the zero terminator, if dequoting does
** occur.
**
** 2002-02-14: This routine is extended to remove MS-Access style
** brackets from around identifiers.  For example:  "[a-b-c]" becomes
** "a-b-c".
*/
SQLITE_PRIVATE void sqlite3Dequote(char *z){
  char quote;
  int i, j;
  if( z==0 ) return;
  quote = z[0];
  if( !sqlite3Isquote(quote) ) return;
  if( quote=='[' ) quote = ']';
  for(i=1, j=0;; i++){
    assert( z[i] );
    if( z[i]==quote ){
      if( z[i+1]==quote ){
        z[j++] = quote;
        i++;
      }else{
        break;
      }
    }else{
      z[j++] = z[i];
    }
  }
  z[j] = 0;
}
SQLITE_PRIVATE void sqlite3DequoteExpr(Expr *p){
  assert( !ExprHasProperty(p, EP_IntValue) );
  assert( sqlite3Isquote(p->u.zToken[0]) );
  p->flags |= p->u.zToken[0]=='"' ? EP_Quoted|EP_DblQuoted : EP_Quoted;
  sqlite3Dequote(p->u.zToken);
}

/*
** Expression p is a QNUMBER (quoted number). Dequote the value in p->u.zToken
** and set the type to INTEGER or FLOAT. "Quoted" integers or floats are those
** that contain '_' characters that must be removed before further processing.
*/
SQLITE_PRIVATE void sqlite3DequoteNumber(Parse *pParse, Expr *p){
  assert( p!=0 || pParse->db->mallocFailed );
  if( p ){
    const char *pIn = p->u.zToken;
    char *pOut = p->u.zToken;
    int bHex = (pIn[0]=='0' && (pIn[1]=='x' || pIn[1]=='X'));
    int iValue;
    assert( p->op==TK_QNUMBER );
    p->op = TK_INTEGER;
    do {
      if( *pIn!=SQLITE_DIGIT_SEPARATOR ){
        *pOut++ = *pIn;
        if( *pIn=='e' || *pIn=='E' || *pIn=='.' ) p->op = TK_FLOAT;
      }else{
        if( (bHex==0 && (!sqlite3Isdigit(pIn[-1]) || !sqlite3Isdigit(pIn[1])))
         || (bHex==1 && (!sqlite3Isxdigit(pIn[-1]) || !sqlite3Isxdigit(pIn[1])))
        ){
          sqlite3ErrorMsg(pParse, "unrecognized token: \"%s\"", p->u.zToken);
        }
      }
    }while( *pIn++ );
    if( bHex ) p->op = TK_INTEGER;

    /* tag-20240227-a: If after dequoting, the number is an integer that
    ** fits in 32 bits, then it must be converted into EP_IntValue.  Other
    ** parts of the code expect this.  See also tag-20240227-b. */
    if( p->op==TK_INTEGER && sqlite3GetInt32(p->u.zToken, &iValue) ){
      p->u.iValue = iValue;
      p->flags |= EP_IntValue;
    }
  }
}

/*
** If the input token p is quoted, try to adjust the token to remove
** the quotes.  This is not always possible:
**
**     "abc"     ->   abc
**     "ab""cd"  ->   (not possible because of the interior "")
**
** Remove the quotes if possible.  This is a optimization.  The overall
** system should still return the correct answer even if this routine
** is always a no-op.
*/
SQLITE_PRIVATE void sqlite3DequoteToken(Token *p){
  unsigned int i;
  if( p->n<2 ) return;
  if( !sqlite3Isquote(p->z[0]) ) return;
  for(i=1; i<p->n-1; i++){
    if( sqlite3Isquote(p->z[i]) ) return;
  }
  p->n -= 2;
  p->z++;
}

/*
** Generate a Token object from a string
*/
SQLITE_PRIVATE void sqlite3TokenInit(Token *p, char *z){
  p->z = z;
  p->n = sqlite3Strlen30(z);
}

/* Convenient short-hand */
#define UpperToLower sqlite3UpperToLower

/*
** Some systems have stricmp().  Others have strcasecmp().  Because
** there is no consistency, we will define our own.
**
** IMPLEMENTATION-OF: R-30243-02494 The sqlite3_stricmp() and
** sqlite3_strnicmp() APIs allow applications and extensions to compare
** the contents of two buffers containing UTF-8 strings in a
** case-independent fashion, using the same definition of "case
** independence" that SQLite uses internally when comparing identifiers.
*/
SQLITE_API int sqlite3_stricmp(const char *zLeft, const char *zRight){
  if( zLeft==0 ){
    return zRight ? -1 : 0;
  }else if( zRight==0 ){
    return 1;
  }
  return sqlite3StrICmp(zLeft, zRight);
}
SQLITE_PRIVATE int sqlite3StrICmp(const char *zLeft, const char *zRight){
  unsigned char *a, *b;
  int c, x;
  a = (unsigned char *)zLeft;
  b = (unsigned char *)zRight;
  for(;;){
    c = *a;
    x = *b;
    if( c==x ){
      if( c==0 ) break;
    }else{
      c = (int)UpperToLower[c] - (int)UpperToLower[x];
      if( c ) break;
    }
    a++;
    b++;
  }
  return c;
}
SQLITE_API int sqlite3_strnicmp(const char *zLeft, const char *zRight, int N){
  register unsigned char *a, *b;
  if( zLeft==0 ){
    return zRight ? -1 : 0;
  }else if( zRight==0 ){
    return 1;
  }
  a = (unsigned char *)zLeft;
  b = (unsigned char *)zRight;
  while( N-- > 0 && *a!=0 && UpperToLower[*a]==UpperToLower[*b]){ a++; b++; }
  return N<0 ? 0 : UpperToLower[*a] - UpperToLower[*b];
}

/*
** Compute an 8-bit hash on a string that is insensitive to case differences
*/
SQLITE_PRIVATE u8 sqlite3StrIHash(const char *z){
  u8 h = 0;
  if( z==0 ) return 0;
  while( z[0] ){
    h += UpperToLower[(unsigned char)z[0]];
    z++;
  }
  return h;
}

#if !defined(SQLITE_DISABLE_INTRINSIC)               \
  && (defined(__GNUC__) || defined(__clang__))       \
  && (defined(__x86_64__) || defined(__aarch64__) || \
      (defined(__riscv) && defined(__riscv_xlen) && (__riscv_xlen>32)))
#define SQLITE_USE_UINT128
#endif

/*
** Two inputs are multiplied to get a 128-bit result.  Write the
** lower 64-bits of the result into *pLo, and return the high-order
** 64 bits.
*/
static u64 sqlite3Multiply128(u64 a, u64 b, u64 *pLo){
#if defined(SQLITE_USE_UINT128)
  __uint128_t r = (__uint128_t)a * b;
  *pLo = (u64)r;
  return (u64)(r>>64);
#elif defined(_WIN64) && !defined(SQLITE_DISABLE_INTRINSIC)
  *pLo = a*b;
  return __umulh(a, b);
#else
  u64 a0 = (u32)a;
  u64 a1 = a >> 32;
  u64 b0 = (u32)b;
  u64 b1 = b >> 32;
  u64 a0b0 = a0 * b0;
  u64 a1b1 = a1 * b1;
  u64 a0b1 = a0 * b1;
  u64 a1b0 = a1 * b0;
  u64 t = (a0b0 >> 32) + (u32)a0b1 + (u32)a1b0;
  *pLo = (a0b0 & UINT64_C(0xffffffff)) | (t << 32);
  return a1b1 + (a0b1>>32) + (a1b0>>32) + (t>>32);
#endif
}

/*
** A is an unsigned 96-bit integer formed by (a<<32)+aLo.
** B is an unsigned 64-bit integer.
**
** Compute the upper 96 bits of 160-bit result of A*B.
**
** Write ((A*B)>>64 & 0xffffffff) (the middle 32 bits of A*B)
** into *pLo.  Return the upper 64 bits of A*B.
**
** The lower 64 bits of A*B are discarded.
*/
static u64 sqlite3Multiply160(u64 a, u32 aLo, u64 b, u32 *pLo){
#if defined(SQLITE_USE_UINT128)
  __uint128_t r = (__uint128_t)a * b;
  r += ((__uint128_t)aLo * b) >> 32;
  *pLo = (r>>32)&0xffffffff;
  return r>>64;
#elif defined(_WIN64) && !defined(SQLITE_DISABLE_INTRINSIC)
  u64 r1_hi = __umulh(a,b);
  u64 r1_lo = a*b;
  u64 r2 = (__umulh((u64)aLo,b)<<32) + ((aLo*b)>>32);
  u64 t = r1_lo + r2;
  if( t<r1_lo ) r1_hi++;
  *pLo = t>>32;
  return r1_hi;
#else
  u64 x2 = a>>32;
  u64 x1 = a&0xffffffff;
  u64 x0 = aLo;
  u64 y1 = b>>32;
  u64 y0 = b&0xffffffff;
  u64 x2y1 = x2*y1;
  u64 r4 = x2y1>>32;
  u64 x2y0 = x2*y0;
  u64 x1y1 = x1*y1;
  u64 r3 = (x2y1 & 0xffffffff) + (x2y0 >>32) + (x1y1 >>32);
  u64 x1y0 = x1*y0;
  u64 x0y1 = x0*y1;
  u64 r2 = (x2y0 & 0xffffffff) + (x1y1 & 0xffffffff) +
             (x1y0 >>32) + (x0y1>>32);
  u64 x0y0 = x0*y0;
  u64 r1 = (x1y0 & 0xffffffff) + (x0y1 & 0xffffffff) +
             (x0y0 >>32);
  r2 += r1>>32;
  r3 += r2>>32;
  *pLo = r2&0xffffffff;
  return (r4<<32) + r3;
#endif
}

#undef SQLITE_USE_UINT128

/*
** Return a u64 with the N-th bit set.
*/
#define U64_BIT(N)  (((u64)1)<<(N))

/*
** Range of powers of 10 that we need to deal with when converting
** IEEE754 doubles to and from decimal.
*/
#define POWERSOF10_FIRST (-348)
#define POWERSOF10_LAST  (+347)

/*
** For any p between -348 and +347, return the integer part of
**
**    pow(10,p) * pow(2,63-pow10to2(p))
**
** Or, in other words, for any p in range, return the most significant
** 64 bits of pow(10,p).  The pow(10,p) value is shifted left or right,
** as appropriate so the most significant 64 bits fit exactly into a
** 64-bit unsigned integer.
**
** Write into *pLo the next 32 significant bits of the answer after
** the first 64.
**
** Algorithm:
**
** (1) For p between 0 and 26, return the value directly from the aBase[]
**     lookup table.
**
** (2) For p outside the range 0 to 26, use aScale[] for the initial value
**     then refine that result (if necessary) by a single multiplication
**     against aBase[].
**
** The constant tables aBase[], aScale[], and aScaleLo[] are generated
** by the C program at ../tool/mkfptab.c run with the --round option.
*/
static u64 powerOfTen(int p, u32 *pLo){
  static const u64 aBase[] = {
    UINT64_C(0x8000000000000000), /*  0: 1.0e+0 << 63 */
    UINT64_C(0xa000000000000000), /*  1: 1.0e+1 << 60 */
    UINT64_C(0xc800000000000000), /*  2: 1.0e+2 << 57 */
    UINT64_C(0xfa00000000000000), /*  3: 1.0e+3 << 54 */
    UINT64_C(0x9c40000000000000), /*  4: 1.0e+4 << 50 */
    UINT64_C(0xc350000000000000), /*  5: 1.0e+5 << 47 */
    UINT64_C(0xf424000000000000), /*  6: 1.0e+6 << 44 */
    UINT64_C(0x9896800000000000), /*  7: 1.0e+7 << 40 */
    UINT64_C(0xbebc200000000000), /*  8: 1.0e+8 << 37 */
    UINT64_C(0xee6b280000000000), /*  9: 1.0e+9 << 34 */
    UINT64_C(0x9502f90000000000), /* 10: 1.0e+10 << 30 */
    UINT64_C(0xba43b74000000000), /* 11: 1.0e+11 << 27 */
    UINT64_C(0xe8d4a51000000000), /* 12: 1.0e+12 << 24 */
    UINT64_C(0x9184e72a00000000), /* 13: 1.0e+13 << 20 */
    UINT64_C(0xb5e620f480000000), /* 14: 1.0e+14 << 17 */
    UINT64_C(0xe35fa931a0000000), /* 15: 1.0e+15 << 14 */
    UINT64_C(0x8e1bc9bf04000000), /* 16: 1.0e+16 << 10 */
    UINT64_C(0xb1a2bc2ec5000000), /* 17: 1.0e+17 << 7 */
    UINT64_C(0xde0b6b3a76400000), /* 18: 1.0e+18 << 4 */
    UINT64_C(0x8ac7230489e80000), /* 19: 1.0e+19 >> 0 */
    UINT64_C(0xad78ebc5ac620000), /* 20: 1.0e+20 >> 3 */
    UINT64_C(0xd8d726b7177a8000), /* 21: 1.0e+21 >> 6 */
    UINT64_C(0x878678326eac9000), /* 22: 1.0e+22 >> 10 */
    UINT64_C(0xa968163f0a57b400), /* 23: 1.0e+23 >> 13 */
    UINT64_C(0xd3c21bcecceda100), /* 24: 1.0e+24 >> 16 */
    UINT64_C(0x84595161401484a0), /* 25: 1.0e+25 >> 20 */
    UINT64_C(0xa56fa5b99019a5c8), /* 26: 1.0e+26 >> 23 */
  };
  static const u64 aScale[] = {
    UINT64_C(0x8049a4ac0c5811ae), /*  0: 1.0e-351 << 1229 */
    UINT64_C(0xcf42894a5dce35ea), /*  1: 1.0e-324 << 1140 */
    UINT64_C(0xa76c582338ed2621), /*  2: 1.0e-297 << 1050 */
    UINT64_C(0x873e4f75e2224e68), /*  3: 1.0e-270 << 960 */
    UINT64_C(0xda7f5bf590966848), /*  4: 1.0e-243 << 871 */
    UINT64_C(0xb080392cc4349dec), /*  5: 1.0e-216 << 781 */
    UINT64_C(0x8e938662882af53e), /*  6: 1.0e-189 << 691 */
    UINT64_C(0xe65829b3046b0afa), /*  7: 1.0e-162 << 602 */
    UINT64_C(0xba121a4650e4ddeb), /*  8: 1.0e-135 << 512 */
    UINT64_C(0x964e858c91ba2655), /*  9: 1.0e-108 << 422 */
    UINT64_C(0xf2d56790ab41c2a2), /* 10: 1.0e-81 << 333 */
    UINT64_C(0xc428d05aa4751e4c), /* 11: 1.0e-54 << 243 */
    UINT64_C(0x9e74d1b791e07e48), /* 12: 1.0e-27 << 153 */
    UINT64_C(0xcccccccccccccccc), /* 13: 1.0e-1 << 67 (special case) */
    UINT64_C(0xcecb8f27f4200f3a), /* 14: 1.0e+27 >> 26 */
    UINT64_C(0xa70c3c40a64e6c51), /* 15: 1.0e+54 >> 116 */
    UINT64_C(0x86f0ac99b4e8dafd), /* 16: 1.0e+81 >> 206 */
    UINT64_C(0xda01ee641a708de9), /* 17: 1.0e+108 >> 295 */
    UINT64_C(0xb01ae745b101e9e4), /* 18: 1.0e+135 >> 385 */
    UINT64_C(0x8e41ade9fbebc27d), /* 19: 1.0e+162 >> 475 */
    UINT64_C(0xe5d3ef282a242e81), /* 20: 1.0e+189 >> 564 */
    UINT64_C(0xb9a74a0637ce2ee1), /* 21: 1.0e+216 >> 654 */
    UINT64_C(0x95f83d0a1fb69cd9), /* 22: 1.0e+243 >> 744 */
    UINT64_C(0xf24a01a73cf2dccf), /* 23: 1.0e+270 >> 833 */
    UINT64_C(0xc3b8358109e84f07), /* 24: 1.0e+297 >> 923 */
    UINT64_C(0x9e19db92b4e31ba9), /* 25: 1.0e+324 >> 1013 */
  };
  static const unsigned int aScaleLo[] = {
    0x205b896d, /*  0: 1.0e-351 << 1229 */
    0x52064cad, /*  1: 1.0e-324 << 1140 */
    0xaf2af2b8, /*  2: 1.0e-297 << 1050 */
    0x5a7744a7, /*  3: 1.0e-270 << 960 */
    0xaf39a475, /*  4: 1.0e-243 << 871 */
    0xbd8d794e, /*  5: 1.0e-216 << 781 */
    0x547eb47b, /*  6: 1.0e-189 << 691 */
    0x0cb4a5a3, /*  7: 1.0e-162 << 602 */
    0x92f34d62, /*  8: 1.0e-135 << 512 */
    0x3a6a07f9, /*  9: 1.0e-108 << 422 */
    0xfae27299, /* 10: 1.0e-81 << 333 */
    0xaa97e14c, /* 11: 1.0e-54 << 243 */
    0x775ea265, /* 12: 1.0e-27 << 153 */
    0xcccccccc, /* 13: 1.0e-1 << 67 (special case) */
    0x00000000, /* 14: 1.0e+27 >> 26 */
    0x999090b6, /* 15: 1.0e+54 >> 116 */
    0x69a028bb, /* 16: 1.0e+81 >> 206 */
    0xe80e6f48, /* 17: 1.0e+108 >> 295 */
    0x5ec05dd0, /* 18: 1.0e+135 >> 385 */
    0x14588f14, /* 19: 1.0e+162 >> 475 */
    0x8f1668c9, /* 20: 1.0e+189 >> 564 */
    0x6d953e2c, /* 21: 1.0e+216 >> 654 */
    0x4abdaf10, /* 22: 1.0e+243 >> 744 */
    0xbc633b39, /* 23: 1.0e+270 >> 833 */
    0x0a862f81, /* 24: 1.0e+297 >> 923 */
    0x6c07a2c2, /* 25: 1.0e+324 >> 1013 */
  };
  int g, n;
  u64 s, x;
  u32 lo;

  assert( p>=POWERSOF10_FIRST && p<=POWERSOF10_LAST );
  if( p<0 ){
    if( p==(-1) ){
      *pLo = aScaleLo[13];
      return aScale[13];
    }
    g = p/27;
    n = p%27;
    if( n ){
      g--;
      n += 27;
    }
  }else if( p<27 ){
    *pLo = 0;
    return aBase[p];
  }else{
    g = p/27;
    n = p%27;
  }
  s = aScale[g+13];
  if( n==0 ){
    *pLo = aScaleLo[g+13];
    return s;
  }
  x = sqlite3Multiply160(s,aScaleLo[g+13],aBase[n],&lo);
  if( (U64_BIT(63) & x)==0 ){
    x  = x<<1 | ((lo>>31)&1);
    lo = (lo<<1) | 1;
  }
  *pLo = lo;
  return x;
}

/*
** pow10to2(x) computes floor(log2(pow(10,x))).
** pow2to10(y) computes floor(log10(pow(2,y))).
**
** Conceptually, pow10to2(p) converts a base-10 exponent p into
** a corresponding base-2 exponent, and pow2to10(e) converts a base-2
** exponent into a base-10 exponent.
**
** The conversions are based on the observation that:
**
**     ln(10.0)/ln(2.0) == 108853/32768     (approximately)
**     ln(2.0)/ln(10.0) == 78913/262144     (approximately)
**
** These ratios are approximate, but they are accurate to 5 digits,
** which is close enough for the usage here.  Right-shift is used
** for division so that rounding of negative numbers happens in the
** right direction.
*/
static int pwr10to2(int p){ return (p*108853) >> 15; }
static int pwr2to10(int p){ return (p*78913) >> 18; }

/*
** Count leading zeros for a 64-bit unsigned integer.
*/
static int countLeadingZeros(u64 m){
#if (defined(__GNUC__) || defined(__clang__)) \
    && !defined(SQLITE_DISABLE_INTRINSIC)
  return __builtin_clzll(m);
#else
  int n = 0;
  if( m <= 0x00000000ffffffffULL) { n += 32; m <<= 32; }
  if( m <= 0x0000ffffffffffffULL) { n += 16; m <<= 16; }
  if( m <= 0x00ffffffffffffffULL) { n += 8;  m <<= 8;  }
  if( m <= 0x0fffffffffffffffULL) { n += 4;  m <<= 4;  }
  if( m <= 0x3fffffffffffffffULL) { n += 2;  m <<= 2;  }
  if( m <= 0x7fffffffffffffffULL) { n += 1;            }
  return n;
#endif
}

/*
** Given m and e, which represent a quantity r == m*pow(2,e),
** return values *pD and *pP such that r == (*pD)*pow(10,*pP),
** approximately.  *pD should contain at least n significant digits.
**
** The input m is required to have its highest bit set.  In other words,
** m should be left-shifted, and e decremented, to maximize the value of m.
*/
static void sqlite3Fp2Convert10(u64 m, int e, int n, u64 *pD, int *pP){
  int p;
  u64 h, d1;
  u32 d2;
  assert( n>=1 && n<=18 );
  p = n - 1 - pwr2to10(e+63);
  h = sqlite3Multiply128(m, powerOfTen(p,&d2), &d1);
  assert( -(e + pwr10to2(p) + 2) >= 0  );
  assert( -(e + pwr10to2(p) + 1) <= 63 );
  if( n==18 ){
    h >>= -(e + pwr10to2(p) + 2);
    *pD = (h + ((h<<1)&2))>>1;
  }else{
    *pD = h >> -(e + pwr10to2(p) + 1);
  }
  *pP = -p;
}

/*
** Return an IEEE754 floating point value that approximates d*pow(10,p).
**
** The (current) algorithm is adapted from the work of Ross Cox at
** https://github.com/rsc/fpfmt
*/
static double sqlite3Fp10Convert2(u64 d, int p){
  int b, lp, e, adj, s;
  u32 pwr10l, mid1;
  u64 pwr10h, x, hi, lo, sticky, u, m;
  double r;
  if( p<POWERSOF10_FIRST ) return 0.0;
  if( p>POWERSOF10_LAST ) return (double)INFINITY;
  b = 64 - countLeadingZeros(d);
  lp = pwr10to2(p);
  e = 53 - b - lp;
  if( e > 1074 ){
    if( e>=1130 ) return 0.0;
    e = 1074;
  }
  s = -(e-(64-b) + lp + 3);
  pwr10h = powerOfTen(p, &pwr10l);
  if( pwr10l!=0 ){
    pwr10h++;
    pwr10l = ~pwr10l;
  }
  x = d<<(64-b);
  hi = sqlite3Multiply128(x,pwr10h,&lo);
  mid1 = lo>>32;
  sticky = 1;
  if( (hi & (U64_BIT(s)-1))==0 ) {
    u32 mid2 = sqlite3Multiply128(x,((u64)pwr10l)<<32,&lo)>>32;
    sticky = (mid1-mid2 > 1);
    hi -= mid1 < mid2;
  }
  u = (hi>>s) | sticky;
  adj = (u >= U64_BIT(55)-2);
  if( adj ){
    u = (u>>adj) | (u&1);
    e -= adj;
  }
  m = (u + 1 + ((u>>2)&1)) >> 2;
  if( e<=(-972) ) return (double)INFINITY;
  if((m & U64_BIT(52)) != 0){
    m = (m & ~U64_BIT(52)) | ((u64)(1075-e)<<52);
  }
  memcpy(&r,&m,8);
  return r;
}

/*
** The string z[] is an text representation of a real number.
** Convert this string to a double and write it into *pResult.
**
** z[] must be UTF-8 and zero-terminated.
**
** Return positive if the result is a valid real number (or integer) and
** zero or negative if the string is empty or contains extraneous text.
** Lower bits of the return value contain addition information about the
** parse:
**
**   bit 0       =>   Set if any prefix of the input is valid.  Clear if
**                    there is no prefix of the input that can be seen as
**                    a valid floating point number.
**   bit 1       =>   Set if the input contains a decimal point or eNNN
**                    clause.  Zero if the input is an integer.
**   bit 2       =>   The input is exactly 0.0, not an underflow from
**                    some value near zero.
**   bit 3       =>   Set if there are more than about 19 significant
**                    digits in the input.
**
** If the input contains a syntax error but begins with text that might
** be a valid number of some kind, then the result is negative.  The
** result is only zero if no prefix of the input could be interpreted as
** a number.
**
** Leading and trailing whitespace is ignored.  Valid numbers are in
** one of the formats below:
**
**    [+-]digits[E[+-]digits]
**    [+-]digits.[digits][E[+-]digits]
**    [+-].digits[E[+-]digits]
**
** Algorithm sketch:  Compute an unsigned 64-bit integer s and a base-10
** exponent d such that the value encoding by the input is s*pow(10,d).
** Then invoke sqlite3Fp10Convert2() to calculated the closest possible
** IEEE754 double.  The sign is added back afterwards, if the input string
** starts with a "-".  The use of an unsigned 64-bit s mantissa means that
** only about the first 19 significant digits of the input can contribute
** to the result.  This can result in suboptimal rounding decisions when
** correct rounding requires more than 19 input digits.  For example,
** this routine renders "3500000000000000.2500001" as
** 3500000000000000.0 instead of 3500000000000000.5 because the decision
** to round up instead of using banker's rounding to round down is determined
** by the 23rd significant digit, which this routine ignores. It is not
** possible to do better without some kind of BigNum.
*/
SQLITE_PRIVATE int sqlite3AtoF(const char *zIn, double *pResult){
#ifndef SQLITE_OMIT_FLOATING_POINT
  const unsigned char *z = (const unsigned char*)zIn;
  int neg = 0;       /* True for a negative value */
  u64 s = 0;         /* mantissa */
  int d = 0;         /* Value is s * pow(10,d) */
  int mState = 0;    /* 1: digit seen 2: fp 4: hard-zero */
  unsigned v;        /* Value of a single digit */

  start_of_text:
  if( (v = (unsigned)z[0] - '0')<10 ){
    parse_integer_part:
    mState = 1;
    s = v;
    z++;
    while( (v = (unsigned)z[0] - '0')<10 ){
      s = s*10 + v;
      z++;
      if( s>=(LARGEST_UINT64-9)/10 ){
        mState = 9;
        while( sqlite3Isdigit(z[0]) ){ z++; d++; }
        break;
      }
    }
  }else if( z[0]=='-' ){
    neg = 1;
    z++;
    if( (v = (unsigned)z[0] - '0')<10 ) goto parse_integer_part;
  }else if( z[0]=='+' ){
    z++;
    if( (v = (unsigned)z[0] - '0')<10 ) goto parse_integer_part;
  }else if( sqlite3Isspace(z[0]) ){
    do{ z++; }while( sqlite3Isspace(z[0]) );
    goto start_of_text;
  }else{
    s = 0;
  }

  /* if decimal point is present */
  if( *z=='.' ){
    z++;
    if( sqlite3Isdigit(z[0]) ){
      mState |= 1;
      do{
        if( s<(LARGEST_UINT64-9)/10 ){
          s = s*10 + z[0] - '0';
          d--;
        }else{
          mState = 11;
        }
      }while( sqlite3Isdigit(*++z) );
    }else if( mState==0 ){
      *pResult = 0.0;
      return 0;
    }
    mState |= 2;
  }else if( mState==0 ){
    *pResult = 0.0;
    return 0;
  }

  /* if exponent is present */
  if( *z=='e' || *z=='E' ){
    int esign;
    z++;

    /* get sign of exponent */
    if( *z=='-' ){
      esign = -1;
      z++;
    }else{
      esign = +1;
      if( *z=='+' ){
        z++;
      }
    }
    /* copy digits to exponent */
    if( (v = (unsigned)z[0] - '0')<10 ){
      int exp = v;
      z++;
      mState |= 2;
      while( (v = (unsigned)z[0] - '0')<10 ){
        exp = exp<10000 ? (exp*10 + v) : 10000;
        z++;
      }
      d += esign*exp;
    }else{
      z--;  /* Leave z[0] at 'e' or '+' or '-',
            ** so that the return is 0 or -1 */
    }
  }

  /* Convert s*pow(10,d) into real */
  if( s==0 ){
    *pResult = 0.0;
    mState |= 4;
  }else{
    *pResult = sqlite3Fp10Convert2(s,d);
  }
  if( neg ) *pResult = -*pResult;
  assert( !sqlite3IsNaN(*pResult) );

  /* return true if number and no extra non-whitespace characters after */
  if( z[0]==0 ){
    return mState;
  }
  if( sqlite3Isspace(z[0]) ){
    do{ z++; }while( sqlite3Isspace(*z) );
    if( z[0]==0 ){
      return mState;
    }
  }
  return 0xfffffff0 | mState;
#else
  return sqlite3Atoi64(zIn, pResult, strlen(zIn), SQLITE_UTF8)==0;
#endif /* SQLITE_OMIT_FLOATING_POINT */
}

/*
** Digit pairs used to convert a U64 or I64 into text, two digits
** at a time.
*/
static const union {
  char a[201];
  short int forceAlignment;
} sqlite3DigitPairs = {
  "00010203040506070809"
  "10111213141516171819"
  "20212223242526272829"
  "30313233343536373839"
  "40414243444546474849"
  "50515253545556575859"
  "60616263646566676869"
  "70717273747576777879"
  "80818283848586878889"
  "90919293949596979899"
};

/*
** ARMv6, ARMv7, PPC32 are known to not support hardware u64 division.
*/
#if (defined(__arm__) && !defined(__aarch64__)) || \
    (defined(__ppc__) && !defined(__ppc64__))
# define SQLITE_AVOID_U64_DIVIDE 1
#endif

#ifdef SQLITE_AVOID_U64_DIVIDE
/*
** Render an unsigned 64-bit integer as text onto the end of a 2-byte
** aligned buffer that is SQLITE_U64_DIGIT+1 bytes long.  The last byte
** of the buffer will be filled with a \000 byte.
**
** Return the index into the buffer of the first byte.
**
** This routine is used on platforms where u64-division is slow because
** it is not available in hardware and has to be emulated in software.
** It seeks to minimize the number of u64 divisions and use u32 divisions
** instead.  It is slower on platforms that have hardware u64 division,
** but much faster on platforms that do not.
*/
static int sqlite3UInt64ToText(u64 v, char *zOut){
  u32 x32, kk;
  int i;
  zOut[SQLITE_U64_DIGITS] = 0;
  i = SQLITE_U64_DIGITS;
  assert( TWO_BYTE_ALIGNMENT(&sqlite3DigitPairs.a[0]) );
  assert( TWO_BYTE_ALIGNMENT(zOut) );
  while( (v>>32)!=0 ){
    u32 y, x0, x1, y0, y1;
    x32 = v % 100000000;
    v   = v / 100000000;
    y = x32 % 10000;
    x32 /= 10000;
    x1 = x32 / 100;
    x0 = x32 % 100;
    y1 = y / 100;
    y0 = y % 100;
    assert( i>=8 );
    i -= 8;
    *(u16*)(&zOut[i]) = *(u16*)&sqlite3DigitPairs.a[x1*2];
    *(u16*)(&zOut[i+2]) = *(u16*)&sqlite3DigitPairs.a[x0*2];
    *(u16*)(&zOut[i+4]) = *(u16*)&sqlite3DigitPairs.a[y1*2];
    *(u16*)(&zOut[i+6]) = *(u16*)&sqlite3DigitPairs.a[y0*2];
  }
  x32 = v;
  while( x32>=10 ){
    kk  = x32 % 100;
    x32 = x32 / 100;
    assert( TWO_BYTE_ALIGNMENT(&sqlite3DigitPairs.a[kk*2]) );
    assert( i>=2 );
    i -= 2;
    assert( TWO_BYTE_ALIGNMENT(&zOut[i]) );
    *(u16*)(&zOut[i]) = *(u16*)&sqlite3DigitPairs.a[kk*2];
  }
  if( x32 ){
    assert( i>0 );
    zOut[--i] = x32 + '0';
  }
  return i;
}
#endif /* defined(SQLITE_AVOID_U64_DIVIDE) */

/*
** Render an signed 64-bit integer as text.  Store the result in zOut[] and
** return the length of the string that was stored, in bytes.  The value
** returned does not include the zero terminator at the end of the output
** string.
**
** The caller must ensure that zOut[] is at least 21 bytes in size.
*/
SQLITE_PRIVATE int sqlite3Int64ToText(i64 v, char *zOut){
  int i;
  u64 x;
  union {
    char a[SQLITE_U64_DIGITS+1];
    u16 forceAlignment;
  } u;
  if( v>0 ){
    x = v;
  }else if( v==0 ){
    zOut[0] = '0';
    zOut[1] = 0;
    return 1;
  }else{
    x = (v==SMALLEST_INT64) ? ((u64)1)<<63 : (u64)-v;
  }
#ifdef SQLITE_AVOID_U64_DIVIDE
  i = sqlite3UInt64ToText(x, u.a);
#else
  i = sizeof(u.a)-1;
  u.a[i] = 0;
  while( x>=10 ){
    int kk = (x%100)*2;
    assert( TWO_BYTE_ALIGNMENT(&sqlite3DigitPairs.a[kk]) );
    assert( TWO_BYTE_ALIGNMENT(&u.a[i-2]) );
    *(u16*)(&u.a[i-2]) = *(u16*)&sqlite3DigitPairs.a[kk];
    i -= 2;
    x /= 100;
  }
  if( x ){
    u.a[--i] = x + '0';
  }
#endif /* SQLITE_AVOID_U64_DIVIDE */
  if( v<0 ) u.a[--i] = '-';
  memcpy(zOut, &u.a[i], sizeof(u.a)-i);
  return sizeof(u.a)-1-i;
}

/*
** Compare the 19-character string zNum against the text representation
** value 2^63:  9223372036854775808.  Return negative, zero, or positive
** if zNum is less than, equal to, or greater than the string.
** Note that zNum must contain exactly 19 characters.
**
** Unlike memcmp() this routine is guaranteed to return the difference
** in the values of the last digit if the only difference is in the
** last digit.  So, for example,
**
**      compare2pow63("9223372036854775800", 1)
**
** will return -8.
*/
static int compare2pow63(const char *zNum, int incr){
  int c = 0;
  int i;
                    /* 012345678901234567 */
  const char *pow63 = "922337203685477580";
  for(i=0; c==0 && i<18; i++){
    c = (zNum[i*incr]-pow63[i])*10;
  }
  if( c==0 ){
    c = zNum[18*incr] - '8';
    testcase( c==(-1) );
    testcase( c==0 );
    testcase( c==(+1) );
  }
  return c;
}

/*
** Convert zNum to a 64-bit signed integer.  zNum must be decimal. This
** routine does *not* accept hexadecimal notation.
**
** Returns:
**
**    -1    Not even a prefix of the input text looks like an integer
**     0    Successful transformation.  Fits in a 64-bit signed integer.
**     1    Excess non-space text after the integer value
**     2    Integer too large for a 64-bit signed integer or is malformed
**     3    Special case of 9223372036854775808
**
** length is the number of bytes in the string (bytes, not characters).
** The string is not necessarily zero-terminated.  The encoding is
** given by enc.
*/
SQLITE_PRIVATE int sqlite3Atoi64(const char *zNum, i64 *pNum, int length, u8 enc){
  int incr;
  u64 u = 0;
  int neg = 0; /* assume positive */
  int i, j;
  unsigned int c = 0;
  int nonNum = 0;  /* True if input contains UTF16 with high byte non-zero */
  int rc;          /* Baseline return code */
  const char *zStart;
  const char *zEnd = zNum + length;
  assert( enc==SQLITE_UTF8 || enc==SQLITE_UTF16LE || enc==SQLITE_UTF16BE );
  if( enc==SQLITE_UTF8 ){
    incr = 1;
  }else{
    incr = 2;
    length &= ~1;
    assert( SQLITE_UTF16LE==2 && SQLITE_UTF16BE==3 );
    for(i=3-enc; i<length && zNum[i]==0; i+=2){}
    nonNum = i<length;
    zEnd = &zNum[i^1];
    zNum += (enc&1);
  }
  while( zNum<zEnd && sqlite3Isspace(*zNum) ) zNum+=incr;
  if( zNum<zEnd ){
    if( *zNum=='-' ){
      neg = 1;
      zNum+=incr;
    }else if( *zNum=='+' ){
      zNum+=incr;
    }
  }
  zStart = zNum;
  while( zNum<zEnd && zNum[0]=='0' ){ zNum+=incr; } /* Skip leading zeros. */
  for(i=0; &zNum[i]<zEnd && (c=(unsigned)zNum[i]-'0')<=9; i+=incr){
    u = u*10 + c;
  }
  testcase( i==18*incr );
  testcase( i==19*incr );
  testcase( i==20*incr );
  if( u>LARGEST_INT64 ){
    /* This test and assignment is needed only to suppress UB warnings
    ** from clang and -fsanitize=undefined.  This test and assignment make
    ** the code a little larger and slower, and no harm comes from omitting
    ** them, but we must appease the undefined-behavior pharisees. */
    *pNum = neg ? SMALLEST_INT64 : LARGEST_INT64;
  }else if( neg ){
    *pNum = -(i64)u;
  }else{
    *pNum = (i64)u;
  }
  rc = 0;
  if( i==0 && zStart==zNum ){    /* No digits */
    rc = -1;
  }else if( nonNum ){            /* UTF16 with high-order bytes non-zero */
    rc = 1;
  }else if( &zNum[i]<zEnd ){     /* Extra bytes at the end */
    int jj = i;
    do{
      if( !sqlite3Isspace(zNum[jj]) ){
        rc = 1;          /* Extra non-space text after the integer */
        break;
      }
      jj += incr;
    }while( &zNum[jj]<zEnd );
  }
  if( i<19*incr ){
    /* Less than 19 digits, so we know that it fits in 64 bits */
    assert( u<=LARGEST_INT64 );
    return rc;
  }else{
    /* zNum is a 19-digit numbers.  Compare it against 9223372036854775808. */
    j = i>19*incr ? 1 : compare2pow63(zNum, incr);
    if( j<0 ){
      /* zNum is less than 9223372036854775808 so it fits */
      assert( u<=LARGEST_INT64 );
      return rc;
    }else{
      *pNum = neg ? SMALLEST_INT64 : LARGEST_INT64;
      if( j>0 ){
        /* zNum is greater than 9223372036854775808 so it overflows */
        return 2;
      }else{
        /* zNum is exactly 9223372036854775808.  Fits if negative.  The
        ** special case 2 overflow if positive */
        assert( u-1==LARGEST_INT64 );
        return neg ? rc : 3;
      }
    }
  }
}

/*
** Transform a UTF-8 integer literal, in either decimal or hexadecimal,
** into a 64-bit signed integer.  This routine accepts hexadecimal literals,
** whereas sqlite3Atoi64() does not.
**
** Returns:
**
**     0    Successful transformation.  Fits in a 64-bit signed integer.
**     1    Excess text after the integer value
**     2    Integer too large for a 64-bit signed integer or is malformed
**     3    Special case of 9223372036854775808
*/
SQLITE_PRIVATE int sqlite3DecOrHexToI64(const char *z, i64 *pOut){
#ifndef SQLITE_OMIT_HEX_INTEGER
  if( z[0]=='0'
   && (z[1]=='x' || z[1]=='X')
  ){
    u64 u = 0;
    int i, k;
    for(i=2; z[i]=='0'; i++){}
    for(k=i; sqlite3Isxdigit(z[k]); k++){
      u = u*16 + sqlite3HexToInt(z[k]);
    }
    memcpy(pOut, &u, 8);
    if( k-i>16 ) return 2;
    if( z[k]!=0 ) return 1;
    return 0;
  }else
#endif /* SQLITE_OMIT_HEX_INTEGER */
  {
    int n = (int)(0x3fffffff&strspn(z,"+- \n\t0123456789"));
    if( z[n] ) n++;
    return sqlite3Atoi64(z, pOut, n, SQLITE_UTF8);
  }
}

/*
** If zNum represents an integer that will fit in 32-bits, then set
** *pValue to that integer and return true.  Otherwise return false.
**
** This routine accepts both decimal and hexadecimal notation for integers.
**
** Any non-numeric characters that following zNum are ignored.
** This is different from sqlite3Atoi64() which requires the
** input number to be zero-terminated.
*/
SQLITE_PRIVATE int sqlite3GetInt32(const char *zNum, int *pValue){
  sqlite_int64 v = 0;
  int i, c;
  int neg = 0;
  if( zNum[0]=='-' ){
    neg = 1;
    zNum++;
  }else if( zNum[0]=='+' ){
    zNum++;
  }
#ifndef SQLITE_OMIT_HEX_INTEGER
  else if( zNum[0]=='0'
        && (zNum[1]=='x' || zNum[1]=='X')
        && sqlite3Isxdigit(zNum[2])
  ){
    u32 u = 0;
    zNum += 2;
    while( zNum[0]=='0' ) zNum++;
    for(i=0; i<8 && sqlite3Isxdigit(zNum[i]); i++){
      u = u*16 + sqlite3HexToInt(zNum[i]);
    }
    if( (u&0x80000000)==0 && sqlite3Isxdigit(zNum[i])==0 ){
      memcpy(pValue, &u, 4);
      return 1;
    }else{
      return 0;
    }
  }
#endif
  if( !sqlite3Isdigit(zNum[0]) ) return 0;
  while( zNum[0]=='0' ) zNum++;
  for(i=0; i<11 && (c = zNum[i] - '0')>=0 && c<=9; i++){
    v = v*10 + c;
  }

  /* The longest decimal representation of a 32 bit integer is 10 digits:
  **
  **             1234567890
  **     2^31 -> 2147483648
  */
  testcase( i==10 );
  if( i>10 ){
    return 0;
  }
  testcase( v-neg==2147483647 );
  if( v-neg>2147483647 ){
    return 0;
  }
  if( neg ){
    v = -v;
  }
  *pValue = (int)v;
  return 1;
}

/*
** Return a 32-bit integer value extracted from a string.  If the
** string is not an integer, just return 0.
*/
SQLITE_PRIVATE int sqlite3Atoi(const char *z){
  int x = 0;
  sqlite3GetInt32(z, &x);
  return x;
}

/*
** Decode a floating-point value into an approximate decimal
** representation.
**
** If iRound<=0 then round to -iRound significant digits to the
** the right of the decimal point, or to a maximum of mxRound total
** significant digits.
**
** If iRound>0 round to min(iRound,mxRound) significant digits total.
**
** mxRound must be positive.
**
** The significant digits of the decimal representation are
** stored in p->z[] which is a often (but not always) a pointer
** into the middle of p->zBuf[].  There are p->n significant digits.
** The p->z[] array is *not* zero-terminated.
*/
SQLITE_PRIVATE void sqlite3FpDecode(FpDecode *p, double r, int iRound, int mxRound){
  int i;               /* Index into zBuf[] where to put next character */
  int n;               /* Number of digits */
  u64 v;               /* mantissa */
  int e, exp = 0;      /* Base-2 and base-10 exponent */
  char *zBuf;          /* Local alias for p->zBuf */
  char *z;             /* Local alias for p->z */

  p->isSpecial = 0;
  assert( mxRound>0 );

  /* Convert negative numbers to positive.  Deal with Infinity, 0.0, and
  ** NaN. */
  if( r<0.0 ){
    p->sign = '-';
    r = -r;
  }else if( r==0.0 ){
    p->sign = '+';
    p->n = 1;
    p->iDP = 1;
    p->z = "0";
    return;
  }else{
    p->sign = '+';
  }
  memcpy(&v,&r,8);
  e = (v>>52)&0x7ff;
  if( e==0x7ff ){
    p->isSpecial = 1 + (v!=0x7ff0000000000000LL);
    p->n = 0;
    p->iDP = 0;
    p->z = p->zBuf;
    return;
  }
  v &= 0x000fffffffffffffULL;
  if( e==0 ){
    int nn = countLeadingZeros(v);
    v <<= nn;
    e = -1074 - nn;
  }else{
    v = (v<<11) | U64_BIT(63);
    e -= 1086;
  }
  sqlite3Fp2Convert10(v, e, (iRound<=0||iRound>=18)?18:iRound+1, &v, &exp);

  /* Extract significant digits, start at the right-most slot in p->zBuf
  ** and working back to the right.  "i" keeps track of the next slot in
  ** which to store a digit. */
  assert( sizeof(p->zBuf)==SQLITE_U64_DIGITS+1 );
  assert( v>0 );
  zBuf = p->zBuf;
#ifdef SQLITE_AVOID_U64_DIVIDE
  i = sqlite3UInt64ToText(v, zBuf);
#else
  i = SQLITE_U64_DIGITS;
  while( v>=10 ){
    int kk = (v%100)*2;
    assert( TWO_BYTE_ALIGNMENT(&sqlite3DigitPairs.a[kk]) );
    assert( TWO_BYTE_ALIGNMENT(&zBuf[i]) );
    assert( i-2>=0 );
    *(u16*)(&zBuf[i-2]) = *(u16*)&sqlite3DigitPairs.a[kk];
    i -= 2;
    v /= 100;
  }
  if( v ){
    assert( v<10 );
    assert( i>0 );
    zBuf[--i] = v + '0';
  }
#endif /* SQLITE_AVOID_U64_DIVIDE */
  assert( i>=0 && i<SQLITE_U64_DIGITS );
  n = SQLITE_U64_DIGITS - i;  /* Total number of digits extracted */
  assert( n>0 );
  assert( n<=SQLITE_U64_DIGITS );
  p->iDP = n + exp;
  if( iRound<=0 ){
    iRound = p->iDP - iRound;
    if( iRound==0 && zBuf[i]>='5' ){
      iRound = 1;
      zBuf[--i] = '0';
      n++;
      p->iDP++;
    }
  }
  z = &zBuf[i];  /* z points to the first digit */
  if( iRound>0 && (iRound<n || n>mxRound) ){
    if( iRound>mxRound ) iRound = mxRound;
    if( iRound==17 ){
      /* If the precision is exactly 17, which only happens with the "!"
      ** flag (ex: "%!.17g") then try to reduce the precision if that
      ** yields text that will round-trip to the original floating-point.
      ** value.  Thus, for exaple, 49.47 will render as 49.47, rather than
      ** as 49.469999999999999. */
      if( z[15]=='9' && z[14]=='9' ){
        int jj, kk;
        u64 v2;
        for(jj=14; jj>0 && z[jj-1]=='9'; jj--){}
        if( jj==0 ){
          v2 = 1;
        }else{
          v2 = z[0] - '0';
          for(kk=1; kk<jj; kk++) v2 = (v2*10) + z[kk] - '0';
          v2++;
        }
        if( r==sqlite3Fp10Convert2(v2, exp + n - jj) ){
          iRound = jj+1;
        }
      }else if( p->iDP>=n || (z[15]=='0' && z[14]=='0' && z[13]=='0') ){
        int jj, kk;
        u64 v2;
        assert( z[0]!='0' );
        for(jj=13; z[jj-1]=='0'; jj--){}
        v2 = z[0] - '0';
        for(kk=1; kk<jj; kk++) v2 = (v2*10) + z[kk] - '0';
        if( r==sqlite3Fp10Convert2(v2, exp + n - jj) ){
          iRound = jj+1;
        }
      }
    }
    n = iRound;
    if( z[iRound]>='5' ){
      int j = iRound-1;
      while( 1 /*exit-by-break*/ ){
        z[j]++;
        if( z[j]<='9' ) break;
        z[j] = '0';
        if( j==0 ){
          z--;
          z[0] = '1';
          n++;
          p->iDP++;
          break;
        }else{
          j--;
        }
      }
    }
  }
  assert( n>0 );
  while( z[n-1]=='0' ){
    n--;
    assert( n>0 );
  }
  p->n = n;
  p->z = z;
}

/*
** Try to convert z into an unsigned 32-bit integer.  Return true on
** success and false if there is an error.
**
** Only decimal notation is accepted.
*/
SQLITE_PRIVATE int sqlite3GetUInt32(const char *z, u32 *pI){
  u64 v = 0;
  int i;
  for(i=0; sqlite3Isdigit(z[i]); i++){
    v = v*10 + z[i] - '0';
    if( v>4294967295LL ){ *pI = 0; return 0; }
  }
  if( i==0 || z[i]!=0 ){ *pI = 0; return 0; }
  *pI = (u32)v;
  return 1;
}

/*
** The variable-length integer encoding is as follows:
**
** KEY:
**         A = 0xxxxxxx    7 bits of data and one flag bit
**         B = 1xxxxxxx    7 bits of data and one flag bit
**         C = xxxxxxxx    8 bits of data
**
**  7 bits - A
** 14 bits - BA
** 21 bits - BBA
** 28 bits - BBBA
** 35 bits - BBBBA
** 42 bits - BBBBBA
** 49 bits - BBBBBBA
** 56 bits - BBBBBBBA
** 64 bits - BBBBBBBBC
*/

/*
** Write a 64-bit variable-length integer to memory starting at p[0].
** The length of data write will be between 1 and 9 bytes.  The number
** of bytes written is returned.
**
** A variable-length integer consists of the lower 7 bits of each byte
** for all bytes that have the 8th bit set and one byte with the 8th
** bit clear.  Except, if we get to the 9th byte, it stores the full
** 8 bits and is the last byte.
*/
static int SQLITE_NOINLINE putVarint64(unsigned char *p, u64 v){
  int i, j, n;
  u8 buf[10];
  if( v & (((u64)0xff000000)<<32) ){
    p[8] = (u8)v;
    v >>= 8;
    for(i=7; i>=0; i--){
      p[i] = (u8)((v & 0x7f) | 0x80);
      v >>= 7;
    }
    return 9;
  }
  n = 0;
  do{
    buf[n++] = (u8)((v & 0x7f) | 0x80);
    v >>= 7;
  }while( v!=0 );
  buf[0] &= 0x7f;
  assert( n<=9 );
  for(i=0, j=n-1; j>=0; j--, i++){
    p[i] = buf[j];
  }
  return n;
}
SQLITE_PRIVATE int sqlite3PutVarint(unsigned char *p, u64 v){
  if( v<=0x7f ){
    p[0] = v&0x7f;
    return 1;
  }
  if( v<=0x3fff ){
    p[0] = ((v>>7)&0x7f)|0x80;
    p[1] = v&0x7f;
    return 2;
  }
  return putVarint64(p,v);
}

/*
** Read a 64-bit variable-length integer from memory starting at p[0].
** Return the number of bytes read.  The value is stored in *v.
*/
SQLITE_PRIVATE u8 sqlite3GetVarint(const unsigned char *p, u64 *v){
  u64 iKey = p[0];
  const u8 *pStart = p;
  if( iKey>=0x80 ){
    u8 x;
    iKey = (iKey<<7) ^ (x = *++p);
    if( x>=0x80 ){
      iKey = (iKey<<7) ^ (x = *++p);
      if( x>=0x80 ){
        iKey = (iKey<<7) ^ 0x10204000 ^ (x = *++p);
        if( x>=0x80 ){
          iKey = (iKey<<7) ^ 0x4000 ^ (x = *++p);
          if( x>=0x80 ){
            iKey = (iKey<<7) ^ 0x4000 ^ (x = *++p);
            if( x>=0x80 ){
              iKey = (iKey<<7) ^ 0x4000 ^ (x = *++p);
              if( x>=0x80 ){
                iKey = (iKey<<7) ^ 0x4000 ^ (x = *++p);
                if( x>=0x80 ){
                  iKey = (iKey<<8) ^ 0x8000 ^ (*++p);
                }
              }
            }
          }
        }
      }else{
        iKey ^= 0x204000;
      }
    }else{
      iKey ^= 0x4000;
    }
  }
  *v = iKey;
  return (u8)(p - pStart) + 1;
}

/*
** Return the value of a variable-length integer without computing
** its length.  This is an optimization on sqlite3GetVarint() for the
** cases when the return value of sqlite3GetVarint() is not needed.
*/
SQLITE_PRIVATE i64 sqlite3VarintValue(const unsigned char *p){
  u64 iKey = p[0];
  if( iKey>=0x80 ){
    u8 x;
    iKey = (iKey<<7) ^ (x = *++p);
    if( x>=0x80 ){
      iKey = (iKey<<7) ^ (x = *++p);
      if( x>=0x80 ){
        iKey = (iKey<<7) ^ 0x10204000 ^ (x = *++p);
        if( x>=0x80 ){
          iKey = (iKey<<7) ^ 0x4000 ^ (x = *++p);
          if( x>=0x80 ){
            iKey = (iKey<<7) ^ 0x4000 ^ (x = *++p);
            if( x>=0x80 ){
              iKey = (iKey<<7) ^ 0x4000 ^ (x = *++p);
              if( x>=0x80 ){
                iKey = (iKey<<7) ^ 0x4000 ^ (x = *++p);
                if( x>=0x80 ){
                  iKey = (iKey<<8) ^ 0x8000 ^ (*++p);
                }
              }
            }
          }
        }
      }else{
        iKey ^= 0x204000;
      }
    }else{
      iKey ^= 0x4000;
    }
  }
  return *(i64*)&iKey;
}

/*
** Read a 32-bit variable-length integer from memory starting at p[0].
** Return the number of bytes read.  The value is stored in *v.
**
** If the varint stored in p[0] is larger than can fit in a 32-bit unsigned
** integer, then set *v to 0xffffffff.
**
** A MACRO version, getVarint32, is provided which inlines the
** single-byte case.  All code should use the MACRO version as
** this function assumes the single-byte case has already been handled.
*/
SQLITE_PRIVATE u8 sqlite3GetVarint32(const unsigned char *p, u32 *v){
  u64 v64;
  u8 n;

  /* Assume that the single-byte case has already been handled by
  ** the getVarint32() macro */
  assert( (p[0] & 0x80)!=0 );

  if( (p[1] & 0x80)==0 ){
    /* This is the two-byte case */
    *v = ((p[0]&0x7f)<<7) | p[1];
    return 2;
  }
  if( (p[2] & 0x80)==0 ){
    /* This is the three-byte case */
    *v = ((p[0]&0x7f)<<14) | ((p[1]&0x7f)<<7) | p[2];
    return 3;
  }
  /* four or more bytes */
  n = sqlite3GetVarint(p, &v64);
  assert( n>3 && n<=9 );
  if( (v64 & SQLITE_MAX_U32)!=v64 ){
    *v = 0xffffffff;
  }else{
    *v = (u32)v64;
  }
  return n;
}

/*
** Return the number of bytes that will be needed to store the given
** 64-bit integer.
*/
SQLITE_PRIVATE int sqlite3VarintLen(u64 v){
  int i;
  for(i=1; (v >>= 7)!=0; i++){ assert( i<10 ); }
  return i;
}


/*
** Read an unsigned 32-bit integer from an unaligned big-endian array of bytes.
*/
SQLITE_PRIVATE u32 sqlite3Get4byte(const u8 *p){
#if SQLITE_BYTEORDER==4321
  u32 x;
  memcpy(&x,p,4);
  return x;
#elif SQLITE_BYTEORDER==1234 && GCC_VERSION>=4003000
  u32 x;
  memcpy(&x,p,4);
  return __builtin_bswap32(x);
#elif SQLITE_BYTEORDER==1234 && MSVC_VERSION>=1300
  u32 x;
  memcpy(&x,p,4);
  return _byteswap_ulong(x);
#else
  /* Test this limb using -DSQLITE_BYTEORDER=0 */
  testcase( p[0]&0x80 );
  return ((unsigned)p[0]<<24) | (p[1]<<16) | (p[2]<<8) | p[3];
#endif
}

/* Write an unsigned 32-bit integer into an unaligned big-endian array of bytes.
*/
SQLITE_PRIVATE void sqlite3Put4byte(unsigned char *p, u32 v){
#if SQLITE_BYTEORDER==4321
  memcpy(p,&v,4);
#elif SQLITE_BYTEORDER==1234 && GCC_VERSION>=4003000
  u32 x = __builtin_bswap32(v);
  memcpy(p,&x,4);
#elif SQLITE_BYTEORDER==1234 && MSVC_VERSION>=1300
  u32 x = _byteswap_ulong(v);
  memcpy(p,&x,4);
#else
  /* Test this limb using -DSQLITE_BYTEORDER=0 */
  p[0] = (u8)(v>>24);
  p[1] = (u8)(v>>16);
  p[2] = (u8)(v>>8);
  p[3] = (u8)v;
#endif
}

/*
** Read an unsigned 64-bit integer from an unaligned big-endian byte array.
*/
SQLITE_PRIVATE SQLITE_OPT_INLINE u64 sqlite3Get8byte(const u8 *p){
#if SQLITE_BYTEORDER==4321
  u64 x;
  memcpy(&x,p,8);
  return x;
#elif SQLITE_BYTEORDER==1234 && GCC_VERSION>=4003000
  u64 x;
  memcpy(&x,p,8);
  return __builtin_bswap64(x);
#elif SQLITE_BYTEORDER==1234 && MSVC_VERSION>=1300
  u64 x;
  memcpy(&x,p,8);
  return _byteswap_uint64(x);
#else
  /* Test this limb using -DSQLITE_BYTEORDER=0 */
  testcase( p[0]&0x80 );
  return (u64)(
    (((u64)p[0]) << 56) +
    (((u64)p[1]) << 48) +
    (((u64)p[2]) << 40) +
    (((u64)p[3]) << 32) +
    (((u64)p[4]) << 24) +
    (((u64)p[5]) << 16) +
    (((u64)p[6]) <<  8) +
    (((u64)p[7]) <<  0)
  );
#endif
}

#if SQLITE_BYTEORDER!=4321  /* Only used for little-endian machines */
/*
** Byte-swap a 64-bit unsigned integer.
*/
SQLITE_PRIVATE SQLITE_OPT_INLINE u64 sqlite3BSwap64(u64 x){
#if SQLITE_BYTEORDER==1234 && GCC_VERSION>=4003000
  return __builtin_bswap64(x);
#elif SQLITE_BYTEORDER==1234 && MSVC_VERSION>=1300
  return _byteswap_uint64(x);
#else
  /* Test this limb using -DSQLITE_BYTEORDER=0 */
  x = (x << 32) | (x >> 32);
  x = ((x & UINT64_C(0x0000ffff0000ffff)) << 16) |
      ((x & UINT64_C(0xffff0000ffff0000)) >> 16);
  x = ((x & UINT64_C(0x00ff00ff00ff00ff)) <<  8) |
      ((x & UINT64_C(0xff00ff00ff00ff00)) >>  8);
  return x;
#endif
}
#endif /* SQLITE_BYTEORDER!=4321 */

/*
** Translate a single byte of Hex into an integer.
** This routine only works if h really is a valid hexadecimal
** character:  0..9a..fA..F
*/
SQLITE_PRIVATE u8 sqlite3HexToInt(int h){
  assert( (h>='0' && h<='9') ||  (h>='a' && h<='f') ||  (h>='A' && h<='F') );
#ifdef SQLITE_ASCII
  h += 9*(1&(h>>6));
#endif
#ifdef SQLITE_EBCDIC
  h += 9*(1&~(h>>4));
#endif
  return (u8)(h & 0xf);
}

#if !defined(SQLITE_OMIT_BLOB_LITERAL)
/*
** Convert a BLOB literal of the form "x'hhhhhh'" into its binary
** value.  Return a pointer to its binary value.  Space to hold the
** binary value has been obtained from malloc and must be freed by
** the calling routine.
*/
SQLITE_PRIVATE void *sqlite3HexToBlob(sqlite3 *db, const char *z, int n){
  char *zBlob;
  int i;

  zBlob = (char *)sqlite3DbMallocRawNN(db, n/2 + 1);
  n--;
  if( zBlob ){
    for(i=0; i<n; i+=2){
      zBlob[i/2] = (sqlite3HexToInt(z[i])<<4) | sqlite3HexToInt(z[i+1]);
    }
    zBlob[i/2] = 0;
  }
  return zBlob;
}
#endif /* !SQLITE_OMIT_BLOB_LITERAL */

/*
** Log an error that is an API call on a connection pointer that should
** not have been used.  The "type" of connection pointer is given as the
** argument.  The zType is a word like "NULL" or "closed" or "invalid".
*/
static void logBadConnection(const char *zType){
  sqlite3_log(SQLITE_MISUSE,
     "API call with %s database connection pointer",
     zType
  );
}

/*
** Check to make sure we have a valid db pointer.  This test is not
** foolproof but it does provide some measure of protection against
** misuse of the interface such as passing in db pointers that are
** NULL or which have been previously closed.  If this routine returns
** 1 it means that the db pointer is valid and 0 if it should not be
** dereferenced for any reason.  The calling function should invoke
** SQLITE_MISUSE immediately.
**
** sqlite3SafetyCheckOk() requires that the db pointer be valid for
** use.  sqlite3SafetyCheckSickOrOk() allows a db pointer that failed to
** open properly and is not fit for general use but which can be
** used as an argument to sqlite3_errmsg() or sqlite3_close().
*/
SQLITE_PRIVATE int sqlite3SafetyCheckOk(sqlite3 *db){
  u8 eOpenState;
  if( db==0 ){
    logBadConnection("NULL");
    return 0;
  }
  eOpenState = db->eOpenState;
  if( eOpenState!=SQLITE_STATE_OPEN ){
    if( sqlite3SafetyCheckSickOrOk(db) ){
      testcase( sqlite3GlobalConfig.xLog!=0 );
      logBadConnection("unopened");
    }
    return 0;
  }else{
    return 1;
  }
}
SQLITE_PRIVATE int sqlite3SafetyCheckSickOrOk(sqlite3 *db){
  u8 eOpenState;
  eOpenState = db->eOpenState;
  if( eOpenState!=SQLITE_STATE_SICK &&
      eOpenState!=SQLITE_STATE_OPEN &&
      eOpenState!=SQLITE_STATE_BUSY ){
    testcase( sqlite3GlobalConfig.xLog!=0 );
    logBadConnection("invalid");
    return 0;
  }else{
    return 1;
  }
}

/*
** Attempt to add, subtract, or multiply the 64-bit signed value iB against
** the other 64-bit signed integer at *pA and store the result in *pA.
** Return 0 on success.  Or if the operation would have resulted in an
** overflow, leave *pA unchanged and return 1.
*/
SQLITE_PRIVATE int sqlite3AddInt64(i64 *pA, i64 iB){
#if GCC_VERSION>=5004000 && !defined(__INTEL_COMPILER)
  return __builtin_add_overflow(*pA, iB, pA);
#else
  i64 iA = *pA;
  testcase( iA==0 ); testcase( iA==1 );
  testcase( iB==-1 ); testcase( iB==0 );
  if( iB>=0 ){
    testcase( iA>0 && LARGEST_INT64 - iA == iB );
    testcase( iA>0 && LARGEST_INT64 - iA == iB - 1 );
    if( iA>0 && LARGEST_INT64 - iA < iB ) return 1;
  }else{
    testcase( iA<0 && -(iA + LARGEST_INT64) == iB + 1 );
    testcase( iA<0 && -(iA + LARGEST_INT64) == iB + 2 );
    if( iA<0 && -(iA + LARGEST_INT64) > iB + 1 ) return 1;
  }
  *pA += iB;
  return 0;
#endif
}
SQLITE_PRIVATE int sqlite3SubInt64(i64 *pA, i64 iB){
#if GCC_VERSION>=5004000 && !defined(__INTEL_COMPILER)
  return __builtin_sub_overflow(*pA, iB, pA);
#else
  testcase( iB==SMALLEST_INT64+1 );
  if( iB==SMALLEST_INT64 ){
    testcase( (*pA)==(-1) ); testcase( (*pA)==0 );
    if( (*pA)>=0 ) return 1;
    *pA -= iB;
    return 0;
  }else{
    return sqlite3AddInt64(pA, -iB);
  }
#endif
}
SQLITE_PRIVATE int sqlite3MulInt64(i64 *pA, i64 iB){
#if GCC_VERSION>=5004000 && !defined(__INTEL_COMPILER)
  return __builtin_mul_overflow(*pA, iB, pA);
#else
  i64 iA = *pA;
  if( iB>0 ){
    if( iA>LARGEST_INT64/iB ) return 1;
    if( iA<SMALLEST_INT64/iB ) return 1;
  }else if( iB<0 ){
    if( iA>0 ){
      if( iB<SMALLEST_INT64/iA ) return 1;
    }else if( iA<0 ){
      if( iB==SMALLEST_INT64 ) return 1;
      if( iA==SMALLEST_INT64 ) return 1;
      if( -iA>LARGEST_INT64/-iB ) return 1;
    }
  }
  *pA = iA*iB;
  return 0;
#endif
}

/*
** Compute the absolute value of a 32-bit signed integer, if possible.  Or
** if the integer has a value of -2147483648, return +2147483647
*/
SQLITE_PRIVATE int sqlite3AbsInt32(int x){
  if( x>=0 ) return x;
  if( x==(int)0x80000000 ) return 0x7fffffff;
  return -x;
}

#ifdef SQLITE_ENABLE_8_3_NAMES
/*
** If SQLITE_ENABLE_8_3_NAMES is set at compile-time and if the database
** filename in zBaseFilename is a URI with the "8_3_names=1" parameter and
** if filename in z[] has a suffix (a.k.a. "extension") that is longer than
** three characters, then shorten the suffix on z[] to be the last three
** characters of the original suffix.
**
** If SQLITE_ENABLE_8_3_NAMES is set to 2 at compile-time, then always
** do the suffix shortening regardless of URI parameter.
**
** Examples:
**
**     test.db-journal    =>   test.nal
**     test.db-wal        =>   test.wal
**     test.db-shm        =>   test.shm
**     test.db-mj7f3319fa =>   test.9fa
*/
SQLITE_PRIVATE void sqlite3FileSuffix3(const char *zBaseFilename, char *z){
#if SQLITE_ENABLE_8_3_NAMES<2
  if( sqlite3_uri_boolean(zBaseFilename, "8_3_names", 0) )
#endif
  {
    int i, sz;
    sz = sqlite3Strlen30(z);
    for(i=sz-1; i>0 && z[i]!='/' && z[i]!='.'; i--){}
    if( z[i]=='.' && ALWAYS(sz>i+4) ) memmove(&z[i+1], &z[sz-3], 4);
  }
}
#endif

/*
** Find (an approximate) sum of two LogEst values.  This computation is
** not a simple "+" operator because LogEst is stored as a logarithmic
** value.
**
*/
SQLITE_PRIVATE LogEst sqlite3LogEstAdd(LogEst a, LogEst b){
  static const unsigned char x[] = {
     10, 10,                         /* 0,1 */
      9, 9,                          /* 2,3 */
      8, 8,                          /* 4,5 */
      7, 7, 7,                       /* 6,7,8 */
      6, 6, 6,                       /* 9,10,11 */
      5, 5, 5,                       /* 12-14 */
      4, 4, 4, 4,                    /* 15-18 */
      3, 3, 3, 3, 3, 3,              /* 19-24 */
      2, 2, 2, 2, 2, 2, 2,           /* 25-31 */
  };
  if( a>=b ){
    if( a>b+49 ) return a;
    if( a>b+31 ) return a+1;
    return a+x[a-b];
  }else{
    if( b>a+49 ) return b;
    if( b>a+31 ) return b+1;
    return b+x[b-a];
  }
}

/*
** Convert an integer into a LogEst.  In other words, compute an
** approximation for 10*log2(x).
*/
SQLITE_PRIVATE LogEst sqlite3LogEst(u64 x){
  static LogEst a[] = { 0, 2, 3, 5, 6, 7, 8, 9 };
  LogEst y = 40;
  if( x<8 ){
    if( x<2 ) return 0;
    while( x<8 ){  y -= 10; x <<= 1; }
  }else{
#if GCC_VERSION>=5004000
    int i = 60 - __builtin_clzll(x);
    y += i*10;
    x >>= i;
#else
    while( x>255 ){ y += 40; x >>= 4; }  /*OPTIMIZATION-IF-TRUE*/
    while( x>15 ){  y += 10; x >>= 1; }
#endif
  }
  return a[x&7] + y - 10;
}

/*
** Convert a double into a LogEst
** In other words, compute an approximation for 10*log2(x).
*/
SQLITE_PRIVATE LogEst sqlite3LogEstFromDouble(double x){
  u64 a;
  LogEst e;
  assert( sizeof(x)==8 && sizeof(a)==8 );
  if( x<=1 ) return 0;
  if( x<=2000000000 ) return sqlite3LogEst((u64)x);
  memcpy(&a, &x, 8);
  e = (a>>52) - 1022;
  return e*10;
}

/*
** Convert a LogEst into an integer.
*/
SQLITE_PRIVATE u64 sqlite3LogEstToInt(LogEst x){
  u64 n;
  n = x%10;
  x /= 10;
  if( n>=5 ) n -= 2;
  else if( n>=1 ) n -= 1;
  if( x>60 ) return (u64)LARGEST_INT64;
  return x>=3 ? (n+8)<<(x-3) : (n+8)>>(3-x);
}

/*
** Add a new name/number pair to a VList.  This might require that the
** VList object be reallocated, so return the new VList.  If an OOM
** error occurs, the original VList returned and the
** db->mallocFailed flag is set.
**
** A VList is really just an array of integers.  To destroy a VList,
** simply pass it to sqlite3DbFree().
**
** The first integer is the number of integers allocated for the whole
** VList.  The second integer is the number of integers actually used.
** Each name/number pair is encoded by subsequent groups of 3 or more
** integers.
**
** Each name/number pair starts with two integers which are the numeric
** value for the pair and the size of the name/number pair, respectively.
** The text name overlays one or more following integers.  The text name
** is always zero-terminated.
**
** Conceptually:
**
**    struct VList {
**      int nAlloc;   // Number of allocated slots
**      int nUsed;    // Number of used slots
**      struct VListEntry {
**        int iValue;    // Value for this entry
**        int nSlot;     // Slots used by this entry
**        // ... variable name goes here
**      } a[0];
**    }
**
** During code generation, pointers to the variable names within the
** VList are taken.  When that happens, nAlloc is set to zero as an
** indication that the VList may never again be enlarged, since the
** accompanying realloc() would invalidate the pointers.
*/
SQLITE_PRIVATE VList *sqlite3VListAdd(
  sqlite3 *db,           /* The database connection used for malloc() */
  VList *pIn,            /* The input VList.  Might be NULL */
  const char *zName,     /* Name of symbol to add */
  int nName,             /* Bytes of text in zName */
  int iVal               /* Value to associate with zName */
){
  int nInt;              /* number of sizeof(int) objects needed for zName */
  char *z;               /* Pointer to where zName will be stored */
  int i;                 /* Index in pIn[] where zName is stored */

  nInt = nName/4 + 3;
  assert( pIn==0 || pIn[0]>=3 );  /* Verify ok to add new elements */
  if( pIn==0 || pIn[1]+nInt > pIn[0] ){
    /* Enlarge the allocation */
    sqlite3_int64 nAlloc = (pIn ? 2*(sqlite3_int64)pIn[0] : 10) + nInt;
    VList *pOut = sqlite3DbRealloc(db, pIn, nAlloc*sizeof(int));
    if( pOut==0 ) return pIn;
    if( pIn==0 ) pOut[1] = 2;
    pIn = pOut;
    pIn[0] = nAlloc;
  }
  i = pIn[1];
  pIn[i] = iVal;
  pIn[i+1] = nInt;
  z = (char*)&pIn[i+2];
  pIn[1] = i+nInt;
  assert( pIn[1]<=pIn[0] );
  memcpy(z, zName, nName);
  z[nName] = 0;
  return pIn;
}

/*
** Return a pointer to the name of a variable in the given VList that
** has the value iVal.  Or return a NULL if there is no such variable in
** the list
*/
SQLITE_PRIVATE const char *sqlite3VListNumToName(VList *pIn, int iVal){
  int i, mx;
  if( pIn==0 ) return 0;
  mx = pIn[1];
  i = 2;
  do{
    if( pIn[i]==iVal ) return (char*)&pIn[i+2];
    i += pIn[i+1];
  }while( i<mx );
  return 0;
}

/*
** Return the number of the variable named zName, if it is in VList.
** or return 0 if there is no such variable.
*/
SQLITE_PRIVATE int sqlite3VListNameToNum(VList *pIn, const char *zName, int nName){
  int i, mx;
  if( pIn==0 ) return 0;
  mx = pIn[1];
  i = 2;
  do{
    const char *z = (const char*)&pIn[i+2];
    if( strncmp(z,zName,nName)==0 && z[nName]==0 ) return pIn[i];
    i += pIn[i+1];
  }while( i<mx );
  return 0;
}

/************** End of util.c ************************************************/
/************** Begin file hash.c ********************************************/
/*
** 2001 September 22
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
*************************************************************************
** This is the implementation of generic hash-tables
** used in SQLite.
*/
/* #include "sqliteInt.h" */
/* #include <assert.h> */

/* Turn bulk memory into a hash table object by initializing the
** fields of the Hash structure.
**
** "pNew" is a pointer to the hash table that is to be initialized.
*/
SQLITE_PRIVATE void sqlite3HashInit(Hash *pNew){
  assert( pNew!=0 );
  pNew->first = 0;
  pNew->count = 0;
  pNew->htsize = 0;
  pNew->ht = 0;
}

/* Remove all entries from a hash table.  Reclaim all memory.
** Call this routine to delete a hash table or to reset a hash table
** to the empty state.
*/
SQLITE_PRIVATE void sqlite3HashClear(Hash *pH){
  HashElem *elem;         /* For looping over all elements of the table */

  assert( pH!=0 );
  elem = pH->first;
  pH->first = 0;
  sqlite3_free(pH->ht);
  pH->ht = 0;
  pH->htsize = 0;
  while( elem ){
    HashElem *next_elem = elem->next;
    sqlite3_free(elem);
    elem = next_elem;
  }
  pH->count = 0;
}

/*
** The hashing function.
*/
static unsigned int strHash(const char *z){
  unsigned int h = 0;
  while( z[0] ){     /*OPTIMIZATION-IF-TRUE*/
    /* Knuth multiplicative hashing.  (Sorting & Searching, p. 510).
    ** 0x9e3779b1 is 2654435761 which is the closest prime number to
    ** (2**32)*golden_ratio, where golden_ratio = (sqrt(5) - 1)/2.
    **
    ** Only bits 0xdf for ASCII and bits 0xbf for EBCDIC each octet are
    ** hashed since the omitted bits determine the upper/lower case difference.
    */
#ifdef SQLITE_EBCDIC
    h += 0xbf & (unsigned char)*(z++);
#else
    h += 0xdf & (unsigned char)*(z++);
#endif
    h *= 0x9e3779b1;
  }
  return h;
}


/* Link pNew element into the hash table pH.  If pEntry!=0 then also
** insert pNew into the pEntry hash bucket.
*/
static void insertElement(
  Hash *pH,              /* The complete hash table */
  struct _ht *pEntry,    /* The entry into which pNew is inserted */
  HashElem *pNew         /* The element to be inserted */
){
  HashElem *pHead;       /* First element already in pEntry */
  if( pEntry ){
    pHead = pEntry->count ? pEntry->chain : 0;
    pEntry->count++;
    pEntry->chain = pNew;
  }else{
    pHead = 0;
  }
  if( pHead ){
    pNew->next = pHead;
    pNew->prev = pHead->prev;
    if( pHead->prev ){ pHead->prev->next = pNew; }
    else             { pH->first = pNew; }
    pHead->prev = pNew;
  }else{
    pNew->next = pH->first;
    if( pH->first ){ pH->first->prev = pNew; }
    pNew->prev = 0;
    pH->first = pNew;
  }
}


/* Resize the hash table so that it contains "new_size" buckets.
**
** The hash table might fail to resize if sqlite3_malloc() fails or
** if the new size is the same as the prior size.
** Return TRUE if the resize occurs and false if not.
*/
static int rehash(Hash *pH, unsigned int new_size){
  struct _ht *new_ht;            /* The new hash table */
  HashElem *elem, *next_elem;    /* For looping over existing elements */

#if SQLITE_MALLOC_SOFT_LIMIT>0
  if( new_size*sizeof(struct _ht)>SQLITE_MALLOC_SOFT_LIMIT ){
    new_size = SQLITE_MALLOC_SOFT_LIMIT/sizeof(struct _ht);
  }
  if( new_size==pH->htsize ) return 0;
#endif

  /* The inability to allocates space for a larger hash table is
  ** a performance hit but it is not a fatal error.  So mark the
  ** allocation as a benign. Use sqlite3Malloc()/memset(0) instead of
  ** sqlite3MallocZero() to make the allocation, as sqlite3MallocZero()
  ** only zeroes the requested number of bytes whereas this module will
  ** use the actual amount of space allocated for the hash table (which
  ** may be larger than the requested amount).
  */
  sqlite3BeginBenignMalloc();
  new_ht = (struct _ht *)sqlite3Malloc( new_size*sizeof(struct _ht) );
  sqlite3EndBenignMalloc();

  if( new_ht==0 ) return 0;
  sqlite3_free(pH->ht);
  pH->ht = new_ht;
  pH->htsize = new_size = sqlite3MallocSize(new_ht)/sizeof(struct _ht);
  memset(new_ht, 0, new_size*sizeof(struct _ht));
  for(elem=pH->first, pH->first=0; elem; elem = next_elem){
    next_elem = elem->next;
    insertElement(pH, &new_ht[elem->h % new_size], elem);
  }
  return 1;
}

/* This function (for internal use only) locates an element in an
** hash table that matches the given key.  If no element is found,
** a pointer to a static null element with HashElem.data==0 is returned.
** If pH is not NULL, then the hash for this key is written to *pH.
*/
static HashElem *findElementWithHash(
  const Hash *pH,     /* The pH to be searched */
  const char *pKey,   /* The key we are searching for */
  unsigned int *pHash /* Write the hash value here */
){
  HashElem *elem;                /* Used to loop thru the element list */
  unsigned int count;            /* Number of elements left to test */
  unsigned int h;                /* The computed hash */
  static HashElem nullElement = { 0, 0, 0, 0, 0 };

  h = strHash(pKey);
  if( pH->ht ){   /*OPTIMIZATION-IF-TRUE*/
    struct _ht *pEntry;
    pEntry = &pH->ht[h % pH->htsize];
    elem = pEntry->chain;
    count = pEntry->count;
  }else{
    elem = pH->first;
    count = pH->count;
  }
  if( pHash ) *pHash = h;
  while( count ){
    assert( elem!=0 );
    if( h==elem->h && sqlite3StrICmp(elem->pKey,pKey)==0 ){
      return elem;
    }
    elem = elem->next;
    count--;
  }
  return &nullElement;
}

/* Remove a single entry from the hash table given a pointer to that
** element and a hash on the element's key.
*/
static void removeElement(
  Hash *pH,         /* The pH containing "elem" */
  HashElem *elem    /* The element to be removed from the pH */
){
  struct _ht *pEntry;
  if( elem->prev ){
    elem->prev->next = elem->next;
  }else{
    pH->first = elem->next;
  }
  if( elem->next ){
    elem->next->prev = elem->prev;
  }
  if( pH->ht ){
    pEntry = &pH->ht[elem->h % pH->htsize];
    if( pEntry->chain==elem ){
      pEntry->chain = elem->next;
    }
    assert( pEntry->count>0 );
    pEntry->count--;
  }
  sqlite3_free( elem );
  pH->count--;
  if( pH->count==0 ){
    assert( pH->first==0 );
    assert( pH->count==0 );
    sqlite3HashClear(pH);
  }
}

/* Attempt to locate an element of the hash table pH with a key
** that matches pKey.  Return the data for this element if it is
** found, or NULL if there is no match.
*/
SQLITE_PRIVATE void *sqlite3HashFind(const Hash *pH, const char *pKey){
  assert( pH!=0 );
  assert( pKey!=0 );
  return findElementWithHash(pH, pKey, 0)->data;
}

/* Insert an element into the hash table pH.  The key is pKey
** and the data is "data".
**
** If no element exists with a matching key, then a new
** element is created and NULL is returned.
**
** If another element already exists with the same key, then the
** new data replaces the old data and the old data is returned.
** The key is not copied in this instance.  If a malloc fails, then
** the new data is returned and the hash table is unchanged.
**
** If the "data" parameter to this function is NULL, then the
** element corresponding to "key" is removed from the hash table.
*/
SQLITE_PRIVATE void *sqlite3HashInsert(Hash *pH, const char *pKey, void *data){
  unsigned int h;       /* the hash of the key modulo hash table size */
  HashElem *elem;       /* Used to loop thru the element list */
  HashElem *new_elem;   /* New element added to the pH */

  assert( pH!=0 );
  assert( pKey!=0 );
  elem = findElementWithHash(pH,pKey,&h);
  if( elem->data ){
    void *old_data = elem->data;
    if( data==0 ){
      removeElement(pH,elem);
    }else{
      elem->data = data;
      elem->pKey = pKey;
    }
    return old_data;
  }
  if( data==0 ) return 0;
  new_elem = (HashElem*)sqlite3Malloc( sizeof(HashElem) );
  if( new_elem==0 ) return data;
  new_elem->pKey = pKey;
  new_elem->h = h;
  new_elem->data = data;
  pH->count++;
  if( pH->count>=5 && pH->count > 2*pH->htsize ){
    rehash(pH, pH->count*3);
  }
  insertElement(pH, pH->ht ? &pH->ht[new_elem->h % pH->htsize] : 0, new_elem);
  return 0;
}


/************** End of hash.c ************************************************/
/************** Begin file opcodes.c *****************************************/
/* Automatically generated.  Do not edit */
/* See the tool/mkopcodec.tcl script for details. */
#if !defined(SQLITE_OMIT_EXPLAIN) \
 || defined(VDBE_PROFILE) \
 || defined(SQLITE_DEBUG)
#if defined(SQLITE_ENABLE_EXPLAIN_COMMENTS) || defined(SQLITE_DEBUG)
# define OpHelp(X) "\0" X
#else
# define OpHelp(X)
#endif
SQLITE_PRIVATE const char *sqlite3OpcodeName(int i){
 static const char *const azName[] = {
    /*   0 */ "Savepoint"        OpHelp(""),
    /*   1 */ "AutoCommit"       OpHelp(""),
    /*   2 */ "Transaction"      OpHelp(""),
    /*   3 */ "Checkpoint"       OpHelp(""),
    /*   4 */ "JournalMode"      OpHelp(""),
    /*   5 */ "Vacuum"           OpHelp(""),
    /*   6 */ "VFilter"          OpHelp("iplan=r[P3] zplan='P4'"),
    /*   7 */ "VUpdate"          OpHelp("data=r[P3@P2]"),
    /*   8 */ "Init"             OpHelp("Start at P2"),
    /*   9 */ "Goto"             OpHelp(""),
    /*  10 */ "Gosub"            OpHelp(""),
    /*  11 */ "InitCoroutine"    OpHelp(""),
    /*  12 */ "Yield"            OpHelp(""),
    /*  13 */ "MustBeInt"        OpHelp(""),
    /*  14 */ "Jump"             OpHelp(""),
    /*  15 */ "Once"             OpHelp(""),
    /*  16 */ "If"               OpHelp(""),
    /*  17 */ "IfNot"            OpHelp(""),
    /*  18 */ "IsType"           OpHelp("if typeof(P1.P3) in P5 goto P2"),
    /*  19 */ "Not"              OpHelp("r[P2]= !r[P1]"),
    /*  20 */ "IfNullRow"        OpHelp("if P1.nullRow then r[P3]=NULL, goto P2"),
    /*  21 */ "SeekLT"           OpHelp("key=r[P3@P4]"),
    /*  22 */ "SeekLE"           OpHelp("key=r[P3@P4]"),
    /*  23 */ "SeekGE"           OpHelp("key=r[P3@P4]"),
    /*  24 */ "SeekGT"           OpHelp("key=r[P3@P4]"),
    /*  25 */ "IfNoHope"         OpHelp("key=r[P3@P4]"),
    /*  26 */ "NoConflict"       OpHelp("key=r[P3@P4]"),
    /*  27 */ "NotFound"         OpHelp("key=r[P3@P4]"),
    /*  28 */ "Found"            OpHelp("key=r[P3@P4]"),
    /*  29 */ "SeekRowid"        OpHelp("intkey=r[P3]"),
    /*  30 */ "NotExists"        OpHelp("intkey=r[P3]"),
    /*  31 */ "Last"             OpHelp(""),
    /*  32 */ "IfSizeBetween"    OpHelp(""),
    /*  33 */ "SorterSort"       OpHelp(""),
    /*  34 */ "Sort"             OpHelp(""),
    /*  35 */ "Rewind"           OpHelp(""),
    /*  36 */ "IfEmpty"          OpHelp("if( empty(P1) ) goto P2"),
    /*  37 */ "SorterNext"       OpHelp(""),
    /*  38 */ "Prev"             OpHelp(""),
    /*  39 */ "Next"             OpHelp(""),
    /*  40 */ "IdxLE"            OpHelp("key=r[P3@P4]"),
    /*  41 */ "IdxGT"            OpHelp("key=r[P3@P4]"),
    /*  42 */ "IdxLT"            OpHelp("key=r[P3@P4]"),
    /*  43 */ "Or"               OpHelp("r[P3]=(r[P1] || r[P2])"),
    /*  44 */ "And"              OpHelp("r[P3]=(r[P1] && r[P2])"),
    /*  45 */ "IdxGE"            OpHelp("key=r[P3@P4]"),
    /*  46 */ "IFindKey"         OpHelp(""),
    /*  47 */ "RowSetRead"       OpHelp("r[P3]=rowset(P1)"),
    /*  48 */ "RowSetTest"       OpHelp("if r[P3] in rowset(P1) goto P2"),
    /*  49 */ "Program"          OpHelp(""),
    /*  50 */ "FkIfZero"         OpHelp("if fkctr[P1]==0 goto P2"),
    /*  51 */ "IsNull"           OpHelp("if r[P1]==NULL goto P2"),
    /*  52 */ "NotNull"          OpHelp("if r[P1]!=NULL goto P2"),
    /*  53 */ "Ne"               OpHelp("IF r[P3]!=r[P1]"),
    /*  54 */ "Eq"               OpHelp("IF r[P3]==r[P1]"),
    /*  55 */ "Gt"               OpHelp("IF r[P3]>r[P1]"),
    /*  56 */ "Le"               OpHelp("IF r[P3]<=r[P1]"),
    /*  57 */ "Lt"               OpHelp("IF r[P3]<r[P1]"),
    /*  58 */ "Ge"               OpHelp("IF r[P3]>=r[P1]"),
    /*  59 */ "ElseEq"           OpHelp(""),
    /*  60 */ "IfPos"            OpHelp("if r[P1]>0 then r[P1]-=P3, goto P2"),
    /*  61 */ "IfNotZero"        OpHelp("if r[P1]!=0 then r[P1]--, goto P2"),
    /*  62 */ "DecrJumpZero"     OpHelp("if (--r[P1])==0 goto P2"),
    /*  63 */ "IncrVacuum"       OpHelp(""),
    /*  64 */ "VNext"            OpHelp(""),
    /*  65 */ "Filter"           OpHelp("if key(P3@P4) not in filter(P1) goto P2"),
    /*  66 */ "PureFunc"         OpHelp("r[P3]=func(r[P2@NP])"),
    /*  67 */ "Function"         OpHelp("r[P3]=func(r[P2@NP])"),
    /*  68 */ "Return"           OpHelp(""),
    /*  69 */ "EndCoroutine"     OpHelp(""),
    /*  70 */ "HaltIfNull"       OpHelp("if r[P3]=null halt"),
    /*  71 */ "Halt"             OpHelp(""),
    /*  72 */ "Integer"          OpHelp("r[P2]=P1"),
    /*  73 */ "Int64"            OpHelp("r[P2]=PINT13"),
    /*  74 */ "String"           OpHelp("r[P2]='P4' (len=P1)"),
    /*  75 */ "BeginSubrtn"      OpHelp("r[P2]=NULL"),
    /*  76 */ "Null"             OpHelp("r[P2..P3]=NULL"),
    /*  77 */ "SoftNull"         OpHelp("r[P1]=NULL"),
    /*  78 */ "Blob"             OpHelp("r[P2]=P4 (len=P1)"),
    /*  79 */ "Variable"         OpHelp("r[P2]=parameter(P1)"),
    /*  80 */ "Move"             OpHelp("r[P2@P3]=r[P1@P3]"),
    /*  81 */ "Copy"             OpHelp("r[P2@P3+1]=r[P1@P3+1]"),
    /*  82 */ "SCopy"            OpHelp("r[P2]=r[P1]"),
    /*  83 */ "IntCopy"          OpHelp("r[P2]=r[P1]"),
    /*  84 */ "FkCheck"          OpHelp(""),
    /*  85 */ "ResultRow"        OpHelp("output=r[P1@P2]"),
    /*  86 */ "CollSeq"          OpHelp(""),
    /*  87 */ "AddImm"           OpHelp("r[P1]=r[P1]+P2"),
    /*  88 */ "RealAffinity"     OpHelp(""),
    /*  89 */ "Cast"             OpHelp("affinity(r[P1])"),
    /*  90 */ "Permutation"      OpHelp(""),
    /*  91 */ "Compare"          OpHelp("r[P1@P3] <-> r[P2@P3]"),
    /*  92 */ "IsTrue"           OpHelp("r[P2] = coalesce(r[P1]==TRUE,P3) ^ P4"),
    /*  93 */ "ZeroOrNull"       OpHelp("r[P2] = 0 OR NULL"),
    /*  94 */ "Offset"           OpHelp("r[P3] = sqlite_offset(P1)"),
    /*  95 */ "Column"           OpHelp("r[P3]=PX cursor P1 column P2"),
    /*  96 */ "TypeCheck"        OpHelp("typecheck(r[P1@P2])"),
    /*  97 */ "Affinity"         OpHelp("affinity(r[P1@P2])"),
    /*  98 */ "MakeRecord"       OpHelp("r[P3]=mkrec(r[P1@P2])"),
    /*  99 */ "Count"            OpHelp("r[P2]=count()"),
    /* 100 */ "ReadCookie"       OpHelp(""),
    /* 101 */ "SetCookie"        OpHelp(""),
    /* 102 */ "ReopenIdx"        OpHelp("root=P2 iDb=P3"),
    /* 103 */ "BitAnd"           OpHelp("r[P3]=r[P1]&r[P2]"),
    /* 104 */ "BitOr"            OpHelp("r[P3]=r[P1]|r[P2]"),
    /* 105 */ "ShiftLeft"        OpHelp("r[P3]=r[P2]<<r[P1]"),
    /* 106 */ "ShiftRight"       OpHelp("r[P3]=r[P2]>>r[P1]"),
    /* 107 */ "Add"              OpHelp("r[P3]=r[P1]+r[P2]"),
    /* 108 */ "Subtract"         OpHelp("r[P3]=r[P2]-r[P1]"),
    /* 109 */ "Multiply"         OpHelp("r[P3]=r[P1]*r[P2]"),
    /* 110 */ "Divide"           OpHelp("r[P3]=r[P2]/r[P1]"),
    /* 111 */ "Remainder"        OpHelp("r[P3]=r[P2]%r[P1]"),
    /* 112 */ "Concat"           OpHelp("r[P3]=r[P2]+r[P1]"),
    /* 113 */ "OpenRead"         OpHelp("root=P2 iDb=P3"),
    /* 114 */ "OpenWrite"        OpHelp("root=P2 iDb=P3"),
    /* 115 */ "BitNot"           OpHelp("r[P2]= ~r[P1]"),
    /* 116 */ "OpenDup"          OpHelp(""),
    /* 117 */ "OpenAutoindex"    OpHelp("nColumn=P2"),
    /* 118 */ "String8"          OpHelp("r[P2]='P4'"),
    /* 119 */ "OpenEphemeral"    OpHelp("nColumn=P2"),
    /* 120 */ "SorterOpen"       OpHelp(""),
    /* 121 */ "SequenceTest"     OpHelp("if( cursor[P1].ctr++ ) pc = P2"),
    /* 122 */ "OpenPseudo"       OpHelp("P3 columns in r[P2]"),
    /* 123 */ "Close"            OpHelp(""),
    /* 124 */ "ColumnsUsed"      OpHelp("Cursor P1 uses columns PHEX23"),
    /* 125 */ "SeekScan"         OpHelp("Scan-ahead up to P1 rows"),
    /* 126 */ "SeekHit"          OpHelp("set P2<=seekHit<=P3"),
    /* 127 */ "Sequence"         OpHelp("r[P2]=cursor[P1].ctr++"),
    /* 128 */ "NewRowid"         OpHelp("r[P2]=rowid"),
    /* 129 */ "Insert"           OpHelp("intkey=r[P3] data=r[P2]"),
    /* 130 */ "RowCell"          OpHelp(""),
    /* 131 */ "Delete"           OpHelp(""),
    /* 132 */ "ResetCount"       OpHelp(""),
    /* 133 */ "SorterCompare"    OpHelp("if key(P1)!=trim(r[P3],P4) goto P2"),
    /* 134 */ "SorterData"       OpHelp("r[P2]=data"),
    /* 135 */ "RowData"          OpHelp("r[P2]=data"),
    /* 136 */ "Rowid"            OpHelp("r[P2]=PX rowid of P1"),
    /* 137 */ "NullRow"          OpHelp(""),
    /* 138 */ "SeekEnd"          OpHelp(""),
    /* 139 */ "IdxInsert"        OpHelp("key=r[P2]"),
    /* 140 */ "SorterInsert"     OpHelp("key=r[P2]"),
    /* 141 */ "IdxDelete"        OpHelp("key=r[P2@P5]"),
    /* 142 */ "DeferredSeek"     OpHelp("Move P3 to P1.rowid if needed"),
    /* 143 */ "IdxRowid"         OpHelp("r[P2]=rowid"),
    /* 144 */ "FinishSeek"       OpHelp(""),
    /* 145 */ "Destroy"          OpHelp(""),
    /* 146 */ "Clear"            OpHelp(""),
    /* 147 */ "ResetSorter"      OpHelp(""),
    /* 148 */ "CreateBtree"      OpHelp("r[P2]=root iDb=P1 flags=P3"),
    /* 149 */ "SqlExec"          OpHelp(""),
    /* 150 */ "ParseSchema"      OpHelp(""),
    /* 151 */ "LoadAnalysis"     OpHelp(""),
    /* 152 */ "DropTable"        OpHelp(""),
    /* 153 */ "DropIndex"        OpHelp(""),
    /* 154 */ "Real"             OpHelp("r[P2]=PDBL13"),
    /* 155 */ "DropTrigger"      OpHelp(""),
    /* 156 */ "IntegrityCk"      OpHelp(""),
    /* 157 */ "RowSetAdd"        OpHelp("rowset(P1)=r[P2]"),
    /* 158 */ "Param"            OpHelp(""),
    /* 159 */ "FkCounter"        OpHelp("fkctr[P1]+=P2"),
    /* 160 */ "MemMax"           OpHelp("r[P1]=max(r[P1],r[P2])"),
    /* 161 */ "OffsetLimit"      OpHelp("if r[P1]>0 then r[P2]=r[P1]+max(0,r[P3]) else r[P2]=(-1)"),
    /* 162 */ "AggInverse"       OpHelp("accum=r[P3] inverse(r[P2@P5])"),
    /* 163 */ "AggStep"          OpHelp("accum=r[P3] step(r[P2@P5])"),
    /* 164 */ "AggStep1"         OpHelp("accum=r[P3] step(r[P2@P5])"),
    /* 165 */ "AggValue"         OpHelp("r[P3]=value N=P2"),
    /* 166 */ "AggFinal"         OpHelp("accum=r[P1] N=P2"),
    /* 167 */ "Expire"           OpHelp(""),
    /* 168 */ "CursorLock"       OpHelp(""),
    /* 169 */ "CursorUnlock"     OpHelp(""),
    /* 170 */ "TableLock"        OpHelp("iDb=P1 root=P2 write=P3"),
    /* 171 */ "VBegin"           OpHelp(""),
    /* 172 */ "VCreate"          OpHelp(""),
    /* 173 */ "VDestroy"         OpHelp(""),
    /* 174 */ "VOpen"            OpHelp(""),
    /* 175 */ "VCheck"           OpHelp(""),
    /* 176 */ "VInitIn"          OpHelp("r[P2]=ValueList(P1,P3)"),
    /* 177 */ "VColumn"          OpHelp("r[P3]=vcolumn(P2)"),
    /* 178 */ "VRename"          OpHelp(""),
    /* 179 */ "Pagecount"        OpHelp(""),
    /* 180 */ "MaxPgcnt"         OpHelp(""),
    /* 181 */ "ClrSubtype"       OpHelp("r[P1].subtype = 0"),
    /* 182 */ "GetSubtype"       OpHelp("r[P2] = r[P1].subtype"),
    /* 183 */ "SetSubtype"       OpHelp("r[P2].subtype = r[P1]"),
    /* 184 */ "FilterAdd"        OpHelp("filter(P1) += key(P3@P4)"),
    /* 185 */ "Trace"            OpHelp(""),
    /* 186 */ "CursorHint"       OpHelp(""),
    /* 187 */ "ReleaseReg"       OpHelp("release r[P1@P2] mask P3"),
    /* 188 */ "Noop"             OpHelp(""),
    /* 189 */ "Explain"          OpHelp(""),
    /* 190 */ "Abortable"        OpHelp(""),
  };
  return azName[i];
}
#endif

/************** End of opcodes.c *********************************************/
