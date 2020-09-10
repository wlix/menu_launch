#pragma once

#include <msxml.h>
#include <comdef.h>
#include "rapidxml.hpp"
#include "ml_defs.hpp"

typedef  std::map<tstring, tstring> AttributeMap;

using namespace rapidxml;

BOOL WINAPI StrToBOOL(LPCTSTR lpstr);
tstring WINAPI ml_ExpandEnvironmentStrings(const tstring& str);
BOOL WINAPI AppendMenuItem(HMENU hMenu, LPCTSTR lpStr, HBITMAP hBitmap, HMENU hSubmenu, UINT wID);
HBITMAP WINAPI IconToBitmap(HICON hIcon);
HBITMAP WINAPI GetIconBitmap(const AttributeMap& attr);
HBITMAP WINAPI GetDesktopIcon();
BOOL WINAPI LoadMenuItems(xml_node<TCHAR>* node, HMENU hMenu, MLMenu& mlm);
BOOL WINAPI LoadMenu(xml_node<TCHAR>* node, MLMenu& mlm);
BOOL WINAPI LoadXML(LPCTSTR szFileName, MLMenu& mlm);
BOOL WINAPI GetAttributes(xml_node<TCHAR>* node, AttributeMap& attr);
BOOL WINAPI LoadSettings(xml_node<TCHAR>* node, MLMenu& mlm);
