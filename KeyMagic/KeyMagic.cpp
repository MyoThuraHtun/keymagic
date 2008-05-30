//Like a operator of Keymagic.
//Copyright (C) 2008  KeyMagic Project
//http://keymagic.googlecode.com
//
//This program is free software; you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

#include "stdafx.h"
#include "KeyMagic.h"
#include "shellapi.h"

#include "../KeyMagicDll/KeyMagicDll.h"

#define MAX_LOADSTRING 100

//Custom message IDs
#define TRAY_ID 100
#define WM_TRAY (WM_USER + 1)
#define IDKM_NORMAL (WM_USER + 2)
#define IDKM_ID (WM_USER + 3)

UINT KM_SETKBID;
UINT KM_KILLFOCUS;
UINT KM_GETFOCUS;
HWND LastHWND;

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name
HHOOK hKH;
HHOOK hWPH;
HHOOK hGM;
UINT KeyBoardNum;
HMENU hKeyMenu;
// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
VOID				SetHook (HWND hwnd);
VOID				UnHook ();
void				GetKeyBoards();
void				CreateMyMenu(HWND hWnd);
void				DrawMyMenu(LPDRAWITEMSTRUCT lpdis);
void				OnMenuMeasure(HWND hwnd, LPMEASUREITEMSTRUCT lpmis);

// Structure associated with menu items 
 
typedef struct tagMYITEM 
{ 
    int   cchItemText; 
    char  szItemText[1]; 
} MYITEM; 
 
#define CCH_MAXITEMTEXT 256 

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	
	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_KEYMAGIC, szWindowClass, MAX_LOADSTRING);
	HWND wnd = FindWindow(szWindowClass, szTitle);
	if (wnd) {
		/*ShowWindow(wnd, SW_SHOW);
		ShowWindow(wnd, SW_RESTORE);*/
		return FALSE;
	}

	MyRegisterClass(hInstance);

	GetKeyBoards();

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_KEYMAGIC));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return (int) msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_KEYMAGIC));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_KEYMAGIC);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_KEYMAGIC));

	return RegisterClassEx(&wcex);
}

void GetKeyBoards(){

	char szCurDir[MAX_PATH];
	char szMenuDisplay[500];
	char szKBNames[500];
	char szKBP[]="KeyBoardPaths";
	char szMS[]="MenuDisplays";

	hKeyMenu = CreatePopupMenu();
	if (!hKeyMenu)
		return;

	GetCurrentDirectory(MAX_PATH, (LPSTR)szCurDir);
	lstrcat(szCurDir, "\\KeyMagic.ini");
	GetPrivateProfileString(szKBP, NULL, NULL, (LPSTR)szKBNames, 500, szCurDir);

	GetPrivateProfileString(szMS, (LPCSTR)&szKBNames[0], NULL, (LPSTR)szMenuDisplay, 50, szCurDir);
	AppendMenu(hKeyMenu, NULL, IDKM_NORMAL, (LPCSTR)&szMenuDisplay);

	for (int i=lstrlen(&szKBNames[0])+1,Length = lstrlen(&szKBNames[i]);
		Length > 0; 
		i+=Length+1, Length = lstrlen(&szKBNames[i])){
			GetPrivateProfileString(szMS, (LPCSTR)&szKBNames[i], NULL, (LPSTR)szMenuDisplay, 50, szCurDir);
			AppendMenu(hKeyMenu, NULL, IDKM_ID+KeyBoardNum, (LPCSTR)&szMenuDisplay);
			KeyBoardNum++;
	}

	CheckMenuRadioItem(hKeyMenu, IDKM_NORMAL, 
	KeyBoardNum + IDKM_ID, 
	IDKM_NORMAL,
	MF_BYCOMMAND);

};

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // Store instance handle in our global variable

   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   //ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   if (hKeyMenu){
	NOTIFYICONDATA nid;
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = hWnd;
	nid.uID = TRAY_ID;
	nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
	nid.uCallbackMessage = WM_TRAY;
	nid.hIcon = (HICON)LoadIcon(hInstance, MAKEINTRESOURCE(IDI_KEYMAGIC));
	lstrcpy(nid.szTip, _T("KeyMagic"));
	Shell_NotifyIcon(NIM_ADD,&nid);
	UpdateWindow(hWnd);
   }

   SetHook(hWnd);
   return TRUE;
}

void CreateMyMenu(HWND hWnd){

	MENUITEMINFO mii;
	MYITEM *pMyItem;
	UINT uID;

	for (uID = IDKM_NORMAL; uID < IDKM_ID+KeyBoardNum; uID++) 
	{
		 // Allocate an item structure, leaving space for a 
         // string of up to CCH_MAXITEMTEXT characters. 
 
        pMyItem = (MYITEM *) LocalAlloc(LMEM_FIXED,
			sizeof(MYITEM) + CCH_MAXITEMTEXT); 
 
        // Save the item text in the item structure. 
 
		mii.cbSize = sizeof(MENUITEMINFO);
        mii.fMask = MIIM_STRING; 
        mii.dwTypeData = pMyItem->szItemText; 
        mii.cch = CCH_MAXITEMTEXT; 
        GetMenuItemInfo(hKeyMenu, uID, FALSE, &mii); 
        pMyItem->cchItemText = mii.cch; 
 
        // Reallocate the structure to the minimum required size. 
 
        pMyItem = (MYITEM *) LocalReAlloc(pMyItem,
			sizeof(MYITEM) + mii.cch, LMEM_MOVEABLE); 
 
        // Change the item to an owner-drawn item, and save 
        // the address of the item structure as item data. 
 
        mii.fMask = MIIM_FTYPE | MIIM_DATA; 
        mii.fType = MFT_OWNERDRAW; 
        mii.dwItemData = (ULONG_PTR) pMyItem; 
        SetMenuItemInfo(hKeyMenu, uID, FALSE, &mii); 

	}

	DrawMenuBar(hWnd);

}

void DestroyMyMenu(){
	MENUITEMINFO mii;
	UINT uID; 
    MYITEM *pMyItem; 

    for (uID = IDKM_NORMAL; uID < IDKM_ID+KeyBoardNum; uID++) 
    { 
        // Get the item data. 
 
		mii.cbSize = sizeof(MENUITEMINFO);
        mii.fMask = MIIM_DATA; 
        GetMenuItemInfo(hKeyMenu, uID, FALSE, &mii); 
        pMyItem = (MYITEM *) mii.dwItemData; 
 
        // free the item structure. 
        LocalFree(pMyItem); 
    } 

}

void DrawMyMenu(LPDRAWITEMSTRUCT lpdis){

	UINT cch;
	MENUITEMINFO mii;
	MYITEM *pMyItem = (MYITEM *) lpdis->itemData;

	if (lpdis->CtlType == ODT_MENU){

		if (lpdis->itemState & ODS_SELECTED)
		{ 
			SelectObject(lpdis->hDC,CreateSolidBrush(RGB(210,240,255)));
			SetTextColor(lpdis->hDC, RGB(0,128,170));
			SelectObject(lpdis->hDC,CreatePen(PS_INSIDEFRAME, 1, RGB(153,217,234)));
		}
		else if (lpdis->itemState & ODS_CHECKED)
		{
			SelectObject(lpdis->hDC,CreateSolidBrush(RGB(240,250,255)));
			SetTextColor(lpdis->hDC, RGB(0,128,192));
			SelectObject(lpdis->hDC,CreatePen(PS_INSIDEFRAME, 1, RGB(110,205,220)));
		}
		else
		{
			SetTextColor(lpdis->hDC, RGB(10,120,245));
			SelectObject(lpdis->hDC,CreatePen(PS_INSIDEFRAME, 1, RGB(255,255,255)));
		} 
		
		SetBkMode(lpdis->hDC, TRANSPARENT);
		RoundRect(lpdis->hDC, lpdis->rcItem.left, lpdis->rcItem.top, lpdis->rcItem.right, lpdis->rcItem.bottom, 5, 5);

		char Temp[256];

		strcpy(Temp, pMyItem->szItemText);

		strtok(Temp, "\t");
		char* szShortCut = strtok(NULL, "\t");

		if (szShortCut)
			cch = szShortCut - Temp;
		else
			cch = pMyItem->cchItemText;

		lpdis->rcItem.top += 3;

		// Determine where to draw and leave space for a check mark. 

		lpdis->rcItem.left += 20;

		//ExtTextOut(lpdis->hDC, x, y, ETO_CLIPPED, 
		//		&lpdis->rcItem, pMyItem->szItemText,
		//		cch, NULL);

		DrawText(lpdis->hDC, pMyItem->szItemText, cch, &lpdis->rcItem, DT_VCENTER);

		lpdis->rcItem.right -= 8;
		DrawText(lpdis->hDC, szShortCut, pMyItem->cchItemText-cch, &lpdis->rcItem,DT_VCENTER | DT_RIGHT);

	}
}

void OnMenuMeasure(HWND hwnd,LPMEASUREITEMSTRUCT lpmis)
{

    MYITEM *pMyItem = (MYITEM *) lpmis->itemData; 
	HDC hdc = GetDC(hwnd); 
    SIZE size; 
 
    GetTextExtentPoint32(hdc, pMyItem->szItemText, 
            pMyItem->cchItemText, &size); 
 
    lpmis->itemWidth = size.cx; 
    lpmis->itemHeight = size.cy+5;

    ReleaseDC(hwnd, hdc);

}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HMENU hMenu, hPopUp;
	POINT pt;

	int wmId, wmEvent;

	if (message == KM_KILLFOCUS){
		LastHWND = (HWND)lParam;
		return 0;
	}

	else if (message == KM_GETFOCUS){
		if (wParam != 0){
			CheckMenuRadioItem(hKeyMenu, IDKM_NORMAL, 
				KeyBoardNum + IDKM_ID , 
				wParam + IDKM_NORMAL, 
				MF_BYCOMMAND);
		}
		else{
			CheckMenuRadioItem(hKeyMenu, IDKM_NORMAL, 
				KeyBoardNum + IDKM_ID, 
				IDKM_NORMAL,
				MF_BYCOMMAND);
		}
		return false;
	}

	switch (message)
	{
	case WM_CREATE:

		CreateMyMenu(hWnd);

		break;

	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:

			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;

		case IDM_EXIT:

			DestroyWindow(hWnd);
			break;

		case IDKM_NORMAL:

			SendMessage(LastHWND, KM_SETKBID, -1, 0);
			CheckMenuRadioItem(hKeyMenu, IDKM_NORMAL, 
			KeyBoardNum + IDKM_ID, 
			IDKM_NORMAL, 
			MF_BYCOMMAND);

			break;

		default:
			if (wmId >= IDKM_ID && wmId <= IDKM_ID + KeyBoardNum){

				SendMessage(LastHWND, KM_SETKBID, wmId-IDKM_NORMAL, 1);

				CheckMenuRadioItem(hKeyMenu, IDKM_NORMAL,
				KeyBoardNum + IDKM_ID, 
				wmId, 
				MF_BYCOMMAND);

			}

			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;

	case WM_TRAY:

		if (lParam==WM_LBUTTONDOWN) {

			GetCursorPos(&pt);

			SetForegroundWindow(hWnd);

			TrackPopupMenu(hKeyMenu, TPM_RIGHTBUTTON | TPM_LEFTBUTTON,
				pt.x, pt.y, 0, hWnd, NULL);

		}

		else if (lParam==WM_RBUTTONDOWN) {
			hMenu = LoadMenu(hInst, MAKEINTRESOURCE(IDC_KEYMAGIC));
			hPopUp = GetSubMenu(hMenu, 0);

			GetCursorPos(&pt);

			SetForegroundWindow(hWnd);

			TrackPopupMenu(hPopUp, TPM_RIGHTBUTTON | TPM_LEFTBUTTON,
				pt.x, pt.y, 0, hWnd, NULL);
		}
		break;

	case WM_DRAWITEM:

		DrawMyMenu((LPDRAWITEMSTRUCT)lParam);

		break;

	case WM_MEASUREITEM:

		OnMenuMeasure(hWnd,(LPMEASUREITEMSTRUCT)lParam);

		break;

	case WM_CLOSE:

		ShowWindow(hWnd, SW_HIDE);
		
		break;

	case WM_DESTROY:

		UnHook();

		NOTIFYICONDATA nid;
		nid.cbSize = sizeof(NOTIFYICONDATA);
		nid.hWnd = hWnd;
		nid.uID = TRAY_ID;
		Shell_NotifyIcon(NIM_DELETE, &nid);

		DestroyMyMenu();

		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

VOID SetHook (HWND hwnd)
{
	char szCurDir[MAX_PATH];

	HMODULE hMod = LoadLibrary("KeyMagicDll.dll");
	if (hMod == NULL)   {
			MessageBox(hwnd, "Log", "Error: cannot start dll, KeyMagicDll.dll not found.", 0);
			return ;
	}

	hKH = SetWindowsHookEx(WH_KEYBOARD, &HookKeyProc, hMod, NULL);

	hWPH = SetWindowsHookEx(WH_CALLWNDPROC, &HookWndProc, hMod, NULL);

	hGM = SetWindowsHookEx(WH_GETMESSAGE, &HookGetMsgProc, hMod, NULL);

	KM_SETKBID = RegisterWindowMessage("KM_SETKBID");

	KM_KILLFOCUS = RegisterWindowMessage("KM_KILLFOCUS");

	KM_GETFOCUS = RegisterWindowMessage("KM_GETFOCUS");
	
	GetCurrentDirectory(MAX_PATH, (LPSTR)szCurDir);

	HookInit(hwnd,hKH, hWPH, hGM, KM_SETKBID, KM_KILLFOCUS, KM_GETFOCUS, szCurDir);
}

VOID UnHook ()
{
	UnhookWindowsHookEx(hKH);
	UnhookWindowsHookEx(hWPH);
	UnhookWindowsHookEx(hGM);
}