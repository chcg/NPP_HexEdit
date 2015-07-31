//this file is part of Hex Edit Plugin for Notepad++
//Copyright (C)2006 Jens Lorenz <jens.plugin.npp@gmx.de>
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#ifndef HEXDLG_DEFINE_H
#define HEXDLG_DEFINE_H

#include "StaticDialog.h"
#include <string>
#include <vector>
#include <algorithm>
#include <shlwapi.h>
#include <zmouse.h>
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>

#include "PluginInterface.h"
#include "tables.h"

using namespace std;

#include "HEXResource.h"

#define	VIEW_ROW (_pCurProp->columns * _pCurProp->bits)


extern tClipboard	g_clipboard;

/* create depenency view for handle/hexEdit pointer */
typedef struct _tActiveView {
	HWND			hWnd;
	class HexEdit*	hexEdit;
} tActiveView;

static tActiveView	gActiveHandle[2] = {0};

class HexEdit : public StaticDialog //Window
{
public:
	HexEdit(void);
	~HexEdit(void);
    void init(HINSTANCE hInst, NppData nppData, LPCTSTR iniFilePath);

	void destroy(void)
	{
		if (_hDefaultParentProc != NULL)
		{
			/* restore subclasses */
			::SetWindowLong(_hParentHandle, GWL_WNDPROC, (LONG)_hDefaultParentProc);
		}

		if (_hFont)
		{
			/* destroy font */
			::DeleteObject(_hFont);
		}
	};

   	void doDialog(BOOL toggle = FALSE);

	void UpdateDocs(const char** pFiles, UINT numFiles, INT openDoc);

	void FileNameChanged(char* newPath)
	{
		if (_openDoc == -1)
			return;

		strcpy(_hexProp[_openDoc].pszFileName, newPath);
	};

	void SetParentNppHandle(HWND hWnd, UINT cont)
	{
		if (_hDefaultParentProc != NULL)
		{
			/* restore subclasses */
			::SetWindowLong(_hParentHandle, GWL_WNDPROC, (LONG)_hDefaultParentProc);
		}

		/* store given parent handle */
		_hParentHandle = hWnd;

		/* create avtive view list */
		gActiveHandle[cont].hWnd = hWnd;
		gActiveHandle[cont].hexEdit = this;

		/* intial subclassing */
		_hDefaultParentProc = (WNDPROC)(::SetWindowLong(_hParentHandle, GWL_WNDPROC, reinterpret_cast<LONG>(wndParentProc)));
	};

	void SetHexProp(tHexProp prop)
	{
		if (_pCurProp != NULL)
		{
			*_pCurProp = prop;
		}
	};

	tHexProp GetHexProp(void)
	{
		if (_pCurProp != NULL)
		{
			GetLineVis();
			return *_pCurProp;
		}
		else
		{
			tHexProp	prop = {0};
			return prop;
		}
	};


	/* functions for subclassing interface */
	void Cut(void);
	void Copy(void);
	void Paste(void);
	void ZoomIn(void);
	void ZoomOut(void);
	void ZoomRestore(void);
	void RedoUndo(UINT position)
	{
		if (_pCurProp == NULL)
			return;

		_posRedoUndo = position;
		::SetTimer(_hSelf, IDC_HEX_REDOUNDO, 50, NULL);
	};

	void TestLineLength()
	{
		if (_pCurProp == NULL)
			return;

		/* correct length of file */
		_currLength = ScintillaMsg(SCI_GETLENGTH);

		/* correct item count */
		if ((_currLength == GetCurrentPos()) && (_pCurProp->anchorPos == 0))
			ListView_SetItemCountEx(_hListCtrl, (_currLength/VIEW_ROW) + 1, LVSICF_NOSCROLL);
		else
			ListView_SetItemCountEx(_hListCtrl, (_currLength/VIEW_ROW) + (_currLength%VIEW_ROW?1:0), LVSICF_NOSCROLL);
	};

	void SetCurrentLine(UINT item)
	{
		if (_pCurProp == NULL)
			return;

		UINT itemMax = ListView_GetItemCount(_hListCtrl);

		if (itemMax < item)
		{
			item = itemMax-1;
		}
		if (0 >= item)
		{
			item = 1;
		}

		if (_pCurProp->editType == HEX_EDIT_HEX)
		{
			SelectItem(item, 1);
		}
		else
		{
			SelectDump(item, 0);
		}
	};

	UINT GetCurrentLine(void)
	{
		if (_pCurProp == NULL)
			return 0;

		return _pCurProp->cursorItem;
	};

	UINT GetItemCount(void)
	{
		if (_pCurProp == NULL)
			return 0;

		return ListView_GetItemCount(_hListCtrl);
	};

	void ResetModificationState(void)
	{
		if (_pCurProp == NULL)
			return;

		_pCurProp->isModified = FALSE;
	};

	BOOL GetModificationState(void)
	{
		if (_pCurProp == NULL)
			return FALSE;

		return _pCurProp->isModified;
	};


protected :
	virtual BOOL CALLBACK run_dlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

private:
	void UpdateHeader(void);
	void ReadArrayToList(LPTSTR text,INT iItem, INT iSubItem);
	void AddressConvert(LPTSTR text, INT length);
	void DumpConvert(LPTSTR text, UINT length);
	void BinHexConvert(LPTSTR text, INT length);
	void MoveView(void);
	void SetLineVis(UINT pos, eLineVis mode);
	void GetLineVis(void);

	void TrackMenu(POINT pt);
	void OnDeleteBlock(void);



	/* for edit in hex */
	void OnMouseClickItem(WPARAM wParam, LPARAM lParam);
	BOOL OnKeyDownItem(WPARAM wParam, LPARAM lParam);
	BOOL OnCharItem(WPARAM wParam, LPARAM lParam);
	void SelectItem(UINT iItem, UINT iSubItem, INT iCursor = 0);
	void DrawItemText(HDC hDc, DWORD item, INT subItem);



	/* for edit in dump */
	void OnMouseClickDump(WPARAM wParam, LPARAM lParam);
	BOOL OnKeyDownDump(WPARAM wParam, LPARAM lParam);
	void OnCharDump(WPARAM wParam, LPARAM lParam);
	void SelectDump(INT iItem, INT iCursor);
	void DrawDumpText(HDC hDc, DWORD item, INT subItem);

	INT  CalcCursorPos(LV_HITTESTINFO info);
	void GlobalKeys(WPARAM wParam, LPARAM lParam);
	void SelectionKeys(WPARAM wParam, LPARAM lParam);
	void SetPosition(UINT pos, BOOL isLittle = FALSE);
	UINT GetCurrentPos(void);
	UINT GetAnchor(void);
	void SetSelection(UINT posBegin, UINT posEnd, eSel selection = HEX_SEL_NORM, BOOL isEND = FALSE);
	void GetSelection(LPINT posBegin, LPINT posEnd);

	void SetStatusBar(void);
	void GrayEncoding(void);


	INT CalcStride(INT posBeg, INT posEnd)
	{
		if (posEnd > posBeg)
			return posEnd - posBeg;
		else
			return posBeg - posEnd;
	};

	/* Subclassing parent */
	LRESULT runProcParent(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK wndParentProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
		if (gActiveHandle[SC_MAINHANDLE].hWnd == hwnd)
			return (gActiveHandle[SC_MAINHANDLE].hexEdit->runProcParent(hwnd, Message, wParam, lParam));
		else
			return (gActiveHandle[	].hexEdit->runProcParent(hwnd, Message, wParam, lParam));
	};

	/* Subclassing list */
	LRESULT runProcList(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK wndListProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
		return (((HexEdit *)(::GetWindowLong(hwnd, GWL_USERDATA)))->runProcList(hwnd, Message, wParam, lParam));
	};


	BOOL SetFontSize(UINT size)
	{
		BOOL	ret = FALSE;

		if (_hFont)
			DeleteObject(_hFont);

		_hFont = ::CreateFont(size, 0, 0, 0, 0, 0, 0, 0, ANSI_CHARSET, 0, 0, 0, 0, "Courier New");
		if (_hFont)
		{
			::SendMessage(_hListCtrl, WM_SETFONT, reinterpret_cast<WPARAM>(_hFont), 0);
			ret = TRUE;
		}

		return ret;
	}

	void runCursor(HWND hwnd, UINT Message, WPARAM wParam, unsigned long lParam)
	{
		_isCurOn ^= TRUE;
		ListView_RedrawItems(_hListCtrl, _pCurProp->cursorItem, _pCurProp->cursorItem);
	};

	static void CALLBACK cursorFunc(HWND hWnd, UINT Message, WPARAM wParam, unsigned long lParam) {
		(((HexEdit *)(::GetWindowLong(hWnd, GWL_USERDATA)))->runCursor(hWnd, Message, wParam, lParam));
	};

	void InvalidateList(void)
	{
		_pCurProp->anchorItem		= _pCurProp->cursorItem;
		_pCurProp->anchorSubItem	= _pCurProp->cursorSubItem;
		_pCurProp->anchorPos		= _pCurProp->cursorPos;
		_oldPaintItem				= 0;
		_oldPaintSubItem			= 0;
		_oldPaintCurPos				= 0;

		/* start cursor new */
		_isCurOn = TRUE;
		::SetTimer(_hListCtrl, IDC_HEX_CURSORTIMER, 500, cursorFunc);
		ListView_Update(_hListCtrl, 0);
		SetStatusBar();
	};

	void InvalidateNotepad(void)
	{
		/* updates notepad icons and menus */
		NMHDR	nm;
		memset(&nm, 0, sizeof(NMHDR));
		::SendMessage(_nppData._nppHandle, WM_NOTIFY, 0, (LPARAM)&nm);
		SetStatusBar();
	};

	void SetFocusNpp(HWND hWnd) {
		::SetFocus(hWnd);
		SetStatusBar();
	};


private:
	/********************************* handle of list ********************************/
	HWND				_hListCtrl;
	HFONT				_hFont;
	
	/* handle of parent handle (points to scintilla main view) */
	HWND				_hParentHandle;
	HHOOK				_hParentHook;

	/* Handles */
	NppData				_nppData;
	HIMAGELIST			_hImageList;

	char				_iniFilePath[MAX_PATH];

	/* subclassing handle */
	WNDPROC				_hDefaultParentProc;
	WNDPROC				_hDefaultListProc;

	/******************************* virables of list *********************************/

	/* current file */
	INT					_openDoc;
	UINT				_currLength;

	/* current font size */
	UINT				_iFontSize;

	/* properties of open files */
	tHexProp*			_pCurProp;
	vector<tHexProp>	_hexProp;

	/* for selection */
	BOOL				_isCurOn;
	UINT				_x;
	UINT				_y;

	UINT				_oldPaintItem;
	UINT				_oldPaintSubItem;
	UINT				_oldPaintCurPos;

	UINT				_posRedoUndo;

	/* mouse states */
	BOOL				_isLBtnDown;
	BOOL				_isRBtnDown;
	BOOL				_isWheel;
};




#endif // HEXDLG_DEFINE_H
