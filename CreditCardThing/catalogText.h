/* Shared text and date helpers for the catalog. Amounts use whole cents. */
#ifndef CATALOG_TEXT_H
#define CATALOG_TEXT_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
bool copyText(char* destination, size_t capacity, const char* source);
bool appendText(char* destination, size_t capacity, const char* format, ...);
bool parseAmount(const char* text, int64_t* cents);
bool parseScaled(const char* text, int places, int64_t maximum, int64_t* value);
void formatAmount(int64_t cents, char* text, size_t capacity);
bool validDate(const char* text);
bool todayDate(char text[11]);
int daysInMonth(int year, int month);
bool dateParts(const char* text, int* year, int* month, int* day);
bool makePeriodKey(int period, const char* date, int anniversaryMonth,
    int anniversaryDay, int statementDay, char* key, size_t capacity);
bool replaceFile(const char* temporary, const char* destination);
bool fileExists(const char* path);
bool joinCatalogPath(char* output, size_t capacity, const char* folder, const char* name);
#endif
