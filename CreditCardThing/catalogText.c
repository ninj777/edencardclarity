/* Eden's assigned area: I check complete amounts and calendar dates here.
   A failed conversion leaves the caller's value alone. These helpers also
   keep the catalog and wallet from inventing different number rules. */
#include "catalogText.h"
#include "catalog.h"
#include <ctype.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <windows.h>
#endif

bool copyText(char* out, size_t capacity, const char* source) {
    if (!source || strlen(source) >= capacity) return false;
    memcpy(out, source, strlen(source) + 1);
    return true;
}

bool appendText(char* out, size_t capacity, const char* format, ...) {
    size_t used = strlen(out);
    if (used >= capacity) return false;
    va_list arguments;
    va_start(arguments, format);
    int length = vsnprintf(out + used, capacity - used, format, arguments);
    va_end(arguments);
    return length >= 0 && (size_t)length < capacity - used;
}

bool parseScaled(const char* text, int places, int64_t maximum, int64_t* result) {
    if (!text || places < 0 || places > 6 || maximum < 0) return false;
    while (isspace((unsigned char)*text)) text++;
    if (!isdigit((unsigned char)*text)) return false;
    int64_t scale = 1, value = 0;
    for (int i = 0; i < places; i++) scale *= 10;
    while (isdigit((unsigned char)*text)) {
        int digit = *text++ - '0';
        if (value > (maximum - digit) / 10) return false;
        value = value * 10 + digit;
    }
    if (value > maximum / scale) return false;
    value *= scale;
    int digits = 0;
    int64_t fraction = 0;
    if (*text == '.') {
        text++;
        while (isdigit((unsigned char)*text)) {
            if (digits == places) return false;
            fraction = fraction * 10 + (*text++ - '0');
            digits++;
        }
        if (!digits) return false;
    }
    while (digits++ < places) fraction *= 10;
    if (value > maximum - fraction) return false;
    value += fraction;
    while (isspace((unsigned char)*text)) text++;
    if (*text) return false;
    *result = value;
    return true;
}

bool parseAmount(const char* text, int64_t* cents) {
    /* One billion dollars is an input bound, not an expected purchase. */
    return parseScaled(text, 2, INT64_C(100000000000), cents);
}

void formatAmount(int64_t cents, char* text, size_t capacity) {
    uint64_t absolute = cents < 0 ? (uint64_t)(-(cents + 1)) + 1 : (uint64_t)cents;
    snprintf(text, capacity, "%s$%" PRIu64 ".%02" PRIu64,
        cents < 0 ? "-" : "", absolute / 100, absolute % 100);
}

int daysInMonth(int year, int month) {
    static const int days[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
    if (month < 1 || month > 12) return 0;
    if (month == 2 && year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) return 29;
    return days[month - 1];
}

bool dateParts(const char* text, int* year, int* month, int* day) {
    if (!text || strlen(text) != 10 || text[4] != '-' || text[7] != '-') return false;
    for (int i = 0; i < 10; i++) if (i != 4 && i != 7 && !isdigit((unsigned char)text[i])) return false;
    int y = 0, m = 0, d = 0;
    if (sscanf(text, "%4d-%2d-%2d", &y, &m, &d) != 3 || y < 1900 || y > 9999
        || m < 1 || m > 12 || d < 1 || d > daysInMonth(y, m)) return false;
    *year = y; *month = m; *day = d;
    return true;
}

bool validDate(const char* text) {
    int y, m, d;
    return dateParts(text, &y, &m, &d);
}

bool todayDate(char text[11]) {
    time_t now = time(NULL);
    struct tm* local = localtime(&now);
    return local && strftime(text, 11, "%Y-%m-%d", local) == 10;
}

bool makePeriodKey(int period, const char* date, int month, int day,
    int statementDay, char* key, size_t capacity) {
    int y, m, d;
    if (!dateParts(date, &y, &m, &d)) return false;
    int length;
    if (period == PERIOD_MONTH) length = snprintf(key, capacity, "%04d-%02d", y, m);
    else if (period == PERIOD_QUARTER) length = snprintf(key, capacity, "%04d-Q%d", y, (m - 1) / 3 + 1);
    else if (period == PERIOD_YEAR) length = snprintf(key, capacity, "%04d", y);
    else if (period == PERIOD_ANNIVERSARY) {
        if (month < 1 || month > 12 || day < 1 || day > daysInMonth(2000, month)) return false;
        int effectiveDay = day > daysInMonth(y, month) ? daysInMonth(y, month) : day;
        if (m < month || (m == month && d < effectiveDay)) y--;
        length = snprintf(key, capacity, "A%04d", y);
    } else if (period == PERIOD_STATEMENT) {
        if (statementDay < 1 || statementDay > 31) return false;
        int effectiveDay = statementDay > daysInMonth(y, m) ? daysInMonth(y, m) : statementDay;
        if (d < effectiveDay && --m == 0) { m = 12; y--; }
        length = snprintf(key, capacity, "S%04d-%02d", y, m);
    } else length = snprintf(key, capacity, "none");
    return length >= 0 && (size_t)length < capacity;
}

bool replaceFile(const char* from, const char* to) {
#ifdef _WIN32
    return MoveFileExA(from, to, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    return rename(from, to) == 0;
#endif
}

bool fileExists(const char* path) {
    struct stat information;
    return stat(path, &information) == 0;
}

bool joinCatalogPath(char* out, size_t size, const char* folder, const char* name) {
    int n = snprintf(out, size, "%s/%s", folder, name);
    return n >= 0 && (size_t)n < size;
}
