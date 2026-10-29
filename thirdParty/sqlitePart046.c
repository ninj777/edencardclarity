/************** Begin file fts3_aux.c ****************************************/
/*
** 2011 Jan 27
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
*/
/* #include "fts3Int.h" */
#if !defined(SQLITE_CORE) || defined(SQLITE_ENABLE_FTS3)

/* #include <string.h> */
/* #include <assert.h> */

typedef struct Fts3auxTable Fts3auxTable;
typedef struct Fts3auxCursor Fts3auxCursor;

struct Fts3auxTable {
  sqlite3_vtab base;              /* Base class used by SQLite core */
  Fts3Table *pFts3Tab;
};

struct Fts3auxCursor {
  sqlite3_vtab_cursor base;       /* Base class used by SQLite core */
  Fts3MultiSegReader csr;        /* Must be right after "base" */
  Fts3SegFilter filter;
  char *zStop;
  int nStop;                      /* Byte-length of string zStop */
  int iLangid;                    /* Language id to query */
  int isEof;                      /* True if cursor is at EOF */
  sqlite3_int64 iRowid;           /* Current rowid */

  int iCol;                       /* Current value of 'col' column */
  int nStat;                      /* Size of aStat[] array */
  struct Fts3auxColstats {
    sqlite3_int64 nDoc;           /* 'documents' values for current csr row */
    sqlite3_int64 nOcc;           /* 'occurrences' values for current csr row */
  } *aStat;
};

/*
** Schema of the terms table.
*/
#define FTS3_AUX_SCHEMA \
  "CREATE TABLE x(term, col, documents, occurrences, languageid HIDDEN)"

/*
** This function does all the work for both the xConnect and xCreate methods.
** These tables have no persistent representation of their own, so xConnect
** and xCreate are identical operations.
*/
static int fts3auxConnectMethod(
  sqlite3 *db,                    /* Database connection */
  void *pUnused,                  /* Unused */
  int argc,                       /* Number of elements in argv array */
  const char * const *argv,       /* xCreate/xConnect argument array */
  sqlite3_vtab **ppVtab,          /* OUT: New sqlite3_vtab object */
  char **pzErr                    /* OUT: sqlite3_malloc'd error message */
){
  char const *zDb;                /* Name of database (e.g. "main") */
  char const *zFts3;              /* Name of fts3 table */
  int nDb;                        /* Result of strlen(zDb) */
  int nFts3;                      /* Result of strlen(zFts3) */
  sqlite3_int64 nByte;            /* Bytes of space to allocate here */
  int rc;                         /* value returned by declare_vtab() */
  Fts3auxTable *p;                /* Virtual table object to return */

  UNUSED_PARAMETER(pUnused);

  /* The user should invoke this in one of two forms:
  **
  **     CREATE VIRTUAL TABLE xxx USING fts4aux(fts4-table);
  **     CREATE VIRTUAL TABLE xxx USING fts4aux(fts4-table-db, fts4-table);
  */
  if( argc!=4 && argc!=5 ) goto bad_args;

  zDb = argv[1];
  nDb = (int)strlen(zDb);
  if( argc==5 ){
    if( nDb==4 && 0==sqlite3_strnicmp("temp", zDb, 4) ){
      zDb = argv[3];
      nDb = (int)strlen(zDb);
      zFts3 = argv[4];
    }else{
      goto bad_args;
    }
  }else{
    zFts3 = argv[3];
  }
  nFts3 = (int)strlen(zFts3);

  rc = sqlite3_declare_vtab(db, FTS3_AUX_SCHEMA);
  if( rc!=SQLITE_OK ) return rc;

  nByte = sizeof(Fts3auxTable) + sizeof(Fts3Table) + nDb + nFts3 + 2;
  p = (Fts3auxTable *)sqlite3_malloc64(nByte);
  if( !p ) return SQLITE_NOMEM;
  memset(p, 0, nByte);

  p->pFts3Tab = (Fts3Table *)&p[1];
  p->pFts3Tab->zDb = (char *)&p->pFts3Tab[1];
  p->pFts3Tab->zName = &p->pFts3Tab->zDb[nDb+1];
  p->pFts3Tab->db = db;
  p->pFts3Tab->nIndex = 1;

  memcpy((char *)p->pFts3Tab->zDb, zDb, nDb);
  memcpy((char *)p->pFts3Tab->zName, zFts3, nFts3);
  sqlite3Fts3Dequote((char *)p->pFts3Tab->zName);

  *ppVtab = (sqlite3_vtab *)p;
  return SQLITE_OK;

 bad_args:
  sqlite3Fts3ErrMsg(pzErr, "invalid arguments to fts4aux constructor");
  return SQLITE_ERROR;
}

/*
** This function does the work for both the xDisconnect and xDestroy methods.
** These tables have no persistent representation of their own, so xDisconnect
** and xDestroy are identical operations.
*/
static int fts3auxDisconnectMethod(sqlite3_vtab *pVtab){
  Fts3auxTable *p = (Fts3auxTable *)pVtab;
  Fts3Table *pFts3 = p->pFts3Tab;
  int i;

  /* Free any prepared statements held */
  for(i=0; i<SizeofArray(pFts3->aStmt); i++){
    sqlite3_finalize(pFts3->aStmt[i]);
  }
  sqlite3_free(pFts3->zSegmentsTbl);
  sqlite3_free(p);
  return SQLITE_OK;
}

#define FTS4AUX_EQ_CONSTRAINT 1
#define FTS4AUX_GE_CONSTRAINT 2
#define FTS4AUX_LE_CONSTRAINT 4

/*
** Return true if constraint iCons of pInfo uses the "binary" collation
** sequence. Or false it it uses anything else.
*/
static int fts3auxIsBinary(sqlite3_index_info *pInfo, int iCons){
  return 0==sqlite3_stricmp("binary", sqlite3_vtab_collation(pInfo, iCons));
}

/*
** xBestIndex - Analyze a WHERE and ORDER BY clause.
*/
static int fts3auxBestIndexMethod(
  sqlite3_vtab *pVTab,
  sqlite3_index_info *pInfo
){
  int i;
  int iEq = -1;
  int iGe = -1;
  int iLe = -1;
  int iLangid = -1;
  int iNext = 1;                  /* Next free argvIndex value */

  UNUSED_PARAMETER(pVTab);

  /* This vtab delivers always results in "ORDER BY term ASC" order. */
  if( pInfo->nOrderBy==1
   && pInfo->aOrderBy[0].iColumn==0
   && pInfo->aOrderBy[0].desc==0
  ){
    pInfo->orderByConsumed = 1;
  }

  /* Search for equality and range constraints on the "term" column.
  ** And equality constraints on the hidden "languageid" column. */
  for(i=0; i<pInfo->nConstraint; i++){
    if( pInfo->aConstraint[i].usable && fts3auxIsBinary(pInfo, i) ){
      int op = pInfo->aConstraint[i].op;
      int iCol = pInfo->aConstraint[i].iColumn;

      if( iCol==0 ){
        if( op==SQLITE_INDEX_CONSTRAINT_EQ ) iEq = i;
        if( op==SQLITE_INDEX_CONSTRAINT_LT ) iLe = i;
        if( op==SQLITE_INDEX_CONSTRAINT_LE ) iLe = i;
        if( op==SQLITE_INDEX_CONSTRAINT_GT ) iGe = i;
        if( op==SQLITE_INDEX_CONSTRAINT_GE ) iGe = i;
      }
      if( iCol==4 ){
        if( op==SQLITE_INDEX_CONSTRAINT_EQ ) iLangid = i;
      }
    }
  }

  if( iEq>=0 ){
    pInfo->idxNum = FTS4AUX_EQ_CONSTRAINT;
    pInfo->aConstraintUsage[iEq].argvIndex = iNext++;
    pInfo->estimatedCost = 5;
  }else{
    pInfo->idxNum = 0;
    pInfo->estimatedCost = 20000;
    if( iGe>=0 ){
      pInfo->idxNum += FTS4AUX_GE_CONSTRAINT;
      pInfo->aConstraintUsage[iGe].argvIndex = iNext++;
      pInfo->estimatedCost /= 2;
    }
    if( iLe>=0 ){
      pInfo->idxNum += FTS4AUX_LE_CONSTRAINT;
      pInfo->aConstraintUsage[iLe].argvIndex = iNext++;
      pInfo->estimatedCost /= 2;
    }
  }
  if( iLangid>=0 ){
    pInfo->aConstraintUsage[iLangid].argvIndex = iNext++;
    pInfo->estimatedCost--;
  }

  return SQLITE_OK;
}

/*
** xOpen - Open a cursor.
*/
static int fts3auxOpenMethod(sqlite3_vtab *pVTab, sqlite3_vtab_cursor **ppCsr){
  Fts3auxCursor *pCsr;            /* Pointer to cursor object to return */

  UNUSED_PARAMETER(pVTab);

  pCsr = (Fts3auxCursor *)sqlite3_malloc(sizeof(Fts3auxCursor));
  if( !pCsr ) return SQLITE_NOMEM;
  memset(pCsr, 0, sizeof(Fts3auxCursor));

  *ppCsr = (sqlite3_vtab_cursor *)pCsr;
  return SQLITE_OK;
}

/*
** xClose - Close a cursor.
*/
static int fts3auxCloseMethod(sqlite3_vtab_cursor *pCursor){
  Fts3Table *pFts3 = ((Fts3auxTable *)pCursor->pVtab)->pFts3Tab;
  Fts3auxCursor *pCsr = (Fts3auxCursor *)pCursor;

  sqlite3Fts3SegmentsClose(pFts3);
  sqlite3Fts3SegReaderFinish(&pCsr->csr);
  sqlite3_free((void *)pCsr->filter.zTerm);
  sqlite3_free(pCsr->zStop);
  sqlite3_free(pCsr->aStat);
  sqlite3_free(pCsr);
  return SQLITE_OK;
}

static int fts3auxGrowStatArray(Fts3auxCursor *pCsr, int nSize){
  if( nSize>pCsr->nStat ){
    struct Fts3auxColstats *aNew;
    aNew = (struct Fts3auxColstats *)sqlite3_realloc64(pCsr->aStat,
        sizeof(struct Fts3auxColstats) * nSize
    );
    if( aNew==0 ) return SQLITE_NOMEM;
    memset(&aNew[pCsr->nStat], 0,
        sizeof(struct Fts3auxColstats) * (nSize - pCsr->nStat)
    );
    pCsr->aStat = aNew;
    pCsr->nStat = nSize;
  }
  return SQLITE_OK;
}

/*
** xNext - Advance the cursor to the next row, if any.
*/
static int fts3auxNextMethod(sqlite3_vtab_cursor *pCursor){
  Fts3auxCursor *pCsr = (Fts3auxCursor *)pCursor;
  Fts3Table *pFts3 = ((Fts3auxTable *)pCursor->pVtab)->pFts3Tab;
  int rc;

  /* Increment our pretend rowid value. */
  pCsr->iRowid++;

  for(pCsr->iCol++; pCsr->iCol<pCsr->nStat; pCsr->iCol++){
    if( pCsr->aStat[pCsr->iCol].nDoc>0 ) return SQLITE_OK;
  }

  rc = sqlite3Fts3SegReaderStep(pFts3, &pCsr->csr);
  if( rc==SQLITE_ROW ){
    int i = 0;
    int nDoclist = pCsr->csr.nDoclist;
    char *aDoclist = pCsr->csr.aDoclist;
    int iCol;

    int eState = 0;

    if( pCsr->zStop ){
      int n = (pCsr->nStop<pCsr->csr.nTerm) ? pCsr->nStop : pCsr->csr.nTerm;
      int mc = memcmp(pCsr->zStop, pCsr->csr.zTerm, n);
      if( mc<0 || (mc==0 && pCsr->csr.nTerm>pCsr->nStop) ){
        pCsr->isEof = 1;
        return SQLITE_OK;
      }
    }

    if( fts3auxGrowStatArray(pCsr, 2) ) return SQLITE_NOMEM;
    memset(pCsr->aStat, 0, sizeof(struct Fts3auxColstats) * pCsr->nStat);
    iCol = 0;
    rc = SQLITE_OK;

    while( i<nDoclist ){
      sqlite3_int64 v = 0;

      i += sqlite3Fts3GetVarint(&aDoclist[i], &v);
      switch( eState ){
        /* State 0. In this state the integer just read was a docid. */
        case 0:
          pCsr->aStat[0].nDoc++;
          eState = 1;
          iCol = 0;
          break;

        /* State 1. In this state we are expecting either a 1, indicating
        ** that the following integer will be a column number, or the
        ** start of a position list for column 0.
        **
        ** The only difference between state 1 and state 2 is that if the
        ** integer encountered in state 1 is not 0 or 1, then we need to
        ** increment the column 0 "nDoc" count for this term.
        */
        case 1:
          assert( iCol==0 );
          if( v>1 ){
            pCsr->aStat[1].nDoc++;
          }
          eState = 2;
          /* no break */ deliberate_fall_through

        case 2:
          if( v==0 ){       /* 0x00. Next integer will be a docid. */
            eState = 0;
          }else if( v==1 ){ /* 0x01. Next integer will be a column number. */
            eState = 3;
          }else{            /* 2 or greater. A position. */
            pCsr->aStat[iCol+1].nOcc++;
            pCsr->aStat[0].nOcc++;
          }
          break;

        /* State 3. The integer just read is a column number. */
        default: assert( eState==3 );
          iCol = (int)v;
          if( iCol<1 || iCol>32767 ){
            rc = SQLITE_CORRUPT_VTAB;
            break;
          }
          if( fts3auxGrowStatArray(pCsr, iCol+2) ) return SQLITE_NOMEM;
          pCsr->aStat[iCol+1].nDoc++;
          eState = 2;
          break;
      }
    }

    pCsr->iCol = 0;
  }else{
    pCsr->isEof = 1;
  }
  return rc;
}

/*
** xFilter - Initialize a cursor to point at the start of its data.
*/
static int fts3auxFilterMethod(
  sqlite3_vtab_cursor *pCursor,   /* The cursor used for this query */
  int idxNum,                     /* Strategy index */
  const char *idxStr,             /* Unused */
  int nVal,                       /* Number of elements in apVal */
  sqlite3_value **apVal           /* Arguments for the indexing scheme */
){
  Fts3auxCursor *pCsr = (Fts3auxCursor *)pCursor;
  Fts3Table *pFts3 = ((Fts3auxTable *)pCursor->pVtab)->pFts3Tab;
  int rc;
  int isScan = 0;
  int iLangVal = 0;               /* Language id to query */

  int iEq = -1;                   /* Index of term=? value in apVal */
  int iGe = -1;                   /* Index of term>=? value in apVal */
  int iLe = -1;                   /* Index of term<=? value in apVal */
  int iLangid = -1;               /* Index of languageid=? value in apVal */
  int iNext = 0;

  UNUSED_PARAMETER(nVal);
  UNUSED_PARAMETER(idxStr);

  assert( idxStr==0 );
  assert( idxNum==FTS4AUX_EQ_CONSTRAINT || idxNum==0
       || idxNum==FTS4AUX_LE_CONSTRAINT || idxNum==FTS4AUX_GE_CONSTRAINT
       || idxNum==(FTS4AUX_LE_CONSTRAINT|FTS4AUX_GE_CONSTRAINT)
  );

  if( idxNum==FTS4AUX_EQ_CONSTRAINT ){
    iEq = iNext++;
  }else{
    isScan = 1;
    if( idxNum & FTS4AUX_GE_CONSTRAINT ){
      iGe = iNext++;
    }
    if( idxNum & FTS4AUX_LE_CONSTRAINT ){
      iLe = iNext++;
    }
  }
  if( iNext<nVal ){
    iLangid = iNext++;
  }

  /* In case this cursor is being reused, close and zero it. */
  testcase(pCsr->filter.zTerm);
  sqlite3Fts3SegReaderFinish(&pCsr->csr);
  sqlite3_free((void *)pCsr->filter.zTerm);
  sqlite3_free(pCsr->aStat);
  sqlite3_free(pCsr->zStop);
  memset(&pCsr->csr, 0, ((u8*)&pCsr[1]) - (u8*)&pCsr->csr);

  pCsr->filter.flags = FTS3_SEGMENT_REQUIRE_POS|FTS3_SEGMENT_IGNORE_EMPTY;
  if( isScan ) pCsr->filter.flags |= FTS3_SEGMENT_SCAN;

  if( (iEq>=0 || iGe>=0) && sqlite3_value_type(apVal[0])==SQLITE_TEXT ){
    const unsigned char *zStr = sqlite3_value_text(apVal[0]);
    assert( (iEq==0 && iGe==-1) || (iEq==-1 && iGe==0) );
    if( zStr ){
      pCsr->filter.zTerm = sqlite3_mprintf("%s", zStr);
      if( pCsr->filter.zTerm==0 ) return SQLITE_NOMEM;
      pCsr->filter.nTerm = (int)strlen(pCsr->filter.zTerm);
    }
  }

  if( iLe>=0 && sqlite3_value_type(apVal[0])==SQLITE_TEXT ){
    pCsr->zStop = sqlite3_mprintf("%s", sqlite3_value_text(apVal[iLe]));
    if( pCsr->zStop==0 ) return SQLITE_NOMEM;
    pCsr->nStop = (int)strlen(pCsr->zStop);
  }

  if( iLangid>=0 ){
    iLangVal = sqlite3_value_int(apVal[iLangid]);

    /* If the user specified a negative value for the languageid, use zero
    ** instead. This works, as the "languageid=?" constraint will also
    ** be tested by the VDBE layer. The test will always be false (since
    ** this module will not return a row with a negative languageid), and
    ** so the overall query will return zero rows.  */
    if( iLangVal<0 ) iLangVal = 0;
  }
  pCsr->iLangid = iLangVal;

  rc = sqlite3Fts3SegReaderCursor(pFts3, iLangVal, 0, FTS3_SEGCURSOR_ALL,
      pCsr->filter.zTerm, pCsr->filter.nTerm, 0, isScan, &pCsr->csr
  );
  if( rc==SQLITE_OK ){
    rc = sqlite3Fts3SegReaderStart(pFts3, &pCsr->csr, &pCsr->filter);
  }

  if( rc==SQLITE_OK ) rc = fts3auxNextMethod(pCursor);
  return rc;
}

/*
** xEof - Return true if the cursor is at EOF, or false otherwise.
*/
static int fts3auxEofMethod(sqlite3_vtab_cursor *pCursor){
  Fts3auxCursor *pCsr = (Fts3auxCursor *)pCursor;
  return pCsr->isEof;
}

/*
** xColumn - Return a column value.
*/
static int fts3auxColumnMethod(
  sqlite3_vtab_cursor *pCursor,   /* Cursor to retrieve value from */
  sqlite3_context *pCtx,          /* Context for sqlite3_result_xxx() calls */
  int iCol                        /* Index of column to read value from */
){
  Fts3auxCursor *p = (Fts3auxCursor *)pCursor;

  assert( p->isEof==0 );
  switch( iCol ){
    case 0: /* term */
      sqlite3_result_text(pCtx, p->csr.zTerm, p->csr.nTerm, SQLITE_TRANSIENT);
      break;

    case 1: /* col */
      if( p->iCol ){
        sqlite3_result_int(pCtx, p->iCol-1);
      }else{
        sqlite3_result_text(pCtx, "*", -1, SQLITE_STATIC);
      }
      break;

    case 2: /* documents */
      sqlite3_result_int64(pCtx, p->aStat[p->iCol].nDoc);
      break;

    case 3: /* occurrences */
      sqlite3_result_int64(pCtx, p->aStat[p->iCol].nOcc);
      break;

    default: /* languageid */
      assert( iCol==4 );
      sqlite3_result_int(pCtx, p->iLangid);
      break;
  }

  return SQLITE_OK;
}

/*
** xRowid - Return the current rowid for the cursor.
*/
static int fts3auxRowidMethod(
  sqlite3_vtab_cursor *pCursor,   /* Cursor to retrieve value from */
  sqlite_int64 *pRowid            /* OUT: Rowid value */
){
  Fts3auxCursor *pCsr = (Fts3auxCursor *)pCursor;
  *pRowid = pCsr->iRowid;
  return SQLITE_OK;
}

/*
** Register the fts3aux module with database connection db. Return SQLITE_OK
** if successful or an error code if sqlite3_create_module() fails.
*/
SQLITE_PRIVATE int sqlite3Fts3InitAux(sqlite3 *db){
  static const sqlite3_module fts3aux_module = {
     0,                           /* iVersion      */
     fts3auxConnectMethod,        /* xCreate       */
     fts3auxConnectMethod,        /* xConnect      */
     fts3auxBestIndexMethod,      /* xBestIndex    */
     fts3auxDisconnectMethod,     /* xDisconnect   */
     fts3auxDisconnectMethod,     /* xDestroy      */
     fts3auxOpenMethod,           /* xOpen         */
     fts3auxCloseMethod,          /* xClose        */
     fts3auxFilterMethod,         /* xFilter       */
     fts3auxNextMethod,           /* xNext         */
     fts3auxEofMethod,            /* xEof          */
     fts3auxColumnMethod,         /* xColumn       */
     fts3auxRowidMethod,          /* xRowid        */
     0,                           /* xUpdate       */
     0,                           /* xBegin        */
     0,                           /* xSync         */
     0,                           /* xCommit       */
     0,                           /* xRollback     */
     0,                           /* xFindFunction */
     0,                           /* xRename       */
     0,                           /* xSavepoint    */
     0,                           /* xRelease      */
     0,                           /* xRollbackTo   */
     0,                           /* xShadowName   */
     0                            /* xIntegrity    */
  };
  int rc;                         /* Return code */

  rc = sqlite3_create_module(db, "fts4aux", &fts3aux_module, 0);
  return rc;
}

#endif /* !defined(SQLITE_CORE) || defined(SQLITE_ENABLE_FTS3) */

/************** End of fts3_aux.c ********************************************/
/************** Begin file fts3_expr.c ***************************************/
/*
** 2008 Nov 28
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
** This module contains code that implements a parser for fts3 query strings
** (the right-hand argument to the MATCH operator). Because the supported
** syntax is relatively simple, the whole tokenizer/parser system is
** hand-coded.
*/
/* #include "fts3Int.h" */
#if !defined(SQLITE_CORE) || defined(SQLITE_ENABLE_FTS3)

/*
** By default, this module parses the legacy syntax that has been
** traditionally used by fts3. Or, if SQLITE_ENABLE_FTS3_PARENTHESIS
** is defined, then it uses the new syntax. The differences between
** the new and the old syntaxes are:
**
**  a) The new syntax supports parenthesis. The old does not.
**
**  b) The new syntax supports the AND and NOT operators. The old does not.
**
**  c) The old syntax supports the "-" token qualifier. This is not
**     supported by the new syntax (it is replaced by the NOT operator).
**
**  d) When using the old syntax, the OR operator has a greater precedence
**     than an implicit AND. When using the new, both implicity and explicit
**     AND operators have a higher precedence than OR.
**
** If compiled with SQLITE_TEST defined, then this module exports the
** symbol "int sqlite3_fts3_enable_parentheses". Setting this variable
** to zero causes the module to use the old syntax. If it is set to
** non-zero the new syntax is activated. This is so both syntaxes can
** be tested using a single build of testfixture.
**
** The following describes the syntax supported by the fts3 MATCH
** operator in a similar format to that used by the lemon parser
** generator. This module does not use actually lemon, it uses a
** custom parser.
**
**   query ::= andexpr (OR andexpr)*.
**
**   andexpr ::= notexpr (AND? notexpr)*.
**
**   notexpr ::= nearexpr (NOT nearexpr|-TOKEN)*.
**   notexpr ::= LP query RP.
**
**   nearexpr ::= phrase (NEAR distance_opt nearexpr)*.
**
**   distance_opt ::= .
**   distance_opt ::= / INTEGER.
**
**   phrase ::= TOKEN.
**   phrase ::= COLUMN:TOKEN.
**   phrase ::= "TOKEN TOKEN TOKEN...".
*/

#ifdef SQLITE_TEST
SQLITE_API int sqlite3_fts3_enable_parentheses = 0;
#else
# ifdef SQLITE_ENABLE_FTS3_PARENTHESIS
#  define sqlite3_fts3_enable_parentheses 1
# else
#  define sqlite3_fts3_enable_parentheses 0
# endif
#endif

/*
** Default span for NEAR operators.
*/
#define SQLITE_FTS3_DEFAULT_NEAR_PARAM 10

/* #include <string.h> */
/* #include <assert.h> */

/*
** isNot:
**   This variable is used by function getNextNode(). When getNextNode() is
**   called, it sets ParseContext.isNot to true if the 'next node' is a
**   FTSQUERY_PHRASE with a unary "-" attached to it. i.e. "mysql" in the
**   FTS3 query "sqlite -mysql". Otherwise, ParseContext.isNot is set to
**   zero.
*/
typedef struct ParseContext ParseContext;
struct ParseContext {
  sqlite3_tokenizer *pTokenizer;      /* Tokenizer module */
  int iLangid;                        /* Language id used with tokenizer */
  const char **azCol;                 /* Array of column names for fts3 table */
  int bFts4;                          /* True to allow FTS4-only syntax */
  int nCol;                           /* Number of entries in azCol[] */
  int iDefaultCol;                    /* Default column to query */
  int isNot;                          /* True if getNextNode() sees a unary - */
  sqlite3_context *pCtx;              /* Write error message here */
  int nNest;                          /* Number of nested brackets */
};

/*
** This function is equivalent to the standard isspace() function.
**
** The standard isspace() can be awkward to use safely, because although it
** is defined to accept an argument of type int, its behavior when passed
** an integer that falls outside of the range of the unsigned char type
** is undefined (and sometimes, "undefined" means segfault). This wrapper
** is defined to accept an argument of type char, and always returns 0 for
** any values that fall outside of the range of the unsigned char type (i.e.
** negative values).
*/
static int fts3isspace(char c){
  return c==' ' || c=='\t' || c=='\n' || c=='\r' || c=='\v' || c=='\f';
}

/*
** Allocate nByte bytes of memory using sqlite3_malloc(). If successful,
** zero the memory before returning a pointer to it. If unsuccessful,
** return NULL.
*/
SQLITE_PRIVATE void *sqlite3Fts3MallocZero(sqlite3_int64 nByte){
  void *pRet = sqlite3_malloc64(nByte);
  if( pRet ) memset(pRet, 0, nByte);
  return pRet;
}

SQLITE_PRIVATE int sqlite3Fts3OpenTokenizer(
  sqlite3_tokenizer *pTokenizer,
  int iLangid,
  const char *z,
  int n,
  sqlite3_tokenizer_cursor **ppCsr
){
  sqlite3_tokenizer_module const *pModule = pTokenizer->pModule;
  sqlite3_tokenizer_cursor *pCsr = 0;
  int rc;

  rc = pModule->xOpen(pTokenizer, z, n, &pCsr);
  assert( rc==SQLITE_OK || pCsr==0 );
  if( rc==SQLITE_OK ){
    pCsr->pTokenizer = pTokenizer;
    if( pModule->iVersion>=1 ){
      rc = pModule->xLanguageid(pCsr, iLangid);
      if( rc!=SQLITE_OK ){
        pModule->xClose(pCsr);
        pCsr = 0;
      }
    }
  }
  *ppCsr = pCsr;
  return rc;
}

/*
** Function getNextNode(), which is called by fts3ExprParse(), may itself
** call fts3ExprParse(). So this forward declaration is required.
*/
static int fts3ExprParse(ParseContext *, const char *, int, Fts3Expr **, int *);

/*
** Search buffer z[], size n, for a '"' character. Or, if enable_parenthesis
** is defined, search for '(' and ')' as well. Return the index of the first
** such character in the buffer. If there is no such character, return -1.
*/
static int findBarredChar(const char *z, int n){
  int ii;
  for(ii=0; ii<n; ii++){
    if( (z[ii]=='"')
     || (sqlite3_fts3_enable_parentheses && (z[ii]=='(' || z[ii]==')'))
    ){
      return ii;
    }
  }
  return -1;
}

/*
** Extract the next token from buffer z (length n) using the tokenizer
** and other information (column names etc.) in pParse. Create an Fts3Expr
** structure of type FTSQUERY_PHRASE containing a phrase consisting of this
** single token and set *ppExpr to point to it. If the end of the buffer is
** reached before a token is found, set *ppExpr to zero. It is the
** responsibility of the caller to eventually deallocate the allocated
** Fts3Expr structure (if any) by passing it to sqlite3_free().
**
** Return SQLITE_OK if successful, or SQLITE_NOMEM if a memory allocation
** fails.
*/
static int getNextToken(
  ParseContext *pParse,                   /* fts3 query parse context */
  int iCol,                               /* Value for Fts3Phrase.iColumn */
  const char *z, int n,                   /* Input string */
  Fts3Expr **ppExpr,                      /* OUT: expression */
  int *pnConsumed                         /* OUT: Number of bytes consumed */
){
  sqlite3_tokenizer *pTokenizer = pParse->pTokenizer;
  sqlite3_tokenizer_module const *pModule = pTokenizer->pModule;
  int rc;
  sqlite3_tokenizer_cursor *pCursor;
  Fts3Expr *pRet = 0;

  *pnConsumed = n;
  rc = sqlite3Fts3OpenTokenizer(pTokenizer, pParse->iLangid, z, n, &pCursor);
  if( rc==SQLITE_OK ){
    const char *zToken;
    int nToken = 0, iStart = 0, iEnd = 0, iPosition = 0;
    sqlite3_int64 nByte;                    /* total space to allocate */

    rc = pModule->xNext(pCursor, &zToken, &nToken, &iStart, &iEnd, &iPosition);
    if( rc==SQLITE_OK ){
      /* Check that this tokenization did not gobble up any " characters. Or,
      ** if enable_parenthesis is true, that it did not gobble up any
      ** open or close parenthesis characters either. If it did, call
      ** getNextToken() again, but pass only that part of the input buffer
      ** up to the first such character.  */
      int iBarred = findBarredChar(z, iEnd);
      if( iBarred>=0 ){
        pModule->xClose(pCursor);
        return getNextToken(pParse, iCol, z, iBarred, ppExpr, pnConsumed);
      }

      nByte = sizeof(Fts3Expr) + SZ_FTS3PHRASE(1) + nToken;
      pRet = (Fts3Expr *)sqlite3Fts3MallocZero(nByte);
      if( !pRet ){
        rc = SQLITE_NOMEM;
      }else{
        pRet->eType = FTSQUERY_PHRASE;
        pRet->pPhrase = (Fts3Phrase *)&pRet[1];
        pRet->pPhrase->nToken = 1;
        pRet->pPhrase->iColumn = iCol;
        pRet->pPhrase->aToken[0].n = nToken;
        pRet->pPhrase->aToken[0].z = (char*)&pRet->pPhrase->aToken[1];
        memcpy(pRet->pPhrase->aToken[0].z, zToken, nToken);

        if( iEnd<n && z[iEnd]=='*' ){
          pRet->pPhrase->aToken[0].isPrefix = 1;
          iEnd++;
        }

        while( 1 ){
          if( !sqlite3_fts3_enable_parentheses
           && iStart>0 && z[iStart-1]=='-'
          ){
            pParse->isNot = 1;
            iStart--;
          }else if( pParse->bFts4 && iStart>0 && z[iStart-1]=='^' ){
            pRet->pPhrase->aToken[0].bFirst = 1;
            iStart--;
          }else{
            break;
          }
        }

      }
      *pnConsumed = iEnd;
    }else if( n && rc==SQLITE_DONE ){
      int iBarred = findBarredChar(z, n);
      if( iBarred>=0 ){
        *pnConsumed = iBarred;
      }
      rc = SQLITE_OK;
    }

    pModule->xClose(pCursor);
  }

  *ppExpr = pRet;
  return rc;
}


/*
** Enlarge a memory allocation.  If an out-of-memory allocation occurs,
** then free the old allocation.
*/
static void *fts3ReallocOrFree(void *pOrig, sqlite3_int64 nNew){
  void *pRet = sqlite3_realloc64(pOrig, nNew);
  if( !pRet ){
    sqlite3_free(pOrig);
  }
  return pRet;
}

/*
** Buffer zInput, length nInput, contains the contents of a quoted string
** that appeared as part of an fts3 query expression. Neither quote character
** is included in the buffer. This function attempts to tokenize the entire
** input buffer and create an Fts3Expr structure of type FTSQUERY_PHRASE
** containing the results.
**
** If successful, SQLITE_OK is returned and *ppExpr set to point at the
** allocated Fts3Expr structure. Otherwise, either SQLITE_NOMEM (out of memory
** error) or SQLITE_ERROR (tokenization error) is returned and *ppExpr set
** to 0.
*/
static int getNextString(
  ParseContext *pParse,                   /* fts3 query parse context */
  const char *zInput, int nInput,         /* Input string */
  Fts3Expr **ppExpr                       /* OUT: expression */
){
  sqlite3_tokenizer *pTokenizer = pParse->pTokenizer;
  sqlite3_tokenizer_module const *pModule = pTokenizer->pModule;
  int rc;
  Fts3Expr *p = 0;
  sqlite3_tokenizer_cursor *pCursor = 0;
  char *zTemp = 0;
  i64 nTemp = 0;

  const int nSpace = sizeof(Fts3Expr) + SZ_FTS3PHRASE(1);
  int nToken = 0;

  /* The final Fts3Expr data structure, including the Fts3Phrase,
  ** Fts3PhraseToken structures token buffers are all stored as a single
  ** allocation so that the expression can be freed with a single call to
  ** sqlite3_free(). Setting this up requires a two pass approach.
  **
  ** The first pass, in the block below, uses a tokenizer cursor to iterate
  ** through the tokens in the expression. This pass uses fts3ReallocOrFree()
  ** to assemble data in two dynamic buffers:
  **
  **   Buffer p: Points to the Fts3Expr structure, followed by the Fts3Phrase
  **             structure, followed by the array of Fts3PhraseToken
  **             structures. This pass only populates the Fts3PhraseToken array.
  **
  **   Buffer zTemp: Contains copies of all tokens.
  **
  ** The second pass, in the block that begins "if( rc==SQLITE_DONE )" below,
  ** appends buffer zTemp to buffer p, and fills in the Fts3Expr and Fts3Phrase
  ** structures.
  */
  rc = sqlite3Fts3OpenTokenizer(
      pTokenizer, pParse->iLangid, zInput, nInput, &pCursor);
  if( rc==SQLITE_OK ){
    int ii;
    for(ii=0; rc==SQLITE_OK; ii++){
      const char *zByte;
      int nByte = 0, iBegin = 0, iEnd = 0, iPos = 0;
      rc = pModule->xNext(pCursor, &zByte, &nByte, &iBegin, &iEnd, &iPos);
      if( rc==SQLITE_OK ){
        Fts3PhraseToken *pToken;

        p = fts3ReallocOrFree(p, nSpace + ii*sizeof(Fts3PhraseToken));
        zTemp = fts3ReallocOrFree(zTemp, nTemp + nByte);
        if( !zTemp || !p ){
          rc = SQLITE_NOMEM;
          goto getnextstring_out;
        }

        assert( nToken==ii );
        pToken = &((Fts3Phrase *)(&p[1]))->aToken[ii];
        memset(pToken, 0, sizeof(Fts3PhraseToken));

        memcpy(&zTemp[nTemp], zByte, nByte);
        nTemp += nByte;

        pToken->n = nByte;
        pToken->isPrefix = (iEnd<nInput && zInput[iEnd]=='*');
        pToken->bFirst = (iBegin>0 && zInput[iBegin-1]=='^');
        nToken = ii+1;
      }
    }
  }

  if( rc==SQLITE_DONE ){
    int jj;
    char *zBuf = 0;

    p = fts3ReallocOrFree(p, nSpace + nToken*sizeof(Fts3PhraseToken) + nTemp);
    if( !p ){
      rc = SQLITE_NOMEM;
      goto getnextstring_out;
    }
    memset(p, 0, (char *)&(((Fts3Phrase *)&p[1])->aToken[0])-(char *)p);
    p->eType = FTSQUERY_PHRASE;
    p->pPhrase = (Fts3Phrase *)&p[1];
    p->pPhrase->iColumn = pParse->iDefaultCol;
    p->pPhrase->nToken = nToken;

    zBuf = (char *)&p->pPhrase->aToken[nToken];
    assert( nTemp==0 || zTemp );
    if( zTemp ){
      memcpy(zBuf, zTemp, nTemp);
    }

    for(jj=0; jj<p->pPhrase->nToken; jj++){
      p->pPhrase->aToken[jj].z = zBuf;
      zBuf += p->pPhrase->aToken[jj].n;
    }
    rc = SQLITE_OK;
  }

 getnextstring_out:
  if( pCursor ){
    pModule->xClose(pCursor);
  }
  sqlite3_free(zTemp);
  if( rc!=SQLITE_OK ){
    sqlite3_free(p);
    p = 0;
  }
  *ppExpr = p;
  return rc;
}

/*
** The output variable *ppExpr is populated with an allocated Fts3Expr
** structure, or set to 0 if the end of the input buffer is reached.
**
** Returns an SQLite error code. SQLITE_OK if everything works, SQLITE_NOMEM
** if a malloc failure occurs, or SQLITE_ERROR if a parse error is encountered.
** If SQLITE_ERROR is returned, pContext is populated with an error message.
*/
static int getNextNode(
  ParseContext *pParse,                   /* fts3 query parse context */
  const char *z, int n,                   /* Input string */
  Fts3Expr **ppExpr,                      /* OUT: expression */
  int *pnConsumed                         /* OUT: Number of bytes consumed */
){
  static const struct Fts3Keyword {
    char *z;                              /* Keyword text */
    unsigned char n;                      /* Length of the keyword */
    unsigned char parenOnly;              /* Only valid in paren mode */
    unsigned char eType;                  /* Keyword code */
  } aKeyword[] = {
    { "OR" ,  2, 0, FTSQUERY_OR   },
    { "AND",  3, 1, FTSQUERY_AND  },
    { "NOT",  3, 1, FTSQUERY_NOT  },
    { "NEAR", 4, 0, FTSQUERY_NEAR }
  };
  int ii;
  int iCol;
  int iColLen;
  int rc;
  Fts3Expr *pRet = 0;

  const char *zInput = z;
  int nInput = n;

  pParse->isNot = 0;

  /* Skip over any whitespace before checking for a keyword, an open or
  ** close bracket, or a quoted string.
  */
  while( nInput>0 && fts3isspace(*zInput) ){
    nInput--;
    zInput++;
  }
  if( nInput==0 ){
    return SQLITE_DONE;
  }

  /* See if we are dealing with a keyword. */
  for(ii=0; ii<(int)(sizeof(aKeyword)/sizeof(struct Fts3Keyword)); ii++){
    const struct Fts3Keyword *pKey = &aKeyword[ii];

    if( (pKey->parenOnly & ~sqlite3_fts3_enable_parentheses)!=0 ){
      continue;
    }

    if( nInput>=pKey->n && 0==memcmp(zInput, pKey->z, pKey->n) ){
      int nNear = SQLITE_FTS3_DEFAULT_NEAR_PARAM;
      int nKey = pKey->n;
      char cNext;

      /* If this is a "NEAR" keyword, check for an explicit nearness. */
      if( pKey->eType==FTSQUERY_NEAR ){
        assert( nKey==4 );
        if( zInput[4]=='/' && zInput[5]>='0' && zInput[5]<='9' ){
          nKey += 1+sqlite3Fts3ReadInt(&zInput[nKey+1], &nNear);
          if( nNear>=1000000000 ) nNear = 1000000000;
        }
      }

      /* At this point this is probably a keyword. But for that to be true,
      ** the next byte must contain either whitespace, an open or close
      ** parenthesis, a quote character, or EOF.
      */
      cNext = zInput[nKey];
      if( fts3isspace(cNext)
       || cNext=='"' || cNext=='(' || cNext==')' || cNext==0
      ){
        pRet = (Fts3Expr *)sqlite3Fts3MallocZero(sizeof(Fts3Expr));
        if( !pRet ){
          return SQLITE_NOMEM;
        }
        pRet->eType = pKey->eType;
        pRet->nNear = nNear;
        *ppExpr = pRet;
        *pnConsumed = (int)((zInput - z) + nKey);
        return SQLITE_OK;
      }

      /* Turns out that wasn't a keyword after all. This happens if the
      ** user has supplied a token such as "ORacle". Continue.
      */
    }
  }

  /* See if we are dealing with a quoted phrase. If this is the case, then
  ** search for the closing quote and pass the whole string to getNextString()
  ** for processing. This is easy to do, as fts3 has no syntax for escaping
  ** a quote character embedded in a string.
  */
  if( *zInput=='"' ){
    for(ii=1; ii<nInput && zInput[ii]!='"'; ii++);
    *pnConsumed = (int)((zInput - z) + ii + 1);
    if( ii==nInput ){
      return SQLITE_ERROR;
    }
    return getNextString(pParse, &zInput[1], ii-1, ppExpr);
  }

  if( sqlite3_fts3_enable_parentheses ){
    if( *zInput=='(' ){
      int nConsumed = 0;
      pParse->nNest++;
#if !defined(SQLITE_MAX_EXPR_DEPTH)
      if( pParse->nNest>1000 ) return SQLITE_ERROR;
#elif SQLITE_MAX_EXPR_DEPTH>0
      if( pParse->nNest>SQLITE_MAX_EXPR_DEPTH ) return SQLITE_ERROR;
#endif
      rc = fts3ExprParse(pParse, zInput+1, nInput-1, ppExpr, &nConsumed);
      *pnConsumed = (int)(zInput - z) + 1 + nConsumed;
      return rc;
    }else if( *zInput==')' ){
      pParse->nNest--;
      *pnConsumed = (int)((zInput - z) + 1);
      *ppExpr = 0;
      return SQLITE_DONE;
    }
  }

  /* If control flows to this point, this must be a regular token, or
  ** the end of the input. Read a regular token using the sqlite3_tokenizer
  ** interface. Before doing so, figure out if there is an explicit
  ** column specifier for the token.
  **
  ** TODO: Strangely, it is not possible to associate a column specifier
  ** with a quoted phrase, only with a single token. Not sure if this was
  ** an implementation artifact or an intentional decision when fts3 was
  ** first implemented. Whichever it was, this module duplicates the
  ** limitation.
  */
  iCol = pParse->iDefaultCol;
  iColLen = 0;
  for(ii=0; ii<pParse->nCol; ii++){
    const char *zStr = pParse->azCol[ii];
    int nStr = zStr ? (int)strlen(zStr) : 0;
    if( nInput>nStr && zInput[nStr]==':'
     && sqlite3_strnicmp(zStr, zInput, nStr)==0
    ){
      iCol = ii;
      iColLen = (int)((zInput - z) + nStr + 1);
      break;
    }
  }
  rc = getNextToken(pParse, iCol, &z[iColLen], n-iColLen, ppExpr, pnConsumed);
  *pnConsumed += iColLen;
  return rc;
}

/*
** The argument is an Fts3Expr structure for a binary operator (any type
** except an FTSQUERY_PHRASE). Return an integer value representing the
** precedence of the operator. Lower values have a higher precedence (i.e.
** group more tightly). For example, in the C language, the == operator
** groups more tightly than ||, and would therefore have a higher precedence.
**
** When using the new fts3 query syntax (when SQLITE_ENABLE_FTS3_PARENTHESIS
** is defined), the order of the operators in precedence from highest to
** lowest is:
**
**   NEAR
**   NOT
**   AND (including implicit ANDs)
**   OR
**
** Note that when using the old query syntax, the OR operator has a higher
** precedence than the AND operator.
*/
static int opPrecedence(Fts3Expr *p){
  assert( p->eType!=FTSQUERY_PHRASE );
  if( sqlite3_fts3_enable_parentheses ){
    return p->eType;
  }else if( p->eType==FTSQUERY_NEAR ){
    return 1;
  }else if( p->eType==FTSQUERY_OR ){
    return 2;
  }
  assert( p->eType==FTSQUERY_AND );
  return 3;
}

/*
** Argument ppHead contains a pointer to the current head of a query
** expression tree being parsed. pPrev is the expression node most recently
** inserted into the tree. This function adds pNew, which is always a binary
** operator node, into the expression tree based on the relative precedence
** of pNew and the existing nodes of the tree. This may result in the head
** of the tree changing, in which case *ppHead is set to the new root node.
*/
static void insertBinaryOperator(
  Fts3Expr **ppHead,       /* Pointer to the root node of a tree */
  Fts3Expr *pPrev,         /* Node most recently inserted into the tree */
  Fts3Expr *pNew           /* New binary node to insert into expression tree */
){
  Fts3Expr *pSplit = pPrev;
  while( pSplit->pParent && opPrecedence(pSplit->pParent)<=opPrecedence(pNew) ){
    pSplit = pSplit->pParent;
  }

  if( pSplit->pParent ){
    assert( pSplit->pParent->pRight==pSplit );
    pSplit->pParent->pRight = pNew;
    pNew->pParent = pSplit->pParent;
  }else{
    *ppHead = pNew;
  }
  pNew->pLeft = pSplit;
  pSplit->pParent = pNew;
}

/*
** Parse the fts3 query expression found in buffer z, length n. This function
** returns either when the end of the buffer is reached or an unmatched
** closing bracket - ')' - is encountered.
**
** If successful, SQLITE_OK is returned, *ppExpr is set to point to the
** parsed form of the expression and *pnConsumed is set to the number of
** bytes read from buffer z. Otherwise, *ppExpr is set to 0 and SQLITE_NOMEM
** (out of memory error) or SQLITE_ERROR (parse error) is returned.
*/
static int fts3ExprParse(
  ParseContext *pParse,                   /* fts3 query parse context */
  const char *z, int n,                   /* Text of MATCH query */
  Fts3Expr **ppExpr,                      /* OUT: Parsed query structure */
  int *pnConsumed                         /* OUT: Number of bytes consumed */
){
  Fts3Expr *pRet = 0;
  Fts3Expr *pPrev = 0;
  Fts3Expr *pNotBranch = 0;               /* Only used in legacy parse mode */
  int nIn = n;
  const char *zIn = z;
  int rc = SQLITE_OK;
  int isRequirePhrase = 1;

  while( rc==SQLITE_OK ){
    Fts3Expr *p = 0;
    int nByte = 0;

    rc = getNextNode(pParse, zIn, nIn, &p, &nByte);
    assert( nByte>0 || (rc!=SQLITE_OK && p==0) );
    if( rc==SQLITE_OK ){
      if( p ){
        int isPhrase;

        if( !sqlite3_fts3_enable_parentheses
            && p->eType==FTSQUERY_PHRASE && pParse->isNot
        ){
          /* Create an implicit NOT operator. */
          Fts3Expr *pNot = sqlite3Fts3MallocZero(sizeof(Fts3Expr));
          if( !pNot ){
            sqlite3Fts3ExprFree(p);
            rc = SQLITE_NOMEM;
            goto exprparse_out;
          }
          pNot->eType = FTSQUERY_NOT;
          pNot->pRight = p;
          p->pParent = pNot;
          if( pNotBranch ){
            pNot->pLeft = pNotBranch;
            pNotBranch->pParent = pNot;
          }
          pNotBranch = pNot;
          p = pPrev;
        }else{
          int eType = p->eType;
          isPhrase = (eType==FTSQUERY_PHRASE || p->pLeft);

          /* The isRequirePhrase variable is set to true if a phrase or
          ** an expression contained in parenthesis is required. If a
          ** binary operator (AND, OR, NOT or NEAR) is encountered when
          ** isRequirePhrase is set, this is a syntax error.
          */
          if( !isPhrase && isRequirePhrase ){
            sqlite3Fts3ExprFree(p);
            rc = SQLITE_ERROR;
            goto exprparse_out;
          }

          if( isPhrase && !isRequirePhrase ){
            /* Insert an implicit AND operator. */
            Fts3Expr *pAnd;
            assert( pRet && pPrev );
            pAnd = sqlite3Fts3MallocZero(sizeof(Fts3Expr));
            if( !pAnd ){
              sqlite3Fts3ExprFree(p);
              rc = SQLITE_NOMEM;
              goto exprparse_out;
            }
            pAnd->eType = FTSQUERY_AND;
            insertBinaryOperator(&pRet, pPrev, pAnd);
            pPrev = pAnd;
          }

          /* This test catches attempts to make either operand of a NEAR
           ** operator something other than a phrase. For example, either of
           ** the following:
           **
           **    (bracketed expression) NEAR phrase
           **    phrase NEAR (bracketed expression)
           **
           ** Return an error in either case.
           */
          if( pPrev && (
            (eType==FTSQUERY_NEAR && !isPhrase && pPrev->eType!=FTSQUERY_PHRASE)
         || (eType!=FTSQUERY_PHRASE && isPhrase && pPrev->eType==FTSQUERY_NEAR)
          )){
            sqlite3Fts3ExprFree(p);
            rc = SQLITE_ERROR;
            goto exprparse_out;
          }

          if( isPhrase ){
            if( pRet ){
              assert( pPrev && pPrev->pLeft && pPrev->pRight==0 );
              pPrev->pRight = p;
              p->pParent = pPrev;
            }else{
              pRet = p;
            }
          }else{
            insertBinaryOperator(&pRet, pPrev, p);
          }
          isRequirePhrase = !isPhrase;
        }
        pPrev = p;
      }
      assert( nByte>0 );
    }
    assert( rc!=SQLITE_OK || (nByte>0 && nByte<=nIn) );
    nIn -= nByte;
    zIn += nByte;
  }

  if( rc==SQLITE_DONE && pRet && isRequirePhrase ){
    rc = SQLITE_ERROR;
  }

  if( rc==SQLITE_DONE ){
    rc = SQLITE_OK;
    if( !sqlite3_fts3_enable_parentheses && pNotBranch ){
      if( !pRet ){
        rc = SQLITE_ERROR;
      }else{
        Fts3Expr *pIter = pNotBranch;
        while( pIter->pLeft ){
          pIter = pIter->pLeft;
        }
        pIter->pLeft = pRet;
        pRet->pParent = pIter;
        pRet = pNotBranch;
      }
    }
  }
  *pnConsumed = n - nIn;

exprparse_out:
  if( rc!=SQLITE_OK ){
    sqlite3Fts3ExprFree(pRet);
    sqlite3Fts3ExprFree(pNotBranch);
    pRet = 0;
  }
  *ppExpr = pRet;
  return rc;
}

/*
** Return SQLITE_ERROR if the maximum depth of the expression tree passed
** as the only argument is more than nMaxDepth.
*/
static int fts3ExprCheckDepth(Fts3Expr *p, int nMaxDepth){
  int rc = SQLITE_OK;
  if( p ){
    if( nMaxDepth<0 ){
      rc = SQLITE_TOOBIG;
    }else{
      rc = fts3ExprCheckDepth(p->pLeft, nMaxDepth-1);
      if( rc==SQLITE_OK ){
        rc = fts3ExprCheckDepth(p->pRight, nMaxDepth-1);
      }
    }
  }
  return rc;
}

/*
** This function attempts to transform the expression tree at (*pp) to
** an equivalent but more balanced form. The tree is modified in place.
** If successful, SQLITE_OK is returned and (*pp) set to point to the
** new root expression node.
**
** nMaxDepth is the maximum allowable depth of the balanced sub-tree.
**
** Otherwise, if an error occurs, an SQLite error code is returned and
** expression (*pp) freed.
*/
static int fts3ExprBalance(Fts3Expr **pp, int nMaxDepth){
  int rc = SQLITE_OK;             /* Return code */
  Fts3Expr *pRoot = *pp;          /* Initial root node */
  Fts3Expr *pFree = 0;            /* List of free nodes. Linked by pParent. */
  int eType = pRoot->eType;       /* Type of node in this tree */

  if( nMaxDepth==0 ){
    rc = SQLITE_ERROR;
  }

  if( rc==SQLITE_OK ){
    if( (eType==FTSQUERY_AND || eType==FTSQUERY_OR) ){
      Fts3Expr **apLeaf;
      apLeaf = (Fts3Expr **)sqlite3_malloc64(sizeof(Fts3Expr *) * nMaxDepth);
      if( 0==apLeaf ){
        rc = SQLITE_NOMEM;
      }else{
        memset(apLeaf, 0, sizeof(Fts3Expr *) * nMaxDepth);
      }

      if( rc==SQLITE_OK ){
        int i;
        Fts3Expr *p;

        /* Set $p to point to the left-most leaf in the tree of eType nodes. */
        for(p=pRoot; p->eType==eType; p=p->pLeft){
          assert( p->pParent==0 || p->pParent->pLeft==p );
          assert( p->pLeft && p->pRight );
        }

        /* This loop runs once for each leaf in the tree of eType nodes. */
        while( 1 ){
          int iLvl;
          Fts3Expr *pParent = p->pParent;     /* Current parent of p */

          assert( pParent==0 || pParent->pLeft==p );
          p->pParent = 0;
          if( pParent ){
            pParent->pLeft = 0;
          }else{
            pRoot = 0;
          }
          rc = fts3ExprBalance(&p, nMaxDepth-1);
          if( rc!=SQLITE_OK ) break;

          for(iLvl=0; p && iLvl<nMaxDepth; iLvl++){
            if( apLeaf[iLvl]==0 ){
              apLeaf[iLvl] = p;
              p = 0;
            }else{
              assert( pFree );
              pFree->pLeft = apLeaf[iLvl];
              pFree->pRight = p;
              pFree->pLeft->pParent = pFree;
              pFree->pRight->pParent = pFree;

              p = pFree;
              pFree = pFree->pParent;
              p->pParent = 0;
              apLeaf[iLvl] = 0;
            }
          }
          if( p ){
            sqlite3Fts3ExprFree(p);
            rc = SQLITE_TOOBIG;
            break;
          }

          /* If that was the last leaf node, break out of the loop */
          if( pParent==0 ) break;

          /* Set $p to point to the next leaf in the tree of eType nodes */
          for(p=pParent->pRight; p->eType==eType; p=p->pLeft);

          /* Remove pParent from the original tree. */
          assert( pParent->pParent==0 || pParent->pParent->pLeft==pParent );
          pParent->pRight->pParent = pParent->pParent;
          if( pParent->pParent ){
            pParent->pParent->pLeft = pParent->pRight;
          }else{
            assert( pParent==pRoot );
            pRoot = pParent->pRight;
          }

          /* Link pParent into the free node list. It will be used as an
          ** internal node of the new tree.  */
          pParent->pParent = pFree;
          pFree = pParent;
        }

        if( rc==SQLITE_OK ){
          p = 0;
          for(i=0; i<nMaxDepth; i++){
            if( apLeaf[i] ){
              if( p==0 ){
                p = apLeaf[i];
                p->pParent = 0;
              }else{
                assert( pFree!=0 );
                pFree->pRight = p;
                pFree->pLeft = apLeaf[i];
                pFree->pLeft->pParent = pFree;
                pFree->pRight->pParent = pFree;

                p = pFree;
                pFree = pFree->pParent;
                p->pParent = 0;
              }
            }
          }
          pRoot = p;
        }else{
          /* An error occurred. Delete the contents of the apLeaf[] array
          ** and pFree list. Everything else is cleaned up by the call to
          ** sqlite3Fts3ExprFree(pRoot) below.  */
          Fts3Expr *pDel;
          for(i=0; i<nMaxDepth; i++){
            sqlite3Fts3ExprFree(apLeaf[i]);
          }
          while( (pDel=pFree)!=0 ){
            pFree = pDel->pParent;
            sqlite3_free(pDel);
          }
        }

        assert( pFree==0 );
        sqlite3_free( apLeaf );
      }
    }else if( eType==FTSQUERY_NOT ){
      Fts3Expr *pLeft = pRoot->pLeft;
      Fts3Expr *pRight = pRoot->pRight;

      pRoot->pLeft = 0;
      pRoot->pRight = 0;
      pLeft->pParent = 0;
      pRight->pParent = 0;

      rc = fts3ExprBalance(&pLeft, nMaxDepth-1);
      if( rc==SQLITE_OK ){
        rc = fts3ExprBalance(&pRight, nMaxDepth-1);
      }

      if( rc!=SQLITE_OK ){
        sqlite3Fts3ExprFree(pRight);
        sqlite3Fts3ExprFree(pLeft);
      }else{
        assert( pLeft && pRight );
        pRoot->pLeft = pLeft;
        pLeft->pParent = pRoot;
        pRoot->pRight = pRight;
        pRight->pParent = pRoot;
      }
    }
  }

  if( rc!=SQLITE_OK ){
    sqlite3Fts3ExprFree(pRoot);
    pRoot = 0;
  }
  *pp = pRoot;
  return rc;
}

/*
** This function is similar to sqlite3Fts3ExprParse(), with the following
** differences:
**
**   1. It does not do expression rebalancing.
**   2. It does not check that the expression does not exceed the
**      maximum allowable depth.
**   3. Even if it fails, *ppExpr may still be set to point to an
**      expression tree. It should be deleted using sqlite3Fts3ExprFree()
**      in this case.
*/
static int fts3ExprParseUnbalanced(
  sqlite3_tokenizer *pTokenizer,      /* Tokenizer module */
  int iLangid,                        /* Language id for tokenizer */
  char **azCol,                       /* Array of column names for fts3 table */
  int bFts4,                          /* True to allow FTS4-only syntax */
  int nCol,                           /* Number of entries in azCol[] */
  int iDefaultCol,                    /* Default column to query */
  const char *z, int n,               /* Text of MATCH query */
  Fts3Expr **ppExpr                   /* OUT: Parsed query structure */
){
  int nParsed;
  int rc;
  ParseContext sParse;

  memset(&sParse, 0, sizeof(ParseContext));
  sParse.pTokenizer = pTokenizer;
  sParse.iLangid = iLangid;
  sParse.azCol = (const char **)azCol;
  sParse.nCol = nCol;
  sParse.iDefaultCol = iDefaultCol;
  sParse.bFts4 = bFts4;
  if( z==0 ){
    *ppExpr = 0;
    return SQLITE_OK;
  }
  if( n<0 ){
    n = (int)strlen(z);
  }
  rc = fts3ExprParse(&sParse, z, n, ppExpr, &nParsed);
  assert( rc==SQLITE_OK || *ppExpr==0 );

  /* Check for mismatched parenthesis */
  if( rc==SQLITE_OK && sParse.nNest ){
    rc = SQLITE_ERROR;
  }

  return rc;
}

/*
** Parameters z and n contain a pointer to and length of a buffer containing
** an fts3 query expression, respectively. This function attempts to parse the
** query expression and create a tree of Fts3Expr structures representing the
** parsed expression. If successful, *ppExpr is set to point to the head
** of the parsed expression tree and SQLITE_OK is returned. If an error
** occurs, either SQLITE_NOMEM (out-of-memory error) or SQLITE_ERROR (parse
** error) is returned and *ppExpr is set to 0.
**
** If parameter n is a negative number, then z is assumed to point to a
** nul-terminated string and the length is determined using strlen().
**
** The first parameter, pTokenizer, is passed the fts3 tokenizer module to
** use to normalize query tokens while parsing the expression. The azCol[]
** array, which is assumed to contain nCol entries, should contain the names
** of each column in the target fts3 table, in order from left to right.
** Column names must be nul-terminated strings.
**
** The iDefaultCol parameter should be passed the index of the table column
** that appears on the left-hand-side of the MATCH operator (the default
** column to match against for tokens for which a column name is not explicitly
** specified as part of the query string), or -1 if tokens may by default
** match any table column.
*/
SQLITE_PRIVATE int sqlite3Fts3ExprParse(
  sqlite3_tokenizer *pTokenizer,      /* Tokenizer module */
  int iLangid,                        /* Language id for tokenizer */
  char **azCol,                       /* Array of column names for fts3 table */
  int bFts4,                          /* True to allow FTS4-only syntax */
  int nCol,                           /* Number of entries in azCol[] */
  int iDefaultCol,                    /* Default column to query */
  const char *z, int n,               /* Text of MATCH query */
  Fts3Expr **ppExpr,                  /* OUT: Parsed query structure */
  char **pzErr                        /* OUT: Error message (sqlite3_malloc) */
){
  int rc = fts3ExprParseUnbalanced(
      pTokenizer, iLangid, azCol, bFts4, nCol, iDefaultCol, z, n, ppExpr
  );

  /* Rebalance the expression. And check that its depth does not exceed
  ** SQLITE_FTS3_MAX_EXPR_DEPTH.  */
  if( rc==SQLITE_OK && *ppExpr ){
    rc = fts3ExprBalance(ppExpr, SQLITE_FTS3_MAX_EXPR_DEPTH);
    if( rc==SQLITE_OK ){
      rc = fts3ExprCheckDepth(*ppExpr, SQLITE_FTS3_MAX_EXPR_DEPTH);
    }
  }

  if( rc!=SQLITE_OK ){
    sqlite3Fts3ExprFree(*ppExpr);
    *ppExpr = 0;
    if( rc==SQLITE_TOOBIG ){
      sqlite3Fts3ErrMsg(pzErr,
          "FTS expression tree is too large (maximum depth %d)",
          SQLITE_FTS3_MAX_EXPR_DEPTH
      );
      rc = SQLITE_ERROR;
    }else if( rc==SQLITE_ERROR ){
      sqlite3Fts3ErrMsg(pzErr, "malformed MATCH expression: [%s]", z);
    }
  }

  return rc;
}

/*
** Free a single node of an expression tree.
*/
static void fts3FreeExprNode(Fts3Expr *p){
  assert( p->eType==FTSQUERY_PHRASE || p->pPhrase==0 );
  sqlite3Fts3EvalPhraseCleanup(p->pPhrase);
  sqlite3_free(p->aMI);
  sqlite3_free(p);
}

/*
** Free a parsed fts3 query expression allocated by sqlite3Fts3ExprParse().
**
** This function would be simpler if it recursively called itself. But
** that would mean passing a sufficiently large expression to ExprParse()
** could cause a stack overflow.
*/
SQLITE_PRIVATE void sqlite3Fts3ExprFree(Fts3Expr *pDel){
  Fts3Expr *p;
  assert( pDel==0 || pDel->pParent==0 );
  for(p=pDel; p && (p->pLeft||p->pRight); p=(p->pLeft ? p->pLeft : p->pRight)){
    assert( p->pParent==0 || p==p->pParent->pRight || p==p->pParent->pLeft );
  }
  while( p ){
    Fts3Expr *pParent = p->pParent;
    fts3FreeExprNode(p);
    if( pParent && p==pParent->pLeft && pParent->pRight ){
      p = pParent->pRight;
      while( p && (p->pLeft || p->pRight) ){
        assert( p==p->pParent->pRight || p==p->pParent->pLeft );
        p = (p->pLeft ? p->pLeft : p->pRight);
      }
    }else{
      p = pParent;
    }
  }
}

/****************************************************************************
*****************************************************************************
** Everything after this point is just test code.
*/

#ifdef SQLITE_TEST

/* #include <stdio.h> */

/*
** Return a pointer to a buffer containing a text representation of the
** expression passed as the first argument. The buffer is obtained from
** sqlite3_malloc(). It is the responsibility of the caller to use
** sqlite3_free() to release the memory. If an OOM condition is encountered,
** NULL is returned.
**
** If the second argument is not NULL, then its contents are prepended to
** the returned expression text and then freed using sqlite3_free().
*/
static char *exprToString(Fts3Expr *pExpr, char *zBuf){
  if( pExpr==0 ){
    return sqlite3_mprintf("");
  }
  switch( pExpr->eType ){
    case FTSQUERY_PHRASE: {
      Fts3Phrase *pPhrase = pExpr->pPhrase;
      int i;
      zBuf = sqlite3_mprintf(
          "%zPHRASE %d 0", zBuf, pPhrase->iColumn);
      for(i=0; zBuf && i<pPhrase->nToken; i++){
        zBuf = sqlite3_mprintf("%z %.*s%s", zBuf,
            pPhrase->aToken[i].n, pPhrase->aToken[i].z,
            (pPhrase->aToken[i].isPrefix?"+":"")
        );
      }
      return zBuf;
    }

    case FTSQUERY_NEAR:
      zBuf = sqlite3_mprintf("%zNEAR/%d ", zBuf, pExpr->nNear);
      break;
    case FTSQUERY_NOT:
      zBuf = sqlite3_mprintf("%zNOT ", zBuf);
      break;
    case FTSQUERY_AND:
      zBuf = sqlite3_mprintf("%zAND ", zBuf);
      break;
    case FTSQUERY_OR:
      zBuf = sqlite3_mprintf("%zOR ", zBuf);
      break;
  }

  if( zBuf ) zBuf = sqlite3_mprintf("%z{", zBuf);
  if( zBuf ) zBuf = exprToString(pExpr->pLeft, zBuf);
  if( zBuf ) zBuf = sqlite3_mprintf("%z} {", zBuf);

  if( zBuf ) zBuf = exprToString(pExpr->pRight, zBuf);
  if( zBuf ) zBuf = sqlite3_mprintf("%z}", zBuf);

  return zBuf;
}

/*
** This is the implementation of a scalar SQL function used to test the
** expression parser. It should be called as follows:
**
**   fts3_exprtest(<tokenizer>, <expr>, <column 1>, ...);
**
** The first argument, <tokenizer>, is the name of the fts3 tokenizer used
** to parse the query expression (see README.tokenizers). The second argument
** is the query expression to parse. Each subsequent argument is the name
** of a column of the fts3 table that the query expression may refer to.
** For example:
**
**   SELECT fts3_exprtest('simple', 'Bill col2:Bloggs', 'col1', 'col2');
*/
static void fts3ExprTestCommon(
  int bRebalance,
  sqlite3_context *context,
  int argc,
  sqlite3_value **argv
){
  sqlite3_tokenizer *pTokenizer = 0;
  int rc;
  char **azCol = 0;
  const char *zExpr;
  int nExpr;
  int nCol;
  int ii;
  Fts3Expr *pExpr;
  char *zBuf = 0;
  Fts3Hash *pHash = (Fts3Hash*)sqlite3_user_data(context);
  const char *zTokenizer = 0;
  char *zErr = 0;

  if( argc<3 ){
    sqlite3_result_error(context,
        "Usage: fts3_exprtest(tokenizer, expr, col1, ...", -1
    );
    return;
  }

  zTokenizer = (const char*)sqlite3_value_text(argv[0]);
  rc = sqlite3Fts3InitTokenizer(pHash, zTokenizer, &pTokenizer, &zErr);
  if( rc!=SQLITE_OK ){
    if( rc==SQLITE_NOMEM ){
      sqlite3_result_error_nomem(context);
    }else{
      sqlite3_result_error(context, zErr, -1);
    }
    sqlite3_free(zErr);
    return;
  }

  zExpr = (const char *)sqlite3_value_text(argv[1]);
  nExpr = sqlite3_value_bytes(argv[1]);
  nCol = argc-2;
  azCol = (char **)sqlite3_malloc64(nCol*sizeof(char *));
  if( !azCol ){
    sqlite3_result_error_nomem(context);
    goto exprtest_out;
  }
  for(ii=0; ii<nCol; ii++){
    azCol[ii] = (char *)sqlite3_value_text(argv[ii+2]);
  }

  if( bRebalance ){
    char *zDummy = 0;
    rc = sqlite3Fts3ExprParse(
        pTokenizer, 0, azCol, 0, nCol, nCol, zExpr, nExpr, &pExpr, &zDummy
    );
    assert( rc==SQLITE_OK || pExpr==0 );
    sqlite3_free(zDummy);
  }else{
    rc = fts3ExprParseUnbalanced(
        pTokenizer, 0, azCol, 0, nCol, nCol, zExpr, nExpr, &pExpr
    );
  }

  if( rc==SQLITE_OK && fts3ExprCheckDepth(pExpr, SQLITE_FTS3_MAX_EXPR_DEPTH) ){
    sqlite3_result_error(context, "Expression nested too deep", -1);
  }else if( rc!=SQLITE_OK && rc!=SQLITE_NOMEM ){
    sqlite3_result_error(context, "Error parsing expression", -1);
  }else if( rc==SQLITE_NOMEM || !(zBuf = exprToString(pExpr, 0)) ){
    sqlite3_result_error_nomem(context);
  }else{
    sqlite3_result_text(context, zBuf, -1, SQLITE_TRANSIENT);
    sqlite3_free(zBuf);
  }

  sqlite3Fts3ExprFree(pExpr);

exprtest_out:
  if( pTokenizer ){
    rc = pTokenizer->pModule->xDestroy(pTokenizer);
  }
  sqlite3_free(azCol);
}

static void fts3ExprTest(
  sqlite3_context *context,
  int argc,
  sqlite3_value **argv
){
  fts3ExprTestCommon(0, context, argc, argv);
}
static void fts3ExprTestRebalance(
  sqlite3_context *context,
  int argc,
  sqlite3_value **argv
){
  fts3ExprTestCommon(1, context, argc, argv);
}

/*
** Register the query expression parser test function fts3_exprtest()
** with database connection db.
*/
SQLITE_PRIVATE int sqlite3Fts3ExprInitTestInterface(sqlite3 *db, Fts3Hash *pHash){
  int rc = sqlite3_create_function(
      db, "fts3_exprtest", -1, SQLITE_UTF8, (void*)pHash, fts3ExprTest, 0, 0
  );
  if( rc==SQLITE_OK ){
    rc = sqlite3_create_function(db, "fts3_exprtest_rebalance",
        -1, SQLITE_UTF8, (void*)pHash, fts3ExprTestRebalance, 0, 0
    );
  }
  return rc;
}

#endif
#endif /* !defined(SQLITE_CORE) || defined(SQLITE_ENABLE_FTS3) */

/************** End of fts3_expr.c *******************************************/
/************** Begin file fts3_hash.c ***************************************/
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
** This is the implementation of generic hash-tables used in SQLite.
** We've modified it slightly to serve as a standalone hash table
** implementation for the full-text indexing module.
*/

/*
** The code in this file is only compiled if:
**
**     * The FTS3 module is being built as an extension
**       (in which case SQLITE_CORE is not defined), or
**
**     * The FTS3 module is being built into the core of
**       SQLite (in which case SQLITE_ENABLE_FTS3 is defined).
*/
/* #include "fts3Int.h" */
#if !defined(SQLITE_CORE) || defined(SQLITE_ENABLE_FTS3)

/* #include <assert.h> */
/* #include <stdlib.h> */
/* #include <string.h> */

/* #include "fts3_hash.h" */

/*
** Malloc and Free functions
*/
static void *fts3HashMalloc(sqlite3_int64 n){
  void *p = sqlite3_malloc64(n);
  if( p ){
    memset(p, 0, n);
  }
  return p;
}
static void fts3HashFree(void *p){
  sqlite3_free(p);
}

/* Turn bulk memory into a hash table object by initializing the
** fields of the Hash structure.
**
** "pNew" is a pointer to the hash table that is to be initialized.
** keyClass is one of the constants
** FTS3_HASH_BINARY or FTS3_HASH_STRING.  The value of keyClass
** determines what kind of key the hash table will use.  "copyKey" is
** true if the hash table should make its own private copy of keys and
** false if it should just use the supplied pointer.
*/
SQLITE_PRIVATE void sqlite3Fts3HashInit(Fts3Hash *pNew, char keyClass, char copyKey){
  assert( pNew!=0 );
  assert( keyClass>=FTS3_HASH_STRING && keyClass<=FTS3_HASH_BINARY );
  pNew->keyClass = keyClass;
  pNew->copyKey = copyKey;
  pNew->first = 0;
  pNew->count = 0;
  pNew->htsize = 0;
  pNew->ht = 0;
}

/* Remove all entries from a hash table.  Reclaim all memory.
** Call this routine to delete a hash table or to reset a hash table
** to the empty state.
*/
SQLITE_PRIVATE void sqlite3Fts3HashClear(Fts3Hash *pH){
  Fts3HashElem *elem;         /* For looping over all elements of the table */

  assert( pH!=0 );
  elem = pH->first;
  pH->first = 0;
  fts3HashFree(pH->ht);
  pH->ht = 0;
  pH->htsize = 0;
  while( elem ){
    Fts3HashElem *next_elem = elem->next;
    if( pH->copyKey && elem->pKey ){
      fts3HashFree(elem->pKey);
    }
    fts3HashFree(elem);
    elem = next_elem;
  }
  pH->count = 0;
}

/*
** Hash and comparison functions when the mode is FTS3_HASH_STRING
*/
static int fts3StrHash(const void *pKey, int nKey){
  const char *z = (const char *)pKey;
  unsigned h = 0;
  if( nKey<=0 ) nKey = (int) strlen(z);
  while( nKey > 0  ){
    h = (h<<3) ^ h ^ *z++;
    nKey--;
  }
  return (int)(h & 0x7fffffff);
}
static int fts3StrCompare(const void *pKey1, int n1, const void *pKey2, int n2){
  if( n1!=n2 ) return 1;
  return strncmp((const char*)pKey1,(const char*)pKey2,n1);
}

/*
** Hash and comparison functions when the mode is FTS3_HASH_BINARY
*/
static int fts3BinHash(const void *pKey, int nKey){
  int h = 0;
  const char *z = (const char *)pKey;
  while( nKey-- > 0 ){
    h = (h<<3) ^ h ^ *(z++);
  }
  return h & 0x7fffffff;
}
static int fts3BinCompare(const void *pKey1, int n1, const void *pKey2, int n2){
  if( n1!=n2 ) return 1;
  return memcmp(pKey1,pKey2,n1);
}

/*
** Return a pointer to the appropriate hash function given the key class.
**
** The C syntax in this function definition may be unfamilar to some
** programmers, so we provide the following additional explanation:
**
** The name of the function is "ftsHashFunction".  The function takes a
** single parameter "keyClass".  The return value of ftsHashFunction()
** is a pointer to another function.  Specifically, the return value
** of ftsHashFunction() is a pointer to a function that takes two parameters
** with types "const void*" and "int" and returns an "int".
*/
static int (*ftsHashFunction(int keyClass))(const void*,int){
  if( keyClass==FTS3_HASH_STRING ){
    return &fts3StrHash;
  }else{
    assert( keyClass==FTS3_HASH_BINARY );
    return &fts3BinHash;
  }
}

/*
** Return a pointer to the appropriate hash function given the key class.
**
** For help in interpreted the obscure C code in the function definition,
** see the header comment on the previous function.
*/
static int (*ftsCompareFunction(int keyClass))(const void*,int,const void*,int){
  if( keyClass==FTS3_HASH_STRING ){
    return &fts3StrCompare;
  }else{
    assert( keyClass==FTS3_HASH_BINARY );
    return &fts3BinCompare;
  }
}

/* Link an element into the hash table
*/
static void fts3HashInsertElement(
  Fts3Hash *pH,            /* The complete hash table */
  struct _fts3ht *pEntry,  /* The entry into which pNew is inserted */
  Fts3HashElem *pNew       /* The element to be inserted */
){
  Fts3HashElem *pHead;     /* First element already in pEntry */
  pHead = pEntry->chain;
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
  pEntry->count++;
  pEntry->chain = pNew;
}


/* Resize the hash table so that it contains "new_size" buckets.
** "new_size" must be a power of 2.  The hash table might fail
** to resize if sqliteMalloc() fails.
**
** Return non-zero if a memory allocation error occurs.
*/
static int fts3Rehash(Fts3Hash *pH, int new_size){
  struct _fts3ht *new_ht;          /* The new hash table */
  Fts3HashElem *elem, *next_elem;  /* For looping over existing elements */
  int (*xHash)(const void*,int);   /* The hash function */

  assert( (new_size & (new_size-1))==0 );
  new_ht = (struct _fts3ht *)fts3HashMalloc( new_size*sizeof(struct _fts3ht) );
  if( new_ht==0 ) return 1;
  fts3HashFree(pH->ht);
  pH->ht = new_ht;
  pH->htsize = new_size;
  xHash = ftsHashFunction(pH->keyClass);
  for(elem=pH->first, pH->first=0; elem; elem = next_elem){
    int h = (*xHash)(elem->pKey, elem->nKey) & (new_size-1);
    next_elem = elem->next;
    fts3HashInsertElement(pH, &new_ht[h], elem);
  }
  return 0;
}

/* This function (for internal use only) locates an element in an
** hash table that matches the given key.  The hash for this key has
** already been computed and is passed as the 4th parameter.
*/
static Fts3HashElem *fts3FindElementByHash(
  const Fts3Hash *pH, /* The pH to be searched */
  const void *pKey,   /* The key we are searching for */
  int nKey,
  int h               /* The hash for this key. */
){
  Fts3HashElem *elem;            /* Used to loop thru the element list */
  int count;                     /* Number of elements left to test */
  int (*xCompare)(const void*,int,const void*,int);  /* comparison function */

  if( pH->ht ){
    struct _fts3ht *pEntry = &pH->ht[h];
    elem = pEntry->chain;
    count = pEntry->count;
    xCompare = ftsCompareFunction(pH->keyClass);
    while( count-- && elem ){
      if( (*xCompare)(elem->pKey,elem->nKey,pKey,nKey)==0 ){
        return elem;
      }
      elem = elem->next;
    }
  }
  return 0;
}

/* Remove a single entry from the hash table given a pointer to that
** element and a hash on the element's key.
*/
static void fts3RemoveElementByHash(
  Fts3Hash *pH,         /* The pH containing "elem" */
  Fts3HashElem* elem,   /* The element to be removed from the pH */
  int h                 /* Hash value for the element */
){
  struct _fts3ht *pEntry;
  if( elem->prev ){
    elem->prev->next = elem->next;
  }else{
    pH->first = elem->next;
  }
  if( elem->next ){
    elem->next->prev = elem->prev;
  }
  pEntry = &pH->ht[h];
  if( pEntry->chain==elem ){
    pEntry->chain = elem->next;
  }
  pEntry->count--;
  if( pEntry->count<=0 ){
    pEntry->chain = 0;
  }
  if( pH->copyKey && elem->pKey ){
    fts3HashFree(elem->pKey);
  }
  fts3HashFree( elem );
  pH->count--;
  if( pH->count<=0 ){
    assert( pH->first==0 );
    assert( pH->count==0 );
    fts3HashClear(pH);
  }
}

SQLITE_PRIVATE Fts3HashElem *sqlite3Fts3HashFindElem(
  const Fts3Hash *pH,
  const void *pKey,
  int nKey
){
  int h;                          /* A hash on key */
  int (*xHash)(const void*,int);  /* The hash function */

  if( pH==0 || pH->ht==0 ) return 0;
  xHash = ftsHashFunction(pH->keyClass);
  assert( xHash!=0 );
  h = (*xHash)(pKey,nKey);
  assert( (pH->htsize & (pH->htsize-1))==0 );
  return fts3FindElementByHash(pH,pKey,nKey, h & (pH->htsize-1));
}

/*
** Attempt to locate an element of the hash table pH with a key
** that matches pKey,nKey.  Return the data for this element if it is
** found, or NULL if there is no match.
*/
SQLITE_PRIVATE void *sqlite3Fts3HashFind(const Fts3Hash *pH, const void *pKey, int nKey){
  Fts3HashElem *pElem;            /* The element that matches key (if any) */

  pElem = sqlite3Fts3HashFindElem(pH, pKey, nKey);
  return pElem ? pElem->data : 0;
}

/* Insert an element into the hash table pH.  The key is pKey,nKey
** and the data is "data".
**
** If no element exists with a matching key, then a new
** element is created.  A copy of the key is made if the copyKey
** flag is set.  NULL is returned.
**
** If another element already exists with the same key, then the
** new data replaces the old data and the old data is returned.
** The key is not copied in this instance.  If a malloc fails, then
** the new data is returned and the hash table is unchanged.
**
** If the "data" parameter to this function is NULL, then the
** element corresponding to "key" is removed from the hash table.
*/
SQLITE_PRIVATE void *sqlite3Fts3HashInsert(
  Fts3Hash *pH,        /* The hash table to insert into */
  const void *pKey,    /* The key */
  int nKey,            /* Number of bytes in the key */
  void *data           /* The data */
){
  int hraw;                 /* Raw hash value of the key */
  int h;                    /* the hash of the key modulo hash table size */
  Fts3HashElem *elem;       /* Used to loop thru the element list */
  Fts3HashElem *new_elem;   /* New element added to the pH */
  int (*xHash)(const void*,int);  /* The hash function */

  assert( pH!=0 );
  xHash = ftsHashFunction(pH->keyClass);
  assert( xHash!=0 );
  hraw = (*xHash)(pKey, nKey);
  assert( (pH->htsize & (pH->htsize-1))==0 );
  h = hraw & (pH->htsize-1);
  elem = fts3FindElementByHash(pH,pKey,nKey,h);
  if( elem ){
    void *old_data = elem->data;
    if( data==0 ){
      fts3RemoveElementByHash(pH,elem,h);
    }else{
      elem->data = data;
    }
    return old_data;
  }
  if( data==0 ) return 0;
  if( (pH->htsize==0 && fts3Rehash(pH,8))
   || (pH->count>=pH->htsize && fts3Rehash(pH, pH->htsize*2))
  ){
    pH->count = 0;
    return data;
  }
  assert( pH->htsize>0 );
  new_elem = (Fts3HashElem*)fts3HashMalloc( sizeof(Fts3HashElem) );
  if( new_elem==0 ) return data;
  if( pH->copyKey && pKey!=0 ){
    new_elem->pKey = fts3HashMalloc( nKey );
    if( new_elem->pKey==0 ){
      fts3HashFree(new_elem);
      return data;
    }
    memcpy((void*)new_elem->pKey, pKey, nKey);
  }else{
    new_elem->pKey = (void*)pKey;
  }
  new_elem->nKey = nKey;
  pH->count++;
  assert( pH->htsize>0 );
  assert( (pH->htsize & (pH->htsize-1))==0 );
  h = hraw & (pH->htsize-1);
  fts3HashInsertElement(pH, &pH->ht[h], new_elem);
  new_elem->data = data;
  return 0;
}

#endif /* !defined(SQLITE_CORE) || defined(SQLITE_ENABLE_FTS3) */

/************** End of fts3_hash.c *******************************************/
/************** Begin file fts3_porter.c *************************************/
/*
** 2006 September 30
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
*************************************************************************
** Implementation of the full-text-search tokenizer that implements
** a Porter stemmer.
*/

/*
** The code in this file is only compiled if:
**
**     * The FTS3 module is being built as an extension
**       (in which case SQLITE_CORE is not defined), or
**
**     * The FTS3 module is being built into the core of
**       SQLite (in which case SQLITE_ENABLE_FTS3 is defined).
*/
/* #include "fts3Int.h" */
#if !defined(SQLITE_CORE) || defined(SQLITE_ENABLE_FTS3)

/* #include <assert.h> */
/* #include <stdlib.h> */
/* #include <stdio.h> */
/* #include <string.h> */

/* #include "fts3_tokenizer.h" */

/*
** Class derived from sqlite3_tokenizer
*/
typedef struct porter_tokenizer {
  sqlite3_tokenizer base;      /* Base class */
} porter_tokenizer;

/*
** Class derived from sqlite3_tokenizer_cursor
*/
typedef struct porter_tokenizer_cursor {
  sqlite3_tokenizer_cursor base;
  const char *zInput;          /* input we are tokenizing */
  int nInput;                  /* size of the input */
  int iOffset;                 /* current position in zInput */
  int iToken;                  /* index of next token to be returned */
  char *zToken;                /* storage for current token */
  int nAllocated;              /* space allocated to zToken buffer */
} porter_tokenizer_cursor;


/*
** Create a new tokenizer instance.
*/
static int porterCreate(
  int argc, const char * const *argv,
  sqlite3_tokenizer **ppTokenizer
){
  porter_tokenizer *t;

  UNUSED_PARAMETER(argc);
  UNUSED_PARAMETER(argv);

  t = (porter_tokenizer *) sqlite3_malloc(sizeof(*t));
  if( t==NULL ) return SQLITE_NOMEM;
  memset(t, 0, sizeof(*t));
  *ppTokenizer = &t->base;
  return SQLITE_OK;
}

/*
** Destroy a tokenizer
*/
static int porterDestroy(sqlite3_tokenizer *pTokenizer){
  sqlite3_free(pTokenizer);
  return SQLITE_OK;
}

/*
** Prepare to begin tokenizing a particular string.  The input
** string to be tokenized is zInput[0..nInput-1].  A cursor
** used to incrementally tokenize this string is returned in
** *ppCursor.
*/
static int porterOpen(
  sqlite3_tokenizer *pTokenizer,         /* The tokenizer */
  const char *zInput, int nInput,        /* String to be tokenized */
  sqlite3_tokenizer_cursor **ppCursor    /* OUT: Tokenization cursor */
){
  porter_tokenizer_cursor *c;

  UNUSED_PARAMETER(pTokenizer);

  c = (porter_tokenizer_cursor *) sqlite3_malloc(sizeof(*c));
  if( c==NULL ) return SQLITE_NOMEM;

  c->zInput = zInput;
  if( zInput==0 ){
    c->nInput = 0;
  }else if( nInput<0 ){
    c->nInput = (int)strlen(zInput);
  }else{
    c->nInput = nInput;
  }
  c->iOffset = 0;                 /* start tokenizing at the beginning */
  c->iToken = 0;
  c->zToken = NULL;               /* no space allocated, yet. */
  c->nAllocated = 0;

  *ppCursor = &c->base;
  return SQLITE_OK;
}

/*
** Close a tokenization cursor previously opened by a call to
** porterOpen() above.
*/
static int porterClose(sqlite3_tokenizer_cursor *pCursor){
  porter_tokenizer_cursor *c = (porter_tokenizer_cursor *) pCursor;
  sqlite3_free(c->zToken);
  sqlite3_free(c);
  return SQLITE_OK;
}
/*
** Vowel or consonant
*/
static const char cType[] = {
   0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0,
   1, 1, 1, 2, 1
};

/*
** isConsonant() and isVowel() determine if their first character in
** the string they point to is a consonant or a vowel, according
** to Porter ruls.
**
** A consonate is any letter other than 'a', 'e', 'i', 'o', or 'u'.
** 'Y' is a consonant unless it follows another consonant,
** in which case it is a vowel.
**
** In these routine, the letters are in reverse order.  So the 'y' rule
** is that 'y' is a consonant unless it is followed by another
** consonent.
*/
static int isVowel(const char*);
static int isConsonant(const char *z){
  int j;
  char x = *z;
  if( x==0 ) return 0;
  assert( x>='a' && x<='z' );
  j = cType[x-'a'];
  if( j<2 ) return j;
  return z[1]==0 || isVowel(z + 1);
}
static int isVowel(const char *z){
  int j;
  char x = *z;
  if( x==0 ) return 0;
  assert( x>='a' && x<='z' );
  j = cType[x-'a'];
  if( j<2 ) return 1-j;
  return isConsonant(z + 1);
}

/*
** Let any sequence of one or more vowels be represented by V and let
** C be sequence of one or more consonants.  Then every word can be
** represented as:
**
**           [C] (VC){m} [V]
**
** In prose:  A word is an optional consonant followed by zero or
** vowel-consonant pairs followed by an optional vowel.  "m" is the
** number of vowel consonant pairs.  This routine computes the value
** of m for the first i bytes of a word.
**
** Return true if the m-value for z is 1 or more.  In other words,
** return true if z contains at least one vowel that is followed
** by a consonant.
**
** In this routine z[] is in reverse order.  So we are really looking
** for an instance of a consonant followed by a vowel.
*/
static int m_gt_0(const char *z){
  while( isVowel(z) ){ z++; }
  if( *z==0 ) return 0;
  while( isConsonant(z) ){ z++; }
  return *z!=0;
}

/* Like mgt0 above except we are looking for a value of m which is
** exactly 1
*/
static int m_eq_1(const char *z){
  while( isVowel(z) ){ z++; }
  if( *z==0 ) return 0;
  while( isConsonant(z) ){ z++; }
  if( *z==0 ) return 0;
  while( isVowel(z) ){ z++; }
  if( *z==0 ) return 1;
  while( isConsonant(z) ){ z++; }
  return *z==0;
}

/* Like mgt0 above except we are looking for a value of m>1 instead
** or m>0
*/
static int m_gt_1(const char *z){
  while( isVowel(z) ){ z++; }
  if( *z==0 ) return 0;
  while( isConsonant(z) ){ z++; }
  if( *z==0 ) return 0;
  while( isVowel(z) ){ z++; }
  if( *z==0 ) return 0;
  while( isConsonant(z) ){ z++; }
  return *z!=0;
}

/*
** Return TRUE if there is a vowel anywhere within z[0..n-1]
*/
static int hasVowel(const char *z){
  while( isConsonant(z) ){ z++; }
  return *z!=0;
}

/*
** Return TRUE if the word ends in a double consonant.
**
** The text is reversed here. So we are really looking at
** the first two characters of z[].
*/
static int doubleConsonant(const char *z){
  return isConsonant(z) && z[0]==z[1];
}

/*
** Return TRUE if the word ends with three letters which
** are consonant-vowel-consonent and where the final consonant
** is not 'w', 'x', or 'y'.
**
** The word is reversed here.  So we are really checking the
** first three letters and the first one cannot be in [wxy].
*/
static int star_oh(const char *z){
  return
    isConsonant(z) &&
    z[0]!='w' && z[0]!='x' && z[0]!='y' &&
    isVowel(z+1) &&
    isConsonant(z+2);
}

/*
** If the word ends with zFrom and xCond() is true for the stem
** of the word that precedes the zFrom ending, then change the
** ending to zTo.
**
** The input word *pz and zFrom are both in reverse order.  zTo
** is in normal order.
**
** Return TRUE if zFrom matches.  Return FALSE if zFrom does not
** match.  Not that TRUE is returned even if xCond() fails and
** no substitution occurs.
*/
static int stem(
  char **pz,             /* The word being stemmed (Reversed) */
  const char *zFrom,     /* If the ending matches this... (Reversed) */
  const char *zTo,       /* ... change the ending to this (not reversed) */
  int (*xCond)(const char*)   /* Condition that must be true */
){
  char *z = *pz;
  while( *zFrom && *zFrom==*z ){ z++; zFrom++; }
  if( *zFrom!=0 ) return 0;
  if( xCond && !xCond(z) ) return 1;
  while( *zTo ){
    *(--z) = *(zTo++);
  }
  *pz = z;
  return 1;
}

/*
** This is the fallback stemmer used when the porter stemmer is
** inappropriate.  The input word is copied into the output with
** US-ASCII case folding.  If the input word is too long (more
** than 20 bytes if it contains no digits or more than 6 bytes if
** it contains digits) then word is truncated to 20 or 6 bytes
** by taking 10 or 3 bytes from the beginning and end.
*/
static void copy_stemmer(const char *zIn, int nIn, char *zOut, int *pnOut){
  int i, mx, j;
  int hasDigit = 0;
  for(i=0; i<nIn; i++){
    char c = zIn[i];
    if( c>='A' && c<='Z' ){
      zOut[i] = c - 'A' + 'a';
    }else{
      if( c>='0' && c<='9' ) hasDigit = 1;
      zOut[i] = c;
    }
  }
  mx = hasDigit ? 3 : 10;
  if( nIn>mx*2 ){
    for(j=mx, i=nIn-mx; i<nIn; i++, j++){
      zOut[j] = zOut[i];
    }
    i = j;
  }
  zOut[i] = 0;
  *pnOut = i;
}


/*
** Stem the input word zIn[0..nIn-1].  Store the output in zOut.
** zOut is at least big enough to hold nIn bytes.  Write the actual
** size of the output word (exclusive of the '\0' terminator) into *pnOut.
**
** Any upper-case characters in the US-ASCII character set ([A-Z])
** are converted to lower case.  Upper-case UTF characters are
** unchanged.
**
** Words that are longer than about 20 bytes are stemmed by retaining
** a few bytes from the beginning and the end of the word.  If the
** word contains digits, 3 bytes are taken from the beginning and
** 3 bytes from the end.  For long words without digits, 10 bytes
** are taken from each end.  US-ASCII case folding still applies.
**
** If the input word contains not digits but does characters not
** in [a-zA-Z] then no stemming is attempted and this routine just
** copies the input into the input into the output with US-ASCII
** case folding.
**
** Stemming never increases the length of the word.  So there is
** no chance of overflowing the zOut buffer.
*/
static void porter_stemmer(const char *zIn, int nIn, char *zOut, int *pnOut){
  int i, j;
  char zReverse[28];
  char *z, *z2;
  if( nIn<3 || nIn>=(int)sizeof(zReverse)-7 ){
    /* The word is too big or too small for the porter stemmer.
    ** Fallback to the copy stemmer */
    copy_stemmer(zIn, nIn, zOut, pnOut);
    return;
  }
  for(i=0, j=sizeof(zReverse)-6; i<nIn; i++, j--){
    char c = zIn[i];
    if( c>='A' && c<='Z' ){
      zReverse[j] = c + 'a' - 'A';
    }else if( c>='a' && c<='z' ){
      zReverse[j] = c;
    }else{
      /* The use of a character not in [a-zA-Z] means that we fallback
      ** to the copy stemmer */
      copy_stemmer(zIn, nIn, zOut, pnOut);
      return;
    }
  }
  memset(&zReverse[sizeof(zReverse)-5], 0, 5);
  z = &zReverse[j+1];


  /* Step 1a */
  if( z[0]=='s' ){
    if(
     !stem(&z, "sess", "ss", 0) &&
     !stem(&z, "sei", "i", 0)  &&
     !stem(&z, "ss", "ss", 0)
    ){
      z++;
    }
  }

  /* Step 1b */
  z2 = z;
  if( stem(&z, "dee", "ee", m_gt_0) ){
    /* Do nothing.  The work was all in the test */
  }else if(
     (stem(&z, "gni", "", hasVowel) || stem(&z, "de", "", hasVowel))
      && z!=z2
  ){
     if( stem(&z, "ta", "ate", 0) ||
         stem(&z, "lb", "ble", 0) ||
         stem(&z, "zi", "ize", 0) ){
       /* Do nothing.  The work was all in the test */
     }else if( doubleConsonant(z) && (*z!='l' && *z!='s' && *z!='z') ){
       z++;
     }else if( m_eq_1(z) && star_oh(z) ){
       *(--z) = 'e';
     }
  }

  /* Step 1c */
  if( z[0]=='y' && hasVowel(z+1) ){
    z[0] = 'i';
  }

  /* Step 2 */
  switch( z[1] ){
   case 'a':
     if( !stem(&z, "lanoita", "ate", m_gt_0) ){
       stem(&z, "lanoit", "tion", m_gt_0);
     }
     break;
   case 'c':
     if( !stem(&z, "icne", "ence", m_gt_0) ){
       stem(&z, "icna", "ance", m_gt_0);
     }
     break;
   case 'e':
     stem(&z, "rezi", "ize", m_gt_0);
     break;
   case 'g':
     stem(&z, "igol", "log", m_gt_0);
     break;
   case 'l':
     if( !stem(&z, "ilb", "ble", m_gt_0)
      && !stem(&z, "illa", "al", m_gt_0)
      && !stem(&z, "iltne", "ent", m_gt_0)
      && !stem(&z, "ile", "e", m_gt_0)
     ){
       stem(&z, "ilsuo", "ous", m_gt_0);
     }
     break;
   case 'o':
     if( !stem(&z, "noitazi", "ize", m_gt_0)
      && !stem(&z, "noita", "ate", m_gt_0)
     ){
       stem(&z, "rota", "ate", m_gt_0);
     }
     break;
   case 's':
     if( !stem(&z, "msila", "al", m_gt_0)
      && !stem(&z, "ssenevi", "ive", m_gt_0)
      && !stem(&z, "ssenluf", "ful", m_gt_0)
     ){
       stem(&z, "ssensuo", "ous", m_gt_0);
     }
     break;
   case 't':
     if( !stem(&z, "itila", "al", m_gt_0)
      && !stem(&z, "itivi", "ive", m_gt_0)
     ){
       stem(&z, "itilib", "ble", m_gt_0);
     }
     break;
  }

  /* Step 3 */
  switch( z[0] ){
   case 'e':
     if( !stem(&z, "etaci", "ic", m_gt_0)
      && !stem(&z, "evita", "", m_gt_0)
     ){
       stem(&z, "ezila", "al", m_gt_0);
     }
     break;
   case 'i':
     stem(&z, "itici", "ic", m_gt_0);
     break;
   case 'l':
     if( !stem(&z, "laci", "ic", m_gt_0) ){
       stem(&z, "luf", "", m_gt_0);
     }
     break;
   case 's':
     stem(&z, "ssen", "", m_gt_0);
     break;
  }

  /* Step 4 */
  switch( z[1] ){
   case 'a':
     if( z[0]=='l' && m_gt_1(z+2) ){
       z += 2;
     }
     break;
   case 'c':
     if( z[0]=='e' && z[2]=='n' && (z[3]=='a' || z[3]=='e')  && m_gt_1(z+4)  ){
       z += 4;
     }
     break;
   case 'e':
     if( z[0]=='r' && m_gt_1(z+2) ){
       z += 2;
     }
     break;
   case 'i':
     if( z[0]=='c' && m_gt_1(z+2) ){
       z += 2;
     }
     break;
   case 'l':
     if( z[0]=='e' && z[2]=='b' && (z[3]=='a' || z[3]=='i') && m_gt_1(z+4) ){
       z += 4;
     }
     break;
   case 'n':
     if( z[0]=='t' ){
       if( z[2]=='a' ){
         if( m_gt_1(z+3) ){
           z += 3;
         }
       }else if( z[2]=='e' ){
         if( !stem(&z, "tneme", "", m_gt_1)
          && !stem(&z, "tnem", "", m_gt_1)
         ){
           stem(&z, "tne", "", m_gt_1);
         }
       }
     }
     break;
   case 'o':
     if( z[0]=='u' ){
       if( m_gt_1(z+2) ){
         z += 2;
       }
     }else if( z[3]=='s' || z[3]=='t' ){
       stem(&z, "noi", "", m_gt_1);
     }
     break;
   case 's':
     if( z[0]=='m' && z[2]=='i' && m_gt_1(z+3) ){
       z += 3;
     }
     break;
   case 't':
     if( !stem(&z, "eta", "", m_gt_1) ){
       stem(&z, "iti", "", m_gt_1);
     }
     break;
   case 'u':
     if( z[0]=='s' && z[2]=='o' && m_gt_1(z+3) ){
       z += 3;
     }
     break;
   case 'v':
   case 'z':
     if( z[0]=='e' && z[2]=='i' && m_gt_1(z+3) ){
       z += 3;
     }
     break;
  }

  /* Step 5a */
  if( z[0]=='e' ){
    if( m_gt_1(z+1) ){
      z++;
    }else if( m_eq_1(z+1) && !star_oh(z+1) ){
      z++;
    }
  }

  /* Step 5b */
  if( m_gt_1(z) && z[0]=='l' && z[1]=='l' ){
    z++;
  }

  /* z[] is now the stemmed word in reverse order.  Flip it back
  ** around into forward order and return.
  */
  *pnOut = i = (int)strlen(z);
  zOut[i] = 0;
  while( *z ){
    zOut[--i] = *(z++);
  }
}

/*
** Characters that can be part of a token.  We assume any character
** whose value is greater than 0x80 (any UTF character) can be
** part of a token.  In other words, delimiters all must have
** values of 0x7f or lower.
*/
static const char porterIdChar[] = {
/* x0 x1 x2 x3 x4 x5 x6 x7 x8 x9 xA xB xC xD xE xF */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,  /* 3x */
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /* 4x */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1,  /* 5x */
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /* 6x */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,  /* 7x */
};
#define isDelim(C) (((ch=C)&0x80)==0 && (ch<0x30 || !porterIdChar[ch-0x30]))

/*
** Extract the next token from a tokenization cursor.  The cursor must
** have been opened by a prior call to porterOpen().
*/
static int porterNext(
  sqlite3_tokenizer_cursor *pCursor,  /* Cursor returned by porterOpen */
  const char **pzToken,               /* OUT: *pzToken is the token text */
  int *pnBytes,                       /* OUT: Number of bytes in token */
  int *piStartOffset,                 /* OUT: Starting offset of token */
  int *piEndOffset,                   /* OUT: Ending offset of token */
  int *piPosition                     /* OUT: Position integer of token */
){
  porter_tokenizer_cursor *c = (porter_tokenizer_cursor *) pCursor;
  const char *z = c->zInput;

  while( c->iOffset<c->nInput ){
    int iStartOffset, ch;

    /* Scan past delimiter characters */
    while( c->iOffset<c->nInput && isDelim(z[c->iOffset]) ){
      c->iOffset++;
    }

    /* Count non-delimiter characters. */
    iStartOffset = c->iOffset;
    while( c->iOffset<c->nInput && !isDelim(z[c->iOffset]) ){
      c->iOffset++;
    }

    if( c->iOffset>iStartOffset ){
      int n = c->iOffset-iStartOffset;
      if( n>c->nAllocated ){
        i64 nNew = n + 20;
        char *pNew;
        pNew = sqlite3_realloc64(c->zToken, nNew);
        if( !pNew ) return SQLITE_NOMEM;
        c->nAllocated = (int)nNew;
        c->zToken = pNew;
      }
      porter_stemmer(&z[iStartOffset], n, c->zToken, pnBytes);
      *pzToken = c->zToken;
      *piStartOffset = iStartOffset;
      *piEndOffset = c->iOffset;
      *piPosition = c->iToken++;
      return SQLITE_OK;
    }
  }
  return SQLITE_DONE;
}

/*
** The set of routines that implement the porter-stemmer tokenizer
*/
static const sqlite3_tokenizer_module porterTokenizerModule = {
  0,
  porterCreate,
  porterDestroy,
  porterOpen,
  porterClose,
  porterNext,
  0
};

/*
** Allocate a new porter tokenizer.  Return a pointer to the new
** tokenizer in *ppModule
*/
SQLITE_PRIVATE void sqlite3Fts3PorterTokenizerModule(
  sqlite3_tokenizer_module const**ppModule
){
  *ppModule = &porterTokenizerModule;
}

#endif /* !defined(SQLITE_CORE) || defined(SQLITE_ENABLE_FTS3) */

/************** End of fts3_porter.c *****************************************/
/************** Begin file fts3_tokenizer.c **********************************/
/*
** 2007 June 22
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
** This is part of an SQLite module implementing full-text search.
** This particular file implements the generic tokenizer interface.
*/

/*
** The code in this file is only compiled if:
**
**     * The FTS3 module is being built as an extension
**       (in which case SQLITE_CORE is not defined), or
**
**     * The FTS3 module is being built into the core of
**       SQLite (in which case SQLITE_ENABLE_FTS3 is defined).
*/
/* #include "fts3Int.h" */
#if !defined(SQLITE_CORE) || defined(SQLITE_ENABLE_FTS3)

/* #include <assert.h> */
/* #include <string.h> */

/*
** Return true if the two-argument version of fts3_tokenizer()
** has been activated via a prior call to sqlite3_db_config(db,
** SQLITE_DBCONFIG_ENABLE_FTS3_TOKENIZER, 1, 0);
*/
static int fts3TokenizerEnabled(sqlite3_context *context){
  sqlite3 *db = sqlite3_context_db_handle(context);
  int isEnabled = 0;
  sqlite3_db_config(db,SQLITE_DBCONFIG_ENABLE_FTS3_TOKENIZER,-1,&isEnabled);
  return isEnabled;
}

/*
** Implementation of the SQL scalar function for accessing the underlying
** hash table. This function may be called as follows:
**
**   SELECT <function-name>(<key-name>);
**   SELECT <function-name>(<key-name>, <pointer>);
**
** where <function-name> is the name passed as the second argument
** to the sqlite3Fts3InitHashTable() function (e.g. 'fts3_tokenizer').
**
** If the <pointer> argument is specified, it must be a blob value
** containing a pointer to be stored as the hash data corresponding
** to the string <key-name>. If <pointer> is not specified, then
** the string <key-name> must already exist in the has table. Otherwise,
** an error is returned.
**
** Whether or not the <pointer> argument is specified, the value returned
** is a blob containing the pointer stored as the hash data corresponding
** to string <key-name> (after the hash-table is updated, if applicable).
*/
static void fts3TokenizerFunc(
  sqlite3_context *context,
  int argc,
  sqlite3_value **argv
){
  Fts3Hash *pHash;
  void *pPtr = 0;
  const unsigned char *zName;
  int nName;

  assert( argc==1 || argc==2 );

  pHash = (Fts3Hash *)sqlite3_user_data(context);

  zName = sqlite3_value_text(argv[0]);
  nName = sqlite3_value_bytes(argv[0])+1;

  if( argc==2 ){
    if( fts3TokenizerEnabled(context) || sqlite3_value_frombind(argv[1]) ){
      void *pOld;
      int n = sqlite3_value_bytes(argv[1]);
      if( zName==0 || n!=sizeof(pPtr) ){
        sqlite3_result_error(context, "argument type mismatch", -1);
        return;
      }
      pPtr = *(void **)sqlite3_value_blob(argv[1]);
      pOld = sqlite3Fts3HashInsert(pHash, (void *)zName, nName, pPtr);
      if( pOld==pPtr ){
        sqlite3_result_error(context, "out of memory", -1);
      }
    }else{
      sqlite3_result_error(context, "fts3tokenize disabled", -1);
      return;
    }
  }else{
    if( zName ){
      pPtr = sqlite3Fts3HashFind(pHash, zName, nName);
    }
    if( !pPtr ){
      char *zErr = sqlite3_mprintf("unknown tokenizer: %s", zName);
      sqlite3_result_error(context, zErr, -1);
      sqlite3_free(zErr);
      return;
    }
  }
  if( fts3TokenizerEnabled(context) || sqlite3_value_frombind(argv[0]) ){
    sqlite3_result_blob(context, (void *)&pPtr, sizeof(pPtr), SQLITE_TRANSIENT);
  }
}

SQLITE_PRIVATE int sqlite3Fts3IsIdChar(char c){
  static const char isFtsIdChar[] = {
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* 0x */
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* 1x */
      0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* 2x */
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,  /* 3x */
      0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /* 4x */
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1,  /* 5x */
      0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /* 6x */
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,  /* 7x */
  };
  return (c&0x80 || isFtsIdChar[(int)(c)]);
}

SQLITE_PRIVATE const char *sqlite3Fts3NextToken(const char *zStr, int *pn){
  const char *z1;
  const char *z2 = 0;

  /* Find the start of the next token. */
  z1 = zStr;
  while( z2==0 ){
    char c = *z1;
    switch( c ){
      case '\0': return 0;        /* No more tokens here */
      case '\'':
      case '"':
      case '`': {
        z2 = z1;
        while( *++z2 && (*z2!=c || *++z2==c) );
        break;
      }
      case '[':
        z2 = &z1[1];
        while( *z2 && z2[0]!=']' ) z2++;
        if( *z2 ) z2++;
        break;

      default:
        if( sqlite3Fts3IsIdChar(*z1) ){
          z2 = &z1[1];
          while( sqlite3Fts3IsIdChar(*z2) ) z2++;
        }else{
          z1++;
        }
    }
  }

  *pn = (int)(z2-z1);
  return z1;
}

SQLITE_PRIVATE int sqlite3Fts3InitTokenizer(
  Fts3Hash *pHash,                /* Tokenizer hash table */
  const char *zArg,               /* Tokenizer name */
  sqlite3_tokenizer **ppTok,      /* OUT: Tokenizer (if applicable) */
  char **pzErr                    /* OUT: Set to malloced error message */
){
  int rc;
  char *z = (char *)zArg;
  int n = 0;
  char *zCopy;
  char *zEnd;                     /* Pointer to nul-term of zCopy */
  sqlite3_tokenizer_module *m;

  zCopy = sqlite3_mprintf("%s", zArg);
  if( !zCopy ) return SQLITE_NOMEM;
  zEnd = &zCopy[strlen(zCopy)];

  z = (char *)sqlite3Fts3NextToken(zCopy, &n);
  if( z==0 ){
    assert( n==0 );
    z = zCopy;
  }
  z[n] = '\0';
  sqlite3Fts3Dequote(z);

  m = (sqlite3_tokenizer_module *)sqlite3Fts3HashFind(pHash,z,(int)strlen(z)+1);
  if( !m ){
    sqlite3Fts3ErrMsg(pzErr, "unknown tokenizer: %s", z);
    rc = SQLITE_ERROR;
  }else{
    char const **aArg = 0;
    int iArg = 0;
    z = &z[n+1];
    while( z<zEnd && (NULL!=(z = (char *)sqlite3Fts3NextToken(z, &n))) ){
      sqlite3_int64 nNew = sizeof(char *)*(iArg+1);
      char const **aNew = (const char **)sqlite3_realloc64((void *)aArg, nNew);
      if( !aNew ){
        sqlite3_free(zCopy);
        sqlite3_free((void *)aArg);
        return SQLITE_NOMEM;
      }
      aArg = aNew;
      aArg[iArg++] = z;
      z[n] = '\0';
      sqlite3Fts3Dequote(z);
      z = &z[n+1];
    }
    rc = m->xCreate(iArg, aArg, ppTok);
    assert( rc!=SQLITE_OK || *ppTok );
    if( rc!=SQLITE_OK ){
      sqlite3Fts3ErrMsg(pzErr, "unknown tokenizer");
    }else{
      (*ppTok)->pModule = m;
    }
    sqlite3_free((void *)aArg);
  }

  sqlite3_free(zCopy);
  return rc;
}


#ifdef SQLITE_TEST

#include "tclsqlite.h"
/* #include <string.h> */

/*
** Implementation of a special SQL scalar function for testing tokenizers
** designed to be used in concert with the Tcl testing framework. This
** function must be called with two or more arguments:
**
**   SELECT <function-name>(<key-name>, ..., <input-string>);
**
** where <function-name> is the name passed as the second argument
** to the sqlite3Fts3InitHashTable() function (e.g. 'fts3_tokenizer')
** concatenated with the string '_test' (e.g. 'fts3_tokenizer_test').
**
** The return value is a string that may be interpreted as a Tcl
** list. For each token in the <input-string>, three elements are
** added to the returned list. The first is the token position, the
** second is the token text (folded, stemmed, etc.) and the third is the
** substring of <input-string> associated with the token. For example,
** using the built-in "simple" tokenizer:
**
**   SELECT fts_tokenizer_test('simple', 'I don't see how');
**
** will return the string:
**
**   "{0 i I 1 dont don't 2 see see 3 how how}"
**
*/
static void testFunc(
  sqlite3_context *context,
  int argc,
  sqlite3_value **argv
){
  Fts3Hash *pHash;
  sqlite3_tokenizer_module *p;
  sqlite3_tokenizer *pTokenizer = 0;
  sqlite3_tokenizer_cursor *pCsr = 0;

  const char *zErr = 0;

  const char *zName;
  int nName;
  const char *zInput;
  int nInput;

  const char *azArg[64];

  const char *zToken;
  int nToken = 0;
  int iStart = 0;
  int iEnd = 0;
  int iPos = 0;
  int i;

  Tcl_Obj *pRet;

  if( argc<2 ){
    sqlite3_result_error(context, "insufficient arguments", -1);
    return;
  }

  nName = sqlite3_value_bytes(argv[0]);
  zName = (const char *)sqlite3_value_text(argv[0]);
  nInput = sqlite3_value_bytes(argv[argc-1]);
  zInput = (const char *)sqlite3_value_text(argv[argc-1]);

  pHash = (Fts3Hash *)sqlite3_user_data(context);
  p = (sqlite3_tokenizer_module *)sqlite3Fts3HashFind(pHash, zName, nName+1);

  if( !p ){
    char *zErr2 = sqlite3_mprintf("unknown tokenizer: %s", zName);
    sqlite3_result_error(context, zErr2, -1);
    sqlite3_free(zErr2);
    return;
  }

  pRet = Tcl_NewObj();
  Tcl_IncrRefCount(pRet);

  for(i=1; i<argc-1; i++){
    azArg[i-1] = (const char *)sqlite3_value_text(argv[i]);
  }

  if( SQLITE_OK!=p->xCreate(argc-2, azArg, &pTokenizer) ){
    zErr = "error in xCreate()";
    goto finish;
  }
  pTokenizer->pModule = p;
  if( sqlite3Fts3OpenTokenizer(pTokenizer, 0, zInput, nInput, &pCsr) ){
    zErr = "error in xOpen()";
    goto finish;
  }

  while( SQLITE_OK==p->xNext(pCsr, &zToken, &nToken, &iStart, &iEnd, &iPos) ){
    Tcl_ListObjAppendElement(0, pRet, Tcl_NewIntObj(iPos));
    Tcl_ListObjAppendElement(0, pRet, Tcl_NewStringObj(zToken, nToken));
    zToken = &zInput[iStart];
    nToken = iEnd-iStart;
    Tcl_ListObjAppendElement(0, pRet, Tcl_NewStringObj(zToken, nToken));
  }

  if( SQLITE_OK!=p->xClose(pCsr) ){
    zErr = "error in xClose()";
    goto finish;
  }
  if( SQLITE_OK!=p->xDestroy(pTokenizer) ){
    zErr = "error in xDestroy()";
    goto finish;
  }

finish:
  if( zErr ){
    sqlite3_result_error(context, zErr, -1);
  }else{
    sqlite3_result_text(context, Tcl_GetString(pRet), -1, SQLITE_TRANSIENT);
  }
  Tcl_DecrRefCount(pRet);
}

static
int registerTokenizer(
  sqlite3 *db,
  char *zName,
  const sqlite3_tokenizer_module *p
){
  int rc;
  sqlite3_stmt *pStmt;
  const char zSql[] = "SELECT fts3_tokenizer(?, ?)";

  rc = sqlite3_prepare_v2(db, zSql, -1, &pStmt, 0);
  if( rc!=SQLITE_OK ){
    return rc;
  }

  sqlite3_bind_text(pStmt, 1, zName, -1, SQLITE_STATIC);
  sqlite3_bind_blob(pStmt, 2, &p, sizeof(p), SQLITE_STATIC);
  sqlite3_step(pStmt);

  return sqlite3_finalize(pStmt);
}


static
int queryTokenizer(
  sqlite3 *db,
  char *zName,
  const sqlite3_tokenizer_module **pp
){
  int rc;
  sqlite3_stmt *pStmt;
  const char zSql[] = "SELECT fts3_tokenizer(?)";

  *pp = 0;
  rc = sqlite3_prepare_v2(db, zSql, -1, &pStmt, 0);
  if( rc!=SQLITE_OK ){
    return rc;
  }

  sqlite3_bind_text(pStmt, 1, zName, -1, SQLITE_STATIC);
  if( SQLITE_ROW==sqlite3_step(pStmt) ){
    if( sqlite3_column_type(pStmt, 0)==SQLITE_BLOB
     && sqlite3_column_bytes(pStmt, 0)==sizeof(*pp)
    ){
      memcpy((void *)pp, sqlite3_column_blob(pStmt, 0), sizeof(*pp));
    }
  }

  return sqlite3_finalize(pStmt);
}

SQLITE_PRIVATE void sqlite3Fts3SimpleTokenizerModule(sqlite3_tokenizer_module const**ppModule);

/*
** Implementation of the scalar function fts3_tokenizer_internal_test().
** This function is used for testing only, it is not included in the
** build unless SQLITE_TEST is defined.
**
** The purpose of this is to test that the fts3_tokenizer() function
** can be used as designed by the C-code in the queryTokenizer and
** registerTokenizer() functions above. These two functions are repeated
** in the README.tokenizer file as an example, so it is important to
** test them.
**
** To run the tests, evaluate the fts3_tokenizer_internal_test() scalar
** function with no arguments. An assert() will fail if a problem is
** detected. i.e.:
**
**     SELECT fts3_tokenizer_internal_test();
**
*/
static void intTestFunc(
  sqlite3_context *context,
  int argc,
  sqlite3_value **argv
){
  int rc;
  const sqlite3_tokenizer_module *p1;
  const sqlite3_tokenizer_module *p2;
  sqlite3 *db = (sqlite3 *)sqlite3_user_data(context);

  UNUSED_PARAMETER(argc);
  UNUSED_PARAMETER(argv);

  /* Test the query function */
  sqlite3Fts3SimpleTokenizerModule(&p1);
  rc = queryTokenizer(db, "simple", &p2);
  assert( rc==SQLITE_OK );
  assert( p1==p2 );
  rc = queryTokenizer(db, "nosuchtokenizer", &p2);
  assert( rc==SQLITE_ERROR );
  assert( p2==0 );
  assert( 0==strcmp(sqlite3_errmsg(db), "unknown tokenizer: nosuchtokenizer") );

  /* Test the storage function */
  if( fts3TokenizerEnabled(context) ){
    rc = registerTokenizer(db, "nosuchtokenizer", p1);
    assert( rc==SQLITE_OK );
    rc = queryTokenizer(db, "nosuchtokenizer", &p2);
    assert( rc==SQLITE_OK );
    assert( p2==p1 );
  }

  sqlite3_result_text(context, "ok", -1, SQLITE_STATIC);
}

#endif

/*
** Set up SQL objects in database db used to access the contents of
** the hash table pointed to by argument pHash. The hash table must
** been initialized to use string keys, and to take a private copy
** of the key when a value is inserted. i.e. by a call similar to:
**
**    sqlite3Fts3HashInit(pHash, FTS3_HASH_STRING, 1);
**
** This function adds a scalar function (see header comment above
** fts3TokenizerFunc() in this file for details) and, if ENABLE_TABLE is
** defined at compilation time, a temporary virtual table (see header
** comment above struct HashTableVtab) to the database schema. Both
** provide read/write access to the contents of *pHash.
**
** The third argument to this function, zName, is used as the name
** of both the scalar and, if created, the virtual table.
*/
SQLITE_PRIVATE int sqlite3Fts3InitHashTable(
  sqlite3 *db,
  Fts3Hash *pHash,
  const char *zName
){
  int rc = SQLITE_OK;
  void *p = (void *)pHash;
  const int any = SQLITE_UTF8|SQLITE_DIRECTONLY;

#ifdef SQLITE_TEST
  char *zTest = 0;
  char *zTest2 = 0;
  void *pdb = (void *)db;
  zTest = sqlite3_mprintf("%s_test", zName);
  zTest2 = sqlite3_mprintf("%s_internal_test", zName);
  if( !zTest || !zTest2 ){
    rc = SQLITE_NOMEM;
  }
#endif

  if( SQLITE_OK==rc ){
    rc = sqlite3_create_function(db, zName, 1, any, p, fts3TokenizerFunc, 0, 0);
  }
  if( SQLITE_OK==rc ){
    rc = sqlite3_create_function(db, zName, 2, any, p, fts3TokenizerFunc, 0, 0);
  }
#ifdef SQLITE_TEST
  if( SQLITE_OK==rc ){
    rc = sqlite3_create_function(db, zTest, -1, any, p, testFunc, 0, 0);
  }
  if( SQLITE_OK==rc ){
    rc = sqlite3_create_function(db, zTest2, 0, any, pdb, intTestFunc, 0, 0);
  }
#endif

#ifdef SQLITE_TEST
  sqlite3_free(zTest);
  sqlite3_free(zTest2);
#endif

  return rc;
}

#endif /* !defined(SQLITE_CORE) || defined(SQLITE_ENABLE_FTS3) */

/************** End of fts3_tokenizer.c **************************************/
/************** Begin file fts3_tokenizer1.c *********************************/
/*
** 2006 Oct 10
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
** Implementation of the "simple" full-text-search tokenizer.
*/

/*
** The code in this file is only compiled if:
**
**     * The FTS3 module is being built as an extension
**       (in which case SQLITE_CORE is not defined), or
**
**     * The FTS3 module is being built into the core of
**       SQLite (in which case SQLITE_ENABLE_FTS3 is defined).
*/
/* #include "fts3Int.h" */
#if !defined(SQLITE_CORE) || defined(SQLITE_ENABLE_FTS3)

/* #include <assert.h> */
/* #include <stdlib.h> */
/* #include <stdio.h> */
/* #include <string.h> */

/* #include "fts3_tokenizer.h" */

typedef struct simple_tokenizer {
  sqlite3_tokenizer base;
  char delim[128];             /* flag ASCII delimiters */
} simple_tokenizer;

typedef struct simple_tokenizer_cursor {
  sqlite3_tokenizer_cursor base;
  const char *pInput;          /* input we are tokenizing */
  int nBytes;                  /* size of the input */
  int iOffset;                 /* current position in pInput */
  int iToken;                  /* index of next token to be returned */
  char *pToken;                /* storage for current token */
  int nTokenAllocated;         /* space allocated to zToken buffer */
} simple_tokenizer_cursor;


static int simpleDelim(simple_tokenizer *t, unsigned char c){
  return c<0x80 && t->delim[c];
}
static int fts3_isalnum(int x){
  return (x>='0' && x<='9') || (x>='A' && x<='Z') || (x>='a' && x<='z');
}

/*
** Create a new tokenizer instance.
*/
static int simpleCreate(
  int argc, const char * const *argv,
  sqlite3_tokenizer **ppTokenizer
){
  simple_tokenizer *t;

  t = (simple_tokenizer *) sqlite3_malloc(sizeof(*t));
  if( t==NULL ) return SQLITE_NOMEM;
  memset(t, 0, sizeof(*t));

  /* TODO(shess) Delimiters need to remain the same from run to run,
  ** else we need to reindex.  One solution would be a meta-table to
  ** track such information in the database, then we'd only want this
  ** information on the initial create.
  */
  if( argc>1 ){
    int i, n = (int)strlen(argv[1]);
    for(i=0; i<n; i++){
      unsigned char ch = argv[1][i];
      /* We explicitly don't support UTF-8 delimiters for now. */
      if( ch>=0x80 ){
        sqlite3_free(t);
        return SQLITE_ERROR;
      }
      t->delim[ch] = 1;
    }
  } else {
    /* Mark non-alphanumeric ASCII characters as delimiters */
    int i;
    for(i=1; i<0x80; i++){
      t->delim[i] = !fts3_isalnum(i) ? -1 : 0;
    }
  }

  *ppTokenizer = &t->base;
  return SQLITE_OK;
}

/*
** Destroy a tokenizer
*/
static int simpleDestroy(sqlite3_tokenizer *pTokenizer){
  sqlite3_free(pTokenizer);
  return SQLITE_OK;
}

/*
** Prepare to begin tokenizing a particular string.  The input
** string to be tokenized is pInput[0..nBytes-1].  A cursor
** used to incrementally tokenize this string is returned in
** *ppCursor.
*/
static int simpleOpen(
  sqlite3_tokenizer *pTokenizer,         /* The tokenizer */
  const char *pInput, int nBytes,        /* String to be tokenized */
  sqlite3_tokenizer_cursor **ppCursor    /* OUT: Tokenization cursor */
){
  simple_tokenizer_cursor *c;

  UNUSED_PARAMETER(pTokenizer);

  c = (simple_tokenizer_cursor *) sqlite3_malloc(sizeof(*c));
  if( c==NULL ) return SQLITE_NOMEM;

  c->pInput = pInput;
  if( pInput==0 ){
    c->nBytes = 0;
  }else if( nBytes<0 ){
    c->nBytes = (int)strlen(pInput);
  }else{
    c->nBytes = nBytes;
  }
  c->iOffset = 0;                 /* start tokenizing at the beginning */
  c->iToken = 0;
  c->pToken = NULL;               /* no space allocated, yet. */
  c->nTokenAllocated = 0;

  *ppCursor = &c->base;
  return SQLITE_OK;
}

/*
** Close a tokenization cursor previously opened by a call to
** simpleOpen() above.
*/
static int simpleClose(sqlite3_tokenizer_cursor *pCursor){
  simple_tokenizer_cursor *c = (simple_tokenizer_cursor *) pCursor;
  sqlite3_free(c->pToken);
  sqlite3_free(c);
  return SQLITE_OK;
}

/*
** Extract the next token from a tokenization cursor.  The cursor must
** have been opened by a prior call to simpleOpen().
*/
static int simpleNext(
  sqlite3_tokenizer_cursor *pCursor,  /* Cursor returned by simpleOpen */
  const char **ppToken,               /* OUT: *ppToken is the token text */
  int *pnBytes,                       /* OUT: Number of bytes in token */
  int *piStartOffset,                 /* OUT: Starting offset of token */
  int *piEndOffset,                   /* OUT: Ending offset of token */
  int *piPosition                     /* OUT: Position integer of token */
){
  simple_tokenizer_cursor *c = (simple_tokenizer_cursor *) pCursor;
  simple_tokenizer *t = (simple_tokenizer *) pCursor->pTokenizer;
  unsigned char *p = (unsigned char *)c->pInput;

  while( c->iOffset<c->nBytes ){
    int iStartOffset;

    /* Scan past delimiter characters */
    while( c->iOffset<c->nBytes && simpleDelim(t, p[c->iOffset]) ){
      c->iOffset++;
    }

    /* Count non-delimiter characters. */
    iStartOffset = c->iOffset;
    while( c->iOffset<c->nBytes && !simpleDelim(t, p[c->iOffset]) ){
      c->iOffset++;
    }

    if( c->iOffset>iStartOffset ){
      int i, n = c->iOffset-iStartOffset;
      if( n>c->nTokenAllocated ){
        char *pNew;
        c->nTokenAllocated = n+20;
        pNew = sqlite3_realloc64(c->pToken, c->nTokenAllocated);
        if( !pNew ) return SQLITE_NOMEM;
        c->pToken = pNew;
      }
      for(i=0; i<n; i++){
        /* TODO(shess) This needs expansion to handle UTF-8
        ** case-insensitivity.
        */
        unsigned char ch = p[iStartOffset+i];
        c->pToken[i] = (char)((ch>='A' && ch<='Z') ? ch-'A'+'a' : ch);
      }
      *ppToken = c->pToken;
      *pnBytes = n;
      *piStartOffset = iStartOffset;
      *piEndOffset = c->iOffset;
      *piPosition = c->iToken++;

      return SQLITE_OK;
    }
  }
  return SQLITE_DONE;
}

/*
** The set of routines that implement the simple tokenizer
*/
static const sqlite3_tokenizer_module simpleTokenizerModule = {
  0,
  simpleCreate,
  simpleDestroy,
  simpleOpen,
  simpleClose,
  simpleNext,
  0,
};

/*
** Allocate a new simple tokenizer.  Return a pointer to the new
** tokenizer in *ppModule
*/
SQLITE_PRIVATE void sqlite3Fts3SimpleTokenizerModule(
  sqlite3_tokenizer_module const**ppModule
){
  *ppModule = &simpleTokenizerModule;
}

#endif /* !defined(SQLITE_CORE) || defined(SQLITE_ENABLE_FTS3) */

/************** End of fts3_tokenizer1.c *************************************/
