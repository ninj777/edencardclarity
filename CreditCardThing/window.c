/*
Oliver Dolfin - Windows window notes

I use the Windows API from C to make this window. CreateWindowW opens it, and
makeControl adds its labels, lists, and buttons. WindowControls keeps the
handles together so I can update the right control when something changes.

The catalog is on the left, the details are on the right, and My wallet is
below the catalog. I use Tahoma, smaller titles and rates, bold headings,
dark gray text, and a cream background. The controls keep their usual Windows
behavior, including moving around with the keyboard.

I attach the real catalog ID to each list row with LB_SETITEMDATA. A wallet
row can move after a removal, so its position is not a reliable saved ID.
Joe's getCardById supplies the record. Eden's functions handle the wallet
and file actions, just as they do in the console.

showCardDetails fills the labels; fillWalletList refreshes the selected-card
list. I disable Add for a duplicate or full list, and Remove until a wallet
card is selected. After a removal or reload, there is no automatic wallet
selection that could make the next click remove a different card.

I handle clicks in WM_COMMAND, resizing in WM_SIZE, drawing in WM_PAINT, and
closing in WM_CLOSE. IsDialogMessageW handles Tab navigation. The minimum size
leaves room for the lists, detail panel, and footer. Fonts and brushes are
released after the controls are finished with them.

Before reloading over unsaved edits, I ask whether to continue. Closing gives
save, discard, and cancel choices. A failed save leaves the window open.
The save path comes from main; the usual default is savedCards.txt.

The window cross-compiles, but I still need a real Windows check for its
appearance, buttons, resizing, keyboard use, and file replacement. On another
system, this mode explains that Windows is needed and returns 1.
*/
#include <stdio.h>
#include "window.h"
#include <windows.h>
#include <stdbool.h>
#include <wchar.h>
#include "cards.h"
#include "wallet.h"
#include "storage.h"
#include "logger.h"

enum ControlId {
	ID_CATALOG = 101,
	ID_WALLET,
	ID_ADD,
	ID_REMOVE,
	ID_SAVE,
	ID_RELOAD
};

typedef struct {
	HWND title, subtitle, catalogHeading, catalogList, addButton;
	HWND walletHeading, walletCount, walletList, removeButton;
	HWND cardName, rateLabel, rate, cardInfo, bonusHeading, bonuses, bonusNote;
	HWND status, saveButton, reloadButton, disclaimer;
} WindowControls;

static WindowControls controls;
static Wallet windowWallet;
static bool changed;
static bool controlsReady;
static const char* savePath;
static HFONT bodyFont, titleFont, headingFont, rateFont;
static HBRUSH backgroundBrush, panelBrush;

static HWND makeControl(HWND parent, const wchar_t* type, const wchar_t* text,
	DWORD style, int id, HFONT font) {
	/* I reuse this setup so the controls get the same font and basic window styles. */
	HWND control = CreateWindowExW(0, type, text, WS_CHILD | WS_VISIBLE | style,
		0, 0, 1, 1, parent, (HMENU)(INT_PTR)id, GetModuleHandleW(NULL), NULL);
	if (control) {
		SendMessageW(control, WM_SETFONT, (WPARAM)font, TRUE);
	}
	return control;
}

static void setStatus(const wchar_t* text) {
	SetWindowTextW(controls.status, text);
}

static int selectedCardId(HWND list) {
	/* I read the stored card ID; an empty selection is not a real card. */
	LRESULT row = SendMessageW(list, LB_GETCURSEL, 0, 0);
	if (row == LB_ERR) {
		return 0;
	}
	LRESULT cardId = SendMessageW(list, LB_GETITEMDATA, (WPARAM)row, 0);
	if (cardId < 1 || cardId > SAMPLE_CARD_COUNT) {
		return 0;
	}
	return (int)cardId;
}

static void updateButtons(void) {
	/* I prevent an invalid action before the click and still check it in wallet.c. */
	int cardId = selectedCardId(controls.catalogList);
	bool canAdd = cardId != 0 && windowWallet.count < MAX_WALLET_CARDS
		&& !walletHasCard(&windowWallet, cardId);
	EnableWindow(controls.addButton, canAdd);
	EnableWindow(controls.removeButton, selectedCardId(controls.walletList) != 0);
	wchar_t count[80];
	swprintf_s(count, 80, L"%d of %d cards%ls", windowWallet.count, MAX_WALLET_CARDS,
		changed ? L" - unsaved changes" : L"");
	SetWindowTextW(controls.walletCount, count);
}

static void showCardDetails(int cardId) {
	const Card* card = getCardById(cardId);
	if (!card) {
		return;
	}
	wchar_t text[128];
	swprintf_s(text, 128, L"%hs", card->issuer);
	SetWindowTextW(controls.cardName, text);
	swprintf_s(text, 128, L"%.2f%%", card->rate);
	SetWindowTextW(controls.rate, text);
	SetWindowTextW(controls.cardInfo, getBaseInfo(*card));
	SetWindowTextW(controls.bonuses, getMultiplierInfo(*card));
	updateButtons();
}

static bool fillWalletList(int keepSelectedId) {
	SendMessageW(controls.walletList, LB_RESETCONTENT, 0, 0);
	int selectedRow = -1;
	/* I only restore a selection if its requested ID is still in the wallet. */
	for (int i = 0; i < windowWallet.count; i++) {
		int cardId = windowWallet.cardIds[i];
		wchar_t name[128];
		swprintf_s(name, 128, L"%hs", getCardById(cardId)->issuer);
		LRESULT row = SendMessageW(controls.walletList, LB_ADDSTRING, 0, (LPARAM)name);
		if (row == LB_ERR || row == LB_ERRSPACE) {
			setStatus(L"Couldn't display the wallet. Your card list is still in memory.");
			EnableWindow(controls.removeButton, FALSE);
			return false;
		}
		SendMessageW(controls.walletList, LB_SETITEMDATA, (WPARAM)row, cardId);
		if (cardId == keepSelectedId) {
			selectedRow = (int)row;
		}
	}
	if (windowWallet.count == 0) {
		SendMessageW(controls.walletList, LB_ADDSTRING, 0, (LPARAM)L"No cards added yet");
		SendMessageW(controls.walletList, LB_SETITEMDATA, 0, 0);
	} else if (selectedRow >= 0) {
		SendMessageW(controls.walletList, LB_SETCURSEL, (WPARAM)selectedRow, 0);
	}
	updateButtons();
	return true;
}

static bool saveFromWindow(void) {
	/* I leave changed set when saving fails, so closing cannot hide an unsaved edit. */
	if (saveWallet(savePath, &windowWallet) != STORAGE_OK) {
		setStatus(L"Couldn't save. Check the folder and any old .tmp file, then try again.");
		return false;
	}
	changed = false;
	updateButtons();
	setStatus(L"Saved your cards. This list will be here next time.");
	return true;
}

static void reloadFromWindow(HWND hwnd) {
	/* I ask before replacing a list that has unsaved changes. */
	if (changed && MessageBoxW(hwnd,
		L"Reload your last save? Changes you haven't saved will be lost.",
		L"Reload cards", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) != IDYES) {
		return;
	}
	enum StorageResult result = loadWallet(savePath, &windowWallet);
	if (result == STORAGE_OK) {
		changed = false;
		if (fillWalletList(0)) {
			setStatus(L"Loaded your saved cards.");
		}
	} else if (result == STORAGE_MISSING) {
		setStatus(L"No saved file yet. Choose Save cards to keep this list.");
	} else if (result == STORAGE_BAD_FILE) {
		setStatus(L"That file looks damaged. Your open card list hasn't changed.");
	} else {
		setStatus(L"Couldn't read the saved file. Your open card list hasn't changed.");
	}
}

static void handleWindowAction(HWND hwnd, int id, int notification) {
	if (id == ID_CATALOG && notification == LBN_SELCHANGE) {
		showCardDetails(selectedCardId(controls.catalogList));
	} else if (id == ID_WALLET && notification == LBN_SELCHANGE) {
		int cardId = selectedCardId(controls.walletList);
		if (cardId) {
			/* I keep Add and the details panel pointed at the same card. */
			SendMessageW(controls.catalogList, LB_SETCURSEL, (WPARAM)(cardId - 1), 0);
			showCardDetails(cardId);
		}
		updateButtons();
	} else if ((id == ID_ADD && notification == BN_CLICKED)
		|| (id == ID_CATALOG && notification == LBN_DBLCLK)) {
		int cardId = selectedCardId(controls.catalogList);
		enum WalletResult result = addWalletCard(&windowWallet, cardId);
		if (result == WALLET_OK) {
			changed = true;
			if (fillWalletList(cardId)) {
				setStatus(L"Added to your wallet. Save when you're ready.");
			}
		} else if (result == WALLET_DUPLICATE) {
			setStatus(L"That card is already in your wallet.");
		} else if (result == WALLET_FULL) {
			setStatus(L"Your wallet is full. Remove a card to make room.");
		} else {
			setStatus(L"Choose a sample card first.");
		}
	} else if (id == ID_REMOVE && notification == BN_CLICKED) {
		if (removeWalletCard(&windowWallet, selectedCardId(controls.walletList))) {
			changed = true;
			if (fillWalletList(0)) {
				setStatus(L"Removed from your wallet. Save to keep this change.");
			}
		}
	} else if (id == ID_SAVE && notification == BN_CLICKED) {
		saveFromWindow();
	} else if (id == ID_RELOAD && notification == BN_CLICKED) {
		reloadFromWindow(hwnd);
	}
}

static void onWindowResize(HWND hwnd) {
	if (!controlsReady) {
		return;
	}
	RECT area;
	GetClientRect(hwnd, &area);
	int width = area.right;
	int height = area.bottom;
	if (width == 0 || height == 0) {
		return;
	}
	int detailWidth = width - 400;
	/* I keep the left list width fixed and let the detail panel use the extra space. */
	MoveWindow(controls.title, 24, 24, width - 48, 32, TRUE);
	MoveWindow(controls.subtitle, 24, 64, width - 48, 26, TRUE);
	MoveWindow(controls.catalogHeading, 24, 112, 300, 26, TRUE);
	MoveWindow(controls.catalogList, 24, 144, 300, 154, TRUE);
	MoveWindow(controls.addButton, 24, 310, 300, 34, TRUE);
	MoveWindow(controls.walletHeading, 24, 356, 300, 26, TRUE);
	MoveWindow(controls.walletCount, 24, 384, 300, 22, TRUE);
	MoveWindow(controls.walletList, 24, 410, 300, height - 564, TRUE);
	MoveWindow(controls.removeButton, 24, height - 144, 300, 34, TRUE);
	MoveWindow(controls.cardName, 376, 136, detailWidth, 34, TRUE);
	MoveWindow(controls.rateLabel, 376, 180, detailWidth, 22, TRUE);
	MoveWindow(controls.rate, 376, 208, detailWidth, 36, TRUE);
	MoveWindow(controls.cardInfo, 376, 268, detailWidth, 72, TRUE);
	MoveWindow(controls.bonusHeading, 376, 354, detailWidth, 24, TRUE);
	MoveWindow(controls.bonuses, 376, 388, detailWidth, 66, TRUE);
	MoveWindow(controls.bonusNote, 376, height - 174, detailWidth, 48, TRUE);
	MoveWindow(controls.status, 24, height - 88, width - 380, 44, TRUE);
	MoveWindow(controls.saveButton, width - 324, height - 88, 144, 36, TRUE);
	MoveWindow(controls.reloadButton, width - 168, height - 88, 144, 36, TRUE);
	MoveWindow(controls.disclaimer, 24, height - 32, width - 48, 22, TRUE);
	InvalidateRect(hwnd, NULL, FALSE);
}

static bool loadWindowWidgets(HWND hwnd) {
	DWORD listStyle = WS_BORDER | WS_TABSTOP | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT;
	DWORD labelStyle = SS_LEFT | SS_NOPREFIX;
	controls.title = makeControl(hwnd, L"STATIC", L"CardClarity", labelStyle, 0, titleFont);
	controls.subtitle = makeControl(hwnd, L"STATIC", L"Select a card to see its rates, then add it to your wallet.", labelStyle, 0, bodyFont);
	controls.catalogHeading = makeControl(hwnd, L"STATIC", L"Sample cards", labelStyle, 0, headingFont);
	controls.catalogList = makeControl(hwnd, L"LISTBOX", L"", listStyle, ID_CATALOG, bodyFont);
	controls.addButton = makeControl(hwnd, L"BUTTON", L"Add to my wallet", WS_TABSTOP | BS_PUSHBUTTON, ID_ADD, bodyFont);
	controls.walletHeading = makeControl(hwnd, L"STATIC", L"My wallet", labelStyle, 0, headingFont);
	controls.walletCount = makeControl(hwnd, L"STATIC", L"", labelStyle, 0, bodyFont);
	controls.walletList = makeControl(hwnd, L"LISTBOX", L"", listStyle, ID_WALLET, bodyFont);
	controls.removeButton = makeControl(hwnd, L"BUTTON", L"Remove selected card", WS_TABSTOP | BS_PUSHBUTTON, ID_REMOVE, bodyFont);
	controls.cardName = makeControl(hwnd, L"STATIC", L"", labelStyle, 0, headingFont);
	controls.rateLabel = makeControl(hwnd, L"STATIC", L"Base reward rate (example)", labelStyle, 0, bodyFont);
	controls.rate = makeControl(hwnd, L"STATIC", L"", labelStyle, 0, rateFont);
	controls.cardInfo = makeControl(hwnd, L"STATIC", L"", labelStyle, 0, bodyFont);
	controls.bonusHeading = makeControl(hwnd, L"STATIC", L"Category bonuses", labelStyle, 0, headingFont);
	controls.bonuses = makeControl(hwnd, L"STATIC", L"", labelStyle, 0, bodyFont);
	controls.bonusNote = makeControl(hwnd, L"STATIC", L"A +5% bonus increases the base reward by 5%.\nIt does not mean five times the reward.", labelStyle, 0, bodyFont);
	controls.status = makeControl(hwnd, L"STATIC", L"Ready. Add a card to start your wallet.", labelStyle, 0, bodyFont);
	controls.saveButton = makeControl(hwnd, L"BUTTON", L"Save cards", WS_TABSTOP | BS_PUSHBUTTON, ID_SAVE, bodyFont);
	controls.reloadButton = makeControl(hwnd, L"BUTTON", L"Reload saved", WS_TABSTOP | BS_PUSHBUTTON, ID_RELOAD, bodyFont);
	controls.disclaimer = makeControl(hwnd, L"STATIC", L"Made-up cards and rates for practice. These are not real bank offers.", labelStyle, 0, bodyFont);
	HWND handles[] = {
		controls.title, controls.subtitle, controls.catalogHeading, controls.catalogList,
		controls.addButton, controls.walletHeading, controls.walletCount, controls.walletList,
		controls.removeButton, controls.cardName, controls.rateLabel, controls.rate,
		controls.cardInfo, controls.bonusHeading, controls.bonuses, controls.bonusNote,
		controls.status, controls.saveButton, controls.reloadButton, controls.disclaimer
	};
	for (size_t i = 0; i < sizeof(handles) / sizeof(handles[0]); i++) {
		if (!handles[i]) {
			return false;
		}
	}
	for (int cardId = 1; cardId <= SAMPLE_CARD_COUNT; cardId++) {
		/* I attach the real ID to each row instead of using a wallet position. */
		const Card* card = getCardById(cardId);
		wchar_t rowText[128];
		swprintf_s(rowText, 128, L"%hs - %.2f%% base", card->issuer, card->rate);
		LRESULT row = SendMessageW(controls.catalogList, LB_ADDSTRING, 0, (LPARAM)rowText);
		if (row == LB_ERR || row == LB_ERRSPACE) {
			return false;
		}
		SendMessageW(controls.catalogList, LB_SETITEMDATA, (WPARAM)row, cardId);
	}
	SendMessageW(controls.catalogList, LB_SETCURSEL, 0, 0);
	controlsReady = true;
	if (!fillWalletList(0)) {
		return false;
	}
	showCardDetails(1);
	onWindowResize(hwnd);
	reloadFromWindow(hwnd);
	return true;
}

static bool detailLabel(HWND control) {
	return control == controls.cardName || control == controls.rateLabel
		|| control == controls.rate || control == controls.cardInfo
		|| control == controls.bonusHeading || control == controls.bonuses
		|| control == controls.bonusNote;
}

static LRESULT CALLBACK applicationWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
	/* I route each Windows message to the part of the interface it affects. */
	switch (message) {
	case WM_CREATE:
		return loadWindowWidgets(hwnd) ? 0 : -1;
	case WM_SIZE:
		onWindowResize(hwnd);
		return 0;
	case WM_GETMINMAXINFO: {
		MINMAXINFO* limits = (MINMAXINFO*)lParam;
		limits->ptMinTrackSize.x = 880;
		limits->ptMinTrackSize.y = 680;
		return 0;
	}
	case WM_COMMAND:
		handleWindowAction(hwnd, LOWORD(wParam), HIWORD(wParam));
		return 0;
	case WM_CTLCOLORSTATIC: {
		HDC dc = (HDC)wParam;
		HWND label = (HWND)lParam;
		SetBkMode(dc, TRANSPARENT);
		bool muted = label == controls.subtitle || label == controls.rateLabel
			|| label == controls.bonusNote || label == controls.walletCount || label == controls.disclaimer;
		SetTextColor(dc, muted ? RGB(96, 96, 96) : RGB(40, 40, 40));
		return (LRESULT)(detailLabel(label) ? panelBrush : backgroundBrush);
	}
	case WM_CTLCOLORLISTBOX:
		SetTextColor((HDC)wParam, RGB(40, 40, 40));
		SetBkColor((HDC)wParam, RGB(255, 255, 255));
		return (LRESULT)panelBrush;
	case WM_PAINT: {
		PAINTSTRUCT paint;
		HDC dc = BeginPaint(hwnd, &paint);
		RECT area;
		GetClientRect(hwnd, &area);
		FillRect(dc, &paint.rcPaint, backgroundBrush);
		RECT panel = { 352, 112, area.right - 24, area.bottom - 112 };
		FillRect(dc, &panel, panelBrush);
		EndPaint(hwnd, &paint);
		return 0;
	}
	case WM_CLOSE:
		if (changed) {
			int answer = MessageBoxW(hwnd, L"Save your changes before closing?", L"Close CardClarity",
				MB_YESNOCANCEL | MB_ICONQUESTION);
			if (answer == IDCANCEL || (answer == IDYES && !saveFromWindow())) {
				return 0;
			}
		}
		DestroyWindow(hwnd);
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	default:
		return DefWindowProcW(hwnd, message, wParam, lParam);
	}
}

static void releaseWindowResources(void) {
	/* I release these objects after the controls no longer need their fonts or colors. */
	if (bodyFont) {
		DeleteObject(bodyFont);
	}
	if (titleFont) {
		DeleteObject(titleFont);
	}
	if (headingFont) {
		DeleteObject(headingFont);
	}
	if (rateFont) {
		DeleteObject(rateFont);
	}
	if (backgroundBrush) {
		DeleteObject(backgroundBrush);
	}
	if (panelBrush) {
		DeleteObject(panelBrush);
	}
}

int runWindow(const char* filePath) {
	initLogger();

	initWallet(&windowWallet);
	changed = false;
	controlsReady = false;
	savePath = filePath;
	backgroundBrush = CreateSolidBrush(RGB(245, 244, 240));
	panelBrush = CreateSolidBrush(RGB(255, 255, 255));
	/* I use one plain font and bold text just for the section headings. */
	bodyFont = CreateFontW(-16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Tahoma");
	titleFont = CreateFontW(-24, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Tahoma");
	headingFont = CreateFontW(-17, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Tahoma");
	rateFont = CreateFontW(-26, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Tahoma");
	if (!backgroundBrush || !panelBrush || !bodyFont || !titleFont || !headingFont || !rateFont) {
		releaseWindowResources();
		return 1;
	}
	WNDCLASSW wc = { 0 };
	wc.lpfnWndProc = applicationWindowProc;
	wc.hInstance = GetModuleHandleW(NULL);
	wc.lpszClassName = L"CardClarityWindow";
	wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
	if (!RegisterClassW(&wc)) {
		releaseWindowResources();
		return 1;
	}
	HWND hwnd = CreateWindowW(wc.lpszClassName, L"CardClarity", WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
		CW_USEDEFAULT, CW_USEDEFAULT, 1000, 720, NULL, NULL, wc.hInstance, NULL);
	int exitCode = 0;
	if (!hwnd) {
		logMessage("Couldn't open the card window.", LOG_ERROR);
		exitCode = 1;
	} else {
		ShowWindow(hwnd, SW_SHOW);
		SetFocus(controls.catalogList);
		MSG message;
		int result;
		while ((result = GetMessageW(&message, NULL, 0, 0)) > 0) {
			/* I let Tab move between controls before sending ordinary window messages. */
			if (!IsDialogMessageW(hwnd, &message)) {
				TranslateMessage(&message);
				DispatchMessageW(&message);
			}
		}
		if (result == -1) {
			exitCode = 1;
		}
		if (IsWindow(hwnd)) {
			DestroyWindow(hwnd);
		}
	}
	UnregisterClassW(wc.lpszClassName, wc.hInstance);
	releaseWindowResources();
	return exitCode;
}
