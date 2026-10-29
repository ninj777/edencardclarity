/* Oliver's assigned menu integration. I keep the catalog work out of the old
   Gate 1 entry point. Both new interfaces call the same storage/rule functions.
   Results and source status are shown together so an unknown value isn't zero. */
#include "catalogApp.h"
#include "catalogText.h"
#include "input.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool askText(CatalogApp* app, const char* prompt, char* text, size_t size) {
    for (;;) {
        printf("%s", prompt); fflush(stdout);
        enum InputResult result = readLine(stdin, text, size);
        if (result == INPUT_OK) return true;
        if (result == INPUT_END || result == INPUT_ERROR) {
            app->ended = true; app->readError = result == INPUT_ERROR; return false;
        }
        puts("That answer is too long or contains an invalid character. Please try again.");
    }
}

static int askChoice(CatalogApp* app, const char* prompt, int maximum) {
    char line[128]; int value;
    while (askText(app, prompt, line, sizeof(line))) {
        if (parseNumber(line, 0, maximum, &value)) return value;
        printf("Please enter a whole number from 0 to %d.\n", maximum);
    }
    return -1;
}

static bool askAmount(CatalogApp* app, const char* prompt, int64_t* value) {
    char line[128];
    while (askText(app, prompt, line, sizeof(line))) {
        if (parseAmount(line, value)) return true;
        puts("Enter a nonnegative amount with at most two decimal places, such as 19.99.");
    }
    return false;
}

bool saveAppWallet(CatalogApp* app) {
    bool okay = saveCatalogWallet(app->walletPath, app->wallet, app->status, sizeof(app->status)) == SAVED_OK;
    if (okay) copyText(app->status, sizeof(app->status), "Saved your wallet. It will be available after restarting.");
    return okay;
}

bool reloadAppWallet(CatalogApp* app) {
    bool okay = loadCatalogWallet(app->walletPath, app->wallet, app->status, sizeof(app->status)) == SAVED_OK;
    if (okay) { app->undoReady = false; copyText(app->status, sizeof(app->status), "Loaded the last saved wallet."); }
    return okay;
}

bool undoWalletRemoval(CatalogApp* app) {
    if (!app->undoReady) return false;
    int64_t revision = app->wallet->revision;
    *app->wallet = *app->undo; app->wallet->revision = revision; app->wallet->changed = true;
    app->undoReady = false;
    copyText(app->status, sizeof(app->status), "Restored the removed entry. Save to keep the change."); return true;
}

bool describeWalletEntry(CatalogApp* app, int position, char* text, size_t size) {
    if (position < 0 || position >= app->wallet->count) return false;
    WalletEntry* entry = &app->wallet->entries[position]; text[0] = '\0';
    if (!entry->cardKey[0]) appendText(text, size, "%s\n%s\nUnlisted, user-entered card. Benefits have not been verified.\n", entry->name, entry->issuer);
    else if (!describeCatalogCard(&app->catalog, entry->cardKey, text, size)) appendText(text, size,
        "%s\n%s\nThis saved card is missing from the active catalog. Its identity and notes have been kept.\n", entry->name, entry->issuer);
    if (!appendText(text, size, "\nYour note: %s\nFavorite: %s\n", entry->note[0] ? entry->note : "none", entry->favorite ? "yes" : "no")) return false;
    if (entry->valuation >= 0 && !appendText(text, size, "Your redemption assumption: %.3f cents per point/mile.\n", entry->valuation / 1000.0)) return false;
    for (int i = 0; i < entry->trackCount; i++) {
        WalletTrack* track = &entry->tracks[i]; char amount[60];
        formatAmount(track->amount, amount, sizeof(amount));
        if (!appendText(text, size, "Manual %s: %s | %s | used %s | condition %s\n",
            track->kind ? "benefit" : "cap/condition", track->ruleKey, track->periodKey,
            track->known ? amount : "unknown", track->activated < 0 ? "unknown" : track->activated ? "confirmed" : "not met")) return false;
    }
    return appendText(text, size, "Tracking is entered manually; it is not connected to a bank.\n");
}

static bool csvField(FILE* file, const char* text) {
    if (fputc('"', file) == EOF) return false;
    if (*text == '=' || *text == '+' || *text == '-' || *text == '@') if (fputc('\'', file) == EOF) return false;
    for (; *text; text++) {
        if (*text == '"' && fputc('"', file) == EOF) return false;
        if (fputc((unsigned char)*text, file) == EOF) return false;
    }
    return fputc('"', file) != EOF;
}

bool writeMissingRequests(CatalogApp* app, const char* path) {
    FILE* file = fopen(path, "wbx");
    if (!file) { copyText(app->status, sizeof(app->status), "Choose a new request-export filename."); return false; }
    bool okay = fputs("cardName,issuer\r\n", file) >= 0;
    int count = 0;
    for (int i = 0; okay && i < app->wallet->count; i++) if (!app->wallet->entries[i].cardKey[0]) {
        okay = csvField(file, app->wallet->entries[i].name) && fputc(',', file) != EOF
            && csvField(file, app->wallet->entries[i].issuer) && fputs("\r\n", file) >= 0;
        count++;
    }
    if (fclose(file) != 0) okay = false;
    if (!okay) { remove(path); copyText(app->status, sizeof(app->status), "The export failed. Your wallet has not changed."); }
    else snprintf(app->status, sizeof(app->status), "Exported %d unlisted card names and issuers. Notes and tracking were not included; nothing was sent online.", count);
    return okay;
}

static void printWallet(CatalogApp* app) {
    printf("\nMy wallet: %d cards%s\n", app->wallet->count, app->wallet->changed ? " - unsaved changes" : "");
    if (!app->wallet->count) puts("Your wallet is empty. Browse the catalog or add an unlisted card.");
    for (int i = 0; i < app->wallet->count; i++) {
        WalletEntry* e = &app->wallet->entries[i]; CatalogCard card;
        const char* state = !e->cardKey[0] ? "unlisted" : !findCatalogCard(&app->catalog, e->cardKey, &card)
            ? "missing catalog record; kept" : card.archived ? "archived" : card.checked ? "key fields checked" : "details incomplete";
        printf("%d. %s | %s | %s%s\n", i + 1, e->name, e->issuer, state, e->favorite ? " | favorite" : "");
    }
}

static int chooseEntry(CatalogApp* app) {
    printWallet(app);
    int choice = askChoice(app, "Choose a wallet entry, or 0 to cancel: ", app->wallet->count);
    return choice <= 0 ? -1 : choice - 1;
}

static void unlisted(CatalogApp* app) {
    char name[CARD_NAME_SIZE], issuer[80];
    if (!askText(app, "Card name (blank cancels): ", name, sizeof(name)) || !name[0]
        || !askText(app, "Issuer: ", issuer, sizeof(issuer))) return;
    if (addUnlistedCard(app->wallet, name, issuer, app->status, sizeof(app->status))) {
        app->undoReady = false; copyText(app->status, sizeof(app->status), "Added an unlisted card. Its rewards remain unverified.");
    }
    puts(app->status);
}

static void filterMenu(CatalogApp* app, CatalogFilter* filter) {
    char issuers[80][80], input[200]; int count = catalogIssuers(&app->catalog, issuers, 80);
    if (count < 0) { puts("Could not list the catalog issuers."); return; }
    for (int i = 0; i < count; i++) printf("%d. %s\n", i + 1, issuers[i]);
    if (!askText(app, "Issuer numbers separated by commas, or blank for any: ", input, sizeof(input))) return;
    CatalogFilter changed = *filter; changed.issuerCount = 0;
    char* start = input;
    while (*start) {
        char* comma = strchr(start, ','); if (comma) *comma = '\0';
        int selected;
        if (changed.issuerCount == 16 || !parseNumber(start, 1, count, &selected)) { puts("That issuer selection is invalid. Filters were not changed."); return; }
        bool duplicate = false;
        for (int i = 0; i < changed.issuerCount; i++) if (!strcmp(changed.issuers[i], issuers[selected - 1])) duplicate = true;
        if (!duplicate) copyText(changed.issuers[changed.issuerCount++], 80, issuers[selected - 1]);
        if (!comma) break;
        start = comma + 1;
        if (!*start) { puts("Do not leave a trailing comma."); return; }
    }
    if (!askText(app, "Maximum ongoing annual fee, blank for any: ", input, sizeof(input))) return;
    int64_t fee;
    if (!input[0]) changed.maxFee = -1;
    else if (!parseAmount(input, &fee) || fee > 1000000000) { puts("Enter a valid fee limit."); return; }
    else changed.maxFee = (int)fee;
    int unit = askChoice(app, "Reward type: 0 any, 1 cashback, 2 points, 3 miles: ", 3);
    if (unit < 0) return;
    changed.unit = unit - 1;
    for (int i = 0; i < spendingCategoryCount; i++) printf("%d. %s\n", i + 1, spendingCategories[i]);
    int category = askChoice(app, "Category: 0 any, or a category number: ", spendingCategoryCount);
    if (category < 0) return;
    copyText(changed.category, sizeof(changed.category), category ? spendingCategories[category - 1] : "");
    int archived = askChoice(app, "Include archived versions? 0 no, 1 yes: ", 1);
    if (archived < 0) return;
    changed.includeArchived = archived != 0;
    int sort = askChoice(app, "Sort: 0 relevance, 1 name, 2 ongoing fee: ", 2);
    if (sort < 0) return;
    changed.sort = sort;
    if (!askText(app, "Country code, such as US; blank for any: ", changed.country, sizeof(changed.country))
        || !askText(app, "Network name, such as Visa; blank for any: ", changed.network, sizeof(changed.network))) return;
    int checked = askChoice(app, "Only records with supported key fields checked? 0 no, 1 yes: ", 1);
    if (checked < 0) return;
    changed.checkedOnly = checked != 0; *filter = changed;
}

static void browse(CatalogApp* app) {
    CatalogFilter filter; initCatalogFilter(&filter); int offset = 0;
    CatalogPage* page = malloc(sizeof(*page)); char* details = malloc(CATALOG_TEXT_SIZE);
    if (!page || !details) { free(page); free(details); puts("Not enough memory to open the catalog."); return; }
    while (!app->ended) {
        if (!searchCatalog(&app->catalog, &filter, offset, page)) { puts("Could not search the catalog. Your wallet is unchanged."); break; }
        printf("\nBrowse cards | Search: %s | %d %s\n", filter.query[0] ? filter.query : "all names",
            page->total, page->suggestions ? "possible spelling matches" : "matches");
        printf("Active filters: %d issuer(s), fee %s, reward %s, category %s%s\n", filter.issuerCount,
            filter.maxFee < 0 ? "any" : "limited", filter.unit < 0 ? "any" : rewardUnitName(filter.unit),
            filter.category[0] ? filter.category : "any", filter.includeArchived ? ", includes archived" : "");
        printf("Country: %s | Network: %s | Checked only: %s | Sort: %d\n",
            filter.country[0] ? filter.country : "any", filter.network[0] ? filter.network : "any",
            filter.checkedOnly ? "yes" : "no", filter.sort);
        if (!page->count) puts("No matches. Clear filters, try another name, or add an unlisted card.");
        for (int i = 0; i < page->count; i++) {
            char fee[60]; formatAmount(page->cards[i].ongoingFee, fee, sizeof(fee));
            printf("%d. %s | %s | ongoing fee: %s | %s\n", i + 1, page->cards[i].name, page->cards[i].issuer,
                page->cards[i].ongoingFee < 0 ? "unknown" : fee, page->cards[i].checked ? "key fields checked" : "incomplete details");
        }
        puts("21. Search\n22. Filters and sorting\n23. Clear filters\n24. Previous page\n25. Next page\n26. Add an unlisted card\n0. Back");
        int choice = askChoice(app, "Choose a result or action: ", 26);
        if (choice <= 0) break;
        if (choice <= page->count) {
            CatalogCard* card = &page->cards[choice - 1];
            if (describeCatalogCard(&app->catalog, card->key, details, CATALOG_TEXT_SIZE)) puts(details);
            int add = askChoice(app, "1 add this version to my wallet, 0 back: ", 1);
            if (add == 1) {
                if (addCatalogCard(&app->catalog, app->wallet, card->key, app->status, sizeof(app->status))) {
                    app->undoReady = false; copyText(app->status, sizeof(app->status), "Added the selected version. Save your wallet to keep it.");
                }
                puts(app->status);
            }
        } else if (choice == 21) {
            if (askText(app, "Search card name or issuer: ", filter.query, sizeof(filter.query))) offset = 0;
        } else if (choice == 22) { filterMenu(app, &filter); offset = 0; }
        else if (choice == 23) { initCatalogFilter(&filter); offset = 0; }
        else if (choice == 24) { offset = offset >= 20 ? offset - 20 : 0; }
        else if (choice == 25) { if (!page->suggestions && offset + page->count < page->total) offset += 20; }
        else if (choice == 26) unlisted(app);
        else puts("Choose one of the displayed results or actions.");
    }
    free(page); free(details);
}

static void editEntry(CatalogApp* app) {
    int selected = chooseEntry(app); if (selected < 0) return;
    WalletEntry* changed = malloc(sizeof(*changed)); if (!changed) return;
    *changed = app->wallet->entries[selected]; char line[100];
    if (!askText(app, "Your note (blank clears it): ", changed->note, sizeof(changed->note))) { free(changed); return; }
    int favorite = askChoice(app, "Favorite? 0 no, 1 yes: ", 1);
    if (favorite < 0) { free(changed); return; }
    changed->favorite = favorite != 0;
    if (!askText(app, "Value per point/mile in cents, up to 3 decimal places; blank for unknown: ", line, sizeof(line))) { free(changed); return; }
    if (!line[0]) changed->valuation = -1;
    else if (!parseScaled(line, 3, 100000, &changed->valuation)) { puts("That valuation is invalid; the entry was not changed."); free(changed); return; }
    if (!askText(app, "Card anniversary YYYY-MM-DD, or blank if unknown: ", line, sizeof(line))) { free(changed); return; }
    if (!line[0]) { changed->anniversaryMonth = 0; changed->anniversaryDay = 0; }
    else {
        int year;
        if (!dateParts(line, &year, &changed->anniversaryMonth, &changed->anniversaryDay)) { puts("That date is invalid."); free(changed); return; }
    }
    int day = askChoice(app, "Statement day 1 to 31, or 0 if unknown: ", 31);
    if (day < 0) { free(changed); return; }
    changed->statementDay = day;
    for (int i = 0; i < spendingCategoryCount; i++) printf("%d. %s\n", i + 1, spendingCategories[i]);
    int category = askChoice(app, "Selected/rotating category, only if it matches your actual terms; 0 unknown: ", spendingCategoryCount);
    if (category < 0) { free(changed); return; }
    copyText(changed->chosenCategory, sizeof(changed->chosenCategory), category ? spendingCategories[category - 1] : "");
    app->wallet->entries[selected] = *changed;
    app->wallet->changed = true; app->undoReady = false; free(changed); puts("Updated your note and settings. Save to keep them.");
}

static void tracking(CatalogApp* app) {
    int selected = chooseEntry(app); if (selected < 0) return;
    WalletEntry* entry = &app->wallet->entries[selected];
    RewardRule* rules = calloc(40, sizeof(*rules)); BenefitRule* benefits = calloc(40, sizeof(*benefits));
    if (!rules || !benefits) { free(rules); free(benefits); return; }
    int ruleCount = catalogRules(&app->catalog, entry->cardKey, rules, 40);
    int benefitCount = catalogBenefits(&app->catalog, entry->cardKey, benefits, 40);
    int map[80], kinds[80], count = 0;
    for (int i = 0; i < ruleCount; i++) if (rules[i].capKey[0] || rules[i].activation) {
        bool duplicate = false;
        for (int j = 0; j < count; j++) if (!kinds[j] && rules[i].capKey[0] && !strcmp(rules[i].capKey, rules[map[j]].capKey)) duplicate = true;
        if (!duplicate) { printf("%d. %s / %s: cap or qualifying condition\n", count + 1, rules[i].category, rules[i].channel); map[count] = i; kinds[count++] = 0; }
    }
    for (int i = 0; i < benefitCount; i++) { printf("%d. Benefit: %s\n", count + 1, benefits[i].label); map[count] = i; kinds[count++] = 1; }
    if (!count) puts("This card has no supported cap or benefit trackers. Unlisted details are not invented.");
    int choice = askChoice(app, "Choose a tracker, or 0 back: ", count);
    if (choice > 0) {
        int index = map[choice - 1], kind = kinds[choice - 1]; WalletTrack track = { 0 }; char line[128];
        int period = kind ? !strcmp(benefits[index].period, "month") ? PERIOD_MONTH : !strcmp(benefits[index].period, "year") ? PERIOD_YEAR : PERIOD_NONE : rules[index].period;
        const char* key = kind ? benefits[index].key : rules[index].capKey[0] ? rules[index].capKey : rules[index].key;
        copyText(track.ruleKey, sizeof(track.ruleKey), key); track.kind = kind;
        if (!makePeriodKey(period, app->today, entry->anniversaryMonth, entry->anniversaryDay, entry->statementDay, track.periodKey, sizeof(track.periodKey))) puts("Set the card anniversary or statement day first.");
        else if (askText(app, kind ? "Credit amount already used this period; blank if unknown: " : "Eligible spending already used this period; blank if unknown: ", line, sizeof(line))) {
            track.known = line[0] != '\0';
            if (track.known && !parseAmount(line, &track.amount)) puts("Invalid amount; tracker not changed.");
            else {
                int active = askChoice(app, "Required condition/activation: 0 unknown, 1 confirmed, 2 not met: ", 2);
                if (active >= 0) {
                    track.activated = active == 0 ? -1 : active == 1 ? 1 : 0;
                    if (setWalletTrack(app->wallet, selected, &track)) { app->undoReady = false; puts("Updated the manual tracker. Save to keep it."); }
                    else puts("The tracker could not be added; its history may be full.");
                }
            }
        }
    }
    free(rules); free(benefits);
}

static void reminders(CatalogApp* app) {
    char* text = malloc(CATALOG_TEXT_SIZE); if (!text) return;
    if (reminderSummary(&app->catalog, app->wallet, app->today, text, CATALOG_TEXT_SIZE)) puts(text);
    for (int i = 0; i < app->wallet->reminderCount; i++) printf("%d. %s | key %s | due %s%s\n", i + 1, app->wallet->reminders[i].label, app->wallet->reminders[i].key, app->wallet->reminders[i].due, app->wallet->reminders[i].done ? " | done" : "");
    int choice = askChoice(app, "1 add reminder, 2 snooze/mark done, 3 remove, 0 back: ", 3);
    if (choice == 1) {
        WalletReminder reminder = { 0 }; int key = 1; bool exists;
        do {
            exists = false; snprintf(reminder.key, sizeof(reminder.key), "reminder%d", key++);
            for (int i = 0; i < app->wallet->reminderCount; i++) if (!strcmp(reminder.key, app->wallet->reminders[i].key)) exists = true;
        } while (exists);
        if (askText(app, "Reminder: ", reminder.label, sizeof(reminder.label)) && askText(app, "Due date YYYY-MM-DD: ", reminder.due, sizeof(reminder.due))) {
            if (setWalletReminder(app->wallet, &reminder)) { app->undoReady = false; puts("Reminder added. It is shown when the app opens, not by a background service."); }
            else puts("Check the date, label, or reminder limit.");
        }
    } else if (choice == 2 || choice == 3) {
        int selected = askChoice(app, "Reminder number, or 0 back: ", app->wallet->reminderCount);
        if (selected > 0) {
            if (choice == 3) removeWalletReminder(app->wallet, selected - 1);
            else {
                WalletReminder reminder = app->wallet->reminders[selected - 1];
                if (askText(app, "Snooze until YYYY-MM-DD, or blank for no snooze: ", reminder.snooze, sizeof(reminder.snooze))) {
                    int done = askChoice(app, "Mark done? 0 no, 1 yes: ", 1);
                    if (done >= 0) { reminder.done = done != 0; if (!setWalletReminder(app->wallet, &reminder)) puts("Invalid reminder date."); }
                }
            }
            app->undoReady = false;
        }
    }
    free(text);
}

/* Eden's wallet continuation. Linking uses the exact key printed in the card
   details, followed by confirmation. It is never a spelling-based guess. */
static void linkEntry(CatalogApp* app) {
    int position=chooseEntry(app); if (position<0) return;
    char key[CARD_KEY_SIZE]; CatalogCard card;
    if (!askText(app,"Exact catalog key from Browse cards, blank cancels: ",key,sizeof(key))||!key[0]) return;
    if (!findCatalogCard(&app->catalog,key,&card)) { puts("That key is not in this catalog."); return; }
    printf("Selected product: %s | %s\n",card.name,card.issuer);
    if (askChoice(app,"Link this exact product to the unlisted entry? 0 no, 1 yes: ",1)!=1) return;
    if (linkUnlistedCard(&app->catalog,app->wallet,position,key,app->status,sizeof(app->status))) {
        app->undoReady=false; puts("Linked the product. Your note, favorite and entry identity were kept. Save to keep it.");
    } else puts(app->status);
}

static void clearTrack(CatalogApp* app) {
    int position=chooseEntry(app); if (position<0) return;
    WalletEntry* entry=&app->wallet->entries[position];
    for (int i=0;i<entry->trackCount;i++) printf("%d. %s | %s\n",i+1,entry->tracks[i].ruleKey,entry->tracks[i].periodKey);
    int choice=askChoice(app,"Remove which history item? 0 cancels: ",entry->trackCount);
    if (choice>0&&askChoice(app,"Remove this saved history item? 0 no, 1 yes: ",1)==1
        &&removeWalletTrack(app->wallet,position,choice-1)) {
        app->undoReady=false; puts("Removed that history item. Other periods were kept. Save to keep the change.");
    }
}

static void walletMenu(CatalogApp* app) {
    while (!app->ended) {
        printWallet(app);
        puts("1. View a card\n2. Add from catalog\n3. Remove a card\n4. Save\n5. Reload (undoes unsaved changes)\n6. Notes, favorite, and valuation\n7. Track caps and benefits\n8. Reminders\n9. Undo last removal\n10. Add an unlisted card\n11. Link an unlisted card\n12. Remove an old tracker item\n0. Back");
        int choice = askChoice(app, "Choose an action: ", 12);
        if (choice <= 0) return;
        if (choice == 1) {
            int selected = chooseEntry(app); char* text = malloc(CATALOG_TEXT_SIZE);
            if (selected >= 0 && text && describeWalletEntry(app, selected, text, CATALOG_TEXT_SIZE)) puts(text);
            free(text);
        } else if (choice == 2) browse(app);
        else if (choice == 3) {
            int selected = chooseEntry(app);
            if (selected >= 0) { *app->undo = *app->wallet; app->undoReady = removeCatalogCard(app->wallet, selected); puts("Removed the entry. Undo last removal can restore it."); }
        } else if (choice == 4) { saveAppWallet(app); puts(app->status); }
        else if (choice == 5) {
            if (!app->wallet->changed || askChoice(app, "Discard unsaved changes and reload? 0 no, 1 yes: ", 1) == 1) { reloadAppWallet(app); puts(app->status); }
        } else if (choice == 6) editEntry(app);
        else if (choice == 7) tracking(app);
        else if (choice == 8) reminders(app);
        else if (choice == 9) puts(undoWalletRemoval(app) ? app->status : "There is no removal to undo after the latest edits.");
        else if (choice == 10) unlisted(app);
        else if (choice == 11) linkEntry(app);
        else if (choice == 12) clearTrack(app);
    }
}

static bool askPurchase(CatalogApp* app, Purchase* purchase) {
    memset(purchase, 0, sizeof(*purchase));
    if (!askAmount(app, "Purchase amount: ", &purchase->amount)) return false;
    for (int i = 0; i < spendingCategoryCount; i++) printf("%d. %s\n", i + 1, spendingCategories[i]);
    int category = askChoice(app, "Category, or 0 cancel: ", spendingCategoryCount);
    if (category <= 0) return false;
    copyText(purchase->category, sizeof(purchase->category), spendingCategories[category - 1]);
    int channel = askChoice(app, "Channel: 0 cancel, 1 direct, 2 issuer portal, 3 online: ", 3);
    if (channel <= 0) return false;
    copyText(purchase->channel, sizeof(purchase->channel), channel == 1 ? "direct" : channel == 2 ? "portal" : "online");
    copyText(purchase->date, sizeof(purchase->date), app->today); return true;
}

static void comparisons(CatalogApp* app) {
    int choice = askChoice(app, "1 purchase comparison, 2 yearly estimate, 3 side-by-side details, 0 back: ", 3);
    char* text = malloc(CATALOG_TEXT_SIZE); if (!text) return;
    if (choice == 1) {
        Purchase purchase;
        if (askPurchase(app, &purchase)) {
            if (compareWallet(&app->catalog, app->wallet, &purchase, text, CATALOG_TEXT_SIZE)) puts(text);
            else puts("Could not calculate that scenario. Check the amount and supported rules.");
        }
    } else if (choice == 2) {
        int selected = chooseEntry(app); int64_t monthly[15] = { 0 }, credits, bonus, interest;
        if (selected < 0) { free(text); return; }
        puts("Fresh calendar-year estimate using direct purchases. Values are your assumptions, not bank history.");
        for (int i = 0; i < spendingCategoryCount && !app->ended; i++) {
            char prompt[100]; snprintf(prompt, sizeof(prompt), "Monthly %s spending: ", spendingCategories[i]);
            if (!askAmount(app, prompt, &monthly[i])) break;
        }
        int first = app->ended ? -1 : askChoice(app, "Fee scenario: 0 ongoing year, 1 first year: ", 1);
        int active = first < 0 ? -1 : askChoice(app, "Assume every required activation/condition is met? 0 no, 1 yes: ", 1);
        if (active >= 0 && askAmount(app, "Benefits you personally expect to use, in cash value: ", &credits)
            && askAmount(app, "Bonus cash value you expect to qualify for (0 if none): ", &bonus)
            && askAmount(app, "Your separately estimated yearly interest cost (0 only if appropriate): ", &interest)) {
            YearEstimate estimate;
            int year, month, day; dateParts(app->today, &year, &month, &day);
            if (estimateYear(&app->catalog, &app->wallet->entries[selected], monthly, year, first != 0, active != 0, credits, bonus, interest, &estimate)) {
                char rewards[60], fee[60], net[60]; formatAmount(estimate.rewards, rewards, sizeof(rewards));
                formatAmount(estimate.fee, fee, sizeof(fee)); formatAmount(estimate.net, net, sizeof(net));
                printf("Rewards subtotal: %s\nAnnual fee: %s\n%s: %s\n", rewards, estimate.fee < 0 ? "unknown" : fee,
                    estimate.complete ? "Estimated net value under your assumptions" : "Incomplete subtotal; not a definite annual value", net);
                puts("Credits and bonus values are entered assumptions. Interest was not calculated from account balances. Points use your valuation. Caps reset using the modeled periods.");
            } else puts("This card or amount cannot be modeled with the supported rules.");
        }
    } else if (choice == 3) {
        int first = chooseEntry(app), second = first < 0 ? -1 : chooseEntry(app);
        if (first >= 0 && second >= 0) {
            if (describeWalletEntry(app, first, text, CATALOG_TEXT_SIZE)) puts(text);
            puts("--- Second selected card ---");
            if (describeWalletEntry(app, second, text, CATALOG_TEXT_SIZE)) puts(text);
            puts("Compare the same fee period, reward unit, and conditions. Missing values remain unknown.");
        }
    }
    free(text);
}

static void toolsMenu(CatalogApp* app) {
    puts("1. Back up saved wallet\n2. Restore wallet backup\n3. Import old sample wallet\n4. Update catalog\n5. Catalog coverage\n6. Export unlisted-card requests\n0. Back");
    int choice = askChoice(app, "Choose a tool: ", 6); char path[1024];
    if (choice == 5) { printf("Catalog %s: %d total records, %d with supported key fields checked. Includes archived legacy samples. Country: U.S. coverage; not every card or personal offer.\n", app->catalog.release, app->catalog.cardCount, app->catalog.checkedCount); return; }
    if (choice <= 0 || !askText(app, "File path (blank cancels): ", path, sizeof(path)) || !path[0]) return;
    if (choice == 1) {
        if (app->wallet->changed && !saveAppWallet(app)) { puts(app->status); return; }
        if (backupCatalogWallet(app->walletPath, path, app->status, sizeof(app->status))) copyText(app->status, sizeof(app->status), "Created a checked backup of the saved wallet.");
    } else if (choice == 2 || choice == 3) {
        if (askChoice(app, "Replace the open wallet? Unsaved edits will be discarded. 0 no, 1 yes: ", 1) != 1) return;
        bool okay = choice == 2 ? restoreCatalogWallet(path, app->walletPath, app->wallet, app->status, sizeof(app->status))
            : importLegacyWallet(&app->catalog, path, app->wallet, app->status, sizeof(app->status));
        if (okay) { app->undoReady = false; copyText(app->status, sizeof(app->status), choice == 2 ? "Restored the backup." : "Imported legacy sample identities. Save the new wallet; the old text file was not changed."); }
    } else if (choice == 4) {
        if (askChoice(app, "Validate and install this catalog, keeping the previous version? 0 no, 1 yes: ", 1) != 1) return;
        int added, changed, missing;
        if (updateCatalog(&app->catalog, path, &added, &changed, &missing, app->status, sizeof(app->status))) snprintf(app->status, sizeof(app->status), "Catalog updated: %d added, %d changed, %d absent. Saved wallet identities and notes were retained.", added, changed, missing);
    } else if (choice == 6) writeMissingRequests(app, path);
    puts(app->status);
}

static int runConsole(CatalogApp* app) {
    char remindersText[4000];
    puts("CardClarity\nYour wallet stays local. No payment numbers or bank passwords are needed.");
    if (reminderSummary(&app->catalog, app->wallet, app->today, remindersText, sizeof(remindersText))) puts(remindersText);
    while (!app->ended) {
        printf("\nMy wallet: %d card(s)%s\n", app->wallet->count, app->wallet->changed ? " - unsaved changes" : "");
        puts("1. My wallet\n2. Browse cards\n3. Compare and estimate\n4. Tools and catalog information\n0. Exit");
        int choice = askChoice(app, "Choose an action: ", 4);
        if (choice < 0) break;
        if (choice == 1) walletMenu(app);
        else if (choice == 2) browse(app);
        else if (choice == 3) comparisons(app);
        else if (choice == 4) toolsMenu(app);
        else {
            if (app->wallet->changed) {
                int answer = askChoice(app, "Unsaved changes: 1 save and exit, 2 exit without saving, 0 keep editing: ", 2);
                if (answer == 0) continue;
                if (answer == 1 && !saveAppWallet(app)) { puts(app->status); continue; }
                if (answer < 0) break;
            }
            puts("Goodbye!"); return 0;
        }
    }
    if (app->wallet->changed) puts("Your last edits were not saved.");
    puts(app->readError ? "Could not read your answer." : "No more input. Goodbye!");
    return app->readError ? 1 : 0;
}

int runCatalogApp(int argc, char* argv[]) {
    if (argc == 4 && !strcmp(argv[1], "--build-catalog")) {
        char error[400] = "";
        if (!buildCatalog(argv[2], argv[3], error, sizeof(error))) { fprintf(stderr, "Catalog build failed: %s\n", error); return 1; }
        puts("Catalog built and validated."); return 0;
    }
    CatalogApp app = { 0 }; const char* catalogPath = "data/cardCatalog.db"; const char* walletPath = "walletCards.db";
    bool gui = false;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--gui")) gui = true;
        else if ((!strcmp(argv[i], "--catalog") || !strcmp(argv[i], "--wallet")) && i + 1 < argc && argv[i + 1][0]) {
            bool cardPath = !strcmp(argv[i], "--catalog"); i++;
            if (cardPath) catalogPath = argv[i]; else walletPath = argv[i];
        } else { fputs("Use --gui, --catalog path, --wallet path, or --build-catalog sourceFolder destination.\n", stderr); return 1; }
    }
    if (!copyText(app.walletPath, sizeof(app.walletPath), walletPath) || !todayDate(app.today)) return 1;
    if (!openCatalog(&app.catalog, catalogPath)) {
        fprintf(stderr, "Could not open the catalog: %s\nBuild it from the included data with --build-catalog data data/cardCatalog.db. Your wallet was not changed.\n", app.catalog.error); return 1;
    }
    app.wallet = malloc(sizeof(*app.wallet)); app.undo = malloc(sizeof(*app.undo));
    if (!app.wallet || !app.undo) { free(app.wallet); free(app.undo); closeCatalog(&app.catalog); return 1; }
    initCatalogWallet(app.wallet);
    enum SavedWalletResult loaded = loadCatalogWallet(app.walletPath, app.wallet, app.status, sizeof(app.status));
    if (loaded == SAVED_OK) copyText(app.status, sizeof(app.status), "Loaded your saved wallet.");
    int result;
    if (gui) {
#ifdef _WIN32
        result = runCatalogWindow(&app);
#else
        fputs("The window needs Windows. Run without --gui for the catalog console.\n", stderr); result = 1;
#endif
    } else { puts(app.status); result = runConsole(&app); }
    free(app.wallet); free(app.undo); closeCatalog(&app.catalog); return result;
}
