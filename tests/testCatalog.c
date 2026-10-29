/* Evan's assigned checks. Fixed expected values test our implementation.
   Disposable catalog copies are fixtures, not claims about bank offers. */
#include "../CreditCardThing/catalogApp.h"
#include "../CreditCardThing/catalogText.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <direct.h>
#define makeDirectory(p) _mkdir(p)
#else
#include <sys/stat.h>
#define makeDirectory(p) mkdir(p,0700)
#endif
static int total,failed;
static char error[400];
static void check(bool okay,const char* name) {
    total++; printf("%02d. %s\nExpected: pass\nActual: %s\n",total,name,okay?"pass":"fail");
    if (!okay) failed++;
}
static bool sql(const char* file,const char* statement) {
    sqlite3* db=NULL; bool okay=sqlite3_open(file,&db)==SQLITE_OK;
    if (okay) okay=sqlite3_exec(db,statement,NULL,NULL,NULL)==SQLITE_OK;
    sqlite3_close(db); return okay;
}
static void path(const char* folder,const char* name,char* out) { snprintf(out,1200,"%s/%s",folder,name); }
int main(int argc,char* argv[]) {
    if (argc!=3||makeDirectory(argv[2])!=0) { puts("Use testCatalog catalogPath newTestFolder"); return 1; }
    Catalog catalog={0}; check(openCatalog(&catalog,argv[1]),"Distributed catalog validates");
    if (!catalog.db) return 1;
    CatalogPage* page=malloc(sizeof(*page));
    CatalogWallet* w=malloc(sizeof(*w)),*loaded=malloc(sizeof(*loaded)),*before=malloc(sizeof(*before));
    CatalogApp* app=calloc(1,sizeof(*app)); char* text=malloc(CATALOG_TEXT_SIZE);
    if (!page||!w||!loaded||!before||!app||!text) return 1;
    initCatalogWallet(w); initCatalogWallet(loaded);
    CatalogFilter filter; initCatalogFilter(&filter);
    check(searchCatalog(&catalog,&filter,0,page)&&page->count==20&&page->total==74,"First page has 20 of 74 real products; legacy hidden");
    char first[64]; copyText(first,sizeof(first),page->cards[0].key);
    check(searchCatalog(&catalog,&filter,20,page)&&strcmp(first,page->cards[0].key),"Next page has a different first product");
    copyText(filter.query,160,"QUICKSILVER");
    check(searchCatalog(&catalog,&filter,0,page)&&page->total>=4,"Case-insensitive prefix search finds variants");
    copyText(filter.query,160,"quick silver");
    check(searchCatalog(&catalog,&filter,0,page)&&page->total==1&&!strcmp(page->cards[0].key,"capitalOneQuicksilver"),"Spaced alias finds the intended card");
    copyText(filter.query,160,"quiksilver");
    check(searchCatalog(&catalog,&filter,0,page)&&page->suggestions&&page->count>0,"Misspelling gives labeled suggestions");
    copyText(filter.query,160,"\" OR 1=1 --");
    check(searchCatalog(&catalog,&filter,0,page)&&page->total==0,"Search text cannot become a SQL command");
    copyText(filter.query,160,"%_");
    check(searchCatalog(&catalog,&filter,0,page)&&page->total==0,"Punctuation alone cannot act as a match-all wildcard");
    initCatalogFilter(&filter); filter.issuerCount=2;
    copyText(filter.issuers[0],80,"Chase"); copyText(filter.issuers[1],80,"Capital One");
    check(searchCatalog(&catalog,&filter,0,page)&&page->total==45,"Issuer filters use either/or: 23 Chase plus 22 Capital One");
    filter.maxFee=0; filter.checkedOnly=true;
    check(searchCatalog(&catalog,&filter,0,page)&&page->total==4,"Fee and review filters narrow results together");
    copyText(filter.query,160,"Venture");
    check(searchCatalog(&catalog,&filter,0,page)&&page->total==1&&!strcmp(page->cards[0].key,"capitalOneVentureOne"),"Unknown and nonzero fees fail a zero-fee limit");
    initCatalogFilter(&filter); copyText(filter.query,160,"nothingLikeThisCard");
    check(searchCatalog(&catalog,&filter,0,page)&&page->total==0,"Missing card returns no invented record");
    int64_t amount=7;
    check(parseAmount("12.34",&amount)&&amount==1234,"12.34 becomes 1234 cents");
    check(!parseAmount("12.345",&amount)&&!parseAmount("-1",&amount)&&!parseAmount("1e4",&amount)&&!parseAmount("2x",&amount),"Invalid decimal inputs are rejected");
    check(validDate("2028-02-29")&&!validDate("2027-02-29"),"Leap years are checked");
    char period[24];
    check(makePeriodKey(PERIOD_QUARTER,"2026-10-29",0,0,0,period,sizeof(period))&&!strcmp(period,"2026-Q4"),"Quarter follows the purchase date");
    check(!makePeriodKey(PERIOD_STATEMENT,"2026-10-29",0,0,0,period,sizeof(period)),"Missing statement day stays unknown");
    check(addCatalogCard(&catalog,w,"capitalOneQuicksilver",error,sizeof(error)),"Add real card by permanent key");
    check(!addCatalogCard(&catalog,w,"capitalOneQuicksilver",error,sizeof(error))&&w->count==1,"Duplicate leaves count unchanged");
    check(addCatalogCard(&catalog,w,"chaseFreedomUnlimited",error,sizeof(error)),"Add second real card");
    check(addUnlistedCard(w,"My missing card","My issuer",error,sizeof(error)),"Keep an unlisted card without guessed rates");
    Purchase p={10000,"groceries","direct","2026-10-29"}; RewardEstimate e;
    check(estimateReward(&catalog,&w->entries[0],&p,&e)&&e.cashCents==150&&e.rankable,"100.00 at 1.5 percent gives 1.50");
    p.amount=1;
    check(estimateReward(&catalog,&w->entries[0],&p,&e)&&e.cashCents==0,"One-cent purchase rounds at the final reward");
    p.amount=10000; copyText(p.category,40,"dining");
    check(estimateReward(&catalog,&w->entries[1],&p,&e)&&e.cashCents==300,"100.00 dining at 3 percent gives 3.00");
    copyText(p.category,40,"hotels"); copyText(p.channel,40,"portal");
    check(estimateReward(&catalog,&w->entries[1],&p,&e)&&e.cashCents==500,"Portal hotel rate is 5 percent total");
    copyText(p.channel,40,"direct");
    check(estimateReward(&catalog,&w->entries[1],&p,&e)&&e.cashCents==150,"Direct booking does not get the portal bonus");
    copyText(p.category,40,"other");
    check(compareWallet(&catalog,w,&p,text,CATALOG_TEXT_SIZE)&&strstr(text,"Tie")&&strstr(text,"missing conditions"),"Tie and unknown-card warning both appear");
    check(addCatalogCard(&catalog,w,"capitalOneVenture",error,sizeof(error)),"Add miles card");
    check(estimateReward(&catalog,&w->entries[3],&p,&e)&&e.numerator==INT64_C(20000000000)&&!e.cashKnown,"200 miles are separate from an unknown cash value");
    w->entries[3].valuation=1000;
    check(estimateReward(&catalog,&w->entries[3],&p,&e)&&e.cashCents==200&&e.rankable,"One-cent-per-mile assumption gives 2.00");
    check(addCatalogCard(&catalog,w,"chaseFreedomFlex",error,sizeof(error)),"Add incomplete rotating-category card");
    copyText(w->entries[4].chosenCategory,40,"groceries"); copyText(p.category,40,"groceries");
    check(estimateReward(&catalog,&w->entries[4],&p,&e)&&e.conditional&&e.cashCents==100&&e.upperCashCents==500&&!e.rankable,"Unknown cap and activation give conditional 1.00 to 5.00");
    WalletTrack t={0}; copyText(t.ruleKey,64,"freedomFlexQuarter"); copyText(t.periodKey,24,"2026-Q4");
    t.known=true; t.activated=1; t.amount=149000;
    check(setWalletTrack(w,4,&t),"Record 1490.00 used against the quarterly cap");
    check(estimateReward(&catalog,&w->entries[4],&p,&e)&&e.cashCents==140&&!e.conditional,"Cap split: 10.00 at 5 percent plus 90.00 at 1 percent gives 1.40");
    t.activated=0; setWalletTrack(w,4,&t);
    check(estimateReward(&catalog,&w->entries[4],&p,&e)&&e.cashCents==100&&!e.conditional,"An unactivated bonus falls back to the base rate");
    t.activated=1; setWalletTrack(w,4,&t);
    copyText(p.date,11,"2027-01-01");
    check(estimateReward(&catalog,&w->entries[4],&p,&e)&&e.conditional,"A new quarter does not reuse the previous quarter's cap history");
    copyText(p.date,11,"2026-10-29");
    copyText(p.category,40,"utilities");
    check(estimateReward(&catalog,&w->entries[4],&p,&e)&&e.cashCents==100&&!e.conditional,"A different category does not receive a selected-category bonus");
    p.amount=0;
    check(estimateReward(&catalog,&w->entries[0],&p,&e)&&e.cashCents==0,"A zero purchase gives zero reward");
    p.amount=INT64_C(100000000001);
    check(!estimateReward(&catalog,&w->entries[0],&p,&e),"Out-of-range purchase is rejected");
    int64_t monthly[24]={0}; monthly[0]=10000; YearEstimate y;
    check(estimateYear(&catalog,&w->entries[0],monthly,2027,false,false,0,0,0,&y)&&y.complete&&y.net==1800,"Twelve monthly 100.00 purchases give 18.00");
    check(estimateYear(&catalog,&w->entries[3],monthly,2027,false,false,0,0,0,&y)&&y.net==-7100,"Yearly miles value subtracts the 95.00 annual fee");
    char save[1200],backup[1200],broken[1200],candidate[1200],active[1200];
    path(argv[2],"wallet.db",save); copyText(w->entries[0].note,800,"Personal note");
    w->entries[0].favorite=true; w->entries[0].anniversaryMonth=11; w->entries[0].anniversaryDay=5;
    WalletReminder r={0}; copyText(r.key,64,"review"); copyText(r.label,160,"Check the fee"); copyText(r.due,11,"2026-11-01");
    check(setWalletReminder(w,&r),"Create manual reminder");
    check(saveCatalogWallet(save,w,error,sizeof(error))==SAVED_OK&&!w->changed&&w->revision==1,"Save commits and advances revision");
    check(loadCatalogWallet(save,loaded,error,sizeof(error))==SAVED_OK&&loaded->count==5&&loaded->entries[0].favorite&&!strcmp(loaded->entries[0].note,"Personal note")&&loaded->entries[4].trackCount==1&&!strcmp(loaded->entries[4].chosenCategory,"groceries"),"Separate load restores notes, favorites, selection and tracking");
    check(reminderSummary(&catalog,loaded,"2026-10-29",text,CATALOG_TEXT_SIZE)&&strstr(text,"Check the fee")&&strstr(text,"anniversary"),"Reminder summary includes entered due date and anniversary");
    check(removeCatalogCard(w,1)&&w->count==4&&!strcmp(w->entries[1].name,"My missing card"),"Remove shifts later entries without changing identity");
    check(saveCatalogWallet(save,w,error,sizeof(error))==SAVED_OK,"Second save succeeds");
    check(saveCatalogWallet(save,loaded,error,sizeof(error))==SAVED_CONFLICT,"Stale open copy cannot overwrite newer revision");
    path(argv[2],"backup.db",backup);
    check(backupCatalogWallet(save,backup,error,sizeof(error)),"SQLite backup succeeds");
    check(!backupCatalogWallet(save,backup,error,sizeof(error)),"Existing backup is not overwritten");
    path(argv[2],"broken.db",broken);
    FILE* f=fopen(broken,"wb"); if (f) { fputs("not a database",f); fclose(f); }
    *before=*loaded;
    check(loadCatalogWallet(broken,loaded,error,sizeof(error))==SAVED_INVALID&&!memcmp(before,loaded,sizeof(*before)),"Damaged load keeps every byte of open wallet unchanged");
    check(!restoreCatalogWallet(broken,save,loaded,error,sizeof(error))&&!memcmp(before,loaded,sizeof(*before)),"Invalid restore keeps open wallet unchanged");
    check(restoreCatalogWallet(backup,save,loaded,error,sizeof(error))&&loaded->count==4,"Valid restore preserves previous save");
    check(loaded->revision>w->revision&&saveCatalogWallet(save,w,error,sizeof(error))==SAVED_CONFLICT,"Restore advances revision so an older open instance cannot overwrite it");
    path(argv[2],"badCounter.db",candidate);
    check(copyDatabase(save,candidate,error,sizeof(error))&&sql(candidate,"UPDATE state SET nextKey=1"),"Prepare damaged identity-counter fixture");
    *before=*loaded;
    check(loadCatalogWallet(candidate,loaded,error,sizeof(error))==SAVED_INVALID&&!memcmp(before,loaded,sizeof(*before)),"Colliding identity counter is rejected before replacing memory");
    path(argv[2],"hiddenText.db",candidate);
    check(copyDatabase(save,candidate,error,sizeof(error))&&sql(candidate,"UPDATE entries SET note='hello'||char(0)||'hidden' WHERE position=0")
        &&loadCatalogWallet(candidate,loaded,error,sizeof(error))==SAVED_INVALID&&!memcmp(before,loaded,sizeof(*before)),"Hidden null text in a saved field is rejected without changing memory");
    path(argv[2],"legacy.txt",candidate); f=fopen(candidate,"wb");
    if (f) { fputs("cardClarityWallet1\n2\n1\n3\n",f); fclose(f); }
    check(importLegacyWallet(&catalog,candidate,loaded,error,sizeof(error))&&!strcmp(loaded->entries[0].cardKey,"legacy001")&&!strcmp(loaded->entries[1].cardKey,"legacy003"),"Old sample IDs remain samples during import");
    path(argv[2],"active.db",active); path(argv[2],"candidate.db",candidate);
    check(copyDatabase(argv[1],active,error,sizeof(error))&&copyDatabase(argv[1],candidate,error,sizeof(error)),"Create independent catalog fixtures");
    check(sql(candidate,"UPDATE cards SET summary='Changed test snapshot' WHERE key='capitalOneQuicksilver'"),"Prepare changed catalog fixture");
    Catalog updated={0}; int added,changed,missing; bool opened=openCatalog(&updated,active);
    check(opened&&updateCatalog(&updated,candidate,&added,&changed,&missing,error,sizeof(error))&&added==0&&changed==1&&missing==0,"Catalog update counts one changed record");
    check(!updateCatalog(&updated,broken,&added,&changed,&missing,error,sizeof(error))&&updated.db,"Bad catalog update leaves the current catalog open");
    closeCatalog(&updated);
    check(!buildCatalog("missingSourceFolder",active,error,sizeof(error))&&openCatalog(&updated,active),"Failed source import preserves previous catalog");
    closeCatalog(&updated);
    app->wallet=w; app->undo=before; path(argv[2],"requests.csv",candidate);
    check(writeMissingRequests(app,candidate),"Unlisted-card export succeeds without personal notes");
    FILE* exported=fopen(candidate,"rb"); char exportedText[800]={0};
    if (exported) { fread(exportedText,1,sizeof(exportedText)-1,exported); fclose(exported); }
    check(strstr(exportedText,"My missing card")&&!strstr(exportedText,"Personal note")&&!strstr(exportedText,"1490"),"Request export contains name and issuer, not notes or cap history");
    initCatalogWallet(loaded);
    check(addCatalogCard(&catalog,loaded,"bankOfAmericaCustomized",error,sizeof(error)),"Add the separately identified Customized Cash Rewards product");
    copyText(loaded->entries[0].chosenCategory,40,"dining");
    copyText(t.ruleKey,64,"customizedQuarter"); copyText(t.periodKey,24,"2026-Q4"); t.activated=1; t.known=true; t.amount=249500;
    setWalletTrack(loaded,0,&t); p.amount=10000; copyText(p.category,40,"groceries");
    check(estimateReward(&catalog,&loaded->entries[0],&p,&e)&&e.cashCents==105,"Shared cap with 5.00 remaining gives 1.05 on 100.00 groceries");
    t.amount=250000; setWalletTrack(loaded,0,&t); copyText(p.category,40,"dining");
    check(estimateReward(&catalog,&loaded->entries[0],&p,&e)&&e.cashCents==100,"Selected category uses the same exhausted allowance as groceries");
    check(addCatalogCard(&catalog,loaded,"americanExpressBlueCashEveryday",error,sizeof(error)),"Add the partially reviewed Blue Cash Everyday record");
    copyText(t.ruleKey,64,"americanExpressBlueCashEverydaygroceriesYear"); copyText(t.periodKey,24,"2026"); t.amount=599000;
    setWalletTrack(loaded,1,&t); copyText(p.category,40,"groceries");
    check(estimateReward(&catalog,&loaded->entries[1],&p,&e)&&e.cashCents==120,"Independent annual grocery cap splits a 100.00 purchase into 1.20 reward");
    copyText(p.category,40,"gas");
    check(estimateReward(&catalog,&loaded->entries[1],&p,&e)&&e.conditional,"Grocery history does not invent known usage for the separate gas cap");
    check(addCatalogCard(&catalog,loaded,"americanExpressBlueCashPreferred",error,sizeof(error)),"Add a record with different introductory and ongoing fees");
    check(estimateYear(&catalog,&loaded->entries[2],monthly,2027,true,false,0,0,0,&y)&&y.fee==0&&y.net==1200
        &&estimateYear(&catalog,&loaded->entries[2],monthly,2027,false,false,0,0,0,&y)&&y.fee==9500&&y.net==-8300,"First-year and ongoing fees remain separate in yearly scenarios");
    initCatalogWallet(loaded);
    bool filled=true;
    for (int i=0;i<100;i++) if (!addUnlistedCard(loaded,"Capacity fixture","Fixture issuer",error,sizeof(error))) filled=false;
    check(filled&&loaded->count==100&&!addUnlistedCard(loaded,"One too many","Fixture issuer",error,sizeof(error))&&loaded->count==100,"Wallet capacity is 100 independently of the larger public catalog");
    app->wallet=loaded; *before=*loaded; app->undoReady=removeCatalogCard(loaded,50);
    check(undoWalletRemoval(app)&&loaded->count==100&&!app->undoReady,"Undo restores the removed entry and is used only once");
    closeCatalog(&catalog); free(page); free(w); free(loaded); free(before); free(app); free(text);
    printf("\n%d of %d checks passed.\n",total-failed,total); return failed?1:0;
}
