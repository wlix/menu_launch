#pragma once

#include <msxml.h>
#include <comdef.h>
#include "ml_defs.hpp"

typedef  std::map<tstring, tstring> AttributeMap;

BOOL WINAPI StrToBOOL(LPCTSTR lpstr);
tstring WINAPI xml_ExpandEnvironmentStrings(const tstring& str);
BOOL WINAPI AppendMenuItem(HMENU hMenu, LPCTSTR lpStr, HBITMAP hBitmap, HMENU hSubmenu, UINT wID);
HBITMAP WINAPI IconToBitmap(HICON hIcon);
HBITMAP WINAPI GetIconBitmap(const AttributeMap& attr);
HBITMAP WINAPI GetDesktopIcon();
BOOL WINAPI LoadMenuItems(IXMLDOMNodePtr node, HMENU hMenu, MLMenu& mlm);
BOOL WINAPI xml_LoadMenu(IXMLDOMNodePtr node, MLMenu& mlm);
BOOL WINAPI LoadXML(LPCTSTR szFileName, MLMenu& mlm);
BOOL WINAPI GetAttributes(IXMLDOMNodePtr node, AttributeMap& attr);
BOOL WINAPI LoadSettings(IXMLDOMNodePtr node, MLMenu& mlm);
