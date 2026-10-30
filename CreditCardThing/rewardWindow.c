/*
Gate 4, Oliver's window and Evan's readability area

I reuse the original window for the catalog and wallet. This smaller owned
window asks about one purchase, then calls the same comparison as the console.
The owner waits while it is open, so the selected wallet cannot change halfway
through a comparison. Tab and Escape work through normal dialog messages.
When resized, the two columns widen and the result box uses the extra height.
The controls move as a group, just as they do in the main window.
*/
#include "rewardWindow.h"
#ifdef _WIN32
#include <stdio.h>
#include <wchar.h>
#include "input.h"
#include "rewards.h"
#include "windowSizing.h"

enum { COMPARE_RUN = 301, COMPARE_CLOSE };
typedef struct {
	const Wallet* wallet;
	HWND owner, category, amount, channel, date, prior, result;
	HWND introduction, categoryLabel, amountLabel, channelLabel, dateLabel, priorLabel;
	HWND compareButton, closeButton;
	HFONT font;
} CompareControls;

static HWND control(HWND parent, const wchar_t* type, const wchar_t* title,
	DWORD style, int id, int x, int y, int width, int height, HFONT font) {
	HWND item = CreateWindowExW(0, type, title, WS_CHILD | WS_VISIBLE | style,
		x, y, width, height, parent, (HMENU)(INT_PTR)id, GetModuleHandleW(NULL), NULL);
	if (item) SendMessageW(item, WM_SETFONT, (WPARAM)font, TRUE);
	return item;
}

static bool inputText(HWND item, char* text, size_t capacity) {
	wchar_t wide[128];
	int length = GetWindowTextLengthW(item);
	if (length < 0 || length >= 128 || (size_t)length >= capacity) return false;
	GetWindowTextW(item, wide, 128);
	for (int i = 0; i < length; i++) {
		if (wide[i] > 127) return false;
		text[i] = (char)wide[i];
	}
	text[length] = '\0';
	return true;
}

static void calculate(CompareControls* form) {
	Purchase purchase = { 0 };
	char line[128];
	purchase.priorCategoryCents = -1;
	purchase.category = (enum PurchaseCategory)SendMessageW(form->category, CB_GETCURSEL, 0, 0);
	purchase.channel = (enum PurchaseChannel)SendMessageW(form->channel, CB_GETCURSEL, 0, 0);
	if (!inputText(form->amount, line, sizeof(line)) || !parseMoney(line, &purchase.amountCents)) {
		SetWindowTextW(form->result, L"Enter an amount from 0 to 1000000 with at most two decimal places."); return;
	}
	if (!inputText(form->date, line, sizeof(line)) || !parseDate(line, &purchase.date)) {
		SetWindowTextW(form->result, L"Enter a real purchase date in YYYY-MM-DD format."); return;
	}
	if (!isPurchaseChannelValid(purchase.category, purchase.channel)) {
		SetWindowTextW(form->result, L"That booking service does not fit this category. Choose direct for an ordinary purchase."); return;
	}
	bool capped = walletHasCard(form->wallet, 104) && (purchase.category == PURCHASE_GROCERIES
		|| purchase.category == PURCHASE_GAS || purchase.category == PURCHASE_ONLINE_RETAIL);
	if (capped && (!inputText(form->prior, line, sizeof(line))
		|| (line[0] && !parseMoney(line, &purchase.priorCategoryCents)))) {
		SetWindowTextW(form->result, L"Enter valid prior category spending, or leave it blank if unknown."); return;
	}
	char result[6000];
	wchar_t wide[6000];
	if (!formatComparison(form->wallet, &purchase, result, sizeof(result))
		|| !MultiByteToWideChar(CP_UTF8, 0, result, -1, wide, 6000)) {
		SetWindowTextW(form->result, L"Couldn't compare this purchase. Check the wallet and inputs."); return;
	}
	SetWindowTextW(form->result, wide);
}

static void resizeForm(HWND hwnd, const CompareControls* form) {
	if (!form || !form->result) return;
	RECT client;
	GetClientRect(hwnd, &client);
	if (client.right < 780 || client.bottom < 640) return;
	int width = client.right - 40;
	int column = (width - 20) / 2;
	int right = 40 + column;
	int buttonWidth = (column - 20) / 2;
	ControlPlacement positions[] = {
		{form->introduction, 20, 16, width, 38},
		{form->categoryLabel, 20, 62, column, 22},
		{form->category, 20, 88, column, 330},
		{form->amountLabel, right, 62, column, 22},
		{form->amount, right, 88, column, 26},
		{form->channelLabel, 20, 128, column, 22},
		{form->channel, 20, 154, column, 180},
		{form->dateLabel, right, 128, column, 22},
		{form->date, right, 154, column, 26},
		{form->priorLabel, 20, 198, width, 26},
		{form->prior, 20, 230, column, 26},
		{form->compareButton, right, 228, buttonWidth, 30},
		{form->closeButton, right + buttonWidth + 20, 228, buttonWidth, 30},
		{form->result, 20, 282, width, client.bottom - 302}
	};
	placeControls(hwnd, positions, sizeof(positions) / sizeof(positions[0]));
}

static bool makeForm(HWND hwnd, CompareControls* form) {
	HFONT font = form->font;
	form->introduction = control(hwnd, L"STATIC", L"Compare eligible domestic U.S. purchases in dollars. Read card restrictions before choosing a category.",
		SS_LEFT, 0, 20, 16, 760, 38, font);
	form->categoryLabel = control(hwnd, L"STATIC", L"Category", 0, 0, 20, 62, 340, 22, font);
	form->category = control(hwnd, L"COMBOBOX", L"", WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,
		0, 20, 88, 360, 330, font);
	form->amountLabel = control(hwnd, L"STATIC", L"Amount in dollars", 0, 0, 400, 62, 360, 22, font);
	form->amount = control(hwnd, L"EDIT", L"100.00", WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL,
		0, 400, 88, 360, 26, font);
	form->channelLabel = control(hwnd, L"STATIC", L"Booking service", 0, 0, 20, 128, 360, 22, font);
	form->channel = control(hwnd, L"COMBOBOX", L"", WS_TABSTOP | CBS_DROPDOWNLIST,
		0, 20, 154, 360, 180, font);
	form->dateLabel = control(hwnd, L"STATIC", L"Purchase date (YYYY-MM-DD)", 0, 0, 400, 128, 360, 22, font);
	SYSTEMTIME today; GetLocalTime(&today);
	wchar_t date[16];
	swprintf(date, 16, L"%04u-%02u-%02u", today.wYear, today.wMonth, today.wDay);
	form->date = control(hwnd, L"EDIT", date, WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL,
		0, 400, 154, 360, 26, font);
	form->priorLabel = control(hwnd, L"STATIC", L"Blue Cash Everyday: prior spending in this category and purchase year (blank = unknown)",
		SS_LEFT, 0, 20, 198, 740, 26, font);
	form->prior = control(hwnd, L"EDIT", L"", WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL,
		0, 20, 230, 360, 26, font);
	form->compareButton = control(hwnd, L"BUTTON", L"Compare", WS_TABSTOP | BS_DEFPUSHBUTTON, COMPARE_RUN, 400, 228, 170, 30, font);
	form->closeButton = control(hwnd, L"BUTTON", L"Close", WS_TABSTOP | BS_PUSHBUTTON, COMPARE_CLOSE, 590, 228, 170, 30, font);
	form->result = control(hwnd, L"EDIT", L"Add real cards to your wallet, then compare a purchase. Unknown prior spending stays unknown.",
		WS_BORDER | WS_VSCROLL | WS_TABSTOP | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
		0, 20, 282, 740, 340, font);
	HWND handles[] = {form->introduction, form->categoryLabel, form->category, form->amountLabel,
		form->amount, form->channelLabel, form->channel, form->dateLabel, form->date,
		form->priorLabel, form->prior, form->compareButton, form->closeButton, form->result};
	for (size_t i = 0; i < sizeof(handles) / sizeof(handles[0]); i++) if (!handles[i]) return false;
	for (int i = 0; i < PURCHASE_CATEGORY_COUNT; i++) {
		wchar_t name[128];
		MultiByteToWideChar(CP_UTF8, 0, purchaseCategoryName((enum PurchaseCategory)i), -1, name, 128);
		if (SendMessageW(form->category, CB_ADDSTRING, 0, (LPARAM)name) < 0) return false;
	}
	for (int i = 0; i < PURCHASE_CHANNEL_COUNT; i++) {
		wchar_t name[128];
		MultiByteToWideChar(CP_UTF8, 0, purchaseChannelName((enum PurchaseChannel)i), -1, name, 128);
		if (SendMessageW(form->channel, CB_ADDSTRING, 0, (LPARAM)name) < 0) return false;
	}
	SendMessageW(form->category, CB_SETCURSEL, 0, 0);
	SendMessageW(form->channel, CB_SETCURSEL, 0, 0);
	SendMessageW(form->amount, EM_SETLIMITTEXT, 120, 0);
	SendMessageW(form->prior, EM_SETLIMITTEXT, 120, 0);
	SendMessageW(form->date, EM_SETLIMITTEXT, 10, 0);
	resizeForm(hwnd, form);
	return true;
}

static LRESULT CALLBACK comparisonProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
	CompareControls* form = (CompareControls*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
	if (message == WM_NCCREATE) {
		form = (CompareControls*)((CREATESTRUCTW*)lParam)->lpCreateParams;
		SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)form);
	}
	if (message == WM_CREATE) return makeForm(hwnd, form) ? 0 : -1;
	if (message == WM_SIZE && wParam != SIZE_MINIMIZED) { resizeForm(hwnd, form); return 0; }
	if (message == WM_GETMINMAXINFO) { minimumClientSize(hwnd, (MINMAXINFO*)lParam, 780, 640); return 0; }
	if (message == WM_EXITSIZEMOVE) {
		RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_UPDATENOW);
		return 0;
	}
	if (message == WM_COMMAND) {
		int id = LOWORD(wParam);
		if (id == COMPARE_RUN || id == IDOK) { calculate(form); return 0; }
		if (id == COMPARE_CLOSE || id == IDCANCEL) { DestroyWindow(hwnd); return 0; }
	}
	if (message == WM_CLOSE) { DestroyWindow(hwnd); return 0; }
	return DefWindowProcW(hwnd, message, wParam, lParam);
}

void openRewardWindow(HWND owner, const Wallet* wallet) {
	CompareControls form = { 0 };
	form.wallet = wallet;
	form.owner = owner;
	form.font = CreateFontW(-16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Tahoma");
	if (!form.font) return;
	WNDCLASSW type = { 0 };
	type.style = CS_HREDRAW | CS_VREDRAW;
	type.lpfnWndProc = comparisonProc;
	type.hInstance = GetModuleHandleW(NULL);
	type.lpszClassName = L"CardClarityPurchase";
	type.hCursor = LoadCursorW(NULL, MAKEINTRESOURCEW(32512));
	type.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	if (!RegisterClassW(&type) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) { DeleteObject(form.font); return; }
	HWND hwnd = CreateWindowExW(WS_EX_CONTROLPARENT, type.lpszClassName, L"Compare a purchase",
		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, 850, 730,
		owner, NULL, type.hInstance, &form);
	if (!hwnd) { MessageBoxW(owner, L"Couldn't open the purchase form.", L"CardClarity", MB_OK); DeleteObject(form.font); return; }
	EnableWindow(owner, FALSE);
	ShowWindow(hwnd, SW_SHOW);
	SetFocus(form.category);
	MSG message;
	while (IsWindow(hwnd)) {
		int status = GetMessageW(&message, NULL, 0, 0);
		if (status <= 0) {
			if (status == 0) PostQuitMessage((int)message.wParam);
			break;
		}
		if (!IsDialogMessageW(hwnd, &message)) { TranslateMessage(&message); DispatchMessageW(&message); }
	}
	if (IsWindow(hwnd)) DestroyWindow(hwnd);
	EnableWindow(owner, TRUE);
	SetActiveWindow(owner);
	DeleteObject(form.font);
}
#else
/* The portable build uses compareFromConsole; no window functions are needed. */
typedef int RewardWindowUsesWindows;
#endif
