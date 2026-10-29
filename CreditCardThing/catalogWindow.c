/* Oliver's assigned window area. Standard Windows controls handle keyboard
   focus, selection and text scaling. The old sample window is still in window.c.
   This view calls Eden's storage functions and the same calculations as the menu. */
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include "catalogApp.h"
#include "catalogText.h"
#include "input.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { ID_WALLET=101, ID_CATALOG, ID_SEARCH, ID_FIND, ID_FILTER, ID_CLEAR,
    ID_PREVIOUS, ID_NEXT, ID_LIST, ID_DETAILS, ID_ADD, ID_REMOVE, ID_SAVE,
    ID_RELOAD, ID_EDIT, ID_UNLISTED, ID_UNDO, ID_COMPARE, ID_YEAR, ID_TRACK,
    ID_REMINDERS, ID_BACKUP, ID_RESTORE, ID_IMPORT, ID_UPDATE, ID_EXPORT,
    ID_COVERAGE, ID_PAIR, ID_LINK, ID_CLEAR_TRACK };
typedef struct { const char* label; char* value; size_t capacity; } Field;
typedef struct {
    HWND window, edits[24]; Field* fields; int count, result;
} Form;
typedef struct {
    CatalogApp* app; HWND window, list, details, search, summary, status;
    HWND controls[20]; int controlCount; HFONT font;
    CatalogFilter filter; CatalogPage* page; bool browsing; int offset, scale;
    char* text;
    char selectedKey[CARD_KEY_SIZE];
} View;

static wchar_t* wideText(const char* text) {
    int size=MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text, -1, NULL, 0);
    if (!size) return NULL;
    wchar_t* result=malloc(sizeof(wchar_t)*(size_t)size);
    if (result) MultiByteToWideChar(CP_UTF8, 0, text, -1, result, size);
    return result;
}
static void setText(HWND control, const char* text) {
    size_t length=strlen(text);
    char* lines=malloc(length*2+1);
    if (!lines) return;
    size_t at=0;
    for (size_t i=0;i<length;i++) {
        if (text[i]=='\n' && (i==0 || text[i-1]!='\r')) lines[at++]='\r';
        lines[at++]=text[i];
    }
    lines[at]='\0';
    wchar_t* converted=wideText(lines);
    SetWindowTextW(control, converted ? converted : L"Text could not be displayed.");
    free(converted); free(lines);
}
static bool getText(HWND control, char* text, size_t size) {
    int length=GetWindowTextLengthW(control);
    wchar_t* buffer=malloc(sizeof(wchar_t)*((size_t)length+1));
    if (!buffer) return false;
    GetWindowTextW(control,buffer,length+1);
    int count=WideCharToMultiByte(CP_UTF8,0,buffer,-1,text,(int)size,NULL,NULL);
    free(buffer); return count>0;
}
static void tell(View* view, const char* text) { setText(view->status,text); }
static bool confirm(View* view,const wchar_t* text) {
    return MessageBoxW(view->window,text,L"CardClarity",MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2)==IDYES;
}
static int pixels(View* view,int value) { return MulDiv(value,view->scale,96); }
static HWND child(HWND parent,const wchar_t* type,const wchar_t* text,DWORD style,int id,HFONT font) {
    HWND control=CreateWindowExW(!wcscmp(type,L"EDIT")?WS_EX_CLIENTEDGE:0,type,text,
        WS_CHILD|WS_VISIBLE|style,0,0,100,25,parent,(HMENU)(INT_PTR)id,GetModuleHandleW(NULL),NULL);
    SendMessageW(control,WM_SETFONT,(WPARAM)font,TRUE); return control;
}
static LRESULT CALLBACK formProc(HWND window,UINT message,WPARAM w,LPARAM l) {
    Form* form=(Form*)GetWindowLongPtrW(window,GWLP_USERDATA);
    if (message==WM_NCCREATE) {
        form=((CREATESTRUCTW*)l)->lpCreateParams;
        SetWindowLongPtrW(window,GWLP_USERDATA,(LONG_PTR)form); form->window=window;
    }
    if (!form) return DefWindowProcW(window,message,w,l);
    if (message==WM_COMMAND && LOWORD(w)==IDOK) {
        bool okay=true;
        for (int i=0;i<form->count;i++) if (!getText(form->edits[i],form->fields[i].value,form->fields[i].capacity)) okay=false;
        if (!okay) { MessageBoxW(window,L"An answer is too long. Please shorten it.",L"Check your answer",MB_OK); return 0; }
        form->result=1; DestroyWindow(window); return 0;
    }
    if ((message==WM_COMMAND && LOWORD(w)==IDCANCEL)||message==WM_CLOSE) {
        form->result=-1; DestroyWindow(window); return 0;
    }
    return DefWindowProcW(window,message,w,l);
}
/* The caller edits temporary values and validates them after this closes.
   Cancel therefore leaves the wallet alone, even after several fields change. */
static bool form(View* view,const wchar_t* title,Field* fields,int count) {
    if (count<1 || count>24) return false;
    Form state={0}; state.fields=fields; state.count=count;
    WNDCLASSW cls={0}; cls.lpfnWndProc=formProc; cls.hInstance=GetModuleHandleW(NULL);
    cls.lpszClassName=L"CardClarityForm"; cls.hCursor=LoadCursorW(NULL,IDC_ARROW);
    cls.hbrBackground=(HBRUSH)(COLOR_WINDOW+1); RegisterClassW(&cls);
    RECT parent; GetWindowRect(view->window,&parent);
    int width=pixels(view,650), row=pixels(view,34), margin=pixels(view,18);
    int height=margin*2+row*count+pixels(view,70);
    MONITORINFO monitor={0}; monitor.cbSize=sizeof(monitor);
    GetMonitorInfoW(MonitorFromWindow(view->window,MONITOR_DEFAULTTONEAREST),&monitor);
    int left=parent.left+pixels(view,40),top=parent.top+pixels(view,25);
    if (monitor.rcWork.right>monitor.rcWork.left) {
        left=monitor.rcWork.left+(monitor.rcWork.right-monitor.rcWork.left-width)/2;
        top=monitor.rcWork.top+(monitor.rcWork.bottom-monitor.rcWork.top-height)/2;
        if (left<monitor.rcWork.left) left=monitor.rcWork.left;
        if (top<monitor.rcWork.top) top=monitor.rcWork.top;
    }
    HWND dialog=CreateWindowExW(WS_EX_DLGMODALFRAME,cls.lpszClassName,title,
        WS_CAPTION|WS_SYSMENU|WS_POPUP,left,top,
        width,height,view->window,NULL,cls.hInstance,&state);
    if (!dialog) return false;
    for (int i=0;i<count;i++) {
        HWND label=child(dialog,L"STATIC",L"",0,0,view->font); setText(label,fields[i].label);
        MoveWindow(label,margin,margin+i*row+pixels(view,4),pixels(view,340),row,TRUE);
        state.edits[i]=child(dialog,L"EDIT",L"",WS_TABSTOP|ES_AUTOHSCROLL,500+i,view->font);
        SendMessageW(state.edits[i],EM_SETLIMITTEXT,(WPARAM)(fields[i].capacity-1),0);
        setText(state.edits[i],fields[i].value);
        MoveWindow(state.edits[i],pixels(view,360),margin+i*row,pixels(view,260),pixels(view,27),TRUE);
    }
    HWND okay=child(dialog,L"BUTTON",L"Apply",WS_TABSTOP|BS_DEFPUSHBUTTON,IDOK,view->font);
    HWND cancel=child(dialog,L"BUTTON",L"Cancel",WS_TABSTOP,IDCANCEL,view->font);
    MoveWindow(okay,width-pixels(view,235),margin+count*row,pixels(view,95),pixels(view,30),TRUE);
    MoveWindow(cancel,width-pixels(view,130),margin+count*row,pixels(view,95),pixels(view,30),TRUE);
    EnableWindow(view->window,FALSE); ShowWindow(dialog,SW_SHOW); SetFocus(state.edits[0]);
    MSG message;
    while (IsWindow(dialog) && GetMessageW(&message,NULL,0,0)>0) {
        if (!IsDialogMessageW(dialog,&message)) { TranslateMessage(&message); DispatchMessageW(&message); }
    }
    EnableWindow(view->window,TRUE); SetActiveWindow(view->window);
    return state.result==1;
}
static int selection(View* view) {
    LRESULT selected=SendMessageW(view->list,LB_GETCURSEL,0,0);
    return selected==LB_ERR ? -1 : (int)selected;
}
static void details(View* view) {
    int selected=selection(view);
    if (selected<0) { setText(view->details,"Choose a card to see its fees, reward rules and sources."); return; }
    copyText(view->selectedKey,sizeof(view->selectedKey),view->browsing?view->page->cards[selected].key:view->app->wallet->entries[selected].key);
    bool okay=view->browsing ? describeCatalogCard(&view->app->catalog,view->page->cards[selected].key,view->text,CATALOG_TEXT_SIZE)
        : describeWalletEntry(view->app,selected,view->text,CATALOG_TEXT_SIZE);
    setText(view->details,okay?view->text:"Could not read the details. Your saved wallet is unchanged.");
}
static void refresh(View* view) {
    SendMessageW(view->list,LB_RESETCONTENT,0,0);
    char label[700];
    if (view->browsing) {
        if (!searchCatalog(&view->app->catalog,&view->filter,view->offset,view->page)) {
            tell(view,"The catalog search failed. This does not mean your card is absent."); return;
        }
        snprintf(label,sizeof(label),"%d %s | page %d | issuer: %s | annual fee: %s | type: %s",
            view->page->total,view->page->suggestions?"possible spelling matches":"matches",
            view->offset/20+1,view->filter.issuerCount?view->filter.issuers[0]:"any",
            view->filter.maxFee<0?"any":"limited",view->filter.unit<0?"any":rewardUnitName(view->filter.unit));
        setText(view->summary,label);
        for (int i=0;i<view->page->count;i++) {
            CatalogCard* c=&view->page->cards[i];
            snprintf(label,sizeof(label),"%s  |  %s%s",c->name,c->issuer,c->checked?"":"  |  incomplete");
            wchar_t* converted=wideText(label);
            if (converted) SendMessageW(view->list,LB_ADDSTRING,0,(LPARAM)converted);
            free(converted);
        }
        if (!view->page->count) setText(view->details,
            "No matches with these filters. Try another name or clear the filters.\r\n\r\n"
            "If your card is missing, add it as unlisted. We will keep its name without making up reward rates.");
        else {
            int selected=0;
            for (int i=0;i<view->page->count;i++) if (!strcmp(view->selectedKey,view->page->cards[i].key)) selected=i;
            SendMessageW(view->list,LB_SETCURSEL,selected,0); details(view);
        }
    } else {
        snprintf(label,sizeof(label),"My wallet: %d card(s)%s",view->app->wallet->count,view->app->wallet->changed?" | unsaved changes":"");
        setText(view->summary,label);
        for (int i=0;i<view->app->wallet->count;i++) {
            WalletEntry* e=&view->app->wallet->entries[i];
            snprintf(label,sizeof(label),"%s%s  |  %s%s",e->favorite?"Favorite: ":"",e->name,e->issuer,e->cardKey[0]?"":"  |  unlisted");
            wchar_t* converted=wideText(label);
            if (converted) SendMessageW(view->list,LB_ADDSTRING,0,(LPARAM)converted);
            free(converted);
        }
        if (view->app->wallet->count) {
            int selected=findWalletEntry(view->app->wallet,view->selectedKey);
            SendMessageW(view->list,LB_SETCURSEL,selected<0?0:selected,0); details(view);
        }
        else setText(view->details,"Your wallet is empty. Browse cards to add a specific version, or add an unlisted card.");
    }
}
static int walletSelection(View* view) {
    int selected=selection(view);
    if (view->browsing || selected<0) { tell(view,"Open My wallet and choose an entry first."); return -1; }
    return selected;
}
static void edit(View* view) {
    int index=walletSelection(view); if (index<0) return;
    WalletEntry* entry=malloc(sizeof(*entry)); if (!entry) return;
    *entry=view->app->wallet->entries[index];
    char favorite[4],value[40]="",anniversary[20]="",statement[8],category[40];
    snprintf(favorite,sizeof(favorite),"%d",entry->favorite?1:0);
    if (entry->valuation>=0) snprintf(value,sizeof(value),"%.3f",entry->valuation/1000.0);
    if (entry->anniversaryMonth) snprintf(anniversary,sizeof(anniversary),"2000-%02d-%02d",entry->anniversaryMonth,entry->anniversaryDay);
    snprintf(statement,sizeof(statement),"%d",entry->statementDay);
    copyText(category,sizeof(category),entry->chosenCategory);
    Field fields[]={{"Note",entry->note,sizeof(entry->note)},{"Favorite: 0 no, 1 yes",favorite,sizeof(favorite)},
        {"Cents per point/mile; blank = unknown",value,sizeof(value)},
        {"Anniversary: YYYY-MM-DD or blank",anniversary,sizeof(anniversary)},
        {"Statement day: 1-31; 0 unknown",statement,sizeof(statement)},
        {"Actual selected category; blank unknown",category,sizeof(category)}};
    if (form(view,L"Your card settings",fields,6)) {
        int flag,day,year=0,month=0,date=0; int64_t valuation=-1;
        bool okay=parseNumber(favorite,0,1,&flag)&&parseNumber(statement,0,31,&day)
            &&(!value[0]||parseScaled(value,3,100000,&valuation))
            &&(!anniversary[0]||dateParts(anniversary,&year,&month,&date));
        bool found=!category[0];
        for (int i=0;i<spendingCategoryCount;i++) if (!strcmp(category,spendingCategories[i])) found=true;
        if (okay&&found) {
            entry->favorite=flag!=0; entry->valuation=valuation; entry->statementDay=day;
            entry->anniversaryMonth=month; entry->anniversaryDay=date;
            copyText(entry->chosenCategory,sizeof(entry->chosenCategory),category);
            view->app->wallet->entries[index]=*entry; view->app->wallet->changed=true;
            view->app->undoReady=false; refresh(view); tell(view,"Updated the entry. Save to keep your changes.");
        } else tell(view,"One of the settings is invalid. The entry was not changed.");
    }
    free(entry);
}
static void filter(View* view) {
    char issuers[600]="",fee[40]="",unit[4],sort[4],archived[4],checked[4],category[40],country[4],network[32];
    for (int i=0;i<view->filter.issuerCount;i++) appendText(issuers,sizeof(issuers),"%s%s",i?"|":"",view->filter.issuers[i]);
    if (view->filter.maxFee>=0) snprintf(fee,sizeof(fee),"%.2f",view->filter.maxFee/100.0);
    snprintf(unit,sizeof(unit),"%d",view->filter.unit+1); snprintf(sort,sizeof(sort),"%d",view->filter.sort);
    snprintf(archived,sizeof(archived),"%d",view->filter.includeArchived?1:0);
    snprintf(checked,sizeof(checked),"%d",view->filter.checkedOnly?1:0);
    copyText(category,sizeof(category),view->filter.category); copyText(country,sizeof(country),view->filter.country);
    copyText(network,sizeof(network),view->filter.network);
    Field fields[]={{"Issuer names, separated by |; blank any",issuers,sizeof(issuers)},
        {"Maximum ongoing annual fee; blank any",fee,sizeof(fee)},
        {"Type: 0 any, 1 cashback, 2 points, 3 miles",unit,sizeof(unit)},
        {"Category name; blank any",category,sizeof(category)},
        {"Country code; blank any",country,sizeof(country)},
        {"Network name; blank any",network,sizeof(network)},
        {"Sort: 0 relevance, 1 name, 2 annual fee",sort,sizeof(sort)},
        {"Include archived: 0 no, 1 yes",archived,sizeof(archived)},
        {"Only checked key fields: 0 no, 1 yes",checked,sizeof(checked)}};
    if (!form(view,L"Filter the catalog",fields,9)) return;
    CatalogFilter changed=view->filter; int type,order,old,reviewed; int64_t money;
    bool okay=parseNumber(unit,0,3,&type)&&parseNumber(sort,0,2,&order)
        &&parseNumber(archived,0,1,&old)&&parseNumber(checked,0,1,&reviewed);
    changed.maxFee=-1;
    if (fee[0]) { if (!parseAmount(fee,&money)||money>1000000000) okay=false; else changed.maxFee=(int)money; }
    changed.issuerCount=0;
    char* start=issuers;
    while (okay&&*start) {
        char* end=strchr(start,'|'); if (end) *end='\0';
        if (changed.issuerCount==16||!copyText(changed.issuers[changed.issuerCount++],80,start)) okay=false;
        if (!end) break;
        start=end+1;
    }
    if (country[0]&&strlen(country)!=2) okay=false;
    if (category[0]) { char normalized[40]; if (!normalizeSpendingCategory(category,normalized)) okay=false; else copyText(category,sizeof(category),normalized); }
    if (!okay) { tell(view,"Check the filter values. The previous filters are still active."); return; }
    changed.unit=type-1; changed.sort=order; changed.includeArchived=old!=0; changed.checkedOnly=reviewed!=0;
    copyText(changed.category,sizeof(changed.category),category); copyText(changed.country,sizeof(changed.country),country);
    copyText(changed.network,sizeof(changed.network),network);
    view->filter=changed; view->offset=0; view->browsing=true; refresh(view);
}
static void unlisted(View* view) {
    char name[160]="",issuer[80]="";
    Field fields[]={{"Card name",name,sizeof(name)},{"Issuer",issuer,sizeof(issuer)}};
    if (form(view,L"Add an unlisted card",fields,2)) {
        if (addUnlistedCard(view->app->wallet,name,issuer,view->app->status,sizeof(view->app->status))) {
            view->app->undoReady=false; copyText(view->app->status,sizeof(view->app->status),"Added an unlisted card. No reward values were guessed.");
        }
        refresh(view); tell(view,view->app->status);
    }
}
/* Eden's continuation: linking only changes the catalog identity after the
   user confirms the exact product. The saved note stays on the same entry. */
static void linkEntry(View* view) {
    int position=walletSelection(view); if (position<0) return;
    char key[CARD_KEY_SIZE]=""; CatalogCard card;
    Field field={"Exact catalog key from card details",key,sizeof(key)};
    if (!form(view,L"Link an unlisted card",&field,1)) return;
    if (!findCatalogCard(&view->app->catalog,key,&card)) { tell(view,"That key is not in the catalog."); return; }
    char question[500]; snprintf(question,sizeof(question),"Link this entry to %s by %s? Your note and favorite will stay.",card.name,card.issuer);
    wchar_t* text=wideText(question); bool accepted=text&&confirm(view,text); free(text);
    if (!accepted) return;
    if (linkUnlistedCard(&view->app->catalog,view->app->wallet,position,key,view->app->status,sizeof(view->app->status))) {
        view->app->undoReady=false; refresh(view); tell(view,"Linked the exact product. Save to keep the change.");
    } else tell(view,view->app->status);
}

static void clearTrack(View* view) {
    int position=walletSelection(view); if (position<0) return;
    WalletEntry* entry=&view->app->wallet->entries[position]; view->text[0]='\0';
    for (int i=0;i<entry->trackCount;i++) appendText(view->text,CATALOG_TEXT_SIZE,"%d. %s | %s\r\n",i+1,entry->tracks[i].ruleKey,entry->tracks[i].periodKey);
    setText(view->details,entry->trackCount?view->text:"There is no tracker history to remove.");
    if (!entry->trackCount) return;
    char number[10]=""; Field field={"History number shown in the details",number,sizeof(number)}; int choice;
    if (!form(view,L"Remove one history item",&field,1)) return;
    if (!parseNumber(number,1,entry->trackCount,&choice)) { tell(view,"Choose a listed history number."); return; }
    if (confirm(view,L"Remove this history item? Other periods will stay.")&&removeWalletTrack(view->app->wallet,position,choice-1)) {
        view->app->undoReady=false; refresh(view); tell(view,"Removed one history item. Save to keep the change.");
    }
}

static void compare(View* view) {
    char amount[40]="",category[40]="groceries",channel[40]="direct",date[11];
    copyText(date,sizeof(date),view->app->today);
    Field fields[]={{"Purchase amount, such as 25.00",amount,sizeof(amount)},
        {"Category, such as groceries or dining",category,sizeof(category)},
        {"Booking channel: direct, portal, online",channel,sizeof(channel)},{"Purchase date: YYYY-MM-DD",date,sizeof(date)}};
    if (!form(view,L"Compare a purchase",fields,4)) return;
    Purchase purchase={0};
    if (!parseAmount(amount,&purchase.amount)||!validDate(date)) { tell(view,"Enter a valid amount and date."); return; }
    copyText(purchase.category,sizeof(purchase.category),category); copyText(purchase.channel,sizeof(purchase.channel),channel);
    copyText(purchase.date,sizeof(purchase.date),date);
    if (compareWallet(&view->app->catalog,view->app->wallet,&purchase,view->text,CATALOG_TEXT_SIZE)) setText(view->details,view->text);
    else tell(view,"The estimate could not be calculated safely.");
}
static void year(View* view) {
    int index=walletSelection(view); if (index<0) return;
    Field fields[24]; char amounts[24][40]; int64_t monthly[24];
    for (int i=0;i<spendingCategoryCount;i++) {
        copyText(amounts[i],sizeof(amounts[i]),"0");
        fields[i]=(Field){spendingCategories[i],amounts[i],sizeof(amounts[i])};
    }
    for (int start=0;start<spendingCategoryCount;start+=6) {
        int count=spendingCategoryCount-start;
        if (count>6) count=6;
        if (!form(view,L"Monthly spending in a fresh-year estimate",fields+start,count)) return;
    }
    for (int i=0;i<spendingCategoryCount;i++) if (!parseAmount(amounts[i],&monthly[i])) { tell(view,"A monthly amount is invalid."); return; }
    char yearText[8]="2027",first[4]="0",activation[4]="0",credits[40]="0",bonus[40]="0",interest[40]="0";
    Field assumptions[]={{"Calendar year",yearText,sizeof(yearText)},{"First-year fee? 0 no, 1 yes",first,sizeof(first)},
        {"Assume activation? 0 unknown, 1 yes",activation,sizeof(activation)},
        {"Credits you actually expect to use",credits,sizeof(credits)},
        {"Bonus value you expect to qualify for",bonus,sizeof(bonus)},{"Expected interest and other costs",interest,sizeof(interest)}};
    if (!form(view,L"Yearly assumptions",assumptions,6)) return;
    int y,f,a; int64_t c,b,in; YearEstimate estimate;
    if (!parseNumber(yearText,1900,9999,&y)||!parseNumber(first,0,1,&f)||!parseNumber(activation,0,1,&a)
        ||!parseAmount(credits,&c)||!parseAmount(bonus,&b)||!parseAmount(interest,&in)
        ||!estimateYear(&view->app->catalog,&view->app->wallet->entries[index],monthly,y,f!=0,a!=0,c,b,in,&estimate)) {
        tell(view,"Could not calculate this scenario. Check the card rules and amounts."); return;
    }
    char reward[60],fee[60],net[60]; formatAmount(estimate.rewards,reward,sizeof(reward));
    formatAmount(estimate.fee,fee,sizeof(fee)); formatAmount(estimate.net,net,sizeof(net));
    snprintf(view->text,CATALOG_TEXT_SIZE,"Fresh-year estimate for %s\r\n\r\nRewards: %s\r\nAnnual fee: %s\r\n"
        "Net after your entered credits, bonus and costs: %s\r\n\r\n%s\r\n"
        "Uses the same monthly spending each month, direct purchases, calendar-year cap simulation, "
        "and your redemption assumption. This does not include your past transactions or predict a bank balance.",
        view->app->wallet->entries[index].name,reward,estimate.fee<0?"unknown":fee,net,
        estimate.complete?"Supported assumptions are complete.":"Some terms or conditions are missing. This total is incomplete.");
    setText(view->details,view->text);
}
static void track(View* view) {
    int index=walletSelection(view); if (index<0) return;
    WalletEntry* entry=&view->app->wallet->entries[index];
    RewardRule* rules=calloc(40,sizeof(*rules)); BenefitRule* benefits=calloc(40,sizeof(*benefits));
    if (!rules||!benefits) { free(rules); free(benefits); return; }
    int count=catalogRules(&view->app->catalog,entry->cardKey,rules,40);
    int benefitsCount=catalogBenefits(&view->app->catalog,entry->cardKey,benefits,40);
    view->text[0]='\0';
    for (int i=0;i<count;i++) appendText(view->text,CATALOG_TEXT_SIZE,"%d. %s / %s: %.4g; %s\r\n",i+1,rules[i].category,rules[i].channel,rules[i].rate/1000000.0,rules[i].notes);
    for (int i=0;i<benefitsCount;i++) appendText(view->text,CATALOG_TEXT_SIZE,"%d. Benefit: %s\r\n",count+i+1,benefits[i].label);
    setText(view->details,view->text);
    char chosen[10]="",amount[40]="",activation[4]="0";
    Field fields[]={{"Rule number shown in the details",chosen,sizeof(chosen)},
        {"Spending/credit used; blank unknown",amount,sizeof(amount)},{"Condition: 0 unknown, 1 yes, 2 no",activation,sizeof(activation)}};
    if (count>=0&&benefitsCount>=0&&count+benefitsCount>0&&form(view,L"Enter manual tracking",fields,3)) {
        int selected,active; WalletTrack value={0}; bool okay=parseNumber(chosen,1,count+benefitsCount,&selected)&&parseNumber(activation,0,2,&active);
        value.known=amount[0]!=0; value.activated=-1;
        if (okay) {
            value.activated=active==0?-1:active==1?1:0;
            if (amount[0]&&!parseAmount(amount,&value.amount)) okay=false;
            int period=PERIOD_NONE;
            if (selected<=count) {
                RewardRule* r=&rules[selected-1];
                copyText(value.ruleKey,sizeof(value.ruleKey),r->capKey[0]?r->capKey:r->key); period=r->period;
            } else {
                BenefitRule* b=&benefits[selected-count-1]; value.kind=1;
                copyText(value.ruleKey,sizeof(value.ruleKey),b->key);
                if (!strcmp(b->period,"month")) period=PERIOD_MONTH;
                if (!strcmp(b->period,"year")) period=PERIOD_YEAR;
            }
            okay=okay&&makePeriodKey(period,view->app->today,entry->anniversaryMonth,entry->anniversaryDay,
                entry->statementDay,value.periodKey,sizeof(value.periodKey));
            if (okay) okay=setWalletTrack(view->app->wallet,index,&value);
        }
        tell(view,okay?"Tracking updated. Save to keep it. No bank data was downloaded.":"Tracking was not changed. Check the rule, amount and period settings.");
        if (okay) view->app->undoReady=false;
    } else if (!count&&!benefitsCount) tell(view,"This record has no supported tracking rules.");
    free(rules); free(benefits);
}
static void reminders(View* view) {
    if (reminderSummary(&view->app->catalog,view->app->wallet,view->app->today,view->text,CATALOG_TEXT_SIZE)) setText(view->details,view->text);
    for (int i=0;i<view->app->wallet->reminderCount;i++) {
        WalletReminder* r=&view->app->wallet->reminders[i];
        appendText(view->text,CATALOG_TEXT_SIZE,"\r\nKey %s | %s | due %s | snooze %s | %s",r->key,r->label,r->due,r->snooze[0]?r->snooze:"none",r->done?"done":"open");
    }
    setText(view->details,view->text);
    char key[64]="",label[160]="",due[11]="",snooze[11]="",done[4]="0",remove[4]="0";
    Field fields[]={{"Reminder key; reuse a key to edit",key,sizeof(key)},{"Label",label,sizeof(label)},
        {"Due date: YYYY-MM-DD",due,sizeof(due)},{"Snooze until: YYYY-MM-DD or blank",snooze,sizeof(snooze)},
        {"Done: 0 no, 1 yes",done,sizeof(done)},{"Delete this key: 0 no, 1 yes",remove,sizeof(remove)}};
    if (!form(view,L"Add, edit or remove a reminder",fields,6)) return;
    int finished,deleting;
    if (!parseNumber(done,0,1,&finished)||!parseNumber(remove,0,1,&deleting)) { tell(view,"Enter 0 or 1 for the last two answers."); return; }
    bool okay=false;
    if (deleting) {
        for (int i=0;i<view->app->wallet->reminderCount;i++) if (!strcmp(key,view->app->wallet->reminders[i].key)) {
            okay=removeWalletReminder(view->app->wallet,i); break;
        }
    } else {
        WalletReminder reminder={0}; copyText(reminder.key,sizeof(reminder.key),key); copyText(reminder.label,sizeof(reminder.label),label);
        copyText(reminder.due,sizeof(reminder.due),due); copyText(reminder.snooze,sizeof(reminder.snooze),snooze); reminder.done=finished!=0;
        okay=setWalletReminder(view->app->wallet,&reminder);
    }
    if (okay) { view->app->undoReady=false; reminderSummary(&view->app->catalog,view->app->wallet,view->app->today,view->text,CATALOG_TEXT_SIZE); setText(view->details,view->text); }
    tell(view,okay?"Reminder updated. Reminders appear while this app is open. Save to keep it.":"Reminder was not changed. Check the key and dates.");
}
static void files(View* view,int action) {
    char path[1024]="";
    Field field={"File path; choose a new filename for exports",path,sizeof(path)};
    if (!form(view,L"Choose a local file",&field,1)||!path[0]) return;
    CatalogApp* app=view->app; bool okay=false;
    if (action==ID_BACKUP) {
        if (app->wallet->changed&&!saveAppWallet(app)) { tell(view,app->status); return; }
        okay=backupCatalogWallet(app->walletPath,path,app->status,sizeof(app->status));
    } else if (action==ID_RESTORE||action==ID_IMPORT) {
        if (!confirm(view,L"Replace the open wallet? This discards unsaved edits.")) return;
        okay=action==ID_RESTORE?restoreCatalogWallet(path,app->walletPath,app->wallet,app->status,sizeof(app->status)):
            importLegacyWallet(&app->catalog,path,app->wallet,app->status,sizeof(app->status));
        if (okay) app->undoReady=false;
    } else if (action==ID_UPDATE) {
        if (!confirm(view,L"Validate and install this catalog, keeping the previous version?")) return;
        int added,changed,missing;
        okay=updateCatalog(&app->catalog,path,&added,&changed,&missing,app->status,sizeof(app->status));
        if (okay) snprintf(app->status,sizeof(app->status),"Catalog updated: %d added, %d changed, %d absent. Wallet identities were kept.",added,changed,missing);
    } else if (action==ID_EXPORT) okay=writeMissingRequests(app,path);
    if (okay&&action!=ID_UPDATE&&action!=ID_EXPORT) copyText(app->status,sizeof(app->status),
        action==ID_IMPORT?"Imported the old sample wallet. Save to keep it.":"File operation completed.");
    refresh(view); tell(view,app->status);
}
static void pair(View* view) {
    char first[10]="1",second[10]="2";
    Field fields[]={{"First wallet position (starts at 1)",first,sizeof(first)},{"Second wallet position",second,sizeof(second)}};
    if (!form(view,L"Compare two wallet entries",fields,2)) return;
    int a,b; if (!parseNumber(first,1,view->app->wallet->count,&a)||!parseNumber(second,1,view->app->wallet->count,&b)) { tell(view,"Choose two existing wallet positions."); return; }
    char* other=malloc(CATALOG_TEXT_SIZE); if (!other) return;
    if (describeWalletEntry(view->app,a-1,view->text,CATALOG_TEXT_SIZE)&&describeWalletEntry(view->app,b-1,other,CATALOG_TEXT_SIZE)) {
        /* A separate native form has two read-only panes for simultaneous reading. */
        HWND window=CreateWindowExW(0,L"STATIC",L"Compare card terms",WS_OVERLAPPEDWINDOW|WS_VISIBLE,
            CW_USEDEFAULT,CW_USEDEFAULT,pixels(view,1100),pixels(view,700),view->window,NULL,GetModuleHandleW(NULL),NULL);
        HWND left=child(window,L"EDIT",L"",ES_MULTILINE|ES_READONLY|WS_VSCROLL|WS_TABSTOP,0,view->font);
        HWND right=child(window,L"EDIT",L"",ES_MULTILINE|ES_READONLY|WS_VSCROLL|WS_TABSTOP,0,view->font);
        MoveWindow(left,pixels(view,12),pixels(view,12),pixels(view,520),pixels(view,610),TRUE);
        MoveWindow(right,pixels(view,545),pixels(view,12),pixels(view,520),pixels(view,610),TRUE);
        setText(left,view->text); setText(right,other);
    }
    free(other);
}
static void command(View* view,int id) {
    CatalogApp* app=view->app; int selected;
    if (id==ID_WALLET||id==ID_CATALOG) { view->browsing=id==ID_CATALOG; view->offset=0; refresh(view); }
    else if (id==ID_FIND) {
        if (getText(view->search,view->filter.query,sizeof(view->filter.query))) { view->browsing=true; view->offset=0; refresh(view); }
    } else if (id==ID_FILTER) filter(view);
    else if (id==ID_CLEAR) { initCatalogFilter(&view->filter); setText(view->search,""); view->offset=0; view->browsing=true; refresh(view); }
    else if (id==ID_PREVIOUS||id==ID_NEXT) {
        if (!view->browsing) return;
        if (id==ID_PREVIOUS) view->offset=view->offset>=20?view->offset-20:0;
        else if (!view->page->suggestions&&view->offset+view->page->count<view->page->total) view->offset+=20;
        refresh(view);
    } else if (id==ID_ADD) {
        selected=selection(view);
        if (!view->browsing||selected<0) { tell(view,"Browse the catalog and choose a card version first."); return; }
        if (addCatalogCard(&app->catalog,app->wallet,view->page->cards[selected].key,app->status,sizeof(app->status))) {
            app->undoReady=false; copyText(app->status,sizeof(app->status),"Added this card version. Save to keep it.");
        }
        tell(view,app->status);
    } else if (id==ID_REMOVE) {
        selected=walletSelection(view); if (selected<0||!confirm(view,L"Remove this entry from your wallet? The catalog record will stay.")) return;
        *app->undo=*app->wallet; app->undoReady=removeCatalogCard(app->wallet,selected); refresh(view); tell(view,"Entry removed. You can undo this removal.");
    } else if (id==ID_SAVE) { saveAppWallet(app); refresh(view); tell(view,app->status); }
    else if (id==ID_RELOAD) {
        if (app->wallet->changed&&!confirm(view,L"Reload the last save and discard unsaved edits?")) return;
        reloadAppWallet(app); refresh(view); tell(view,app->status);
    } else if (id==ID_UNDO) { bool restored=undoWalletRemoval(app); if (restored) refresh(view); tell(view,restored?app->status:"Use Undo immediately after a removal, before another edit."); }
    else if (id==ID_EDIT) edit(view);
    else if (id==ID_UNLISTED) unlisted(view);
    else if (id==ID_LINK) linkEntry(view);
    else if (id==ID_CLEAR_TRACK) clearTrack(view);
    else if (id==ID_COMPARE) compare(view);
    else if (id==ID_YEAR) year(view);
    else if (id==ID_TRACK) track(view);
    else if (id==ID_REMINDERS) reminders(view);
    else if (id==ID_PAIR) pair(view);
    else if (id==ID_COVERAGE) {
        snprintf(view->text,CATALOG_TEXT_SIZE,"CardClarity catalog %s\r\n\r\n%d records, including five archived practice cards.\r\n"
            "%d records have supported key fields checked. Other records stay incomplete and are excluded from definite rankings.\r\n\r\n"
            "Coverage: U.S. products from selected issuer lists. This is not every card or every personal offer. "
            "Every product shows a source link and a check date. Unknown values stay unknown.\r\n\r\n"
            "Wallets stay on this computer. No account numbers, security codes, passwords or bank connections are needed.",
            app->catalog.release,app->catalog.cardCount,app->catalog.checkedCount); setText(view->details,view->text);
    } else if (id>=ID_BACKUP&&id<=ID_EXPORT) files(view,id);
}
static void layout(View* view) {
    RECT rect; GetClientRect(view->window,&rect);
    int margin=pixels(view,18),gap=pixels(view,9),row=pixels(view,32);
    int width=rect.right, height=rect.bottom,top=margin;
    /* Navigation and the main actions stay in predictable rows when resized. */
    for (int i=0;i<view->controlCount;i++) {
        int group=i/6, column=i%6;
        int available=(width-margin*2-gap*5)/6;
        MoveWindow(view->controls[i],margin+column*(available+gap),top+group*(row+gap),available,row,TRUE);
    }
    top+=((view->controlCount+5)/6)*(row+gap);
    MoveWindow(view->search,margin,top,width-margin*2-pixels(view,110),row,TRUE);
    MoveWindow(GetDlgItem(view->window,ID_FIND),width-margin-pixels(view,100),top,pixels(view,100),row,TRUE);
    top+=row+gap;
    MoveWindow(view->summary,margin,top,width-margin*2,pixels(view,40),TRUE); top+=pixels(view,44);
    int left=(width-margin*2-gap)*2/5;
    MoveWindow(view->list,margin,top,left,height-top-margin-pixels(view,48),TRUE);
    MoveWindow(view->details,margin+left+gap,top,width-margin*2-left-gap,height-top-margin-pixels(view,48),TRUE);
    MoveWindow(view->status,margin,height-margin-pixels(view,40),width-margin*2,pixels(view,40),TRUE);
}
static LRESULT CALLBACK windowProc(HWND window,UINT message,WPARAM w,LPARAM l) {
    View* view=(View*)GetWindowLongPtrW(window,GWLP_USERDATA);
    if (message==WM_NCCREATE) { view=((CREATESTRUCTW*)l)->lpCreateParams; view->window=window; SetWindowLongPtrW(window,GWLP_USERDATA,(LONG_PTR)view); }
    if (!view) return DefWindowProcW(window,message,w,l);
    if (message==WM_SIZE) { layout(view); return 0; }
    if (message==WM_GETMINMAXINFO) {
        MINMAXINFO* size=(MINMAXINFO*)l; size->ptMinTrackSize.x=pixels(view,950); size->ptMinTrackSize.y=pixels(view,660); return 0;
    }
    if (message==WM_COMMAND) {
        if (LOWORD(w)==ID_LIST&&HIWORD(w)==LBN_SELCHANGE) details(view);
        else if (HIWORD(w)==BN_CLICKED) command(view,LOWORD(w));
        return 0;
    }
    if (message==WM_CLOSE) {
        if (view->app->wallet->changed) {
            int answer=MessageBoxW(window,L"Save your wallet before closing?",L"Unsaved changes",MB_YESNOCANCEL|MB_ICONQUESTION);
            if (answer==IDCANCEL) return 0;
            if (answer==IDYES&&!saveAppWallet(view->app)) { tell(view,view->app->status); return 0; }
        }
        DestroyWindow(window); return 0;
    }
    if (message==WM_DESTROY) { PostQuitMessage(0); return 0; }
    return DefWindowProcW(window,message,w,l);
}
int runCatalogWindow(CatalogApp* app) {
    SetProcessDPIAware();
    INITCOMMONCONTROLSEX controls={sizeof(controls),ICC_STANDARD_CLASSES};
    InitCommonControlsEx(&controls);
    View* view=calloc(1,sizeof(*view)); if (!view) return 1;
    view->app=app; view->scale=96;
    HDC dc=GetDC(NULL); if (dc) { view->scale=GetDeviceCaps(dc,LOGPIXELSX); ReleaseDC(NULL,dc); }
    view->page=malloc(sizeof(*view->page)); view->text=malloc(CATALOG_TEXT_SIZE);
    view->font=CreateFontW(-MulDiv(10,view->scale,72),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,
        DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
    if (!view->page||!view->text||!view->font) { free(view->page); free(view->text); if (view->font) DeleteObject(view->font); free(view); return 1; }
    initCatalogFilter(&view->filter);
    WNDCLASSW cls={0}; cls.lpfnWndProc=windowProc; cls.hInstance=GetModuleHandleW(NULL);
    cls.lpszClassName=L"CardClarityCatalog"; cls.hCursor=LoadCursorW(NULL,IDC_ARROW);
    cls.hbrBackground=(HBRUSH)(COLOR_WINDOW+1); RegisterClassW(&cls);
    HWND window=CreateWindowExW(WS_EX_CONTROLPARENT,cls.lpszClassName,L"CardClarity",
        WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,pixels(view,1200),pixels(view,800),
        NULL,NULL,cls.hInstance,view);
    if (!window) { DeleteObject(view->font); free(view->page); free(view->text); free(view); return 1; }
    const int ids[]={ID_WALLET,ID_CATALOG,ID_ADD,ID_REMOVE,ID_SAVE,ID_RELOAD,ID_EDIT,ID_UNLISTED,
        ID_FILTER,ID_CLEAR,ID_PREVIOUS,ID_NEXT};
    const wchar_t* labels[]={L"My wallet",L"Browse cards",L"Add selected",L"Remove",L"Save wallet",L"Reload",
        L"Notes and settings",L"Unlisted card",L"Filters",L"Clear filters",L"Previous",L"Next"};
    for (int i=0;i<12;i++) view->controls[view->controlCount++]=child(window,L"BUTTON",labels[i],WS_TABSTOP,ids[i],view->font);
    view->search=child(window,L"EDIT",L"",WS_TABSTOP|ES_AUTOHSCROLL,ID_SEARCH,view->font);
    SendMessageW(view->search,EM_SETCUEBANNER,TRUE,(LPARAM)L"Search a card name or issuer");
    SendMessageW(view->search,EM_SETLIMITTEXT,159,0);
    child(window,L"BUTTON",L"Search",WS_TABSTOP,ID_FIND,view->font);
    view->summary=child(window,L"STATIC",L"",0,0,view->font);
    view->list=child(window,L"LISTBOX",L"",WS_TABSTOP|WS_VSCROLL|WS_HSCROLL|LBS_NOTIFY|LBS_NOINTEGRALHEIGHT,ID_LIST,view->font);
    SendMessageW(view->list,LB_SETHORIZONTALEXTENT,pixels(view,1000),0);
    view->details=child(window,L"EDIT",L"",WS_TABSTOP|WS_VSCROLL|ES_MULTILINE|ES_READONLY|ES_AUTOVSCROLL,ID_DETAILS,view->font);
    SendMessageW(view->details,EM_SETLIMITTEXT,CATALOG_TEXT_SIZE,0);
    view->status=child(window,L"STATIC",L"",0,0,view->font);
    HMENU menu=CreateMenu(),tools=CreatePopupMenu();
    HMENU walletMenu=CreatePopupMenu(),compareMenu=CreatePopupMenu();
    AppendMenuW(walletMenu,MF_STRING,ID_UNDO,L"Undo last removal");
    AppendMenuW(walletMenu,MF_STRING,ID_TRACK,L"Track cap and benefit usage");
    AppendMenuW(walletMenu,MF_STRING,ID_CLEAR_TRACK,L"Remove an old tracker item");
    AppendMenuW(walletMenu,MF_STRING,ID_LINK,L"Link an unlisted card");
    AppendMenuW(walletMenu,MF_STRING,ID_REMINDERS,L"Reminders");
    AppendMenuW(compareMenu,MF_STRING,ID_COMPARE,L"Purchase rewards");
    AppendMenuW(compareMenu,MF_STRING,ID_YEAR,L"Yearly estimate");
    AppendMenuW(compareMenu,MF_STRING,ID_PAIR,L"Two cards' terms");
    AppendMenuW(tools,MF_STRING,ID_BACKUP,L"Back up saved wallet");
    AppendMenuW(tools,MF_STRING,ID_RESTORE,L"Restore wallet backup");
    AppendMenuW(tools,MF_STRING,ID_IMPORT,L"Import old sample wallet");
    AppendMenuW(tools,MF_STRING,ID_UPDATE,L"Install checked catalog");
    AppendMenuW(tools,MF_STRING,ID_EXPORT,L"Export unlisted-card requests");
    AppendMenuW(tools,MF_STRING,ID_COVERAGE,L"Sources and coverage");
    AppendMenuW(menu,MF_POPUP,(UINT_PTR)walletMenu,L"&Wallet");
    AppendMenuW(menu,MF_POPUP,(UINT_PTR)compareMenu,L"&Compare");
    AppendMenuW(menu,MF_POPUP,(UINT_PTR)tools,L"&Tools"); SetMenu(window,menu);
    layout(view); refresh(view); tell(view,app->status); ShowWindow(window,SW_SHOW); UpdateWindow(window);
    MSG message; int received;
    while ((received=(int)GetMessageW(&message,NULL,0,0))>0) {
        if (message.message==WM_KEYDOWN&&message.wParam==VK_RETURN&&GetFocus()==view->search) { command(view,ID_FIND); continue; }
        if (message.message==WM_KEYDOWN&&GetKeyState(VK_CONTROL)<0) {
            if (message.wParam=='F') { SetFocus(view->search); SendMessageW(view->search,EM_SETSEL,0,-1); continue; }
            if (message.wParam=='S') { command(view,ID_SAVE); continue; }
        }
        if (!IsDialogMessageW(window,&message)) { TranslateMessage(&message); DispatchMessageW(&message); }
    }
    DeleteObject(view->font); free(view->page); free(view->text); free(view);
    return received<0?1:0;
}
#endif
