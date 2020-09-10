#include "ml_item_id_list.hpp"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CItemIDList::CItemIDList(void) : m_pidl(NULL) {
}

CItemIDList::CItemIDList(const CItemIDList& iidl) : m_pidl(NULL) {
	if (!iidl.IsEmpty()) {
		m_pidl = CopyItemIDList(iidl);
	}
}

CItemIDList::CItemIDList(LPCTSTR pcszPath) : m_pidl(NULL) {
	LPSHELLFOLDER lpsf = NULL;
	if (FAILED(SHGetDesktopFolder(&lpsf))) {
		throw TEXT("Construct CItemIDList failed!");
	}

	WCHAR wzPath[MAX_PATH];
	MultiByteToWideChar(CP_ACP, 0, (LPCSTR)pcszPath, -1, wzPath, MAX_PATH);
	if (FAILED(lpsf->ParseDisplayName(NULL, NULL, wzPath, NULL, &m_pidl, NULL))) {
		throw TEXT("ParseDisplayName failed!");
	}

	SAFE_RELEASE(lpsf);
}

CItemIDList::CItemIDList(LPITEMIDLIST pidl) : m_pidl(NULL) {
	m_pidl = CopyItemIDList(pidl);
}

CItemIDList::~CItemIDList(void) {
	Empty();
}

void WINAPI CItemIDList::FreePidl(LPITEMIDLIST& pidl) {
	if (!pidl) { return; }

	LPITEMIDLIST p = pidl;
	pidl = NULL;
	LPMALLOC lpMalloc = NULL;
	if (FAILED(SHGetMalloc(&lpMalloc))) { return; }

	lpMalloc->Free(p);
	SAFE_RELEASE(lpMalloc);
}

LPITEMIDLIST WINAPI CItemIDList::CopyItemIDList(LPITEMIDLIST pidl) {
	if (!pidl)return NULL;
	UINT nSize = GetPidlSize(pidl);

	LPMALLOC lpMalloc = NULL;
	if (FAILED(SHGetMalloc(&lpMalloc))) { return NULL; }

	LPITEMIDLIST pidlNew = (LPITEMIDLIST)lpMalloc->Alloc(nSize);
	if (pidlNew) {
		memcpy(pidlNew, pidl, nSize);
	}

	SAFE_RELEASE(lpMalloc);
	return pidlNew;
}

LPITEMIDLIST WINAPI CItemIDList::Next(LPITEMIDLIST pidl) {
	LPBYTE lpMem = (LPBYTE)pidl;

	lpMem += pidl->mkid.cb;

	return (LPITEMIDLIST)lpMem;
}

UINT WINAPI CItemIDList::GetPidlCount(LPITEMIDLIST pidl) {
	if (!pidl) { return 0; }
	LPITEMIDLIST pidlr = pidl;
	UINT nCount = 0;

	while (pidlr->mkid.cb) {
		nCount++;
		pidlr = Next(pidlr);
	}
	return nCount;
}

UINT WINAPI CItemIDList::GetPidlSize(LPITEMIDLIST pidl) {
	if (!pidl) { return 0; }
	UINT nSize = sizeof(pidl->mkid.cb);

	while (pidl->mkid.cb) {
		nSize += pidl->mkid.cb;
		pidl = Next(pidl);
	}
	return nSize;
}

CItemIDList WINAPI CItemIDList::operator+(CItemIDList& piidl) {
	CItemIDList ciidl;
	LPITEMIDLIST pidlf = ConcatPidls(m_pidl, piidl);

	ciidl.Create(pidlf);
	FreePidl(pidlf);

	return ciidl;
}

const CItemIDList& WINAPI CItemIDList::operator=(const CItemIDList& ciidl1) {
	Empty();
	if (!ciidl1.IsEmpty()) {
		m_pidl = CopyItemIDList(ciidl1);
	}
	return *this;
}

LPITEMIDLIST WINAPI CItemIDList::ConcatPidls(LPITEMIDLIST pidl1, LPITEMIDLIST pidl2) {
	LPITEMIDLIST pidlNew;
	UINT cb1;
	UINT cb2;

	if (pidl1) { cb1 = GetPidlSize(pidl1) - sizeof(pidl1->mkid.cb); }
	else { cb1 = 0; }

	cb2 = GetPidlSize(pidl2);
	pidlNew = CreatePidl(cb1 + cb2);
	if (pidlNew) {
		if (pidl1) { memcpy(pidlNew, pidl1, cb1); }
		memcpy(((LPSTR)pidlNew) + cb1, pidl2, cb2);
	}
	return pidlNew;
}

LPITEMIDLIST WINAPI CItemIDList::CreatePidl(UINT nSize) {
	LPMALLOC lpMalloc;
	LPITEMIDLIST pidl = NULL;

	if (FAILED(SHGetMalloc(&lpMalloc))) { return NULL; }

	pidl = (LPITEMIDLIST)lpMalloc->Alloc(nSize);
	if (pidl) { memset(pidl, 0, nSize); }

	SAFE_RELEASE(lpMalloc);
	return pidl;
}

const CItemIDList& WINAPI CItemIDList::operator=(LPITEMIDLIST pidl) {
	Empty();
	if (pidl) { m_pidl = CopyItemIDList(pidl); }
	return *this;
}

BOOL WINAPI CItemIDList::Create(LPITEMIDLIST pidlf) {
	Empty();
	m_pidl = CopyItemIDList(pidlf);

	return TRUE;
}

CItemIDList& WINAPI CItemIDList::operator+=(CItemIDList& ciidl) {
	LPITEMIDLIST pidlp = m_pidl;
	m_pidl = ConcatPidls(m_pidl, ciidl);
	FreePidl(pidlp);
	return *this;
}

CItemIDList WINAPI CItemIDList::operator[](UINT nIndex) const {
	CItemIDList ciidl;
	if (nIndex < GetCount()) {
		LPITEMIDLIST pidl = CreateOnePidl(m_pidl, nIndex);
		ciidl.Create(pidl);
		FreePidl(pidl);
	}
	return ciidl;
}

LPITEMIDLIST WINAPI CItemIDList::CreateOnePidl(LPITEMIDLIST pidlf, int Index) {
	if (Index < 0) { return NULL; }
	int Count = GetPidlCount(pidlf);
	if (!Count || Index > Count - 1) { return NULL; }

	while (Index) {
		Index--;
		pidlf = Next(pidlf);
	}
	LPITEMIDLIST pidlRet = NULL;
	UINT uSize = pidlf->mkid.cb;
	pidlRet = CreatePidl(uSize + sizeof(pidlf->mkid.cb));
	ZeroMemory(pidlRet, uSize + sizeof(pidlf->mkid.cb));
	memcpy(pidlRet, pidlf, uSize);

	return pidlRet;
}

CItemIDList WINAPI CItemIDList::GetAt(UINT nIndex) const {
	CItemIDList ciidl;
	if (nIndex < GetCount()) {
		LPITEMIDLIST pidl = CreateOnePidl(m_pidl, nIndex);
		ciidl.Create(pidl);
		FreePidl(pidl);
	}
	return ciidl;
}

BOOL WINAPI CItemIDList::operator==(CItemIDList& ciidl) const {
	LPSHELLFOLDER lpsfDesktop = NULL;
	if (FAILED(SHGetDesktopFolder(&lpsfDesktop))) { return FALSE; }
	BOOL bRet = (lpsfDesktop->CompareIDs(0, m_pidl, ciidl) == 0);

	SAFE_RELEASE(lpsfDesktop);
	return bRet;
}

CItemIDList WINAPI CItemIDList::Duplicate(UINT nCount) const {
	CItemIDList ciidl;
	if (nCount == -1) { nCount = GetCount(); }
	if (nCount < 1) { return ciidl; }
	UINT iCount = GetCount();
	if (nCount > iCount) { nCount = iCount; }

	LPITEMIDLIST pidlRes = NULL;
	LPMALLOC lpMalloc = NULL;
	size_t nSize = GetPidlSize(m_pidl, nCount);
	if (FAILED(SHGetMalloc(&lpMalloc))) { return ciidl; }
	pidlRes = (LPITEMIDLIST)lpMalloc->Alloc(sizeof(m_pidl->mkid.cb) + nSize);
	ZeroMemory(pidlRes, nSize + sizeof(m_pidl->mkid.cb));

	memcpy((LPVOID)pidlRes, (LPVOID)m_pidl, nSize);
	SAFE_RELEASE(lpMalloc);

	ciidl.Create(pidlRes);
	FreePidl(pidlRes);
	return ciidl;
}

UINT WINAPI CItemIDList::GetPidlSize(LPITEMIDLIST pidl, UINT nCount) {
	UINT iCount = GetPidlCount(pidl);
	if (iCount <= nCount) { return GetPidlSize(pidl); }
	UINT cbTotal = 0;
	if (pidl) {
		while (nCount--) {
			cbTotal += pidl->mkid.cb;
			pidl = Next(pidl);
		}
	}
	return cbTotal;
}

BOOL WINAPI CItemIDList::GetDisplayName(LPTSTR pszBuf, DWORD dwFlags) const {
	if (!pszBuf) { return FALSE; }

	memset(pszBuf, 0, 2);
	LPSHELLFOLDER lpsf = NULL, lpsfDesktop = NULL;
	HRESULT hr;
	CItemIDList ciidl;
	if (FAILED(SHGetDesktopFolder(&lpsfDesktop))) { return FALSE; }
	UINT nCount = GetCount();
	if (nCount > 1) {
		ciidl = Duplicate(nCount - 1);
		hr = lpsfDesktop->BindToObject(ciidl, 0, IID_IShellFolder, (LPVOID*)&lpsf);
		if (FAILED(hr)) {
			SAFE_RELEASE(lpsfDesktop);
			return FALSE;
		}
	}
	else {
		lpsf = lpsfDesktop;
		lpsf->AddRef();
	}
	SAFE_RELEASE(lpsfDesktop);

	ciidl.Empty();
	ciidl = GetAt(nCount - 1);

	return GetDispName(lpsf, ciidl, dwFlags, pszBuf);
}

void WINAPI CItemIDList::GetPath(LPTSTR pszPath) const {
	if (!pszPath) { return; }

	SHGetPathFromIDList(m_pidl, pszPath);
}

int WINAPI CItemIDList::GetIconIndex(UINT uFlags) const {
	return GetIconIndex(m_pidl, uFlags);
}

void WINAPI CItemIDList::Attach(LPITEMIDLIST pidl) {
	Empty();
	m_pidl = pidl;
}

LPITEMIDLIST WINAPI CItemIDList::Detach(void) {
	LPITEMIDLIST pidl = m_pidl;

	m_pidl = NULL;

	return pidl;
}

void WINAPI CItemIDList::GetToolTipInfo(LPTSTR pszToolTip, UINT cbSize) const {
	if (!pszToolTip || cbSize == 0)
		return;
	LPSHELLFOLDER psfDesktop = NULL, psf = NULL;
	if (FAILED(SHGetDesktopFolder(&psfDesktop))) { return; }
	UINT count = GetCount();
	CItemIDList item = Duplicate(count - 1);

	HRESULT hr = psfDesktop->BindToObject(item, 0, IID_IShellFolder, (LPVOID*)&psf);
	SAFE_RELEASE(psfDesktop);
	if (FAILED(hr)) { return; }

	item = GetAt(count - 1);
	GetFileToolTip(psf, item, pszToolTip, cbSize);
	SAFE_RELEASE(psf);
}

BOOL WINAPI CItemIDList::GetFileToolTip(LPSHELLFOLDER lpsf, LPITEMIDLIST pidl,
	LPSTR pszToolTip, UINT cbSize) {
	if (!lpsf || !pszToolTip || cbSize == 0) { return FALSE; }
	*pszToolTip = 0;
	LPWSTR pwzTooltip = NULL;
	IQueryInfo* pInfo = NULL;

	lpsf->GetUIObjectOf(NULL, 1, (LPCITEMIDLIST*)&(pidl), IID_IQueryInfo, 0,
		(LPVOID*)&pInfo);
	if (!pInfo) { return FALSE; }

	pInfo->GetInfoTip(0, &pwzTooltip);
	SAFE_RELEASE(pInfo);

	WideCharToMultiByte(CP_ACP, 0, pwzTooltip, -1, pszToolTip, cbSize, NULL, NULL);

	return TRUE;
}

BOOL WINAPI CItemIDList::GetFileToolTip(LPSHELLFOLDER lpsf, LPITEMIDLIST pidl,
	LPWSTR pszToolTip, UINT cbSize) {
	if (!lpsf || !pszToolTip || cbSize == 0) { return FALSE; }
	*pszToolTip = 0;
	size_t i;
	char   path[MAX_PATH];
	LPWSTR pwzTooltip = NULL;
	IQueryInfo* pInfo = NULL;

	lpsf->GetUIObjectOf(NULL, 1, (LPCITEMIDLIST*)&(pidl), IID_IQueryInfo, 0,
		(LPVOID*)&pInfo);
	if (!pInfo) { return FALSE; }

	pInfo->GetInfoTip(0, &pwzTooltip);
	SAFE_RELEASE(pInfo);

	wcstombs_s(&i, path, pszToolTip, MAX_PATH);
	WideCharToMultiByte(CP_ACP, 0, pwzTooltip, -1, path, cbSize, NULL, NULL);

	return TRUE;
}

LPITEMIDLIST WINAPI CItemIDList::GetFullyQualPidl(LPSHELLFOLDER lpsf, LPITEMIDLIST lpi) {
	TCHAR szBuff[MAX_PATH + 1];
	OLECHAR szOleChar[(MAX_PATH + 1) * 2];
	LPSHELLFOLDER lpsfDeskTop;
	LPITEMIDLIST lpifq;
	ULONG ulAttribs = 0;
	HRESULT hr;

	if (!GetDispName(lpsf, lpi, SHGDN_FORPARSING, szBuff)) { return NULL; }

	hr = SHGetDesktopFolder(&lpsfDeskTop);
	if (FAILED(hr)) { return NULL; }

	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (LPCCH)szBuff, -1, (LPWSTR)szOleChar,
		sizeof(szOleChar) / 2);

	hr = lpsfDeskTop->ParseDisplayName(NULL, NULL, szOleChar, NULL, &lpifq, &ulAttribs);
	SAFE_RELEASE(lpsfDeskTop);

	if (FAILED(hr)) { return NULL; }

	return lpifq;
}

HRESULT WINAPI CItemIDList::GetUIObjectOf(REFIID riid, LPVOID* ppOut, LPITEMIDLIST pidlf,
	HWND hWnd) {
	LPSHELLFOLDER lpsf = NULL;
	HRESULT hr = S_OK;
	CItemIDList ciidf(pidlf), ciid;
	ciidf.Split(lpsf, ciid);
	if (!lpsf) { return E_FAIL; }

	hr = lpsf->GetUIObjectOf(hWnd, 1, ciid, riid, NULL, ppOut);
	SAFE_RELEASE(lpsf);

	return hr;
}

CItemIDList WINAPI CItemIDList::GetLastPidl(void) const {
	UINT nCount = GetCount();
	CItemIDList ciid;
	if (nCount > 1) { ciid = GetAt(nCount - 1); }

	return ciid;
}

void WINAPI CItemIDList::Split(LPSHELLFOLDER& lpsf, LPITEMIDLIST& pidl,
	LPITEMIDLIST pidlf) {
	pidl = NULL;
	lpsf = NULL;
	if (!pidlf) {
		if (FAILED(SHGetDesktopFolder(&lpsf))) { return; }
		return;
	}
	LPSHELLFOLDER lpsfDesktop = NULL;
	if (FAILED(SHGetDesktopFolder(&lpsfDesktop))) { return; }
	UINT nCount = GetPidlCount(pidlf);
	LPITEMIDLIST pidlFolder = CreatePidl(pidlf, nCount - 1);

	if (pidlFolder) {
		lpsfDesktop->BindToObject(pidlFolder, NULL, IID_IShellFolder, (LPVOID*)&lpsf);
		pidl = CreateOnePidl(pidlf, nCount - 1);
	}
	else {
		lpsf = lpsfDesktop;
		lpsf->AddRef();
		pidl = CopyItemIDList(pidlf);
	}
	SAFE_RELEASE(lpsfDesktop);
	FreePidl(pidlFolder);
}

LPITEMIDLIST WINAPI CItemIDList::CreatePidl(LPITEMIDLIST pidlOrg, UINT nCount) {
	if (nCount < 1) { return NULL; }

	LPITEMIDLIST pidlRes = NULL;
	LPMALLOC lpMalloc = NULL;
	size_t nSize = GetPidlSize(pidlOrg, nCount);
	HRESULT hr;
	UINT nPidlCount = GetPidlCount(pidlOrg);
	if (nPidlCount < nCount) { nCount = nPidlCount; }
	hr = SHGetMalloc(&lpMalloc);
	if (FAILED(hr)) { return NULL; }
	pidlRes = (LPITEMIDLIST)lpMalloc->Alloc(sizeof(pidlOrg->mkid.cb) + nSize);
	ZeroMemory(pidlRes, nSize + sizeof(pidlOrg->mkid.cb));

	memcpy((LPVOID)pidlRes, (LPVOID)pidlOrg, nSize);

	SAFE_RELEASE(lpMalloc);
	return pidlRes;
}

short WINAPI CItemIDList::ComparePIDL(LPITEMIDLIST pidlf1, LPITEMIDLIST pidlf2,
	LPSHELLFOLDER psfFolder, LPARAM lParam) {
	LPSHELLFOLDER psfDesktop = psfFolder;
	if (!psfDesktop) {
		if (FAILED(SHGetDesktopFolder(&psfDesktop))) { return 0; }
	}
	else {
		psfDesktop->AddRef();
	}
	if (!psfDesktop)return 0;

	HRESULT hr = psfDesktop->CompareIDs(lParam, pidlf1, pidlf2);

	SAFE_RELEASE(psfDesktop);
	return short(HRESULT_CODE(hr));
}

HRESULT WINAPI CItemIDList::PathToItemIDList(LPITEMIDLIST& pidl, LPCTSTR pcszPath, LPSHELLFOLDER psfFolder) {
	if (!psfFolder)return E_FAIL;
	OLECHAR olePath[MAX_PATH + 1];
	HRESULT hr;

	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (LPCCH)pcszPath, -1, olePath, MAX_PATH);
	hr = psfFolder->ParseDisplayName(NULL, NULL, olePath, NULL, &pidl, NULL);

	return hr;
}

HRESULT WINAPI CItemIDList::PathToItemIDList(LPITEMIDLIST& pidlf, LPCTSTR pcszPath) {
	LPSHELLFOLDER psfDesktop = NULL;
	HRESULT hr;
	OLECHAR olePath[MAX_PATH + 1];

	if (FAILED(SHGetDesktopFolder(&psfDesktop)))
		return E_FAIL;

	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (LPCCH)pcszPath, -1, olePath, MAX_PATH);
	hr = psfDesktop->ParseDisplayName(NULL, NULL, olePath, NULL, &pidlf, NULL);

	SAFE_RELEASE(psfDesktop);
	return hr;
}

int WINAPI CItemIDList::GetIconIndex(LPITEMIDLIST pidlf, UINT uFlags) {
	SHFILEINFO sfi{ nullptr };
	SHGetFileInfo((LPCTSTR)pidlf, 0, &sfi, sizeof(SHFILEINFO), uFlags);

	return sfi.iIcon;
}

int WINAPI CItemIDList::GetOverlayIconIndex(LPSHELLFOLDER lpsfFolder, LPITEMIDLIST pidl) {
	if (!lpsfFolder || !pidl) { return -1; }
	LPSHELLICON lpsiIcon = NULL;
	IShellIconOverlay* lpsiIconOl = NULL;
	int index = -1;

	lpsfFolder->QueryInterface(IID_IShellIcon, (LPVOID*)&lpsiIcon);
	if (!lpsiIcon) { return -1; }
	lpsiIcon->QueryInterface(IID_IShellIconOverlay, (LPVOID*)&lpsiIconOl);
	if (!lpsiIconOl) {
		SAFE_RELEASE(lpsiIcon);
		return -1;
	}
	if (lpsiIconOl->GetOverlayIndex(pidl, &index) != S_OK)
		index = -1;
	SAFE_RELEASE(lpsiIcon);
	SAFE_RELEASE(lpsiIconOl);

	return index;
}

BOOL WINAPI CItemIDList::GetDispName(LPSHELLFOLDER lpsf, LPITEMIDLIST lpi, DWORD dwFlags,
	LPTSTR lpFriendlyName) {
	BOOL bSuccess = TRUE;
	STRRET str;

	if (NOERROR == lpsf->GetDisplayNameOf(lpi, dwFlags, &str)) {
		if (StrRetToBuf(&str, lpi, lpFriendlyName, MAX_PATH) == S_OK) {
			bSuccess = TRUE;
		}
	}
	else {
		bSuccess = FALSE;
	}
	return bSuccess;
}
