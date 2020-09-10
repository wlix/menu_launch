#pragma once

#include <shlobj.h>
#include <shlwapi.h>

#define SAFE_RELEASE(p) if(p){p->Release();p=NULL;}

typedef IShellIcon* LPSHELLICON;

class CItemIDList {
public:
	inline HRESULT WINAPI GetUIObjectOf(REFIID riid, LPVOID* ppOut, HWND hWnd = NULL) {
		return GetUIObjectOf(riid, ppOut, m_pidl, hWnd);
	}
	//Get relative pidl and the corresponding ShellFolder interface.
	inline void WINAPI Split(LPSHELLFOLDER& lpsf, CItemIDList& ciid) {
		ciid.Empty();
		Split(lpsf, ciid.m_pidl, m_pidl);
	}
	//get last relative pidl.
	CItemIDList WINAPI GetLastPidl(void) const;
	void WINAPI GetToolTipInfo(LPTSTR pszToolTip, UINT cbSize) const;
	void WINAPI Attach(LPITEMIDLIST pidl);
	LPITEMIDLIST WINAPI Detach(void);
	int WINAPI GetIconIndex(UINT uFlags = SHGFI_PIDL | SHGFI_SYSICONINDEX | SHGFI_SMALLICON) const;
	BOOL WINAPI GetDisplayName(LPTSTR pszBuf, DWORD dwFlags = SHGDN_NORMAL) const;//Retrieve pidl's dislpay name.
	//Copy a pidl due to the count number.
	//nCount=-1 indicate copy all pidl.
	CItemIDList WINAPI Duplicate(UINT nCount = -1) const;
	inline BOOL WINAPI IsEmpty(void) const { return m_pidl ? FALSE : TRUE; }
	CItemIDList WINAPI GetAt(UINT nIndex) const;//Return a relative pidl at specified index.
	BOOL WINAPI Create(LPITEMIDLIST pidlf);
	inline UINT WINAPI GetCount(void) const { return GetPidlCount(m_pidl); }//Get pidl count.
	inline UINT WINAPI GetSize(void) const { return GetPidlSize(m_pidl); }
	inline void WINAPI Empty(void) { FreePidl(m_pidl); }
	void WINAPI GetPath(LPTSTR pszPath) const;//Retrieve full path.(only available for full-quality pidl)

	//operator overload.
	CItemIDList WINAPI operator+(CItemIDList& piidl);//Concat two pidls.
	inline WINAPI operator LPITEMIDLIST(void) const { return m_pidl; }
	inline WINAPI operator LPCITEMIDLIST(void) const { return (LPCITEMIDLIST)m_pidl; }
	inline WINAPI operator LPITEMIDLIST* (void) { return &m_pidl; }
	inline WINAPI operator LPCITEMIDLIST* (void) { return (LPCITEMIDLIST*)&m_pidl; }
	const CItemIDList& WINAPI operator=(const CItemIDList& ciidl1);
	const CItemIDList& WINAPI operator=(LPITEMIDLIST pidl);
	CItemIDList& WINAPI operator+=(CItemIDList& ciidl);//Add a new pidl to tail.
	BOOL WINAPI operator==(CItemIDList& ciidl) const;
	CItemIDList WINAPI operator[](UINT nIndex) const;//Return a relative pidl at specified index.

	//constructor,destructor.
	CItemIDList(LPCTSTR pcszPath);
	CItemIDList(LPITEMIDLIST pidl);
	CItemIDList(const CItemIDList& iidl);
	CItemIDList(void);
	virtual ~CItemIDList(void);

	//member variable
	LPITEMIDLIST m_pidl;

	//static member function for export.
public:
	static int WINAPI GetIconIndex(LPITEMIDLIST pidlf, UINT uFlags = SHGFI_PIDL | SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
	static void WINAPI Split(LPSHELLFOLDER& lpsf, LPITEMIDLIST& pidl, LPITEMIDLIST pidlf);
	static BOOL WINAPI GetFileToolTip(LPSHELLFOLDER lpsf, LPITEMIDLIST pidl, LPSTR pszToolTip, UINT cbSize);
	static BOOL WINAPI GetFileToolTip(LPSHELLFOLDER lpsf, LPITEMIDLIST pidl, LPWSTR pszToolTip, UINT cbSize);
	static LPITEMIDLIST WINAPI CreatePidl(UINT nSize);
	static LPITEMIDLIST WINAPI ConcatPidls(LPITEMIDLIST pidl1, LPITEMIDLIST pidl2);//Concat two pidl.
	//Create a relative pidl through a full-quality pidl at specified index.
	static LPITEMIDLIST WINAPI CreateOnePidl(LPITEMIDLIST pidlf, int Index);
	static UINT WINAPI GetPidlCount(LPITEMIDLIST pidl);
	static UINT WINAPI GetPidlSize(LPITEMIDLIST pidl);
	static UINT WINAPI GetPidlSize(LPITEMIDLIST pidl, UINT nCount);//Get pidl size throught the specified count.
	static void WINAPI FreePidl(LPITEMIDLIST& pidl);
	static LPITEMIDLIST WINAPI CopyItemIDList(LPITEMIDLIST pidl);//Duplicate specified pidl,and you need free it.
	//Get full-quality pidl throught a relative pidl and it's corresponding ShellFolder interface
	static LPITEMIDLIST WINAPI GetFullyQualPidl(LPSHELLFOLDER lpsf, LPITEMIDLIST lpi);
	static BOOL WINAPI GetDispName(LPSHELLFOLDER lpsf, LPITEMIDLIST lpi, DWORD dwFlags,
		LPTSTR lpFriendlyName);
	static HRESULT WINAPI GetUIObjectOf(REFIID riid, LPVOID* ppOut, LPITEMIDLIST pidlf, HWND hWnd);
	static LPITEMIDLIST WINAPI CreatePidl(LPITEMIDLIST pidlOrg, UINT nCount);
	static short WINAPI ComparePIDL(LPITEMIDLIST pidlf1, LPITEMIDLIST pidlf2,
		LPSHELLFOLDER psfFolder = NULL, LPARAM lParam = 0);//return zero means same,non-zero means different.
	static HRESULT WINAPI PathToItemIDList(LPITEMIDLIST& pidl, LPCTSTR pcszPath, LPSHELLFOLDER psfFolder);
	static HRESULT WINAPI PathToItemIDList(LPITEMIDLIST& pidlf, LPCTSTR pcszPath);
	static int WINAPI GetOverlayIconIndex(LPSHELLFOLDER lpsfFolder, LPITEMIDLIST pidl);

protected:
	static LPITEMIDLIST WINAPI Next(LPITEMIDLIST pidl);
};