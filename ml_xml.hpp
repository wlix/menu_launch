#pragma once

#include "rapidxml.hpp"
#include "ml_defs.hpp"

enum class XML_TEXT_ENCODING {
	SHIFT_JIS = 0,
	UTF_8,
	UTF_16,
	XML_TEXT_ENCODING_NUMS
};

typedef  std::map<std::string, std::wstring> AttributeMap;

using namespace rapidxml;

BOOL WINAPI StrToBOOL(LPCTSTR lpstr);
std::wstring WINAPI ml_ExpandEnvironmentStrings(const std::wstring& str);
BOOL WINAPI AppendMenuItem(HMENU hMenu, LPCTSTR lpStr, HBITMAP hBitmap, HMENU hSubmenu, UINT wID);
HBITMAP WINAPI IconToBitmap(HICON hIcon);
HBITMAP WINAPI GetIconBitmap(const AttributeMap& attr);
HBITMAP WINAPI GetDesktopIcon();
BOOL WINAPI LoadMenuItems(xml_node<>* node, HMENU hMenu, MLMenu& mlm);
BOOL WINAPI LoadMenu(xml_node<>* node, MLMenu& mlm);
BOOL WINAPI LoadXML(LPCTSTR szFileName, MLMenu& mlm);
BOOL WINAPI GetAttributes(xml_node<>* node, AttributeMap& attr);
BOOL WINAPI LoadSettings(xml_node<>* node, MLMenu& mlm);
