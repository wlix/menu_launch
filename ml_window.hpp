#pragma once

#include "ml_defs.hpp"

HWND WINAPI CreateMainWnd(HINSTANCE hInst, LPCTSTR ClassName, LPCTSTR WindowName);
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);
void WINAPI OnMenuRbuttonUp(HWND hWnd, WPARAM wp, LPARAM lp);
int WINAPI GetMenuItemPosFromID(HMENU hMenu, int ID);
void WINAPI ContextMenu(HWND hWnd, size_t wID, POINT pt);
void WINAPI OnExecuteCommand(HWND hWnd, WPARAM wp, LPARAM lp);
void WINAPI Init(HWND hWnd);
void WINAPI Uninit();
BOOL WINAPI Execute(const tstring& Path, const tstring& Param, HWND hWnd);
BOOL WINAPI TrackPopupMenuExPos(HMENU hmenu, UINT fuFlags, TPMPOS* ptp, HWND hwnd, LPTPMPARAMS lptpm);
LRESULT CALLBACK KeyboardProc(int code, WPARAM wParam, LPARAM lParam);
