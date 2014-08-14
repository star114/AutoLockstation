// ProtectMySelf.cpp : 응용 프로그램에 대한 진입점을 정의합니다.
//

#include "stdafx.h"
#include "ProtectMySelf.h"

#define IDC_TIMER 999

#define DEFAULT_INTERVAL 50*60*1000

#define TRAYICONID	1//				ID number for the Notify Icon
#define PMS_TRAYMSG	WM_APP//		the message ID sent to our window

#define PMS_SHOW	WM_APP + 1//	show the window
#define PMS_HIDE	WM_APP + 2//	hide the window
#define PMS_EXIT	WM_APP + 3//	close the window

#define PMS_DISABLE WM_APP + 10 // disable autolock
#define PMS_ENABLE	WM_APP + 11 // enable autolock

// Global Variables:
HINSTANCE g_hInst;								
BOOL g_fBlocked;
BOOL g_fDisabled;
UINT g_nInterval;
NOTIFYICONDATA	g_niData;	// notify icon data

// Function Declaration
BOOL				InitInstance(HINSTANCE, int);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

BOOL				OnInitDialog(HWND hWnd);
INT_PTR CALLBACK	DlgProc(HWND, UINT, WPARAM, LPARAM);

void				ShowContextMenu(HWND hWnd);
ULONGLONG			GetDllVersion(LPCTSTR lpszDllName);
void				SetTimer(HWND, UINT);
void				KillTimer(HWND);
void				Disable(HWND hWnd);
void				Enable(HWND hWnd);

int APIENTRY _tWinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPTSTR    lpCmdLine,
	int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	MSG msg;
	HACCEL hAccelTable;
	// Initailze Application
	if (!InitInstance (hInstance, nCmdShow)) 
		return FALSE;

	hAccelTable = ::LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_PROTECTMYSELF));

	// Massage Loop
	while (::GetMessage(&msg, NULL, 0, 0))
	{
		if (!::TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}


//
//   Function: InitInstance(HINSTANCE, int)
//
//   Purpose: Save the instance handle, and Create a main window.
//
//   설명:
//
//        이 함수를 통해 인스턴스 핸들을 전역 변수에 저장하고
//        주 프로그램 창을 만든 다음 표시합니다.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	// prepare for XP style controls
	//InitCommonControls();

	HWND hWnd;

	// CreateDialog
	g_hInst = hInstance;
	hWnd = ::CreateDialogW( hInstance, MAKEINTRESOURCE(IDD_MAIN),
		NULL, (DLGPROC)DlgProc );
	if (!hWnd)
	{
		return FALSE;
	}

	// Fill the NOTIFYICONDATA structure and call Shell_NotifyIcon

	// zero the structure - note:	Some Windows funtions require this but
	//								I can't be bothered which ones do and
	//								which ones don't.
	ZeroMemory(&g_niData,sizeof(NOTIFYICONDATA));

	// get Shell32 version number and set the size of the structure
	//		note:	the MSDN documentation about this is a little
	//				dubious and I'm not at all sure if the method
	//				bellow is correct
	ULONGLONG ullVersion = GetDllVersion(_T("Shell32.dll"));
	if(ullVersion >= MAKEDLLVERULL(5, 0,0,0))
		g_niData.cbSize = sizeof(NOTIFYICONDATA);
	else g_niData.cbSize = NOTIFYICONDATA_V2_SIZE;

	// the ID number can be anything you choose
	g_niData.uID = TRAYICONID;

	// state which structure members are valid
	g_niData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;

	// load the icon
	g_niData.hIcon = (HICON)::LoadImage(hInstance,MAKEINTRESOURCE(IDI_PROTECTMYSELF),
		IMAGE_ICON, GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),
		LR_DEFAULTCOLOR);

	// the window to send messages to and the message to send
	//		note:	the message value should be in the
	//				range of WM_APP through 0xBFFF
	g_niData.hWnd = hWnd;
	g_niData.uCallbackMessage = PMS_TRAYMSG;

	// tooltip message
	lstrcpyn(g_niData.szTip, _T("Protect Yourself From Workstation."), sizeof(g_niData.szTip)/sizeof(TCHAR));

	::Shell_NotifyIcon(NIM_ADD,&g_niData);

	// free icon handle
	if(g_niData.hIcon && ::DestroyIcon(g_niData.hIcon))
		g_niData.hIcon = NULL;

	// Default - Encable.
	g_fDisabled = FALSE;
	
	// Register SessionNotification for catching Lock / UnLock time.
	::WTSRegisterSessionNotification(hWnd, NOTIFY_FOR_ALL_SESSIONS);

	g_nInterval = DEFAULT_INTERVAL;
	Enable(hWnd);

	// call ShowWindow here to make the dialog initially visible
	// Default = Hide;

	return TRUE;
}

BOOL OnInitDialog(HWND hWnd)
{

	HICON hIcon = (HICON)::LoadImage(g_hInst,
		MAKEINTRESOURCE(IDI_PROTECTMYSELF),
		IMAGE_ICON, 0,0, LR_SHARED|LR_DEFAULTSIZE);
	::SendMessage(hWnd,WM_SETICON,ICON_BIG,(LPARAM)hIcon);
	::SendMessage(hWnd,WM_SETICON,ICON_SMALL,(LPARAM)hIcon);


	return TRUE;
}


// Message handler for the app
INT_PTR CALLBACK DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;

	switch (message) 
	{
	case WM_TIMER:
		if(IDC_TIMER == wParam && FALSE == g_fBlocked)
		{
			::BlockInput(TRUE);
			::LockWorkStation();
		}
		break;
	case WM_WTSSESSION_CHANGE:
		if(WTS_SESSION_LOCK == wParam)
		{
			Disable(hWnd);
		}
		else if(WTS_SESSION_UNLOCK == wParam)
		{
			Enable(hWnd);
		}
		break;
	case PMS_TRAYMSG:
		switch(lParam)
		{
		case WM_LBUTTONDBLCLK:
			::ShowWindow(hWnd, SW_RESTORE);
			break;
		case WM_RBUTTONDOWN:
		case WM_CONTEXTMENU:
			::ShowContextMenu(hWnd);
		}
		break;
	case WM_SYSCOMMAND:
		if((wParam & 0xFFF0) == SC_MINIMIZE)
		{
			::ShowWindow(hWnd, SW_HIDE);
			return 1;
		}
		else if(wParam == IDM_ABOUT)
			::DialogBox(g_hInst, (LPCTSTR)IDD_ABOUTBOX, hWnd, (DLGPROC)About);
		break;
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam); 

		switch (wmId)
		{
		case PMS_SHOW:
			::ShowWindow(hWnd, SW_RESTORE);
			break;
		case PMS_HIDE:
			::ShowWindow(hWnd, SW_HIDE);
			break;
		case IDOK:
			if(g_fDisabled)
			{
				Enable(hWnd);
			}
			else
			{
				Disable(hWnd);
			}

			g_fDisabled = !g_fDisabled;
			break;
		case PMS_EXIT:
			::DestroyWindow(hWnd);
			break;
		case IDM_ABOUT:
			::DialogBox(g_hInst, (LPCTSTR)IDD_ABOUTBOX, hWnd, (DLGPROC)About);
			break;
		}
		return 1;
	case WM_INITDIALOG:
		return OnInitDialog(hWnd);
	case WM_CLOSE:
		::ShowWindow(hWnd, SW_HIDE);
		break;
	case WM_DESTROY:
		Disable(hWnd);
		g_niData.uFlags = 0;
		::Shell_NotifyIcon(NIM_DELETE,&g_niData);
		::PostQuitMessage(0);
		break;
	}
	return 0;
}

// About Dlg
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			::EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

// Name says it all
void ShowContextMenu(HWND hWnd)
{
	POINT pt;
	::GetCursorPos(&pt);
	HMENU hMenu = ::CreatePopupMenu();
	if(hMenu)
	{
		if( ::IsWindowVisible(hWnd) )
			::InsertMenu(hMenu, -1, MF_BYPOSITION, PMS_HIDE, _T("Hide"));
		else
			::InsertMenu(hMenu, -1, MF_BYPOSITION, PMS_SHOW, _T("Show"));
		::InsertMenu(hMenu, -1, MF_BYPOSITION, PMS_EXIT, _T("Exit"));

		// note:	must set window to the foreground or the
		//			menu won't disappear when it should
		::SetForegroundWindow(hWnd);

		::TrackPopupMenu(hMenu, TPM_BOTTOMALIGN,
			pt.x, pt.y, 0, hWnd, NULL );
		::DestroyMenu(hMenu);
	}
}

// Get dll version number
ULONGLONG GetDllVersion(LPCTSTR lpszDllName)
{
	ULONGLONG ullVersion = 0;
	HINSTANCE hinstDll;
	hinstDll = ::LoadLibrary(lpszDllName);
	if(hinstDll)
	{
		DLLGETVERSIONPROC pDllGetVersion;
		pDllGetVersion = (DLLGETVERSIONPROC)::GetProcAddress(hinstDll, "DllGetVersion");
		if(pDllGetVersion)
		{
			DLLVERSIONINFO dvi;
			HRESULT hr;
			ZeroMemory(&dvi, sizeof(dvi));
			dvi.cbSize = sizeof(dvi);
			hr = (*pDllGetVersion)(&dvi);
			if(SUCCEEDED(hr))
				ullVersion = MAKEDLLVERULL(dvi.dwMajorVersion, dvi.dwMinorVersion,0,0);
		}
		::FreeLibrary(hinstDll);
	}
	return ullVersion;
}


void SetTimer(HWND hWnd, UINT nInterval)
{
	// SetTimer : Interval Time - Default 50 minutes;
	g_fBlocked = FALSE;
	::SetTimer(hWnd, IDC_TIMER,  nInterval, 0);
}

void KillTimer(HWND hWnd)
{
	g_fBlocked = TRUE;
	::KillTimer(hWnd, IDC_TIMER);
}

void Disable(HWND hWnd)
{
	if(NULL != ::GetDlgItem(hWnd, IDOK))
	{
		::SetDlgItemTextW(hWnd, IDOK, L"ENABLE");
	}
	SetWindowTextW(hWnd, L"Disabled...");
	KillTimer(hWnd);
}

void Enable(HWND hWnd)
{
	if(NULL != ::GetDlgItem(hWnd, IDOK))
	{
		::SetDlgItemTextW(hWnd, IDOK, L"DISABLE");

		BOOL bChk = ::IsDlgButtonChecked(hWnd, IDC_INTERVAL);
		if(bChk)
		{
			int nInterval = 1000 * 60 * ::GetDlgItemInt(hWnd, IDC_MINUTES,NULL, FALSE); 
			if(nInterval != g_nInterval)
			{
				::MessageBoxW(hWnd, L"You change the interval time.", L"Alert", 0);
			}
			g_nInterval = nInterval;
		}
		else
			g_nInterval = DEFAULT_INTERVAL;
	}
	SetWindowTextW(hWnd, L"Now Running...");
	SetTimer(hWnd, g_nInterval);
}