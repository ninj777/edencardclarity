/*
Evan's window-check area. Run this on Windows with the compiled app's path.
I use a new temporary save, open both wallet modes, and resize their actual
controls. This checks layout and responsiveness, not whether flickering is
visible to a person. Watch the windows too. A cross-build is not a test run.
*/
#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#include <wchar.h>

typedef struct { DWORD process; const wchar_t* className; HWND found; } WindowSearch;
typedef struct { HWND parent; RECT bounds[32]; int count, errors; } LayoutCheck;

static BOOL CALLBACK findWindow(HWND hwnd, LPARAM data) {
	WindowSearch* search = (WindowSearch*)data;
	DWORD process;
	wchar_t className[80];
	GetWindowThreadProcessId(hwnd, &process);
	GetClassNameW(hwnd, className, 80);
	if (process == search->process && wcscmp(className, search->className) == 0) search->found = hwnd;
	return TRUE;
}

static HWND waitForWindow(DWORD process, const wchar_t* className) {
	WindowSearch search = {process, className, NULL};
	for (int i = 0; i < 100 && !search.found; i++) {
		EnumWindows(findWindow, (LPARAM)&search);
		if (!search.found) Sleep(50);
	}
	return search.found;
}

static BOOL CALLBACK checkControl(HWND child, LPARAM data) {
	LayoutCheck* check = (LayoutCheck*)data;
	if (GetParent(child) != check->parent || !IsWindowVisible(child)) return TRUE;
	RECT rectangle, client;
	GetClientRect(check->parent, &client);
	GetWindowRect(child, &rectangle);
	MapWindowPoints(NULL, check->parent, (POINT*)&rectangle, 2);
	if (rectangle.left < 0 || rectangle.top < 0 || rectangle.right > client.right
		|| rectangle.bottom > client.bottom || rectangle.right <= rectangle.left
		|| rectangle.bottom <= rectangle.top) check->errors++;
	for (int i = 0; i < check->count; i++) {
		RECT overlap;
		if (IntersectRect(&overlap, &rectangle, &check->bounds[i])) check->errors++;
	}
	if (check->count < 32) check->bounds[check->count++] = rectangle;
	else check->errors++;
	return TRUE;
}

static int checkLayout(HWND hwnd) {
	DWORD_PTR answer;
	if (!SendMessageTimeoutW(hwnd, WM_NULL, 0, 0, SMTO_ABORTIFHUNG, 3000, &answer)) {
		puts("Failed: window stopped responding."); return 1;
	}
	RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_UPDATENOW);
	LayoutCheck check = {0};
	check.parent = hwnd;
	EnumChildWindows(hwnd, checkControl, (LPARAM)&check);
	if (check.errors || !check.count) {
		printf("Failed: %d layout problems among %d visible controls.\n", check.errors, check.count);
		return 1;
	}
	return 0;
}

static int resizeWindow(HWND hwnd, int clientWidth, int clientHeight) {
	RECT outer = {0, 0, clientWidth, clientHeight};
	if (!AdjustWindowRectEx(&outer, (DWORD)GetWindowLongPtrW(hwnd, GWL_STYLE), FALSE,
		(DWORD)GetWindowLongPtrW(hwnd, GWL_EXSTYLE))) return 1;
	if (!SetWindowPos(hwnd, NULL, 0, 0, outer.right - outer.left, outer.bottom - outer.top,
		SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE)) return 1;
	return checkLayout(hwnd);
}

static int exerciseWindow(HWND hwnd, int minimumWidth) {
	for (int i = 0; i < 60; i++) {
		if (resizeWindow(hwnd, minimumWidth + (i % 6) * 67, 640 + (i % 5) * 37)) return 1;
	}
	ShowWindow(hwnd, SW_MINIMIZE);
	ShowWindow(hwnd, SW_RESTORE);
	if (checkLayout(hwnd)) return 1;
	ShowWindow(hwnd, SW_MAXIMIZE);
	if (checkLayout(hwnd)) return 1;
	ShowWindow(hwnd, SW_RESTORE);
	return checkLayout(hwnd);
}

static int checkMode(const wchar_t* program, BOOL gateFour) {
	wchar_t folder[MAX_PATH], save[MAX_PATH], command[2048];
	if (!GetTempPathW(MAX_PATH, folder) || !GetTempFileNameW(folder, L"ccr", 0, save)) return 1;
	DeleteFileW(save);
	int length = swprintf(command, 2048, L"\"%ls\" %ls--gui --wallet-file \"%ls\"",
		program, gateFour ? L"--gate4 " : L"", save);
	if (length < 0 || length >= 2048) return 1;
	STARTUPINFOW startup = {0}; startup.cb = sizeof(startup);
	PROCESS_INFORMATION process = {0};
	if (!CreateProcessW(program, command, NULL, NULL, FALSE, 0, NULL, NULL, &startup, &process)) return 1;
	int errors = 0;
	WaitForInputIdle(process.hProcess, 5000);
	HWND mainWindow = waitForWindow(process.dwProcessId, L"CardClarityWindow");
	HWND comparison = NULL;
	if (!mainWindow) { puts("Failed: wallet window did not open."); errors++; }
	else {
		/* The first catalog card is selected. Add and save through the existing buttons. */
		DWORD_PTR answer;
		if (!SendMessageTimeoutW(mainWindow, WM_COMMAND, 103, 0, SMTO_ABORTIFHUNG, 3000, &answer)
			|| !SendMessageTimeoutW(mainWindow, WM_COMMAND, 105, 0, SMTO_ABORTIFHUNG, 3000, &answer)) errors++;
		errors += exerciseWindow(mainWindow, 880);
		if (SendDlgItemMessageW(mainWindow, 102, LB_GETCOUNT, 0, 0) != 1) {
			puts("Failed: resize changed the wallet list."); errors++;
		}
		if (gateFour) {
			/* Searching must keep real IDs and leave the saved wallet alone. */
			HWND search = GetDlgItem(mainWindow, 108);
			SetWindowTextW(search, L"aUtOgRaPh");
			if (SendDlgItemMessageW(mainWindow, 101, LB_GETCOUNT, 0, 0) != 1
				|| SendDlgItemMessageW(mainWindow, 101, LB_GETITEMDATA, 0, 0) != 120) {
				puts("Failed: filtered name did not preserve ID 120."); errors++;
			}
			errors += exerciseWindow(mainWindow, 880);
			SetWindowTextW(search, L"a card missing from this catalog");
			if (SendDlgItemMessageW(mainWindow, 101, LB_GETCOUNT, 0, 0) != 0
				|| IsWindowEnabled(GetDlgItem(mainWindow, 103))) {
				puts("Failed: missing product was offered as an addable card."); errors++;
			}
			SetWindowTextW(search, L"");
			if (SendDlgItemMessageW(mainWindow, 101, LB_GETCOUNT, 0, 0) != 20
				|| SendDlgItemMessageW(mainWindow, 102, LB_GETCOUNT, 0, 0) != 1) {
				puts("Failed: clearing search changed catalog or wallet contents."); errors++;
			}
			PostMessageW(mainWindow, WM_COMMAND, 107, 0);
			comparison = waitForWindow(process.dwProcessId, L"CardClarityPurchase");
			if (!comparison) { puts("Failed: purchase window did not open."); errors++; }
			else {
				errors += exerciseWindow(comparison, 780);
				SendMessageTimeoutW(comparison, WM_CLOSE, 0, 0, SMTO_ABORTIFHUNG, 3000, &answer);
			}
		}
		PostMessageW(mainWindow, WM_CLOSE, 0, 0);
	}
	if (WaitForSingleObject(process.hProcess, 4000) != WAIT_OBJECT_0) {
		puts("Failed: the test's app copy did not close."); errors++;
		TerminateProcess(process.hProcess, 1);
		WaitForSingleObject(process.hProcess, 3000);
	}
	CloseHandle(process.hThread); CloseHandle(process.hProcess);
	DeleteFileW(save);
	printf("%s: %s\n", gateFour ? "Gate 4 wallet and purchase window" : "Original sample window",
		errors ? "failed" : "passed");
	return errors;
}

int main(int argc, char* argv[]) {
	if (argc != 2) { puts("Pass the compiled app's path. Watch its windows during the check."); return 1; }
	char fullPath[MAX_PATH];
	wchar_t program[MAX_PATH];
	DWORD length = GetFullPathNameA(argv[1], MAX_PATH, fullPath, NULL);
	if (!length || length >= MAX_PATH || !MultiByteToWideChar(CP_ACP, 0, fullPath, -1, program, MAX_PATH)) return 1;
	puts("Expected: controls stay inside each window without overlapping; the wallet keeps its card.");
	int errors = checkMode(program, FALSE);
	errors += checkMode(program, TRUE);
	puts("Visual flickering, text trails and interactive dragging also need a person's check.");
	return errors ? 1 : 0;
}
#else
int main(void) { puts("This check needs an actual Windows desktop."); return 77; }
#endif
