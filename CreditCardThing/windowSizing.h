/* Oliver's window area. I move the whole layout before repainting it. */
#ifndef WINDOW_SIZING_H
#define WINDOW_SIZING_H

#ifdef _WIN32
#include <windows.h>
#include <stddef.h>

typedef struct {
	HWND control;
	int x, y, width, height;
} ControlPlacement;

static inline void placeControls(HWND parent, const ControlPlacement* positions, size_t count) {
	UINT flags = SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW | SWP_NOCOPYBITS;
	HDWP batch = BeginDeferWindowPos((int)count);
	for (size_t i = 0; batch && i < count; i++) {
		const ControlPlacement* p = &positions[i];
		batch = DeferWindowPos(batch, p->control, NULL, p->x, p->y, p->width, p->height, flags);
	}
	/* If Windows cannot make a batch, finish the same layout without redrawing between moves. */
	if (!batch || !EndDeferWindowPos(batch)) {
		for (size_t i = 0; i < count; i++) {
			const ControlPlacement* p = &positions[i];
			SetWindowPos(p->control, NULL, p->x, p->y, p->width, p->height, flags);
		}
	}
	/* Old control positions and labels need their backgrounds repainted too. */
	RedrawWindow(parent, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
}

static inline void minimumClientSize(HWND hwnd, MINMAXINFO* limits, int width, int height) {
	RECT outer = { 0, 0, width, height };
	if (AdjustWindowRectEx(&outer, (DWORD)GetWindowLongPtrW(hwnd, GWL_STYLE),
		GetMenu(hwnd) != NULL, (DWORD)GetWindowLongPtrW(hwnd, GWL_EXSTYLE))) {
		limits->ptMinTrackSize.x = outer.right - outer.left;
		limits->ptMinTrackSize.y = outer.bottom - outer.top;
	}
}
#endif
#endif
