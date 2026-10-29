/* Evan's assigned scale check. These are synthetic records, not researched cards. */
#include "../CreditCardThing/catalog.h"
#include "../CreditCardThing/catalogText.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
static int compareTimes(const void* a,const void* b) {
    double x=*(const double*)a,y=*(const double*)b; return (x>y)-(x<y);
}
int main(int argc,char* argv[]) {
    if (argc!=3) { puts("Use testCatalogScale sourceCatalog freshFixturePath"); return 1; }
    char error[400];
    if (!copyDatabase(argv[1],argv[2],error,sizeof(error))) { puts(error); return 1; }
    sqlite3* db=NULL; sqlite3_stmt* row=NULL;
    bool okay=sqlite3_open(argv[2],&db)==SQLITE_OK
        &&sqlite3_exec(db,"BEGIN; DELETE FROM benefits; DELETE FROM fees; DELETE FROM rules; DELETE FROM caps; DELETE FROM cardSearch; DELETE FROM cards;",NULL,NULL,NULL)==SQLITE_OK
        &&sqlite3_prepare_v2(db,"INSERT INTO cards VALUES(?,'Fixture issuer',?,'','US','USD',0,0,0,NULL,NULL,NULL,'2026-10-29',0,0,?,'Synthetic test record','https://github.com/ninj777/edencardclarity')",-1,&row,NULL)==SQLITE_OK;
    for (int i=1;okay&&i<=10000;i++) {
        char key[64],name[160],alias[160]; snprintf(key,sizeof(key),"fixture%d",i);
        snprintf(name,sizeof(name),"Fixture Card %05d",i); snprintf(alias,sizeof(alias),"caf\xc3\xa9 test%05d",i);
        sqlite3_reset(row); sqlite3_bind_text(row,1,key,-1,SQLITE_TRANSIENT);
        sqlite3_bind_text(row,2,name,-1,SQLITE_TRANSIENT); sqlite3_bind_text(row,3,alias,-1,SQLITE_TRANSIENT);
        okay=sqlite3_step(row)==SQLITE_DONE;
    }
    sqlite3_finalize(row);
    okay=okay&&sqlite3_exec(db,"INSERT INTO cardSearch SELECT key,name,issuer,aliases FROM cards; COMMIT;",NULL,NULL,NULL)==SQLITE_OK;
    sqlite3_close(db);
    Catalog catalog={0}; if (okay) okay=openCatalog(&catalog,argv[2]);
    CatalogPage* page=malloc(sizeof(*page)); CatalogFilter filter; initCatalogFilter(&filter);
    double times[100];
    for (int i=0;okay&&page&&i<100;i++) {
        snprintf(filter.query,sizeof(filter.query),"Fixture Card %05d",i*97+1);
        clock_t start=clock();
        okay=searchCatalog(&catalog,&filter,0,page)&&page->total==1;
        times[i]=1000.0*(double)(clock()-start)/CLOCKS_PER_SEC;
    }
    if (okay&&page) {
        copyText(filter.query,160,"cafe");
        okay=searchCatalog(&catalog,&filter,0,page)&&page->total==10000&&page->count==20;
        printf("Accented alias across 10000 synthetic records: %s\n",okay?"pass":"fail");
        copyText(filter.query,160,"Fi");
        okay=okay&&searchCatalog(&catalog,&filter,0,page)&&page->total==10000;
        printf("Two-letter prefix across 10000 synthetic records: %s\n",okay?"pass":"fail");
        qsort(times,100,sizeof(times[0]),compareTimes);
        printf("100 exact-name searches: %s\n",okay?"pass":"fail");
        printf("95th percentile CPU time: %.3f ms; maximum: %.3f ms.\n",times[94],times[99]);
        puts("CPU time here is not wall-clock time on the presentation computer.");
    } else okay=false;
    closeCatalog(&catalog); free(page); return okay?0:1;
}
