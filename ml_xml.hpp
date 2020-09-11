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

std::wstring WINAPI multi_to_wide(std::string const& src);
std::wstring WINAPI utf8_to_wide(std::string const& src);
std::string WINAPI utf16_to_utf8(std::u16string const& src);
BOOL WINAPI StrToBOOL(LPCTSTR lpstr);
std::wstring WINAPI WrappedExpandEnvironmentStrings(const std::wstring& str);
BOOL WINAPI AppendMenuItem(HMENU hMenu, LPCTSTR lpStr, HBITMAP hBitmap, HMENU hSubmenu, UINT wID);
HBITMAP WINAPI IconToBitmap(HICON hIcon);
HBITMAP WINAPI GetIconBitmap(const AttributeMap& attr);
HBITMAP WINAPI GetDesktopIcon();
BOOL WINAPI LoadMenuItems(xml_node<>* node, HMENU hMenu, MLMenu& mlm);
BOOL WINAPI LoadMenu(xml_node<>* node, MLMenu& mlm);
BOOL WINAPI LoadXML(LPCTSTR szFileName, MLMenu& mlm);
BOOL WINAPI GetAttributes(xml_node<>* node, AttributeMap& attr);
std::wstring WINAPI to_mbcs(std::string str);
BOOL WINAPI LoadSettings(xml_node<>* node, MLMenu& mlm);
