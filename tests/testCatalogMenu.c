/* Evan's assigned menu checks. A separate process makes the restart checks
   meaningful. Each case gets a temporary wallet and leaves personal saves alone. */
#include "testSupport.h"
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <direct.h>
#define currentFolder _getcwd
#else
#include <unistd.h>
#define currentFolder getcwd
#endif
static TestContext test;
static char catalog[TEST_PATH_SIZE];
static void run(const char* answers) {
    const char* arguments[]={"--catalog",catalog,"--wallet",test.save,NULL};
    runProgram(&test,arguments,answers,strlen(answers),0,false);
}
static void contains(const char* text) { expect(&test,strstr(test.output,text)!=NULL,text); }
int main(int argc,char* argv[]) {
    if (argc!=3) { puts("Use testCatalogMenu absoluteProgramPath absoluteCatalogPath"); return 1; }
    if (strlen(argv[2])>=sizeof(catalog)) return 1;
    if (argv[2][0]=='/'||argv[2][0]=='\\'||(strlen(argv[2])>1&&argv[2][1]==':')) strcpy(catalog,argv[2]);
    else {
        char folder[TEST_PATH_SIZE];
        if (!currentFolder(folder,sizeof(folder))) return 1;
        joinPath(catalog,sizeof(catalog),folder,argv[2]);
    }
    initTests(&test,argv[1],"Catalog menu checks");
    startCase(&test,1,"Menu and exit","The numbered catalog menu opens and exits successfully.");
    run("0\n"); contains("Browse cards"); contains("Goodbye!"); finishCase(&test);
    startCase(&test,2,"Invalid menu answer","Text and an out-of-range choice recover before Exit.");
    run("hello\n99\n0\n"); contains("Please enter a whole number"); contains("Goodbye!"); finishCase(&test);
    startCase(&test,3,"Search and add","Search an alias, choose its exact version, add it and save.");
    run("2\n21\nquick silver\n1\n1\n0\n1\n4\n0\n0\n");
    contains("Quicksilver Rewards"); contains("Added the selected version"); contains("Saved your wallet");
    expect(&test,pathExists(test.save),"a save file exists"); finishCase(&test);
    startCase(&test,4,"Restart","A second process loads the exact saved entry.");
    run("2\n21\nquick silver\n1\n1\n0\n1\n4\n0\n0\n");
    run("1\n1\n1\n0\n0\n");
    contains("Loaded your saved wallet"); contains("Quicksilver Rewards"); finishCase(&test);
    startCase(&test,5,"Missing card","A card absent from the catalog produces no matches.");
    run("2\n21\nnothingLikeThisCard\n0\n0\n");
    contains("No matches"); finishCase(&test);
    startCase(&test,6,"Unlisted entry and unsaved exit","An unlisted card is kept after save and restart.");
    run("1\n10\nMy other card\nExample issuer\n0\n0\n1\n");
    contains("Added an unlisted card"); run("1\n1\n1\n0\n0\n");
    contains("My other card"); contains("not been verified"); finishCase(&test);
    startCase(&test,7,"Discard","Discarding an unsaved addition does not rewrite the save.");
    run("1\n10\nKeep me\nExample issuer\n4\n0\n0\n");
    run("1\n10\nDiscard me\nExample issuer\n0\n0\n2\n");
    run("1\n1\n1\n0\n0\n");
    expect(&test,strstr(test.output,"Discard me")==NULL,"discarded entry does not appear after restart"); contains("Keep me"); finishCase(&test);
    startCase(&test,8,"Compare","A purchase comparison shows the computed reward and unknown entries.");
    run("2\n21\nquick silver\n1\n1\n0\n1\n10\nUnknown card\nExample issuer\n4\n0\n0\n");
    run("3\n1\n100\n1\n1\n0\n");
    contains("1.50"); contains("Unlisted or unresolved"); finishCase(&test);
    startCase(&test,9,"End of input","Ended input closes without saving an unfinished edit.");
    run("1\n10\nUnfinished card\n");
    contains("No more input"); finishCase(&test);
    startCase(&test,10,"Missing catalog","An unavailable database reports an error instead of claiming no matching cards.");
    char absent[TEST_PATH_SIZE]; joinPath(absent,sizeof(absent),test.folder,"missingCatalog.db");
    const char* arguments[]={"--catalog",absent,"--wallet",test.save,NULL};
    runProgram(&test,arguments,"",0,1,false);
    expect(&test,strstr(test.errors,"Could not open the catalog")!=NULL,"catalog-open error is explained");
    expect(&test,!pathExists(test.save)&&!pathExists(absent),"no empty catalog or wallet was created"); finishCase(&test);
    startCase(&test,11,"Link an unlisted card","Confirming a specific catalog key keeps one entry and saves the matched product.");
    run("1\n10\nMy travel card\nUnknown issuer\n11\n1\ncapitalOneVentureX\n1\n4\n0\n0\n");
    contains("Linked the product"); run("1\n1\n1\n0\n0\n");
    contains("Venture X Rewards"); contains("Catalog key: capitalOneVentureX"); finishCase(&test);
    startCase(&test,12,"Clear old tracking","Removing one history item stays removed after saving and reopening.");
    run("2\n21\nfreedom flex\n1\n1\n0\n1\n7\n1\n1\n10\n1\n12\n1\n1\n1\n4\n0\n0\n");
    contains("Removed that history item"); run("1\n1\n1\n0\n0\n");
    expect(&test,strstr(test.output,"Manual cap/condition:")==NULL,"deleted tracker is absent after restarting"); finishCase(&test);
    startCase(&test,13,"Cancel linking","Declining the selected product keeps the original unlisted name.");
    run("1\n10\nKeep my original name\nUnknown issuer\n11\n1\ncapitalOneVentureX\n0\n4\n0\n0\n");
    run("1\n1\n1\n0\n0\n"); contains("Keep my original name"); contains("not been verified"); finishCase(&test);
    return finishTests(&test);
}
