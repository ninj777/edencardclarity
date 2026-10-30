/* I open the comparison form from the existing wallet window. */
#ifndef REWARD_WINDOW_H
#define REWARD_WINDOW_H
#ifdef _WIN32
#include <windows.h>
#include "wallet.h"
void openRewardWindow(HWND owner, const Wallet* wallet);
#endif
#endif
