/* Eden's assigned continuation: building and checking the saved-card project.
   This tool is C too. Run it from the project root after compiling this file.
   The compiler commands are fixed strings. Only generated numeric folder names
   are added to them, so a typed card name can never become a shell command. */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define makeFolder(path) _mkdir(path)
#else
#include <dirent.h>
#include <unistd.h>
#define makeFolder(path) mkdir(path,0700)
#endif

static const char* common[]={"catalog","catalogBuild","catalogText","catalogWallet",
    "catalogFiles","catalogApp","rewardMath","input",NULL};
static const char* legacy[]={"cards","logger","wallet","storage","window",NULL};
static bool crossBuild;
static int markdownFiles,otherCodeFiles;

static bool sameEnding(const char* name,const char* ending) {
    size_t length=strlen(name),size=strlen(ending);
    if (length<size) return false;
    for (size_t i=0;i<size;i++) if (tolower((unsigned char)name[length-size+i])!=ending[i]) return false;
    return true;
}
static void checkName(const char* path) {
    if (sameEnding(path,".md")||sameEnding(path,".markdown")||sameEnding(path,".mdx")) {
        printf("Markdown file must be removed: %s\n",path); markdownFiles++;
    }
    const char* forbidden[]={".py",".pyw",".pyc",".cpp",".cc",".cxx",".c++",
        ".js",".ts",".html",".css",".sh",".cmd",".bat",".ps1",".tcl",NULL};
    for (int i=0;forbidden[i];i++) if (sameEnding(path,forbidden[i])) {
        printf("Non-C program or script: %s\n",path); otherCodeFiles++; break;
    }
}
static bool ignored(const char* name) {
    return !strcmp(name,".")||!strcmp(name,"..")||!strcmp(name,".git")
        ||!strcmp(name,".vs")||!strcmp(name,"build");
}
static bool auditFolder(const char* folder) {
    char path[4096];
#ifdef _WIN32
    if (snprintf(path,sizeof(path),"%s/*",folder)>=(int)sizeof(path)) return false;
    WIN32_FIND_DATAA entry; HANDLE handle=FindFirstFileA(path,&entry);
    if (handle==INVALID_HANDLE_VALUE) return false;
    bool okay=true;
    do {
        if (ignored(entry.cFileName)) continue;
        if (snprintf(path,sizeof(path),"%s/%s",folder,entry.cFileName)>=(int)sizeof(path)) { okay=false; break; }
        if (entry.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT) { checkName(path); continue; }
        if (entry.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) { if (!auditFolder(path)) okay=false; }
        else checkName(path);
    } while (FindNextFileA(handle,&entry));
    if (GetLastError()!=ERROR_NO_MORE_FILES) okay=false;
    FindClose(handle); return okay;
#else
    DIR* directory=opendir(folder); if (!directory) return false;
    struct dirent* entry; bool okay=true;
    while ((entry=readdir(directory))!=NULL) {
        if (ignored(entry->d_name)) continue;
        if (snprintf(path,sizeof(path),"%s/%s",folder,entry->d_name)>=(int)sizeof(path)) { okay=false; break; }
        struct stat info;
        if (lstat(path,&info)!=0) { okay=false; break; }
        if (S_ISDIR(info.st_mode)) { if (!auditFolder(path)) okay=false; }
        else checkName(path);
    }
    if (closedir(directory)!=0) okay=false;
    return okay;
#endif
}
static bool run(const char* command) {
    printf("\nRunning: %s\n",command); fflush(stdout);
    if (system(command)==0) return true;
    fputs("That command failed. The build stopped here.\n",stderr); return false;
}
static bool folder(const char* name) {
    if (makeFolder(name)==0) return true;
    struct stat info;
    if (errno!=EEXIST||stat(name,&info)!=0) return false;
#ifdef _WIN32
    return (info.st_mode&_S_IFDIR)!=0;
#else
    return S_ISDIR(info.st_mode);
#endif
}
#ifndef _WIN32
static const char* compiler(void) {
    return crossBuild?"x86_64-w64-mingw32-gcc":"cc";
}
#endif
static bool windowsTarget(void) {
#ifdef _WIN32
    return true;
#else
    return crossBuild;
#endif
}
static const char* objectExtension(void) {
#ifdef _WIN32
    return "obj";
#else
    return "o";
#endif
}
static bool object(const char* source,const char* name,bool dependency) {
    char command[3000];
#ifdef _WIN32
    snprintf(command,sizeof(command),
        "cl /nologo /TC /std:c11 /utf-8 %s /D_CRT_SECURE_NO_WARNINGS /DUNICODE /D_UNICODE %s /c %s /Fobuild/%s.obj",
        dependency?"/O1 /W0":"/W4 /WX",
        dependency?"/DSQLITE_THREADSAFE=0 /DSQLITE_OMIT_LOAD_EXTENSION /DSQLITE_ENABLE_FTS5":"",source,name);
#else
    snprintf(command,sizeof(command),"%s -std=c11 -DUNICODE -D_UNICODE %s %s -c %s -o build/%s.o",
        compiler(),dependency?"-O1":"-Wall -Wextra -Wpedantic -Werror",
        dependency?"-DSQLITE_THREADSAFE=0 -DSQLITE_OMIT_LOAD_EXTENSION -DSQLITE_ENABLE_FTS5":"",source,name);
#endif
    return run(command);
}
static bool append(char* text,size_t capacity,const char* more) {
    if (strlen(text)+strlen(more)>=capacity) return false;
    strcat(text,more); return true;
}
static bool linkProgram(const char* name,bool app,bool support,bool modules) {
    char command[8000],part[300];
#ifdef _WIN32
    snprintf(command,sizeof(command),"link /nologo /OUT:build/%s.exe",name);
#else
    snprintf(command,sizeof(command),"%s -o build/%s%s",compiler(),name,windowsTarget()?".exe":"");
#endif
    snprintf(part,sizeof(part)," build/%s.%s",app?"appMain":name,objectExtension());
    if (!append(command,sizeof(command),part)) return false;
    if (support) {
        snprintf(part,sizeof(part)," build/testSupport.%s",objectExtension());
        if (!append(command,sizeof(command),part)) return false;
    } else if (modules) {
        snprintf(part,sizeof(part)," build/cards.%s build/logger.%s",objectExtension(),objectExtension());
        if (!append(command,sizeof(command),part)) return false;
    } else {
        for (int i=0;common[i];i++) {
            snprintf(part,sizeof(part)," build/%s.%s",common[i],objectExtension());
            if (!append(command,sizeof(command),part)) return false;
        }
        snprintf(part,sizeof(part)," build/sqlite3.%s",objectExtension());
        if (!append(command,sizeof(command),part)) return false;
        if (windowsTarget()) {
            snprintf(part,sizeof(part)," build/catalogWindow.%s",objectExtension());
            if (!append(command,sizeof(command),part)) return false;
            if (app) for (int i=0;legacy[i];i++) {
                snprintf(part,sizeof(part)," build/%s.%s",legacy[i],objectExtension());
                if (!append(command,sizeof(command),part)) return false;
            }
        }
    }
#ifdef _WIN32
    if (!append(command,sizeof(command)," user32.lib gdi32.lib comctl32.lib")) return false;
    if (app&&!append(command,sizeof(command)," /MANIFEST:EMBED /MANIFESTINPUT:CreditCardThing/appManifest.xml")) return false;
#else
    if (windowsTarget()&&!append(command,sizeof(command)," -luser32 -lgdi32 -lcomctl32")) return false;
    if (!append(command,sizeof(command)," -lm")) return false;
    if (app&&windowsTarget()&&!append(command,sizeof(command)," build/appResource.o")) return false;
#endif
    return run(command);
}
int main(int argc,char* argv[]) {
    bool auditOnly=false,buildOnly=false;
    for (int i=1;i<argc;i++) {
        if (!strcmp(argv[i],"--audit")) auditOnly=true;
        else if (!strcmp(argv[i],"--build-only")) buildOnly=true;
        else if (!strcmp(argv[i],"--cross")) crossBuild=true;
        else { puts("Use --audit, --build-only, --cross, or no options."); return 1; }
    }
    if (!auditFolder(".")||markdownFiles||otherCodeFiles) {
        puts("Source audit failed. Keep program and helper code in C, and documentation in comments or text files."); return 1;
    }
    puts("Source audit passed: no Markdown files or non-C programs/scripts.");
    if (auditOnly) return 0;
    if (!folder("build")) return 1;
    if (!object("thirdParty/sqlite3.c","sqlite3",true)) return 1;
    char source[300];
    for (int i=0;common[i];i++) {
        snprintf(source,sizeof(source),"CreditCardThing/%s.c",common[i]);
        if (!object(source,common[i],false)) return 1;
    }
    if (windowsTarget()) {
        if (!object("CreditCardThing/catalogWindow.c","catalogWindow",false)) return 1;
        for (int i=0;legacy[i];i++) {
            snprintf(source,sizeof(source),"CreditCardThing/%s.c",legacy[i]);
            if (!object(source,legacy[i],false)) return 1;
        }
#ifndef _WIN32
        if (!run("x86_64-w64-mingw32-windres -I CreditCardThing CreditCardThing/appResource.rc -O coff -o build/appResource.o")) return 1;
#endif
    }
    if (!object(windowsTarget()?"CreditCardThing/main.c":"tools/catalogConsole.c","appMain",false)
        ||!linkProgram("cardClarity",true,false,false)) return 1;
    if (!crossBuild&&!run(windowsTarget()?"build\\cardClarity.exe --build-catalog data data/cardCatalog.db"
        :"build/cardClarity --build-catalog data data/cardCatalog.db")) return 1;
    if (buildOnly) { puts("Build completed."); return 0; }
    if (!object("tests/testSupport.c","testSupport",false)) return 1;
    const char* names[]={"testCatalog","testCatalogMenu","testCatalogScale","testGate1","testGate2","testModules",NULL};
    for (int i=0;names[i];i++) {
        if (!windowsTarget()&&i>=3) break; /* The legacy app deliberately uses Windows headers. */
        snprintf(source,sizeof(source),"tests/%s.c",names[i]);
        if (!object(source,names[i],false)||!linkProgram(names[i],false,i==1||i==3||i==4,i==5)) return 1;
    }
    if (crossBuild) { puts("Windows cross-build completed. Windows programs were not run."); return 0; }
    char testFolder[120],command[800]; bool created=false;
    for (int i=0;i<1000;i++) {
        snprintf(testFolder,sizeof(testFolder),"build/checks%lld_%d",(long long)time(NULL),i);
        if (makeFolder(testFolder)==0) { created=true; break; }
        if (errno!=EEXIST) break;
    }
    if (!created) return 1;
    snprintf(command,sizeof(command),"build/%s data/cardCatalog.db %s/core",windowsTarget()?"testCatalog.exe":"testCatalog",testFolder);
    if (!run(command)||!run(windowsTarget()?"build\\testCatalogMenu.exe build/cardClarity.exe data/cardCatalog.db"
        :"build/testCatalogMenu build/cardClarity data/cardCatalog.db")) return 1;
    snprintf(command,sizeof(command),"build/%s data/cardCatalog.db %s/scale.db",windowsTarget()?"testCatalogScale.exe":"testCatalogScale",testFolder);
    if (!run(command)) return 1;
    if (windowsTarget()&&(!run("build\\testGate1.exe build/cardClarity.exe")
        ||!run("build\\testGate2.exe build/cardClarity.exe")||!run("build\\testModules.exe"))) return 1;
    puts("C11 build and C checks completed. Desktop and source review still need real observations.");
    return 0;
}
