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
#include "Hex.h"
#include "SciSubClassWrp.h"
#include <string>
#include <vector>
#include <algorithm>
#include <shlwapi.h>
#include <zmouse.h>
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>

#include "tables.h"

using namespace std;

#include "HEXResource.h"

#define HEX_FIRST_TIME_VIS	(0xFFFFFFFF)

#define DT_HEX_VIEW			(DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX)
#define	VIEW_ROW			(_pCurProp->columns * _pCurProp->bits)
#define FACTOR				((_pCurProp->isBin == TRUE)?8:2)
#define	SUBITEM_LENGTH		(_pCurProp->bits * FACTOR)
#define FULL_SUBITEM		((_pCurProp->cursorItem * VIEW_ROW + (_pCurProp->cursorSubItem * _pCurProp->bits)) <= _currLength)
#define DUMP_FIELD			(_pCurProp->columns + 1)

extern tClipboard	g_clipboard;


class HexEdit : public StaticDialog, private SciSubClassWrp
{
public:
	HexEdit(void);
	~HexEdit(void);
    void init(HINSTANCE hInst, NppData nppData, LPCTSTR iniFilePath);

	void destroy(void)
	{
		/* restore subclasses */
		SciSubClassWrp::CleanUp();

		if (_hFont) {
			/* destroy font */
			::DeleteObject(_hFont);
		}
	};

   	void doDialog(BOOL toggle = FALSE);

	void UpdateDocs(LPCTSTR* pFiles, UINT numFiles, INT openDoc);

	void FileNameChanged(LPTSTR newPath)
	{
		if (_openDoc == -1)
			return;

		_tcscpy(_hexProp[_openDoc].szFileName, newPath);
	};

	void SetParentNppHandle(HWND hWnd, UINT cont)
	{
		/* restore subclasses */
		SciSubClassWrp::CleanUp();

		/* store given parent handle */
		_hParentHandle = hWnd;

		/* intial subclassing */
		if (cont == MAIN_VIEW) {
			SciSubClassWrp::Init(hWnd, wndParentProc0);
		} else {
			SciSubClassWrp::Init(hWnd, wndParentProc1);
		}
	};

	void SetHexProp(tHexProp prop)
	{
		if (_pCurProp != NULL)
		{
			*_pCurProp = prop;

			_oldAnchorItem		= prop.anchorItem;
			_oldAnchorSubItem	= prop.anchorSubItem;
			_oldAnchorCurPos	= prop.anchorPos;
			_oldCursorItem		= prop.cursorItem;
			_oldCursorSubItem	= prop.cursorSubItem;
			_oldCursorCurPos	= prop.cursorPos;

			if (_pCurProp->isVisible == TRUE) {
				UpdateHeader();
				::RedrawWindow(_hListCtrl, NULL, NULL, TRUE);
			}
		}
	};

	tHexProp GetHexProp(void)
	{
		if (_pCurProp != NULL) {
			GetLineVis();
			return *_pCurProp;
		} 
		tHexProp prop;
		return prop;
	};


	/* functions for subclassing interface */
	void Cut(void);
	void Copy(void);
	void Paste(void);
	void Delete(void);
	void SelectAll(void);
	void ZoomIn(void);
	void ZoomOut(void);
	void ZoomRestore(void);
	void ToggleBookmark(void);
	void NextBookmark(void);
	void PrevBookmark(void);
	void ClearBookmarks(void);
	void CutBookmarkLines(void);
	void CopyBookmarkLines(void);
	void PasteBookmarkLines(void);
	void DeleteBookmarkLines(void);

	void RedoUndo(UINT position, INT length, UINT code)
	{
		if (_pCurProp == NULL)
			return;
		if (_pCurProp->isVisible == FALSE)
			return;

		if (_iUnReCnt == -1) {
			_uUnReCode		= code & (SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT);
			_uFirstPos		= position;
			_uLastPos		= position;
			_uLastLength	= length;
			_iUnReCnt++;
		} else if (_uUnReCode & code) {
			if ((_pCurProp->isLittle == TRUE) && (_pCurProp->bits != HEX_BYTE)) {
				if (position != (_uLastPos + 1)) {
					_iUnReCnt++;
					_uLastLength = length;
				} else {
					_uLastLength += length;
				}
				_uLastPos = position;
			} else {
				_iUnReCnt++;
			}
		}

		UpdateBookmarks(position, (code & SC_MOD_DELETETEXT ? -1:1) * length);

		if (code & SC_LASTSTEPINUNDOREDO) {
			if (code & SC_MOD_INSERTTEXT) {
				if (_iUnReCnt > 1) {
					/* changed a block */
					if ((_pCurProp->isLittle == TRUE) && (_pCurProp->bits != HEX_BYTE)) {
						UINT offset = position % _pCurProp->bits;
						position = (position - offset) + (_pCurProp->bits - offset - 1);

						offset = _uFirstPos % _pCurProp->bits;
						_uFirstPos = (_uFirstPos - offset) + (_pCurProp->bits - offset - 1);
					}

					if (position < _uFirstPos) {
						SetSelection(position, (position + (VIEW_ROW * _iUnReCnt)) + _uLastLength, HEX_SEL_BLOCK);
					} else {
						SetSelection(_uFirstPos, (_uFirstPos + (VIEW_ROW * _iUnReCnt)) + _uLastLength, HEX_SEL_BLOCK);
					}
				} else {
					/* changed only one character */
					if ((_pCurProp->isLittle == TRUE) && (_pCurProp->bits != HEX_BYTE)) {
						UINT offset = position % _pCurProp->bits;
						position = (position - offset) + (_pCurProp->bits - offset - 1);
					}
					SetSelection(position, position + _uLastLength, HEX_SEL_NORM, (position + _uLastLength) % VIEW_ROW == 0);
				}
			} else {
				/* delete some characters */
				SetSelection(_uFirstPos, _uFirstPos);
			}
			_iUnReCnt = -1;
			_uUnReCode = 0;
		}
		EnsureVisible(_pCurProp->cursorItem, _pCurProp->cursorSubItem);
	};

	void TestLineLength()
	{
		if (_pCurProp == NULL)
			return;

		/* correct length of file */
		_currLength = (UINT)SciSubClassWrp::execute(SCI_GETLENGTH);

		/* correct item count */
		if ((_currLength == GetCurrentPos()) && (_pCurProp->anchorPos == 0)) {
			ListView_SetItemCountEx(_hListCtrl, (_currLength/VIEW_ROW ) + 1, LVSICF_NOSCROLL);
		} else {
			ListView_SetItemCountEx(_hListCtrl, (_currLength/VIEW_ROW) + (_currLength%VIEW_ROW?1:0), LVSICF_NOSCROLL);
		}
	};

	void SetCurrentLine(UINT item)
	{
		if (_pCurProp == NULL)
			return;

		UINT itemMax = ListView_GetItemCount(_hListCtrl);

		if (itemMax < item)
		{
			item = itemMax;
		}
		if (0 > item)
		{
			item = 0;
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

		return (ListView_GetItemCount(_hListCtrl) - 1);
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

	void UpdateFont(void)
	{
		_fontSize	= getFontSizeElem();
		SetFont();
	};


	void ClearAllCompareResults()
	{
		for (size_t i = 0; i < _hexProp.size(); i++) {
			if (_hexProp[i].pCmpResult != NULL) {
				if (_hexProp[i].pCmpResult->pCmpRef != NULL) {
                    _hexProp[i].pCmpResult->pCmpRef->pCmpRef = NULL;
                } else {
					::CloseHandle(_hexProp[i].pCmpResult->hFile);
					::DeleteFile(_hexProp[i].pCmpResult->szFileName);
				}
				_hexProp[i].pCmpResult = NULL;
			}
		}
	};

	void SetCompareResult(tCmpResult* pCmpResult, tCmpResult* pCmpRef = NULL)
	{
		if (pCmpResult == NULL) {
			if (_pCurProp->pCmpResult != NULL) {
                /* if a reference exist mark in them that this was deleted */
				if (_pCurProp->pCmpResult->pCmpRef != NULL) {
                    _pCurProp->pCmpResult->pCmpRef->pCmpRef = NULL;
                } else {
					::CloseHandle(_pCurProp->pCmpResult->hFile);
					::DeleteFile(_pCurProp->pCmpResult->szFileName);
				}
    			delete _pCurProp->pCmpResult;
				_pCurProp->pCmpResult = NULL;
			}
		} else {
            /* this avoids resource leak */
            if (_pCurProp->pCmpResult != NULL) {
				if (_pCurProp->pCmpResult->pCmpRef != NULL)
                    _pCurProp->pCmpResult->pCmpRef->pCmpRef = NULL;
                delete _pCurProp->pCmpResult;
            }
			_pCurProp->pCmpResult = pCmpResult;
			_pCurProp->pCmpResult->pCmpRef = pCmpRef;
		}
		::RedrawWindow(_hListCtrl, NULL, NULL, TRUE);
	}

	void SetStatusBar(void);

protected :
	BOOL CALLBACK run_dlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

private:
	void UpdateHeader(BOOL isFirstTime = FALSE);
	void ReadArrayToList(LPSTR text,INT iItem, INT iSubItem);
	void AddressConvert(LPSTR text, INT length);
	void DumpConvert(LPSTR text, UINT length);
	void BinHexConvert(LPSTR text, INT length);
	void MoveView(void);
	void SetLineVis(UINT pos, eLineVis mode);
	void GetLineVis(void);

	void TrackMenu(POINT pt);
	BOOL ShouldDeleteCompare(void);

	/* for address text */
	void DrawAddressText(HDC hDc, DWORD item);

	/* for edit in hex */
	void OnMouseClickItem(WPARAM wParam, LPARAM lParam);
	BOOL OnKeyDownItem(WPARAM wParam, LPARAM lParam);
	BOOL OnCharItem(WPARAM wParam, LPARAM lParam);
	void SelectItem(UINT iItem, UINT iSubItem, INT iCursor = 0);
	void DrawItemText(HDC hDc, DWORD item, INT subItem);
	void DrawPartOfItemText(HDC hDc, RECT rc, RECT rcText, LPTSTR text, UINT beg, UINT length, eSelItem sel, eSelType type);

	/* for edit in dump */
	void OnMouseClickDump(WPARAM wParam, LPARAM lParam);
	BOOL OnKeyDownDump(WPARAM wParam, LPARAM lParam);
	BOOL OnCharDump(WPARAM wParam, LPARAM lParam);
	void SelectDump(INT iItem, INT iCursor);
	void DrawDumpText(HDC hDc, DWORD item, INT subItem);
	void DrawPartOfDumpText(HDC hDc, RECT rc, LPTSTR text, UINT beg, UINT length, eSelType type);


	INT  CalcCursorPos(LV_HITTESTINFO info);
	void ConvertCursorPos(void);
	BOOL GlobalKeys(WPARAM wParam, LPARAM lParam);
	void SelectionKeys(WPARAM wParam, LPARAM lParam);
	void SetPosition(UINT pos, BOOL isLittle = FALSE);
	UINT GetCurrentPos(void);
	UINT GetAnchor(void);
	void SetSelection(UINT posBegin, UINT posEnd, eSel selection = HEX_SEL_NORM, BOOL isEND = FALSE);
	void GetSelection(LPINT posBegin, LPINT posEnd);

	void ToggleBookmark(UINT item);
	void UpdateBookmarks(UINT firstElem, INT length);

	void ConvertSelNppToHEX(void);
	void ConvertSelHEXToNpp(void);

	INT CalcStride(INT posBeg, INT posEnd)
	{
		if (posEnd > posBeg)
			return posEnd - posBeg;
		else
			return posBeg - posEnd;
	};

	/* Subclassing parent */
	LRESULT runProcParent(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK wndParentProc0(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
		extern HexEdit	hexEdit1;
		return (hexEdit1.runProcParent(hwnd, Message, wParam, lParam));
	};
	static LRESULT CALLBACK wndParentProc1(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
		extern HexEdit	hexEdit2;
		return (hexEdit2.runProcParent(hwnd, Message, wParam, lParam));
	};

	/* Subclassing list */
	LRESULT runProcList(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK wndListProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
		return (((HexEdit *)(::GetWindowLong(hwnd, GWL_USERDATA)))->runProcList(hwnd, Message, wParam, lParam));
	};

	BOOL SetFont(void)
	{
		BOOL	ret			= FALSE;
		HDC		hDc			= ::GetDC(_hSelf);
		INT		zoomFactor	= 0;

		if (_hFont) {
			::DeleteObject(_hFont);
		}

		if (_pCurProp != NULL) {
			zoomFactor = _pCurProp->fontZoom;
		}

		if (hDc != NULL) {
			_hFont = ::CreateFont(-MulDiv(g_iFontSize[_fontSize], GetDeviceCaps(hDc, LOGPIXELSY), 72) - zoomFactor, 0, 0, 0,
				(isFontBold() == TRUE) ? FW_BOLD : FW_NORMAL, isFontItalic(), isFontUnderline(),
				0, ANSI_CHARSET, OUT_TT_ONLY_PRECIS, 0, ANTIALIASED_QUALITY, FIXED_PITCH | FF_MODERN, getFontName());
			if (_hFont)
			{
				::SendMessage(_hListCtrl, WM_SETFONT, reinterpret_cast<WPARAM>(_hFont), 0);
				UpdateHeader();
				ret = TRUE;
			}
			::DeleteDC(hDc);
		}

		return ret;
	};

	void runCursor(HWND hwnd, UINT Message, WPARAM wParam, unsigned long lParam)
	{
		if (_pCurProp != NULL)
		{
			_isCurOn ^= TRUE;
			ListView_RedrawItems(_hListCtrl, _pCurProp->cursorItem, _pCurProp->cursorItem);
		}
	};

	void RestartCursor(void)
	{
		if (_pCurProp != NULL)
		{
			_isCurOn = TRUE;
			ListView_RedrawItems(_hListCtrl, _pCurProp->cursorItem, _pCurProp->cursorItem);
			::SetTimer(_hSelf, IDC_HEX_CURSORTIMER, 500, cursorFunc);
		}
	};

	void DisableCursor(void)
	{
		if (_pCurProp != NULL)
		{
			_isCurOn = FALSE;
			::KillTimer(_hSelf, IDC_HEX_CURSORTIMER);
			ListView_RedrawItems(_hListCtrl, _pCurProp->cursorItem, _pCurProp->cursorItem);
		}
	}

	static void CALLBACK cursorFunc(HWND hWnd, UINT Message, WPARAM wParam, unsigned long lParam) {
		(((HexEdit *)(::GetWindowLong(hWnd, GWL_USERDATA)))->runCursor(hWnd, Message, wParam, lParam));
	};

	BOOL TabMessage(void)
	{
		if (_pCurProp != NULL)
		{
			if (_pCurProp->isVisible == TRUE)
			{
				INT		posBeg, posEnd;
				GetSelection(&posBeg, &posEnd);
				_pCurProp->editType = (_pCurProp->editType == HEX_EDIT_HEX ? HEX_EDIT_ASCII : HEX_EDIT_HEX);
				SetSelection(posBeg, posEnd, _pCurProp->selection, _pCurProp->cursorSubItem == DUMP_FIELD);
				EnsureVisible(_pCurProp->cursorItem, (_pCurProp->editType == HEX_EDIT_ASCII) ? DUMP_FIELD : _pCurProp->cursorSubItem);
				RestartCursor();
				return TRUE;
			}
		}
		return FALSE;
	};

	void DrawCursor(HDC hDc, RECT & rcPos, BOOL drawRect = FALSE)
	{
		if (drawRect == TRUE) {
			if (isFocusRect() == TRUE) {
				::DrawFocusRect(hDc, &rcPos);
			} else {
				::PatBlt(hDc, rcPos.left, rcPos.bottom - 2, rcPos.right - rcPos.left, 2, DSTINVERT);
			}
		} else if (_isCurOn == TRUE) {
			::PatBlt(hDc, rcPos.left, rcPos.top, 2, rcPos.bottom - rcPos.top, PATINVERT);
		}
	};

	void UpdateListChanges(void)
	{
		/* update list only on changes (prevent flickering) */
		if ((_oldCursorItem != _pCurProp->cursorItem) || (_oldCursorSubItem != _pCurProp->cursorSubItem) ||
			(_oldCursorCurPos != _pCurProp->cursorPos))
		{
			/* repaint last selection */
			RECT	rc, rcTop;
			if (_oldAnchorItem <= _oldCursorItem)
			{
				ListView_GetItemRect(_hListCtrl, _oldAnchorItem, &rcTop, LVIR_BOUNDS);
				ListView_GetItemRect(_hListCtrl, _oldCursorItem, &rc, LVIR_BOUNDS);
			}
			else
			{
				ListView_GetItemRect(_hListCtrl, _oldCursorItem, &rcTop, LVIR_BOUNDS);
				ListView_GetItemRect(_hListCtrl, _oldAnchorItem, &rc, LVIR_BOUNDS);
			}
			rc.top = rcTop.top;
			::RedrawWindow(_hListCtrl, &rc, NULL, TRUE);

			if (_pCurProp->anchorItem <= _pCurProp->cursorItem)
			{
				ListView_GetItemRect(_hListCtrl, _pCurProp->anchorItem, &rcTop, LVIR_BOUNDS);
				ListView_GetItemRect(_hListCtrl, _pCurProp->cursorItem, &rc, LVIR_BOUNDS);
			}
			else
			{
				ListView_GetItemRect(_hListCtrl, _pCurProp->cursorItem, &rcTop, LVIR_BOUNDS);
				ListView_GetItemRect(_hListCtrl, _pCurProp->anchorItem, &rc, LVIR_BOUNDS);
			}
			rc.top = rcTop.top;
			::RedrawWindow(_hListCtrl, &rc, NULL, TRUE);

			_oldAnchorItem		= _pCurProp->anchorItem;
			_oldAnchorSubItem	= _pCurProp->anchorSubItem;
			_oldAnchorCurPos	= _pCurProp->anchorPos;
			_oldCursorItem		= _pCurProp->cursorItem;
			_oldCursorSubItem	= _pCurProp->cursorSubItem;
			_oldCursorCurPos	= _pCurProp->cursorPos;
		}
		InvalidateNotepad();
	};

	void EnsureVisible(UINT iItem, UINT iSubItem)
	{
		RECT	rcView		= {0};
		RECT	rcSubItem	= {0};

		ListView_EnsureVisible(_hListCtrl, iItem, TRUE);

		::GetClientRect(_hListCtrl, &rcView);
		ListView_GetSubItemRect(_hListCtrl, iItem, iSubItem, LVIR_BOUNDS, &rcSubItem);
		if (rcSubItem.left < rcView.left) {
			ListView_Scroll(_hListCtrl, -(rcView.left - rcSubItem.left), 0);
		} else if (rcView.right < rcSubItem.right) {
			ListView_Scroll(_hListCtrl, rcSubItem.right - rcView.right, 0);
		}
	};

	void InvalidateList(void)
	{
		_pCurProp->anchorItem		= _pCurProp->cursorItem;
		_pCurProp->anchorSubItem	= _pCurProp->cursorSubItem;
		_pCurProp->anchorPos		= _pCurProp->cursorPos;

		RestartCursor();
		UpdateListChanges();
	};

	void InvalidateNotepad(void)
	{
		/* select something in scintilla, it doesn't metter what your selected */
		SciSubClassWrp::execute(SCI_SETSEL, 0, _pCurProp->isSel);

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

	void QuickSortRecursive(INT d, INT h)
	{
		INT		i		= 0;
		INT		j		= 0;
		LONG	lAddr	= 0;

		/* return on empty list */
		if (d > h || d < 0)
			return;

		i = h;
		j = d;

		lAddr = _pCurProp->vBookmarks[((INT) ((d+h) / 2))].lAddress;
		do
		{
			while (_pCurProp->vBookmarks[j].lAddress < lAddr) j++;
			while (_pCurProp->vBookmarks[i].lAddress > lAddr) i--;

			if ( i >= j )
			{
				if ( i != j )
				{
					tBkMk buf = _pCurProp->vBookmarks[i];
					_pCurProp->vBookmarks[i] = _pCurProp->vBookmarks[j];
					_pCurProp->vBookmarks[j] = buf;
				}
				i--;
				j++;
			}
		} while (j <= i);

		if (d < i) QuickSortRecursive(d,i);
		if (j < h) QuickSortRecursive(j,h);
	};

private:
	/********************************* handle of list ********************************/
	HWND				_hListCtrl;
	HWND				_hHeader;
	HFONT				_hFont;
	
	/* handle of parent handle (points to scintilla main view) */
	HWND				_hParentHandle;
	HHOOK				_hParentHook;

	/* Handles */
	NppData				_nppData;
	HIMAGELIST			_hImageList;

	LPCTSTR				_iniFilePath;

	/* subclassing handle */
	WNDPROC				_hDefaultListProc;

	/* double buffer context */
	RECT				_rcMemDc;
	HDC					_hMemDc;
	HBITMAP				_hBmp;
	HBITMAP				_hOldBmp;
	HFONT				_hOldFont;
	HBRUSH				_hBkBrush;
	UINT				_uFirstVisSubItem;
	UINT				_uLastVisSubItem;

	/******************************* virables of list *********************************/

	/* current file */
	INT					_openDoc;
	INT					_lastOpenHex;
	UINT				_currLength;

	/* properties of open files */
	tHexProp*			_pCurProp;
	vector<tHexProp>	_hexProp;

	/* for selection */
	BOOL				_onChar;
	BOOL				_isCurOn;
	UINT				_fontSize;
	UINT				_x;
	UINT				_y;

	INT					_iOldHorDiff;
	INT					_iOldVerDiff;
	UINT				_oldAnchorItem;
	UINT				_oldAnchorSubItem;
	UINT				_oldAnchorCurPos;
	UINT				_oldCursorItem;
	UINT				_oldCursorSubItem;
	UINT				_oldCursorCurPos;

	/* to reconstuate selecton */
	INT					_iUnReCnt;
	UINT				_uUnReCode;
	UINT				_uFirstPos;
	UINT				_uLastPos;
	UINT				_uLastLength;

	/* mouse states */
	BOOL				_isLBtnDown;
	BOOL				_isRBtnDown;
	BOOL				_isWheel;
};




#endif // HEXDLG_DEFINE_H
