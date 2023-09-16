//this file is part of Function List Plugin for Notepad++
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

#include "HEXDialog.h"
#include "HEXResource.h"
#include "ColumnDialog.h"
#include "ModifyMenu.h"
#include "Scintilla.h"
#include "resource.h"

#include <windows.h>
#include <commctrl.h>

using namespace std;

extern char	hexMask[256][3];

void HexEdit::init(HINSTANCE hInst, NppData nppData, LPCTSTR iniFilePath)
{
	_nppData = nppData;
	Window::init(hInst, nppData._nppHandle);
	_iniFilePath = iniFilePath;

	if (!isCreated())
	{
		create(IDD_HEX_DLG, false, false);
	}
}

INT_PTR CALLBACK HexEdit::run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
	case WM_INITDIALOG:
	{
		_hListCtrl = ::GetDlgItem(_hSelf, IDC_HEX_LIST);
		UpdateFont();

		/* intial subclassing for key mapping */
		::SetWindowLongPtr(_hListCtrl, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
		_hDefaultListProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtr(_hListCtrl, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(wndListProc)));
		ListView_SetExtendedListViewStyleEx(_hListCtrl, LVS_EX_ONECLICKACTIVATE, LVS_EX_ONECLICKACTIVATE);
		break;
	}
	case WM_SIZE:
	case WM_SIZING:
	case WM_MOVING:
	case WM_ENTERSIZEMOVE:
	case WM_EXITSIZEMOVE:
	{
		RECT   rc;

		getClientRect(rc);
		::SetWindowPos(_hListCtrl, NULL,
			rc.left, rc.top, rc.right, rc.bottom,
			SWP_NOZORDER | SWP_SHOWWINDOW);
		break;
	}
	case WM_DESTROY:
	{
		for (size_t i = 0; i < _hexProp.size(); i++) {
			if (_hexProp[i].pCmpResult != NULL)
			{
				::CloseHandle(_hexProp[i].pCmpResult->hFile);
				::DeleteFile(_hexProp[i].pCmpResult->szFileName);
			}
		}
		_hexProp.clear();
		break;
	}
	case WM_ACTIVATE:
	{
		if (_pCurProp == NULL)
			break;

		if (wParam == TRUE)
		{
			if (_pCurProp->isVisible == TRUE)
			{
				/* set cursor timer */
				RestartCursor();
				::SetFocus(_hListCtrl);
			}
			/* check menu and tb icon */
			checkMenu(_pCurProp->isVisible);
		}
		else
		{
			DisableCursor();
		}
		ListView_RedrawItems(_hListCtrl, _pCurProp->cursorItem, _pCurProp->cursorItem);
		break;
	}
	case WM_NOTIFY:
	{
		if (((LPNMHDR)lParam)->hwndFrom == _hListCtrl)
		{
			switch (((LPNMHDR)lParam)->code)
			{
			case LVN_GETDISPINFO:
			{
				static CHAR  text[129] = "\0";
				LV_ITEM* lvItem = (LV_ITEM*)(&((NMLVDISPINFO*)lParam)->item);

				if (lvItem->mask & LVIF_TEXT)
				{
					ReadArrayToList(text, lvItem->iItem, lvItem->iSubItem);
#ifdef UNICODE
					static WCHAR wText[129] = _T("\0");
					::MultiByteToWideChar(CP_ACP, 0, text, -1, wText, 129);
					lvItem->pszText = wText;
#else
					lvItem->pszText = text;
#endif
				}
				return TRUE;
			}
			case NM_CUSTOMDRAW:
			{
				if (_pCurProp == NULL)
					break;

				RECT	rc = { 0 };
				RECT	rcSubItem = { 0 };
				LPNMLVCUSTOMDRAW lpCD = (LPNMLVCUSTOMDRAW)lParam;

				switch (lpCD->nmcd.dwDrawStage)
				{
				case CDDS_PREPAINT:
					/* delete previous memory dc rect */
					::ZeroMemory(&_rcMemDc, sizeof(RECT));

					/* get window rect */
					::GetWindowRect(_hListCtrl, &rc);

					/* create memory DC for flicker free paint */
					_hMemDc = ::CreateCompatibleDC(lpCD->nmcd.hdc);
					_hBmp = ::CreateCompatibleBitmap(lpCD->nmcd.hdc, rc.right - rc.left, rc.bottom - rc.top);
					_hOldBmp = (HBITMAP)::SelectObject(_hMemDc, _hBmp);
					_hOldFont = (HFONT)::SelectObject(_hMemDc, _hFont);

					/* get first and last visible sub items */
					::GetClientRect(_hListCtrl, &rc);
					for (UINT uSubItem = 1; uSubItem <= (UINT)DUMP_FIELD; uSubItem++) {
						ListView_GetSubItemRect(_hListCtrl, lpCD->nmcd.dwItemSpec, uSubItem, LVIR_BOUNDS, &rcSubItem);
						if ((rc.left <= rcSubItem.right) && (rc.right >= rcSubItem.left)) {
							if (_uFirstVisSubItem == 0) {
								_uFirstVisSubItem = uSubItem;
							}
							_uLastVisSubItem = uSubItem;
						}
					}

					SetWindowLongPtr(_hSelf, DWLP_MSGRESULT, (LONG)(CDRF_NOTIFYITEMDRAW | CDRF_NOTIFYPOSTPAINT));
					return TRUE;

				case CDDS_ITEMPREPAINT:
					/* update compare cache if necessary */
					if (_pCurProp->pCmpResult != NULL) {
						INT curPos = (UINT)(lpCD->nmcd.dwItemSpec * _pCurProp->columns);
						tCmpResult* pCmpResult = _pCurProp->pCmpResult;
						if ((pCmpResult->lenCmpCache == 0) ||
							((curPos < pCmpResult->offCmpCache) ||
								((curPos - pCmpResult->offCmpCache) >= CACHE_FILL))) {

							DWORD	hasRead = 0;
							INT     offCmpCache = (curPos - CACHE_FILL) - (curPos % _pCurProp->columns);

							pCmpResult->offCmpCache = (offCmpCache < 0 ? 0 : offCmpCache);
							pCmpResult->lenCmpCache = CACHE_SIZE;

							::SetFilePointer(pCmpResult->hFile, (LONG)pCmpResult->offCmpCache, NULL, FILE_BEGIN);
							BOOL ret = ::ReadFile(pCmpResult->hFile, pCmpResult->cmpCache, CACHE_SIZE, &hasRead, NULL);
							if ((ret == 0) && (GetLastError() != ERROR_HANDLE_EOF)) {
								SetCompareResult(NULL);
							}
							else {
								pCmpResult->lenCmpCache = (INT)hasRead;
							}
						}
					}

					/* calculate new drawing context size for blitting */
					ListView_GetItemRect(_hListCtrl, lpCD->nmcd.dwItemSpec, &rc, LVIR_BOUNDS);

					/* get background brush */
					if (lpCD->nmcd.dwItemSpec == _pCurProp->cursorItem) {
						_hBkBrush = ::CreateSolidBrush(getColor(eColorType::HEX_COLOR_CUR_LINE));
					}
					else {
						_hBkBrush = ::CreateSolidBrush(getColor(eColorType::HEX_COLOR_REG_BK));
					}

					if (_rcMemDc.bottom == 0) {

						/* set initial mem rect */
						_rcMemDc = rc;

						/* calculate header position */
						RECT	rcHeader = { 0 };
						::GetWindowRect(_hHeader, &rcHeader);
						ScreenToClient(_hHeader, &rcHeader);

						/* if item rect top is within the header recalculate top */
						if (rcHeader.bottom > rc.top) {
							_rcMemDc.top = rcHeader.bottom;
						}
					}
					else {
						_rcMemDc.bottom = rc.bottom;
					}

					/* draw background */
					::SetBkMode(_hMemDc, TRANSPARENT);
					::FillRect(_hMemDc, &rc, _hBkBrush);

					/* draw first element */
					DrawAddressText(_hMemDc, (DWORD)lpCD->nmcd.dwItemSpec);

					/* draw other elements */
					for (UINT uSubItem = _uFirstVisSubItem; uSubItem <= _uLastVisSubItem; uSubItem++) {
						if (uSubItem == DUMP_FIELD) {
							DrawDumpText(_hMemDc, (DWORD)lpCD->nmcd.dwItemSpec, uSubItem);
						}
						else {
							DrawItemText(_hMemDc, (DWORD)lpCD->nmcd.dwItemSpec, uSubItem);
						}
					}

					/* destroy background brush */
					::DeleteObject(_hBkBrush);

					SetWindowLongPtr(_hSelf, DWLP_MSGRESULT, (LONG)(CDRF_SKIPDEFAULT));
					return TRUE;

				case CDDS_POSTPAINT:
				{
					::BitBlt(lpCD->nmcd.hdc, _rcMemDc.left, _rcMemDc.top, _rcMemDc.right - _rcMemDc.left, _rcMemDc.bottom - _rcMemDc.top,
						_hMemDc, _rcMemDc.left, _rcMemDc.top, SRCCOPY);

					/* destroy memory DC */
					::SelectObject(_hMemDc, _hOldFont);
					::SelectObject(_hMemDc, _hOldBmp);
					::DeleteObject(_hBmp);
					::DeleteDC(_hMemDc);

					/* reset the boundary */
					_uFirstVisSubItem = 0;
					_uLastVisSubItem = 0;

					SetWindowLongPtr(_hSelf, DWLP_MSGRESULT, (LONG)(CDRF_SKIPDEFAULT));
					return TRUE;
				}
				default:
					return FALSE;
				}
				break;
			}
			default:
				break;
			}
		}
		break;
	}
	case HEXM_SETSEL:
	{
		if (_pCurProp == NULL)
			break;

		SetSelection((INT)wParam, (INT)lParam, eSel::HEX_SEL_NORM, (((INT)lParam) % VIEW_ROW == 0) && ((INT)wParam != (INT)lParam));
		if (_pCurProp->editType == eEdit::HEX_EDIT_HEX) {
			EnsureVisible(_pCurProp->cursorItem, _pCurProp->cursorSubItem);
		}
		else {
			EnsureVisible(_pCurProp->cursorItem, DUMP_FIELD);
		}
		break;
	}
	case HEXM_GETSEL:
	{
		GetSelection((INT*)wParam, (INT*)lParam);
		break;
	}
	case HEXM_SETPOS:
	{
		if (_pCurProp == NULL)
			break;

		SetPosition((INT)lParam, _pCurProp->isLittle);
		break;
	}
	case HEXM_GETPOS:
	{
		*((INT*)lParam) = GetCurrentPos();
		break;
	}
	case HEXM_ENSURE_VISIBLE:
	{
		SetLineVis(UINT(wParam / VIEW_ROW), (eLineVis)lParam);
		break;
	}
	case HEXM_GETSETTINGS:
	{
		*((tHexProp*)lParam) = *_pCurProp;
		break;
	}
	case HEXM_SETCURLINE:
	{
		SetCurrentLine((UINT)lParam);
		break;
	}
	case HEXM_GETCURLINE:
	{
		*((UINT*)lParam) = GetCurrentLine();
		break;
	}
	case HEXM_SETCOLUMNCNT:
	{
		if (_pCurProp == NULL)
			break;

		GetLineVis();
		_pCurProp->columns = (SHORT)lParam;
		UpdateHeader();
		break;
	}
	case HEXM_GETLINECNT:
	{
		*((UINT*)lParam) = GetItemCount();
		break;
	}
	case HEXM_UPDATEBKMK:
	{
		UpdateBookmarks((UINT)wParam, (INT)lParam);
		break;
	}
	case HEXM_GETLENGTH:
	{
		*((UINT*)lParam) = (UINT)SciSubClassWrp::execute(SCI_GETLENGTH);
		break;
	}
	case HEXM_GETDOCCP:
	{
		if (_pCurProp == NULL)
			break;

		*((UINT*)lParam) = static_cast<UINT>(_pCurProp->codePage);
		break;
	}
	default:
		break;
	}

	return FALSE;
}


LRESULT HexEdit::runProcParent(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	if (_pCurProp != NULL)
	{
		switch (Message)
		{
		case WM_SIZE:
		{
			MoveView();
			break;
		}
		case WM_KEYUP:
		case WM_KEYDOWN:
		case WM_SYSKEYUP:
		case WM_SYSKEYDOWN:
		{
			if (_pCurProp->isVisible == TRUE) {
				::SetFocus(_hListCtrl);
				::SendMessage(_hListCtrl, Message, wParam, lParam);
				return FALSE;
			}
			break;
		}
		case SCI_TAB:
		{
			if (TabMessage() == TRUE) {
				return TRUE;
			}
			break;
		}
		case WM_DESTROYCLIPBOARD:
		{
			if (g_clipboard.text != NULL)
			{
				delete g_clipboard.text;
				g_clipboard.text = NULL;
			}
			break;
		}
		default:
			break;
		}
	}

	return SciSubClassWrp::CallScintillaWndProc(hwnd, Message, wParam, lParam);
}


LRESULT HexEdit::runProcList(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	if (_pCurProp != NULL)
	{
		switch (Message)
		{
		case WM_SETFOCUS:
		{
			if (_pCurProp->isVisible == TRUE)
			{
				if (((HWND)wParam != _hParentHandle) && ((HWND)wParam != _hParent))
				{
					SetFocusNpp(_hParentHandle);
					InvalidateNotepad();
				}
				else if ((0x80 & ::GetKeyState(VK_CONTROL)) == ((HWND)wParam != _hParent))
				{
					SetFocusNpp(_hParentHandle);
				}
			}

			/* check menu and tb icon */
			checkMenu(_pCurProp->isVisible);
			RestartCursor();
			break;
		}
		case WM_LBUTTONDOWN:
		{
			LV_HITTESTINFO	info{};

			/* get selected sub item */
			info.pt.x = LOWORD(lParam);
			info.pt.y = HIWORD(lParam);
			ListView_SubItemHitTest(_hListCtrl, &info);

			/* if it's not within scope break */
			if ((info.iItem == -1) || (info.iSubItem == -1)) {
				break;
			}

			if (info.iSubItem == 0)
			{
				ToggleBookmark(info.iItem);
				return TRUE;
			}
			else
			{
				/* change edit type */
				_pCurProp->editType = (info.iSubItem == static_cast<int>(DUMP_FIELD)) ? eEdit::HEX_EDIT_ASCII : eEdit::HEX_EDIT_HEX;

				/* keep sure that selection is off */
				if (~0x80 & ::GetKeyState(VK_SHIFT)) {
					_pCurProp->isSel = FALSE;
				}

				/* goto position in parent view */
				if (_pCurProp->editType == eEdit::HEX_EDIT_HEX) {
					OnMouseClickItem(wParam, lParam);
				}
				else {
					OnMouseClickDump(wParam, lParam);
				}
				UpdateListChanges();

				/* store last position */
				_x = LOWORD(lParam);
				_y = HIWORD(lParam);
				_isLBtnDown = TRUE;
			}
			break;
		}
		case WM_LBUTTONUP:
		{
			_isLBtnDown = FALSE;
			break;
		}
		case WM_RBUTTONDOWN:
		{
			_isRBtnDown = TRUE;
			SetFocusNpp(_hParent);
			::SendMessage(_hParentHandle, Message, wParam, lParam);
			break;
		}
		case WM_RBUTTONUP:
		{
			::SetFocus(_hListCtrl);
			InvalidateNotepad();
			if ((_isWheel == FALSE) && (_isRBtnDown == TRUE))
			{
				POINT	pt = { LOWORD(lParam), HIWORD(lParam) };
				::ClientToScreen(_hParentHandle, &pt);
				TrackMenu(pt);
			}

			_isRBtnDown = FALSE;
			_isWheel = FALSE;
			break;
		}
		case WM_MBUTTONDOWN:
		{
			if ((_pCurProp->isVisible == TRUE) && (wParam & MK_CONTROL))
			{
				ZoomRestore();
				return TRUE;
			}
			break;
		}
		case WM_MOUSEWHEEL:
		{
			if (_pCurProp->isVisible == TRUE)
			{
				if (_isRBtnDown == TRUE)
				{
					_isWheel = TRUE;
					::SendMessage(_hParent, Message, wParam, lParam);
					return TRUE;
				}
				else if (wParam & MK_CONTROL)
				{
					if ((short)HIWORD(wParam) >= WHEEL_DELTA) {
						ZoomIn();
					}
					else {
						ZoomOut();
					}
					return TRUE;
				}
			}
			break;
		}
		case WM_MOUSEMOVE:
		{
			if ((_isLBtnDown == TRUE) && ((_x != LOWORD(lParam)) || (_y != HIWORD(lParam))))
			{
				if (_pCurProp->isSel == FALSE)
				{
					if (0x80 & ::GetKeyState(VK_LMENU))
					{
						_pCurProp->selection = eSel::HEX_SEL_BLOCK;
					}
					else
					{
						_pCurProp->selection = eSel::HEX_SEL_NORM;
					}
					_pCurProp->isSel = TRUE;

					if (_pCurProp->editType == eEdit::HEX_EDIT_HEX)
					{
						/* correct the cursor position */
						_pCurProp->anchorPos = (_pCurProp->anchorSubItem - 1) * _pCurProp->bits + _pCurProp->anchorPos / FACTOR;
					}
					else
					{
						/* correct column */
						_pCurProp->anchorSubItem = _pCurProp->anchorPos / _pCurProp->bits + 1;
					}
				}

				LV_HITTESTINFO	info{};

				::GetCursorPos(&info.pt);
				::ScreenToClient(_hListCtrl, &info.pt);
				ListView_SubItemHitTest(_hListCtrl, &info);

				/* test if last row is empty */
				if ((_currLength == ((GetItemCount()) * VIEW_ROW)) &&
					((info.iItem + 1) == ListView_GetItemCount(_hListCtrl)))
					return FALSE;

				/* store row */
				_pCurProp->cursorItem = info.iItem;

				if (info.iSubItem <= 0)
					return FALSE;


				if (info.iSubItem == static_cast<int>(DUMP_FIELD))
				{
					/* calculate cursor */
					_pCurProp->cursorPos = CalcCursorPos(info);

					/* correct view */
					if (ListView_GetItemCount(_hListCtrl) == (info.iItem + 1))
					{
						UINT pos = _currLength - info.iItem * VIEW_ROW;
						if (_pCurProp->cursorPos > pos)
						{
							_pCurProp->cursorPos = pos;
						}
					}

					/* calculate column */
					_pCurProp->cursorSubItem = _pCurProp->cursorPos / _pCurProp->bits + 1;
				}
				else
				{
					/* calculate cursor */
					UINT	cursorPos = CalcCursorPos(info) / FACTOR;

					_pCurProp->cursorPos = (info.iSubItem - 1) * _pCurProp->bits + cursorPos;

					/* calculate column */
					_pCurProp->cursorSubItem = info.iSubItem;

					if (cursorPos >= (UINT)_pCurProp->bits)
					{
						/* offset */
						_pCurProp->cursorSubItem++;
					}

					if ((ListView_GetItemCount(_hListCtrl) == (info.iItem + 1)) &&
						(_currLength < (_pCurProp->cursorItem * VIEW_ROW + _pCurProp->cursorPos + 1)))
					{
						_pCurProp->cursorPos = (_currLength % VIEW_ROW);
						_pCurProp->cursorSubItem = ((_currLength % VIEW_ROW) / _pCurProp->bits) + 1;
					}
				}
				UpdateListChanges();
			}
			break;
		}
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
		{
			UINT uCmdID = MapShortCutToMenuId((BYTE)wParam);
			switch (uCmdID)
			{
			case IDM_EDIT_SELECTALL:
				SelectAll();
				return FALSE;
			case IDM_EDIT_CUT:
				Cut();
				return FALSE;
			case IDM_EDIT_COPY:
				Copy();
				return FALSE;
			case IDM_EDIT_PASTE:
				Paste();
				return FALSE;
			case IDM_EDIT_DELETE:
				Delete();
				return FALSE;
			case IDM_EDIT_INS_TAB:
				TabMessage();
				return FALSE;
			case IDM_EDIT_REDO:
			case IDM_EDIT_UNDO:
			case IDM_VIEW_ZOOMIN:
			case IDM_VIEW_ZOOMOUT:
			case IDM_VIEW_ZOOMRESTORE:
				::SendMessage(_hParent, WM_COMMAND, uCmdID, 0);
				return FALSE;
			default:
				if (_pCurProp->editType == eEdit::HEX_EDIT_HEX) {
					if (OnKeyDownItem(wParam, lParam) == FALSE)
						return FALSE;
				}
				else {
					if (OnKeyDownDump(wParam, lParam) == FALSE)
						return FALSE;
				}
				break;
			}
			break;
		}
		case WM_CHAR:
		{
			/* in case additional system keys are set ignore call of function */
			if (((0x80 & ::GetKeyState(VK_CONTROL)) == 0x80) ||
				((0x80 & ::GetKeyState(VK_MENU)) == 0x80)) {
				return FALSE;
			}

			if (_pCurProp->editType == eEdit::HEX_EDIT_HEX) {
				if (OnCharItem(wParam, lParam) == FALSE)
					return FALSE;
			}
			else {
				if (OnCharDump(wParam, lParam) == FALSE)
					return FALSE;
			}
			break;
		}
		case WM_DROPFILES:
		{
			HDROP hDrop = (HDROP)wParam;

			::SetFocus(_hParentHandle);

			INT filesDropped = ::DragQueryFile(hDrop, 0xffffffff, NULL, 0);
			for (INT i = 0; i < filesDropped; ++i)
			{
				TCHAR pszFilePath[MAX_PATH];
				::DragQueryFile(hDrop, i, pszFilePath, MAX_PATH);
				::SendMessage(_hParent, NPPM_DOOPEN, 0, (LPARAM)pszFilePath);
			}
			::DragFinish(hDrop);

			if (::IsIconic(_hParent))
			{
				::ShowWindow(_hParent, SW_RESTORE);
			}
			::SetForegroundWindow(_hParent);
			return TRUE;
		}
		case WM_ERASEBKGND:
		{
			HBRUSH	hbrush = ::CreateSolidBrush(getColor(eColorType::HEX_COLOR_REG_BK));

			/* get the not used space */
			UINT	right;
			RECT	rcList, rc;

			::GetWindowRect(_hListCtrl, &rcList);
			ScreenToClient(_hListCtrl, &rcList);

			/* fill left gray fields */
			rc = rcList;
			rc.right = rc.left + 6;
			right = rc.right;
			::FillRect((HDC)wParam, &rc, hbrush);

			/* fill left not drawing rect */
			ListView_GetSubItemRect(_hListCtrl, 0, _pCurProp->columns + 1, LVIR_BOUNDS, &rc);
			rcList.left = rc.right;
			::FillRect((HDC)wParam, &rcList, hbrush);

			/* fill bottom rect if is missed */
			rcList.right = rcList.left;
			rcList.left = right;
			ListView_GetItemRect(_hListCtrl, ListView_GetItemCount(_hListCtrl) - 1, &rc, LVIR_BOUNDS);
			rcList.top = rc.bottom;
			::FillRect((HDC)wParam, &rcList, hbrush);

			::DeleteObject(hbrush);
			return FALSE;
		}
		default:
			break;
		}
	}

	return ::CallWindowProc(_hDefaultListProc, hwnd, Message, wParam, lParam);
}


void HexEdit::UpdateDocs(LPCTSTR* pFiles, UINT numFiles, INT openDoc)
{
	/* update current visible line */
	GetLineVis();

	std::vector<tHexProp>	tmpList;

	/* attach (un)known files */
	for (size_t i = 0; i < numFiles; i++)
	{
		BOOL isCopy = FALSE;


		for (size_t j = 0; j < _hexProp.size(); j++) {
			if (_tcsicmp(pFiles[i], _hexProp[j].szFileName) == 0) {
				tmpList.push_back(_hexProp[j]);
				isCopy = TRUE;
				break;
			}
		}

		if (isCopy == FALSE)
		{
			/* attach new file */
			tHexProp	prop = getProp();

			_tcscpy(prop.szFileName, pFiles[i]);
			prop.isModified = FALSE;
			prop.fontZoom = 0;
			prop.pCmpResult = NULL;

			/* test if extension of file is registered */
			prop.isVisible = IsExtensionRegistered(pFiles[i]);

			if (prop.isVisible == FALSE)
			{
				prop.isVisible = IsPercentReached(pFiles[i]);
			}

			tmpList.push_back(prop);
		}
	}

	/* delete possible allocated compare buffer */
	if (numFiles < _hexProp.size())
	{
		for (size_t i = 0; i < _hexProp.size(); i++) {
			BOOL isCopy = FALSE;
			for (size_t j = 0; j < numFiles; j++) {
				if (_tcsicmp(_hexProp[i].szFileName, tmpList[j].szFileName) == 0) {
					isCopy = TRUE;
					break;
				}
			}
			if (isCopy == FALSE) {
				SetCompareResult(NULL);
			}
		}
	}

	/* copy new list into current list */
	_hexProp = tmpList;

	if (_openDoc != openDoc)
	{
		/* store current open document */
		_openDoc = openDoc;

		if ((_hexProp.size() != 0) && _openDoc >= 0)
		{
			/* set the current file attributes */
			_pCurProp = &_hexProp[_openDoc];
			if ((_lastOpenHex != _openDoc) && (_pCurProp->isVisible == TRUE)) {
				UpdateHeader(TRUE);
			}
			doDialog();

			/* init open last open hex */
			if (_lastOpenHex == -1) {
				_lastOpenHex = _openDoc;
			}
		}
		else
		{
			doDialog();
			_pCurProp = NULL;
		}
	}
	else if ((_hexProp.size() != 0) && (openDoc >= 0))
	{
		/* set the current file attributes */
		_pCurProp = &_hexProp[openDoc];
		if ((_pCurProp->isVisible == TRUE) && (!isVisible())) {
			UpdateHeader(TRUE);
		}
		doDialog();
	}
	else
	{
		_pCurProp = NULL;
	}
}


void HexEdit::doDialog(BOOL toggle)
{
	if (_pCurProp == NULL)
		return;

	/* toggle view if user requested */
	if (toggle == TRUE)
	{
		_pCurProp->isVisible ^= TRUE;

		/* get modification state */
		BOOL isModified = (BOOL)SciSubClassWrp::execute(SCI_GETMODIFY);
		BOOL isModifiedBefore = _pCurProp->isModified;

		if (_pCurProp->isVisible == TRUE)
		{
			_pCurProp->isSel = FALSE;
			_pCurProp->anchorItem = 0;
			_pCurProp->anchorSubItem = 1;
			_pCurProp->anchorPos = 0;
			_pCurProp->cursorItem = 0;
			_pCurProp->cursorSubItem = 1;
			_pCurProp->cursorPos = 0;
			_pCurProp->editType = eEdit::HEX_EDIT_HEX;
			_pCurProp->selection = eSel::HEX_SEL_NORM;
			_pCurProp->firstVisRow = HEX_FIRST_TIME_VIS;

			/* get code page information before convert */
			_pCurProp->codePage = GetNppEncoding();

			/* convert cursor positions */
			ConvertSelNppToHEX();
		}
		else
		{
			/* convert cursor positions */
			ConvertSelHEXToNpp();
		}

		if ((isModified == FALSE) && (isModifiedBefore == FALSE)) {
			SciSubClassWrp::execute(SCI_SETSAVEPOINT);
		}
		else {
			_pCurProp->isModified = TRUE;
		}

		/* update the header */
		UpdateHeader(_pCurProp->isVisible);
	}

	/* set window position and display informations */
	SetFont();
	MoveView();

	/* set focus */
	ActivateWindow();

	/* set coding entries gray */
	ChangeNppMenu(_nppData._nppHandle, _pCurProp->isVisible, _hParentHandle);

	/* check menu and tb icon */
	checkMenu(_pCurProp->isVisible);

	/* view/hide the editor */
	display(_pCurProp->isVisible == TRUE);

	/* set possible hidden subitem in focus */
	if ((_lastOpenHex != _openDoc) && (_pCurProp->isVisible == TRUE)) {
		InvalidateNotepad();
		_lastOpenHex = _openDoc;
	}
}


void HexEdit::MoveView(void)
{
	if (_openDoc == -1)
	{
		::ShowWindow(_hParentHandle, SW_HIDE);
		::ShowWindow(_hSelf, SW_HIDE);
		::ShowWindow(_hListCtrl, SW_HIDE);
	}
	else
	{
		if ((_pCurProp != NULL) && (_pCurProp->isVisible == TRUE))
		{
			RECT	rc;

			::GetWindowRect(_hParentHandle, &rc);
			ScreenToClient(_nppData._nppHandle, &rc);

			INT	iNewHorDiff = rc.right - rc.left;
			INT	iNewVerDiff = rc.bottom - rc.top;

			::SetWindowPos(_hSelf, NULL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, FALSE);
			if (!isVisible()) {
				::RedrawWindow(_hListCtrl, NULL, NULL, RDW_INVALIDATE);
				::SetFocus(_hListCtrl);
			}
			else if ((abs(iNewHorDiff - _iOldHorDiff) > 50) ||
				(abs(iNewVerDiff - _iOldVerDiff) > 50)) {
				::RedrawWindow(_hListCtrl, NULL, NULL, RDW_INVALIDATE);
			}
			::ShowWindow(_hSelf, SW_SHOW);
			::ShowWindow(_hListCtrl, SW_SHOW);
			::ShowWindow(_hParentHandle, SW_HIDE);

			_iOldHorDiff = iNewHorDiff;
			_iOldVerDiff = iNewVerDiff;
		}
		else if (::IsWindowVisible(_hParentHandle) == FALSE)
		{
			::ShowWindow(_hParentHandle, SW_SHOW);
#ifdef ENABLE_CRASH_51
			::SetFocus(_hParentHandle);
#endif
			::ShowWindow(_hSelf, SW_HIDE);
			::ShowWindow(_hListCtrl, SW_HIDE);
		}
	}
}


void HexEdit::UpdateHeader(BOOL)
{
	if (_pCurProp == NULL)
		return;

	if (_pCurProp->isVisible)
	{
		_currLength = (UINT)SciSubClassWrp::execute(SCI_GETLENGTH, 0, 0);

		/* get current font width */
		SIZE	size;
		HDC		hDc = ::GetDC(_hListCtrl);
		SelectObject(hDc, _hFont);
		::GetTextExtentPoint32(hDc, _T("0"), 1, &size);
		::ReleaseDC(_hListCtrl, hDc);

		/* update header now */
		TCHAR	temp[17];

		while (ListView_DeleteColumn(_hListCtrl, 0));

		LVCOLUMN ColSetupTermin = { LVCF_FMT | LVCF_WIDTH | LVCF_TEXT, LVCFMT_LEFT,
			(INT)(size.cx * (LONG)(_pCurProp->addWidth + 1)), _T("Address"), 7, 0 };
		if (_pCurProp->addWidth < 8)
			ColSetupTermin.pszText = _T("Add");
		ListView_InsertColumn(_hListCtrl, 0, &ColSetupTermin);

		for (UINT i = 0; i < _pCurProp->columns; i++)
		{
			if (getCLM()) {
				_stprintf(temp, _T("%X"), i * _pCurProp->bits);
			}
			else {
				_stprintf(temp, _T("%x"), i * _pCurProp->bits);
			}
			ColSetupTermin.mask = LVCF_TEXT | LVCF_FMT | LVCF_WIDTH;
			ColSetupTermin.fmt = LVCFMT_CENTER;
			ColSetupTermin.pszText = temp;
			ColSetupTermin.cchTextMax = 6;
			ColSetupTermin.cx = size.cx * (((_pCurProp->isBin) ? 8 : 2) * _pCurProp->bits + 1);
			ListView_InsertColumn(_hListCtrl, i + 1, &ColSetupTermin);
		}

		ColSetupTermin.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
		ColSetupTermin.fmt = LVCFMT_LEFT;
		ColSetupTermin.cx = size.cx * (_pCurProp->columns * _pCurProp->bits + 1);
		ColSetupTermin.pszText = _T("Dump");
		ColSetupTermin.cchTextMax = 4;
		ListView_InsertColumn(_hListCtrl, _pCurProp->columns + 2, &ColSetupTermin);

		/* create line rows */
		UINT	items = _currLength;
		if (items)
		{
			items = items / _pCurProp->columns / _pCurProp->bits;
			ListView_SetItemCountEx(_hListCtrl, ((_currLength % VIEW_ROW) ? items + 1 : items), LVSICF_NOSCROLL);
		}
		else
		{
			ListView_SetItemCountEx(_hListCtrl, 1, LVSICF_NOSCROLL);
		}

		/* set list view to old position */
		if (_pCurProp->firstVisRow == HEX_FIRST_TIME_VIS) {
			GetLineVis();
			EnsureVisible(_pCurProp->cursorItem, _pCurProp->editType == eEdit::HEX_EDIT_ASCII ? DUMP_FIELD : _pCurProp->cursorSubItem);
		}
		else {
			SetLineVis(_pCurProp->firstVisRow, eLineVis::HEX_LINE_FIRST);
			EnsureVisible(_pCurProp->firstVisRow, _pCurProp->editType == eEdit::HEX_EDIT_ASCII ? DUMP_FIELD : _pCurProp->cursorSubItem);
		}
	}
}
void HexEdit::Copy(void)
{
	if (_pCurProp == NULL)
		return;

	if (_pCurProp->isSel == TRUE)
	{
		HWND		hSciTgt = nullptr;
		INT			offset = 0;
		INT			length = 0;
		INT			posBeg = 0;
		INT			posEnd = 0;
		tClipboard	clipboard = { 0 };

		/* store selection */
		clipboard.selection = _pCurProp->selection;

		/* copy data into scintilla handle (encoded if necessary) */
		hSciTgt = (HWND)::SendMessage(_hParent, NPPM_CREATESCINTILLAHANDLE, 0, (LPARAM)_hSelf);

		/* get text */
		switch (_pCurProp->selection)
		{
		case eSel::HEX_SEL_NORM:
		{
			GetSelection(&posBeg, &posEnd);
			offset = posBeg;
			length = CalcStride(posBeg, posEnd);
			clipboard.length = length;
			clipboard.text = (char*)new char[length + 1];
			if (clipboard.text != NULL) {
				if (LittleEndianChange(hSciTgt, _hParentHandle, &offset, &length) == TRUE) {
					::SendMessage(hSciTgt, SCI_SETSEL, posBeg - offset, (LPARAM)posBeg - offset + clipboard.length);
					::SendMessage(hSciTgt, SCI_GETSELTEXT, 0, (LPARAM)clipboard.text);
				}
			}
			else {
				::MessageBox(_hParent, _T("Couldn't create memory."), _T("Explorer"), MB_OK | MB_ICONERROR);
			}
			break;
		}
		case eSel::HEX_SEL_BLOCK:
		{
			/* get columns and count */
			UINT	first = _pCurProp->anchorItem;
			UINT	last = _pCurProp->cursorItem;

			if (_pCurProp->anchorItem == _pCurProp->cursorItem)
			{
				GetSelection(&posBeg, &posEnd);
				offset = posBeg;
				length = CalcStride(posBeg, posEnd);
				clipboard.length = length;
				clipboard.text = (char*)new char[length + 1];
				if (clipboard.text != NULL) {
					if (LittleEndianChange(hSciTgt, _hParentHandle, &offset, &length) == TRUE) {
						::SendMessage(hSciTgt, SCI_SETSEL, posBeg - offset, (LPARAM)posBeg - offset + clipboard.length);
						::SendMessage(hSciTgt, SCI_GETSELTEXT, 0, (LPARAM)clipboard.text);
					}
				}
				else {
					::MessageBox(_hParent, _T("Couldn't create memory."), _T("Explorer"), MB_OK | MB_ICONERROR);
				}
				break;
			}
			else if (_pCurProp->anchorItem > _pCurProp->cursorItem)
			{
				first = _pCurProp->cursorItem;
				last = _pCurProp->anchorItem;
			}

			/* get item count */
			clipboard.items = last - first + 1;

			/* get stride */
			if (_pCurProp->anchorPos > _pCurProp->cursorPos)
			{
				posBeg = first * VIEW_ROW + _pCurProp->cursorPos;
				clipboard.stride = _pCurProp->anchorPos - _pCurProp->cursorPos;
			}
			else
			{
				posBeg = first * VIEW_ROW + _pCurProp->anchorPos;
				clipboard.stride = _pCurProp->cursorPos - _pCurProp->anchorPos;
			}

			/* get text */
			clipboard.length = clipboard.stride * (last - first + 1);
			clipboard.text = (char*)new char[clipboard.length + 1];

			if (clipboard.text != NULL) {
				posEnd = posBeg + clipboard.stride;
				for (UINT i = 0; i < clipboard.items; i++)
				{
					offset = posBeg;
					length = clipboard.stride;
					if (LittleEndianChange(hSciTgt, _hParentHandle, &offset, &length) == TRUE) {
						::SendMessage(hSciTgt, SCI_SETSEL, posBeg - offset, (LPARAM)posBeg - offset + clipboard.stride);
						::SendMessage(hSciTgt, SCI_GETSELTEXT, 0, (LPARAM)&clipboard.text[i * clipboard.stride]);
						posBeg += VIEW_ROW;
					}
				}
			}
			else {
				::MessageBox(_hParent, _T("Couldn't create memory."), _T("Explorer"), MB_OK | MB_ICONERROR);
			}
			break;
		}
		default:
			break;
		}

		/* convert to hex if usefull */
		if (clipboard.text != NULL)
		{
			if (_pCurProp->editType == eEdit::HEX_EDIT_HEX)
			{
				tClipboard	data = clipboard;
				ChangeClipboardDataToHex(&data);
				/* store selected text in scintilla clipboard */
				SciSubClassWrp::execute(SCI_COPYTEXT, data.length + 1, (LPARAM)data.text);
				delete[] data.text;
			}
			else
			{
				/* store selected text in scintilla clipboard */
				SciSubClassWrp::execute(SCI_COPYTEXT, clipboard.length + 1, (LPARAM)clipboard.text);
			}

			/* delete old text and store to clipboard */
			delete[] g_clipboard.text;
		}

		/* destory scintilla handle */
		CleanScintillaBuf(hSciTgt);
		::SendMessage(_hParent, NPPM_DESTROYSCINTILLAHANDLE, 0, (LPARAM)hSciTgt);

		g_clipboard = clipboard;
	}
}


void HexEdit::Cut(void)
{
	if (_pCurProp == NULL)
		return;

	if (_pCurProp->isSel == TRUE)
	{
		HWND		hSciTgt = nullptr;
		INT			offset = 0;
		INT			length = 0;
		INT			posBeg = 0;
		INT			posEnd = 0;
		tClipboard	clipboard = { 0 };

		/* store selection */
		clipboard.selection = _pCurProp->selection;

		/* copy data into scintilla handle (encoded if necessary) */
		hSciTgt = (HWND)::SendMessage(_hParent, NPPM_CREATESCINTILLAHANDLE, 0, NULL);

		SciSubClassWrp::execute(SCI_BEGINUNDOACTION);

		/* get text */
		switch (_pCurProp->selection)
		{
		case eSel::HEX_SEL_BLOCK:
		{
			/* get columns and count */
			UINT	first = _pCurProp->anchorItem;
			UINT	last = _pCurProp->cursorItem;

			/* if anchor = cursor row goto eSel::HEX_SEL_NORM */
			if (_pCurProp->anchorItem != _pCurProp->cursorItem)
			{
				if (_pCurProp->anchorItem > _pCurProp->cursorItem)
				{
					first = _pCurProp->cursorItem;
					last = _pCurProp->anchorItem;
				}

				/* get item count */
				clipboard.items = last - first + 1;

				/* get stride */
				if (_pCurProp->anchorPos > _pCurProp->cursorPos)
				{
					posBeg = first * VIEW_ROW + _pCurProp->cursorPos;
					clipboard.stride = _pCurProp->anchorPos - _pCurProp->cursorPos;
				}
				else
				{
					posBeg = first * VIEW_ROW + _pCurProp->anchorPos;
					clipboard.stride = _pCurProp->cursorPos - _pCurProp->anchorPos;
				}

				/* get text */
				clipboard.length = clipboard.stride * (last - first + 1);
				clipboard.text = (LPSTR)new CHAR[clipboard.length + 1];

				if (clipboard.text != NULL)
				{
					INT	tempBeg = posBeg;
					offset = posBeg;
					length = VIEW_ROW * (last - first + 1);
					posEnd = posBeg + clipboard.stride;
					if (LittleEndianChange(hSciTgt, _hParentHandle, &offset, &length) == TRUE)
					{
						for (UINT i = 0; i < clipboard.items; i++)
						{
							::SendMessage(hSciTgt, SCI_SETSEL, posBeg - offset, (LPARAM)posBeg - offset + clipboard.stride);
							::SendMessage(hSciTgt, SCI_GETSELTEXT, 0, (LPARAM)&clipboard.text[i * clipboard.stride]);
							::SendMessage(hSciTgt, SCI_REPLACESEL, 0, (LPARAM)"\0");
							/* replace selection with "" */
							if (eError::E_OK != replaceLittleToBig(_hParentHandle, NULL, 0, posBeg, clipboard.stride, 0))
							{
								LITTLE_DELETE_ERROR;
								/* destory scintilla handle */
								CleanScintillaBuf(hSciTgt);
								::SendMessage(_hParent, NPPM_DESTROYSCINTILLAHANDLE, 0, (LPARAM)hSciTgt);
								return;
							}
							UpdateBookmarks(posBeg, -((INT)clipboard.stride));
							posBeg += VIEW_ROW - (INT)clipboard.stride;
						}
					}
					SetPosition(tempBeg);
				}
				else {
					::MessageBox(_hParent, _T("Couldn't create memory."), _T("Explorer"), MB_OK | MB_ICONERROR);
				}
				break;
			}
		}
		case eSel::HEX_SEL_NORM:
		{
			/* get selection and correct positions */
			GetSelection(&posBeg, &posEnd);
			offset = posBeg;
			if (posEnd < posBeg) {
				posBeg = posEnd;
				posEnd = offset;
				offset = posBeg;
			}

			/* get length and initialize clipboard */
			length = CalcStride(posBeg, posEnd);
			clipboard.length = length;
			clipboard.text = (LPSTR)new CHAR[length + 1];
			if (clipboard.text != NULL)
			{
				if (LittleEndianChange(hSciTgt, _hParentHandle, &offset, &length) == TRUE)
				{
					::SendMessage(hSciTgt, SCI_SETSEL, posBeg - offset, (LPARAM)posBeg - offset + clipboard.length);
					::SendMessage(hSciTgt, SCI_GETSELTEXT, 0, (LPARAM)clipboard.text);
					/* replace selection with "" */
					if (eError::E_OK != replaceLittleToBig(_hParentHandle, NULL, 0, posBeg, clipboard.length, 0))
					{
						LITTLE_DELETE_ERROR;
						/* destory scintilla handle */
						CleanScintillaBuf(hSciTgt);
						::SendMessage(_hParent, NPPM_DESTROYSCINTILLAHANDLE, 0, (LPARAM)hSciTgt);
						return;
					}
					SetPosition(posBeg);
					UpdateBookmarks(posBeg, -(INT)clipboard.length);
				}
			}
			else {
				::MessageBox(_hParent, _T("Couldn't create memory."), _T("Explorer"), MB_OK | MB_ICONERROR);
			}
			break;
		}
		default:
			break;
		}

		/* destory scintilla handle */
		CleanScintillaBuf(hSciTgt);
		::SendMessage(_hParent, NPPM_DESTROYSCINTILLAHANDLE, 0, (LPARAM)hSciTgt);

		/* convert to hex if usefull */
		if (clipboard.text != NULL)
		{
			if (_pCurProp->editType == eEdit::HEX_EDIT_HEX)
			{
				tClipboard	data = clipboard;
				ChangeClipboardDataToHex(&data);
				/* store selected text in scintilla clipboard */
				SciSubClassWrp::execute(SCI_COPYTEXT, data.length + 1, (LPARAM)data.text);
				delete[] data.text;
			}
			else
			{
				/* store selected text in scintilla clipboard */
				SciSubClassWrp::execute(SCI_COPYTEXT, clipboard.length + 1, (LPARAM)clipboard.text);
			}

			/* delete old text and store to clipboard */
			delete[] g_clipboard.text;
			g_clipboard = clipboard;
		}

		SciSubClassWrp::execute(SCI_ENDUNDOACTION);
	}
}


void HexEdit::Paste(void)
{
	if (_pCurProp == NULL)
		return;

	SciSubClassWrp::execute(SCI_BEGINUNDOACTION);

	if (g_clipboard.text == NULL)
	{
		/* copy data into scintilla handle (encoded if necessary) */
		HWND hSciTgt = (HWND)::SendMessage(_hParent, NPPM_CREATESCINTILLAHANDLE, 0, (LPARAM)_hSelf);
		ScintillaMsg(hSciTgt, SCI_PASTE);

		UINT	length = (UINT)SciSubClassWrp::execute(SCI_GETTEXTLENGTH);

		/* test if first chars are digits only */
		ScintillaMsg(hSciTgt, SCI_SETSEARCHFLAGS, SCFIND_REGEXP | SCFIND_POSIX);
		ScintillaMsg(hSciTgt, SCI_SETTARGETSTART, 0);
		ScintillaMsg(hSciTgt, SCI_SETTARGETEND, length);

		UINT posFind = ScintillaMsg(hSciTgt, SCI_SEARCHINTARGET, 14, (LPARAM)"^[0-9a-fA-F]+ ");
		if (posFind == 0)
		{
			/* if test again and extract informations */
			UINT	lineCnt = ScintillaMsg(hSciTgt, SCI_GETLINECOUNT);
			UINT	charPerLine = ScintillaMsg(hSciTgt, SCI_LINELENGTH, 0);
			LPSTR	buffer = (LPSTR)new CHAR[charPerLine + 1];
			LPSTR   target = (LPSTR)new CHAR[charPerLine * lineCnt + 1];

			if ((buffer != NULL) && (target != NULL))
			{
				LPSTR	p_buffer = NULL;
				LPSTR	p_target = target;
				BOOL	isOk = TRUE;
				INT		expLine = 0;

				while ((posFind != -1) && (isOk))
				{
					INT		posBeg = ScintillaMsg(hSciTgt, SCI_GETTARGETSTART);
					INT		posEnd = ScintillaMsg(hSciTgt, SCI_GETTARGETEND);
					INT		size = posEnd - posBeg;
					INT		curLine = ScintillaMsg(hSciTgt, SCI_LINEFROMPOSITION, posBeg);
					ScintillaGetText(hSciTgt, buffer, posBeg, posEnd - 1);
					buffer[size - 1] = 0;

					if ((ASCIIConvert(buffer) / 0x10) == expLine)
					{
						UINT	curLength = (curLine != (static_cast<INT>(lineCnt) - 1) ? charPerLine - 3 : length % charPerLine);
						ScintillaGetText(hSciTgt, buffer, posBeg, posBeg + curLength);

						/* get chars */
						UINT	i = 0;
						char* p_end = buffer + curLength;
						p_buffer = &buffer[size];

						while (i < (UINT)(p_end - p_buffer))
						{
							*p_target = 0;
							for (INT j = 0; j < 2; j++)
							{
								*p_target <<= 4;
								*p_target |= decMask[*p_buffer];
								p_buffer++;
							}
							p_target++;
							p_buffer++;
							i++;
						}

						ScintillaMsg(hSciTgt, SCI_SETTARGETSTART, posEnd);
						ScintillaMsg(hSciTgt, SCI_SETTARGETEND, length);
						posFind = ScintillaMsg(hSciTgt, SCI_SEARCHINTARGET, 14, (LPARAM)"^[0-9a-fA-F]+ ");

						/* increment expected values */
						expLine++;
					}
					else
					{
						isOk = FALSE;
						TCHAR	TEMP[256];
#ifdef UNICODE
						TCHAR tempBuffer[256];
						::MultiByteToWideChar(CP_UTF8, 0, buffer, -1, tempBuffer, 256);
						tempBuffer[255] = L'\0';
						_stprintf(TEMP, _T("%s %d\n"), tempBuffer, ASCIIConvert(buffer) / 0x10);
#else
						_stprintf(TEMP, _T("%s %d\n"), buffer, ASCIIConvert(buffer) / 0x10);
#endif
						OutputDebugString(TEMP);
					}
				}

				/* copy into target scintilla */
				if (isOk == TRUE)
				{
					UINT	posBeg = GetAnchor();
					UINT	posEnd = GetCurrentPos();

					if (posBeg > posEnd) {
						posBeg = posEnd;
						posEnd = GetAnchor();
					}

					SciSubClassWrp::execute(SCI_SETSEL, posBeg, posEnd);
					SciSubClassWrp::execute(SCI_TARGETFROMSELECTION);
					SciSubClassWrp::execute(SCI_REPLACETARGET, p_target - target, (LPARAM)target);
					UpdateBookmarks(posBeg, posEnd - posBeg);
				}
			}

			/* remove resources */
			if (buffer != NULL) {
				delete[] buffer;
			}
			if (target != NULL) {
				delete[] target;
			}
		}
		else
		{
			UINT	uCF = 0;
			UINT	lenData = 0;
			LPSTR	pchData = NULL;

			/* Insure desired format is there, and open clipboard */
			if (::IsClipboardFormatAvailable(CF_UNICODETEXT) == TRUE) {
				uCF = CF_UNICODETEXT;
			}
			else if (::IsClipboardFormatAvailable(CF_TEXT) == TRUE) {
				uCF = CF_TEXT;
			}

			/* open clipboard */
			if (uCF != 0) {
				if (::OpenClipboard(NULL) == FALSE)
					return;
			}
			else {
				return;
			}

			/* get clipboard data */
			HANDLE hClipboardData = ::GetClipboardData(uCF);

			if (uCF == CF_TEXT) {
				LPSTR	pchBuffer = (LPSTR)GlobalLock(hClipboardData);
				if (pchBuffer != NULL) {
					lenData = (UINT)strlen(pchBuffer);
					pchData = new CHAR[lenData + 1];
					if (pchData != NULL) {
						::CopyMemory(pchData, pchBuffer, lenData);
					}
				}
			}
			else if (uCF == CF_UNICODETEXT) {
				LPWSTR	pwchBuffer = (LPWSTR)GlobalLock(hClipboardData);
				UINT	lenBuffer = (UINT)wcslen(pwchBuffer);
				switch (_pCurProp->codePage)
				{
				case eNppCoding::HEX_CODE_NPP_ASCI:
				{
					lenBuffer++;
					lenData = ::WideCharToMultiByte(CP_ACP, 0, pwchBuffer, lenBuffer, pchData, 0, NULL, NULL) - 1;
					pchData = new CHAR[lenData + 1];
					if (pchData != NULL) {
						::WideCharToMultiByte(CP_ACP, 0, pwchBuffer, lenBuffer, pchData, lenData, NULL, NULL);
					}
					break;
				}
				case eNppCoding::HEX_CODE_NPP_UTF8:
				case eNppCoding::HEX_CODE_NPP_UTF8_BOM:
				{
					lenBuffer++;
					lenData = ::WideCharToMultiByte(CP_UTF8, 0, pwchBuffer, lenBuffer, pchData, 0, NULL, NULL) - 1;
					pchData = new CHAR[lenData + 1];
					if (pchData != NULL) {
						::WideCharToMultiByte(CP_UTF8, 0, pwchBuffer, lenBuffer, pchData, lenData, NULL, NULL);
					}
					break;
				}
				case eNppCoding::HEX_CODE_NPP_USCBE:
				{
					lenData = lenBuffer * 2;
					pchData = new CHAR[lenData + 1];
					if (pchData != NULL) {
						LPSTR	pchBuffer = (LPSTR)pwchBuffer;
						for (UINT pos = 0; pos < lenData; pos += 2) {
							pchData[pos + 1] = pchBuffer[pos];
							pchData[pos] = pchBuffer[pos + 1];
						}
					}
					break;
				}
				case eNppCoding::HEX_CODE_NPP_USCLE:
				{
					lenData = lenBuffer * 2;
					pchData = new CHAR[lenData + 1];
					if (pchData != NULL) {
						::CopyMemory(pchData, pwchBuffer, lenData);
					}
					break;
				}
				}
			}

			/* copy into target scintilla */
			if (pchData != NULL)
			{
				UINT	posBeg = GetAnchor();
				UINT	posEnd = GetCurrentPos();

				if (posBeg > posEnd) {
					posBeg = posEnd;
					posEnd = GetAnchor();
				}

				SciSubClassWrp::execute(SCI_SETSEL, posBeg, posEnd);
				SciSubClassWrp::execute(SCI_TARGETFROMSELECTION);
				SciSubClassWrp::execute(SCI_REPLACETARGET, lenData, (LPARAM)pchData);
				UpdateBookmarks(posBeg, (posEnd - posBeg) - lenData);
				delete[] pchData;
			}

			GlobalUnlock(hClipboardData);
			::CloseClipboard();
		}

		/* destory scintilla handle */
		CleanScintillaBuf(hSciTgt);
		::SendMessage(_hParent, NPPM_DESTROYSCINTILLAHANDLE, 0, (LPARAM)hSciTgt);
		}
	else
	{
		HWND	hSciTgt = NULL;
		UINT	posBeg = GetAnchor();
		UINT	posEnd = GetCurrentPos();

		if (posBeg > posEnd)
			posBeg = posEnd;

		/* copy data into scintilla handle (encoded if necessary) */
		hSciTgt = (HWND)::SendMessage(_hParent, NPPM_CREATESCINTILLAHANDLE, 0, (LPARAM)_hSelf);

		switch (g_clipboard.selection)
		{
		case eSel::HEX_SEL_NORM:
		{
			::SendMessage(hSciTgt, SCI_ADDTEXT, g_clipboard.length, (LPARAM)g_clipboard.text);
			UpdateBookmarks(posBeg, g_clipboard.length);
			if (eError::E_OK == replaceLittleToBig(_hParentHandle, hSciTgt, 0, posBeg, posEnd - posBeg, g_clipboard.length))
			{
				SetSelection(posBeg + g_clipboard.length, posBeg + g_clipboard.length);
			}
			else
			{
				LITTLE_DELETE_ERROR;
				return;
			}
			break;
		}
		case eSel::HEX_SEL_BLOCK:
		{
			UINT	stride = 0;
			UINT	items = 0;

			/* set start and end of block */
			if (_pCurProp->isSel == TRUE)
			{
				UINT	first = _pCurProp->anchorItem;
				UINT	last = _pCurProp->cursorItem;

				if (_pCurProp->anchorItem > _pCurProp->cursorItem)
				{
					first = _pCurProp->cursorItem;
					last = _pCurProp->anchorItem;
				}
				items = last - first + 1;

				if (_pCurProp->anchorPos > _pCurProp->cursorPos)
				{
					posBeg = first * VIEW_ROW + _pCurProp->cursorPos;
					stride = _pCurProp->anchorPos - _pCurProp->cursorPos;
				}
				else
				{
					posBeg = first * VIEW_ROW + _pCurProp->anchorPos;
					stride = _pCurProp->cursorPos - _pCurProp->anchorPos;
				}

				if ((stride != g_clipboard.stride) || (items != g_clipboard.items))
				{
					if (NLMessageBox(_hInst, _hParent, _T("MsgBox SameWidth"), MB_OK | MB_ICONERROR) == FALSE)
						MessageBox(_hSelf, _T("Clipboard info has not the same width!"), _T("Hex-Editor Error"), MB_OK | MB_ICONERROR);
					SciSubClassWrp::execute(SCI_ENDUNDOACTION);
					return;
				}
			}

			posEnd = posBeg + g_clipboard.stride;
			for (UINT i = 0; i < g_clipboard.items; i++)
			{
				::SendMessage(hSciTgt, SCI_ADDTEXT, g_clipboard.stride, (LPARAM)&g_clipboard.text[i * g_clipboard.stride]);
				UpdateBookmarks(posBeg, g_clipboard.stride);
				if (eError::E_OK != replaceLittleToBig(_hParentHandle, hSciTgt, i * g_clipboard.stride, posBeg, stride, g_clipboard.stride))
				{
					LITTLE_DELETE_ERROR;
					return;
				}

				posBeg += VIEW_ROW;
				posEnd += VIEW_ROW;
			}

			SetSelection(posEnd - VIEW_ROW, posEnd - VIEW_ROW);
			break;
		}
		default:
			break;
		}
		/* destory scintilla handle */
		CleanScintillaBuf(hSciTgt);
		::SendMessage(_hParent, NPPM_DESTROYSCINTILLAHANDLE, 0, (LPARAM)hSciTgt);

	}
	SciSubClassWrp::execute(SCI_ENDUNDOACTION);
	}

void HexEdit::SelectAll(void)
{
	SetSelection(0, _currLength);
	SetLineVis(_currLength, eLineVis::HEX_LINE_FIRST);
}

void HexEdit::ZoomIn(void)
{
	if ((g_iFontSize[_fontSize] + _pCurProp->fontZoom) < g_iFontSize[G_FONTSIZE_MAX - 1])
	{
		_pCurProp->fontZoom++;

		if (SetFont() == FALSE)
			OutputDebugString(_T("Could not change font\n"));

		GetLineVis();
	}
}


void HexEdit::ZoomOut(void)
{
	if ((g_iFontSize[_fontSize] + _pCurProp->fontZoom) > 0)
	{
		_pCurProp->fontZoom--;

		if (SetFont() == FALSE)
			OutputDebugString(_T("Could not change font\n"));

		GetLineVis();
	}
}


void HexEdit::ZoomRestore(void)
{
	_pCurProp->fontZoom = 0;

	if (SetFont() == FALSE)
		OutputDebugString(_T("Could not change font\n"));

	GetLineVis();
}


void HexEdit::ReadArrayToList(LPSTR text, INT iItem, INT iSubItem)
{
	if (_pCurProp == NULL)
		return;

	/* create addresses */
	if (iSubItem == 0)
	{
		if (getCLM()) {
			switch (_pCurProp->addWidth)
			{
			case 4: sprintf(text, "%04X", iItem * VIEW_ROW); break;
			case 5: sprintf(text, "%05X", iItem * VIEW_ROW); break;
			case 6: sprintf(text, "%06X", iItem * VIEW_ROW); break;
			case 7: sprintf(text, "%07X", iItem * VIEW_ROW); break;
			case 8: sprintf(text, "%08X", iItem * VIEW_ROW); break;
			case 9: sprintf(text, "%09X", iItem * VIEW_ROW); break;
			case 10: sprintf(text, "%010X", iItem * VIEW_ROW); break;
			case 11: sprintf(text, "%011X", iItem * VIEW_ROW); break;
			case 12: sprintf(text, "%012X", iItem * VIEW_ROW); break;
			case 13: sprintf(text, "%013X", iItem * VIEW_ROW); break;
			case 14: sprintf(text, "%014X", iItem * VIEW_ROW); break;
			case 15: sprintf(text, "%015X", iItem * VIEW_ROW); break;
			case 16: sprintf(text, "%016X", iItem * VIEW_ROW); break;
			}
		}
		else {
			switch (_pCurProp->addWidth)
			{
			case 4: sprintf(text, "%04x", iItem * VIEW_ROW); break;
			case 5: sprintf(text, "%05x", iItem * VIEW_ROW); break;
			case 6: sprintf(text, "%06x", iItem * VIEW_ROW); break;
			case 7: sprintf(text, "%07x", iItem * VIEW_ROW); break;
			case 8: sprintf(text, "%08x", iItem * VIEW_ROW); break;
			case 9: sprintf(text, "%09x", iItem * VIEW_ROW); break;
			case 10: sprintf(text, "%010x", iItem * VIEW_ROW); break;
			case 11: sprintf(text, "%011x", iItem * VIEW_ROW); break;
			case 12: sprintf(text, "%012x", iItem * VIEW_ROW); break;
			case 13: sprintf(text, "%013x", iItem * VIEW_ROW); break;
			case 14: sprintf(text, "%014x", iItem * VIEW_ROW); break;
			case 15: sprintf(text, "%015x", iItem * VIEW_ROW); break;
			case 16: sprintf(text, "%016x", iItem * VIEW_ROW); break;
			}
		}
	}
	/* create dump */
	else if (iSubItem == static_cast<int>(DUMP_FIELD))
	{
		UINT	posBeg = iItem * VIEW_ROW;

		if ((posBeg + VIEW_ROW) <= _currLength)
		{
			ScintillaGetText(_hParentHandle, text, posBeg, posBeg + VIEW_ROW);
			DumpConvert(text, VIEW_ROW);
		}
		else
		{
			ScintillaGetText(_hParentHandle, text, posBeg, _currLength);
			DumpConvert(text, _currLength - posBeg);
		}
	}
	/* create hex */
	else
	{
		UINT	posBeg = iItem * VIEW_ROW + (iSubItem - 1) * _pCurProp->bits;

		if (posBeg < _currLength)
		{
			if ((posBeg + _pCurProp->bits) < _currLength)
			{
				ScintillaGetText(_hParentHandle, text, posBeg, posBeg + _pCurProp->bits);
				AddressConvert(text, _pCurProp->bits);
			}
			else
			{
				ScintillaGetText(_hParentHandle, text, posBeg, _currLength);
				AddressConvert(text, _currLength - posBeg);
				memset(&text[(_currLength - posBeg) * FACTOR], 0x20, SUBITEM_LENGTH);
				text[SUBITEM_LENGTH] = 0;
			}
		}
		else
		{
			memset(text, 0x20, SUBITEM_LENGTH);
			text[SUBITEM_LENGTH] = 0;
		}
	}
}


void HexEdit::AddressConvert(LPSTR text, INT length)
{
	CHAR temp[65];

	memcpy(temp, text, length);

	if (_pCurProp->isLittle == TRUE)
	{
		if (_pCurProp->isBin)
		{
			strcpy(text, binMask[(UCHAR)temp[--length]]);
			for (INT i = length - 1; i >= 0; --i)
			{
				strcat(text, binMask[(UCHAR)temp[i]]);
			}
		}
		else
		{
			strcpy(text, hexMask[(UCHAR)temp[--length]]);
			for (INT i = length - 1; i >= 0; --i)
			{
				strcat(text, hexMask[(UCHAR)temp[i]]);
			}
		}
	}
	else
	{
		if (_pCurProp->isBin)
		{
			strcpy(text, binMask[(UCHAR)temp[0]]);
			for (INT i = 1; i < length; i++)
			{
				strcat(text, binMask[(UCHAR)temp[i]]);
			}
		}
		else
		{
			strcpy(text, hexMask[(UCHAR)temp[0]]);
			for (INT i = 1; i < length; i++)
			{
				strcat(text, hexMask[(UCHAR)temp[i]]);
			}
		}
	}
}

void HexEdit::DumpConvert(LPSTR text, UINT length)
{
	if (_pCurProp->isLittle == FALSE)
	{
		/* i must be unsigned */
		for (INT i = length - 1; i >= 0; --i)
		{
			text[i] = ascii[(UCHAR)text[i]];
		}
	}
	else
	{
		CHAR	temp[129]{};
		LPSTR	pText = text;

		/* i must be unsigned */
		for (UINT i = 0; i < length; i++)
		{
			temp[i] = ascii[(UCHAR)text[i]];
		}

		UINT offset = length % _pCurProp->bits;
		UINT max = length / _pCurProp->bits + 1;

		for (UINT i = 1; i <= max; i++)
		{
			if (i == max)
			{
				for (UINT j = 1; j <= offset && j <= length; j++)
				{
					*pText = temp[length - j];
					pText++;
				}
			}
			else
			{
				for (UINT j = 1; j <= _pCurProp->bits; j++)
				{
					*pText = temp[_pCurProp->bits * i - j];
					pText++;
				}
			}
		}
		*pText = NULL;
	}
}


void HexEdit::BinHexConvert(LPSTR text, INT length)
{
	CHAR temp[65];

	memcpy(temp, text, length);

	if (_pCurProp->isBin == FALSE)
	{
		for (INT i = length - 1; i >= 0; --i)
		{
			text[i] = 0;
			for (INT j = 0; j < 2; j++)
			{
				text[i] <<= 4;
				text[i] |= decMask[temp[2 * i + j]];
			}
		}
	}
	else
	{
		char* pText = temp;
		for (INT i = 0; i < length; i++)
		{
			text[i] = 0;
			for (INT j = 0; j < 8; j++)
			{
				text[i] <<= 1;
				text[i] |= (*pText & 0x01);
				pText++;
			}
		}
	}


	length = length / FACTOR;

	if (_pCurProp->isLittle == TRUE)
	{
		memcpy(temp, text, length);
		for (INT i = 0; i < length; i++)
		{
			text[i] = temp[length - 1 - i];
		}
	}
	text[length] = 0;
}


void HexEdit::TrackMenu(POINT pt)
{
	HMENU	hMenu = ::CreatePopupMenu();
	HMENU	hSubMenu = ::CreatePopupMenu();
	BOOL	isChanged = TRUE;
	UINT	anchorPos = GetAnchor();
	UINT	cursorPos = GetCurrentPos();
	UINT	oldColumns = _pCurProp->columns;
	TCHAR	txtMenu[16];

	/* set bit decoding */
	::AppendMenu(hSubMenu, MF_STRING | ((_pCurProp->bits == HEX_BYTE) ? MF_CHECKED : 0), 7, _T("8-Bit"));
	::AppendMenu(hSubMenu, MF_STRING | ((_pCurProp->bits == HEX_WORD) ? MF_CHECKED : 0), 8, _T("16-Bit"));
	::AppendMenu(hSubMenu, MF_STRING | ((_pCurProp->bits == HEX_DWORD) ? MF_CHECKED : 0), 9, _T("32-Bit"));
	::AppendMenu(hSubMenu, MF_STRING | ((_pCurProp->bits == HEX_LONG) ? MF_CHECKED : 0), 10, _T("64-Bit"));
	::AppendMenu(hSubMenu, MF_SEPARATOR, 0, _T("-----------------"));
	/* set binary decoding */
	if (NLGetText(_hInst, _hParent, _pCurProp->isBin == TRUE ? _T("to Hex") : _T("to Binary"), txtMenu, sizeof(txtMenu)) == FALSE)
		_tcscpy(txtMenu, _pCurProp->isBin == TRUE ? _T("to Hex") : _T("to Binary"));
	::AppendMenu(hSubMenu, MF_STRING, 11, txtMenu);
	/* change between big- and little-endian */
	if (_pCurProp->bits > HEX_BYTE)
	{
		if (NLGetText(_hInst, _hParent, _pCurProp->isLittle == TRUE ? _T("to BigEndian") : _T("to LittleEndian"), txtMenu, sizeof(txtMenu)) == FALSE)
			_tcscpy(txtMenu, _pCurProp->isLittle == TRUE ? _T("to BigEndian") : _T("to LittleEndian"));
		::AppendMenu(hSubMenu, MF_STRING, 12, txtMenu);
	}

	UINT style = 0;

	if (!(_pCurProp->isSel && ((_pCurProp->selection == eSel::HEX_SEL_NORM) ||
		((_pCurProp->selection == eSel::HEX_SEL_BLOCK) && (_pCurProp->anchorPos != _pCurProp->cursorPos))))) {
		style = MF_GRAYED;
	}

	::AppendMenu(hMenu, MF_STRING | style, 1, _T("Cut"));
	::AppendMenu(hMenu, MF_STRING | style, 2, _T("Copy"));
	::AppendMenu(hMenu, MF_STRING | ((!SciSubClassWrp::execute(SCI_CANPASTE)) ? MF_GRAYED : 0), 3, _T("Paste"));
	::AppendMenu(hMenu, MF_STRING | style, 4, _T("Delete"));
	::AppendMenu(hMenu, MF_SEPARATOR, 0, _T("-----------------"));
	::AppendMenu(hMenu, MF_STRING | style, 15, _T("Cut Binary Content"));
	::AppendMenu(hMenu, MF_STRING | style, 13, _T("Copy Binary Content"));
	::AppendMenu(hMenu, MF_STRING | ((!SciSubClassWrp::execute(SCI_CANPASTE)) ? MF_GRAYED : 0), 14, _T("Paste Binary Content"));
	::AppendMenu(hMenu, MF_SEPARATOR, 0, _T("-----------------"));
	::AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT_PTR)hSubMenu, _T("View in"));
	::AppendMenu(hMenu, MF_SEPARATOR, 0, _T("-----------------"));

	/* set columns */
	::AppendMenu(hMenu, MF_STRING, 5, _T("Address Width..."));
	::AppendMenu(hMenu, MF_STRING, 6, _T("Columns..."));

	/* change language */
	NLChangeMenu(_hInst, _hParent, hMenu, _T("SettingsMenu"), MF_BYPOSITION);

	/* create menu */
	switch (::TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY,
		pt.x, pt.y, 0, _hListCtrl, NULL))
	{
	case 1:
	{
		Cut();
		break;
	}
	case 2:
	{
		Copy();
		break;
	}
	case 3:
	{
		Paste();
		break;
	}
	case 4:
	{
		Delete();
		break;
	}
	case 5:
	{
		ColumnDlg	dlg;
		dlg.init(_hInst, _nppData);
		UINT	val = dlg.doDialogAddWidth(_pCurProp->addWidth);
		if ((val >= 4) && (val <= 16))
		{
			_pCurProp->addWidth = val;
		}
		else
		{
			if (NLMessageBox(_hInst, _hParent, _T("MsgBox MaxAddCnt"), MB_OK | MB_ICONERROR) == FALSE)
				::MessageBox(_hParent, _T("Only values between 4 and 16 possible."), _T("Hex-Editor"), MB_OK | MB_ICONERROR);
			isChanged = FALSE;
		}
		break;
	}
	case 6:
	{
		ColumnDlg	dlg;
		dlg.init(_hInst, _nppData);
		UINT	val = dlg.doDialogColumn(_pCurProp->columns);
		if ((val > 0) && (val <= (128 / (UINT)_pCurProp->bits)))
		{
			_pCurProp->columns = val;
		}
		else
		{
			if (NLMessageBox(_hInst, _hParent, _T("MsgBox MaxColCnt"), MB_OK | MB_ICONERROR) == FALSE)
				::MessageBox(_hParent, _T("Maximum of 128 bytes can be shown in a row."), _T("Hex-Editor"), MB_OK | MB_ICONERROR);
			isChanged = FALSE;
		}
		break;
	}
	case 7:
	{
		isChanged = ShouldDeleteCompare();

		if (isChanged == TRUE) {
			_pCurProp->firstVisRow = 0;
			_pCurProp->columns = VIEW_ROW;
			_pCurProp->bits = HEX_BYTE;
		}
		break;
	}
	case 8:
	{
		isChanged = ShouldDeleteCompare();

		if (isChanged == TRUE) {
			_pCurProp->firstVisRow = 0;
			if (VIEW_ROW >= HEX_WORD) {
				_pCurProp->columns = VIEW_ROW / HEX_WORD;
			}
			else {
				_pCurProp->columns = 1;
			}
			_pCurProp->bits = HEX_WORD;
		}
		break;
	}
	case 9:
	{
		isChanged = ShouldDeleteCompare();

		if (isChanged == TRUE) {
			_pCurProp->firstVisRow = 0;
			if (VIEW_ROW >= HEX_DWORD) {
				_pCurProp->columns = VIEW_ROW / HEX_DWORD;
			}
			else {
				_pCurProp->columns = 1;
			}
			_pCurProp->bits = HEX_DWORD;
		}
		break;
	}
	case 10:
	{
		isChanged = ShouldDeleteCompare();

		if (isChanged == TRUE) {
			_pCurProp->firstVisRow = 0;
			if (VIEW_ROW >= HEX_LONG) {
				_pCurProp->columns = VIEW_ROW / HEX_LONG;
			}
			else {
				_pCurProp->columns = 1;
			}
			_pCurProp->bits = HEX_LONG;
		}
		break;
	}
	case 11:
	{
		_pCurProp->isBin ^= TRUE;
		break;
	}
	case 12:
	{
		_pCurProp->isLittle ^= TRUE;
		break;
	}
	case 13:
	{
		CopyBinary();
		break;
	}
	case 14:
	{
		PasteBinary();
		break;
	}
	case 15:
	{
		CutBinary();
	}
	default:
		isChanged = FALSE;
		break;
	}

	if (isChanged == TRUE)
	{
		GetLineVis();
		UINT	firstVisRow = _pCurProp->firstVisRow;

		if ((_pCurProp->bits == HEX_BYTE) && (_pCurProp->isLittle == TRUE))
		{
			_pCurProp->isLittle = FALSE;
		}

		UpdateHeader();

		/* correct selection or cursor position */
		if (oldColumns != _pCurProp->columns)
		{
			SetSelection(anchorPos, cursorPos, _pCurProp->selection, cursorPos % VIEW_ROW == 0);
		}

		firstVisRow = ((firstVisRow * oldColumns) / _pCurProp->columns);
		SetLineVis(firstVisRow, eLineVis::HEX_LINE_FIRST);
	}

	::DestroyMenu(hMenu);
	::DestroyMenu(hSubMenu);
}


BOOL HexEdit::ShouldDeleteCompare(void)
{
	if (_pCurProp->pCmpResult != NULL)
	{
		INT		retMsg = FALSE;
		retMsg = NLMessageBox(_hInst, _hParent, _T("MsgBox CompDelete"), MB_YESNO | MB_ICONERROR);
		if (retMsg == FALSE) {
			retMsg = ::MessageBox(_hParent, _T("Compare results will be cleared! Continue?"), _T("Hex-Editor Compare"), MB_YESNO | MB_ICONERROR);
		}
		if (retMsg == IDNO) {
			return FALSE;
		}
		else {
			SetCompareResult(NULL);
		}
	}
	setMenu();
	return TRUE;
}


void HexEdit::Delete(void)
{
	INT		posBeg = 0;
	INT		posItr = 0;
	INT		count = 0;
	UINT	lines = 0;

	GetSelection(&posBeg, &posItr);

	/* get horizontal and vertical gap size */
	if (_pCurProp->selection == eSel::HEX_SEL_BLOCK)
	{
		count = abs(int(_pCurProp->anchorPos - _pCurProp->cursorPos));
		lines = abs(int(_pCurProp->anchorItem - _pCurProp->cursorItem));
	}
	else
	{
		count = abs(posItr - posBeg);
	}

	/* set iterator to begin of selection */
	if (posBeg < posItr) {
		posItr = posBeg;
	}

	SciSubClassWrp::execute(SCI_BEGINUNDOACTION);

	/* replace block with "" */
	for (UINT i = 0; i <= lines; i++)
	{
		if (eError::E_OK != replaceLittleToBig(_hParentHandle, NULL, 0, posItr, count, 0))
		{
			LITTLE_DELETE_ERROR;
			return;
		}
		/* reposition bookmarks */
		UpdateBookmarks(posItr, -count);

		posItr += VIEW_ROW - count;
	}

	/* set cursor position */
	SetPosition(posBeg, _pCurProp->isLittle);

	SciSubClassWrp::execute(SCI_ENDUNDOACTION);
}















void HexEdit::DrawAddressText(HDC hDc, DWORD iItem)
{
	RECT		rc = { 0 };
	SIZE		size = { 0 };
	COLORREF    color = getColor(eColorType::HEX_COLOR_REG_TXT);
	TCHAR		text[17]{};

	/* get list information */
	ListView_GetItemText(_hListCtrl, iItem, 0, text, 17);
	ListView_GetSubItemRect(_hListCtrl, iItem, 0, LVIR_LABEL, &rc);

	/* get size of text */
	::GetTextExtentPoint32(hDc, text, _pCurProp->addWidth, &size);

	for (UINT i = 0; (i < _pCurProp->vBookmarks.size()) && (_pCurProp->vBookmarks[i].iItem <= iItem); i++)
	{
		/* draw bookmark if set */
		if (_pCurProp->vBookmarks[i].iItem == iItem)
		{
			/* draw background */
			HBRUSH	hBrush = ::CreateSolidBrush(getColor(eColorType::HEX_COLOR_REG_BK) ^ getColor(eColorType::HEX_COLOR_BKMK_BK));
			HBRUSH	hOldBrush = (HBRUSH)::SelectObject(hDc, hBrush);
			::PatBlt(hDc, rc.left, rc.top, size.cx, rc.bottom - rc.top, PATINVERT);
			::SelectObject(hDc, hOldBrush);
			::DeleteObject(hBrush);

			/* select different text color */
			color = getColor(eColorType::HEX_COLOR_BKMK_TXT);

			break;
		}
	}

	::SetTextColor(hDc, color);
	::DrawText(hDc, text, _pCurProp->addWidth, &rc, DT_LEFT | DT_HEX_VIEW);
}

















void HexEdit::SelectItem(UINT iItem, UINT iSubItem, INT iCursor)
{
	UINT	editMax = iItem * VIEW_ROW + iSubItem * _pCurProp->bits;

	if ((iItem != -1) && (iSubItem >= 1) && (editMax <= (_currLength + _pCurProp->bits)))
	{
		/* set cursor position and update previous and new row */
		_pCurProp->cursorItem = iItem;
		_pCurProp->cursorSubItem = iSubItem;
		_pCurProp->cursorPos = iCursor;
		_pCurProp->isSel = FALSE;

		if ((editMax == _currLength + _pCurProp->bits) && (_pCurProp->cursorSubItem == 1) &&
			((_currLength / VIEW_ROW) == static_cast<UINT>(ListView_GetItemCount(_hListCtrl))))
		{
			UINT itemCnt = ListView_GetItemCount(_hListCtrl);
			ListView_SetItemCountEx(_hListCtrl, itemCnt + 1, 0);
		}

		/* make item visible */
		EnsureVisible(iItem, iSubItem);
		InvalidateList();
	}
}


void HexEdit::OnMouseClickItem(WPARAM, LPARAM lParam)
{
	LV_HITTESTINFO	info{};

	/* get selected sub item */
	info.pt.x = LOWORD(lParam);
	info.pt.y = HIWORD(lParam);
	ListView_SubItemHitTest(_hListCtrl, &info);

	if (0x80 & ::GetKeyState(VK_SHIFT))
	{
		UINT	posBeg = GetAnchor();
		bool	isMenu = ((0x80 & ::GetKeyState(VK_MENU)) == 0x80);

		SetSelection(posBeg, info.iItem * VIEW_ROW + (info.iSubItem - 1) * _pCurProp->bits + CalcCursorPos(info) / FACTOR, isMenu ? eSel::HEX_SEL_BLOCK : eSel::HEX_SEL_NORM);
	}
	else
	{
		/* set cursor if the same as before */
		UINT	cursor = 0;
		if ((_pCurProp->cursorItem == static_cast<UINT>(info.iItem)) && (_pCurProp->cursorSubItem == static_cast<UINT>(info.iSubItem)) &&
			(_pCurProp->editType == eEdit::HEX_EDIT_HEX))
		{
			cursor = CalcCursorPos(info);
		}

		SelectItem(info.iItem, info.iSubItem, cursor);
	}
}

#pragma warning(push)
#pragma warning(disable: 4702) //unreachable code
BOOL HexEdit::OnKeyDownItem(WPARAM wParam, LPARAM lParam)
{
	bool	isShift = ((0x80 & ::GetKeyState(VK_SHIFT)) == 0x80);
	bool	isCtrl = ((0x80 & ::GetKeyState(VK_CONTROL)) == 0x80);

	/* test if box should be switched into lines or culumns */
	switch (wParam)
	{
	case VK_UP:
	{
		if (_pCurProp->cursorItem != 0)
		{
			if (isShift)
			{
				SelectionKeys(wParam, lParam);
			}
			else
			{
				if (_pCurProp->cursorSubItem == DUMP_FIELD)
					SelectItem(_pCurProp->cursorItem - 1, _pCurProp->cursorSubItem - 1);
				else
					SelectItem(_pCurProp->cursorItem - 1, _pCurProp->cursorSubItem);
			}
		}
		return FALSE;
	}
	case VK_DOWN:
	{
		if (_pCurProp->cursorItem != (_currLength / _pCurProp->columns / _pCurProp->bits))
		{
			if (isShift)
			{
				SelectionKeys(wParam, lParam);
			}
			else
			{
				if (_pCurProp->cursorSubItem == DUMP_FIELD)
					SelectItem(_pCurProp->cursorItem + 1, _pCurProp->cursorSubItem - 1);
				else
					SelectItem(_pCurProp->cursorItem + 1, _pCurProp->cursorSubItem);
			}
		}
		return FALSE;
	}
	case VK_LEFT:
	{
		if (isShift)
		{
			SelectionKeys(wParam, lParam);
		}
		else if (_pCurProp->isSel == TRUE)
		{
			INT posBeg, posEnd;
			GetSelection(&posBeg, &posEnd);
			if (posBeg < posEnd) {
				SetSelection(posBeg, posBeg);
			}
			else {
				SetSelection(posEnd, posEnd);
			}
		}
		else if (isCtrl)
		{
			if (_pCurProp->cursorPos == 0) {
				if ((_pCurProp->cursorSubItem == 1) && (_pCurProp->cursorItem != 0)) {
					SelectItem(_pCurProp->cursorItem - 1, _pCurProp->columns, SUBITEM_LENGTH - FACTOR);
				}
				else {
					SelectItem(_pCurProp->cursorItem, _pCurProp->cursorSubItem - 1, SUBITEM_LENGTH - FACTOR);
				}
			}
			else if (_pCurProp->cursorPos % FACTOR) {
				SelectItem(_pCurProp->cursorItem, _pCurProp->cursorSubItem, _pCurProp->cursorPos - (_pCurProp->cursorPos % FACTOR));
			}
			else {
				SelectItem(_pCurProp->cursorItem, _pCurProp->cursorSubItem, _pCurProp->cursorPos - FACTOR);
			}
		}
		else
		{
			if ((_pCurProp->cursorSubItem == 1) && (_pCurProp->cursorItem != 0)) {
				SelectItem(_pCurProp->cursorItem - 1, _pCurProp->columns);
			}
			else if (_pCurProp->cursorPos != 0) {
				SelectItem(_pCurProp->cursorItem, _pCurProp->cursorSubItem, 0);
			}
			else {
				SelectItem(_pCurProp->cursorItem, _pCurProp->cursorSubItem - 1);
			}
		}
		return FALSE;
	}
	case VK_RIGHT:
	{
		if (isShift && (_onChar == FALSE))
		{
			SelectionKeys(wParam, lParam);
		}
		else if (_pCurProp->isSel == TRUE)
		{
			INT posBeg, posEnd;
			GetSelection(&posBeg, &posEnd);
			if (posBeg < posEnd) {
				SetSelection(posEnd, posEnd);
			}
			else {
				SetSelection(posBeg, posBeg);
			}
		}
		else if (FULL_SUBITEM)
		{
			if (isCtrl)
			{
				if (_pCurProp->cursorPos == (SUBITEM_LENGTH - FACTOR)) {
					if (_pCurProp->cursorSubItem < (UINT)_pCurProp->columns) {
						SelectItem(_pCurProp->cursorItem, _pCurProp->cursorSubItem + 1);
					}
					else {
						SelectItem(_pCurProp->cursorItem + 1, 1);
					}
				}
				else if (_pCurProp->cursorPos % FACTOR) {
					SelectItem(_pCurProp->cursorItem, _pCurProp->cursorSubItem, _pCurProp->cursorPos + (_pCurProp->cursorPos % FACTOR));
				}
				else {
					SelectItem(_pCurProp->cursorItem, _pCurProp->cursorSubItem, _pCurProp->cursorPos + FACTOR);
				}
			}
			else
			{
				if (_pCurProp->cursorSubItem < (UINT)_pCurProp->columns)
					SelectItem(_pCurProp->cursorItem, _pCurProp->cursorSubItem + 1);
				else
					SelectItem(_pCurProp->cursorItem + 1, 1);
			}
		}
		return FALSE;
	}
	default:
		return GlobalKeys(wParam, lParam);
	}

	return TRUE;
}
#pragma warning(pop)


BOOL HexEdit::OnCharItem(WPARAM wParam, LPARAM)
{
	CHAR	text[65];
	UINT	posBeg = 0;
	UINT	textPos = 0;

	if (_pCurProp->isBin == FALSE) {
		if (((wParam < 0x30) || (wParam > 0x66)) ||
			((wParam > 0x39) && (wParam < 0x41)) ||
			((wParam > 0x46) && (wParam < 0x61))) {
			return TRUE;
		}
	}
	else if ((wParam != 0x30) && (wParam != 0x31)) {
		return TRUE;
	}

	/* test if correct char is pressed */
	if (_pCurProp->isSel == TRUE) {
		SetSelection(GetAnchor(), GetAnchor());
	}

	if (_pCurProp->isLittle == FALSE) {
		textPos = _pCurProp->cursorPos / FACTOR;
		posBeg = _pCurProp->cursorItem * VIEW_ROW + (_pCurProp->cursorSubItem - 1) * _pCurProp->bits + textPos;
	}
	else if (FULL_SUBITEM) {
		textPos = (_pCurProp->bits - 1) - _pCurProp->cursorPos / FACTOR;
		posBeg = _pCurProp->cursorItem * VIEW_ROW + (_pCurProp->cursorSubItem - 1) * _pCurProp->bits + textPos;
	}
	else {
		textPos = 0;
		posBeg = _pCurProp->cursorItem * VIEW_ROW + (_pCurProp->cursorSubItem - 1) * _pCurProp->bits;
	}

	/* prepare text */
#ifdef UNICODE
	WCHAR	wText[65]{};
	ListView_GetItemText(_hListCtrl, _pCurProp->cursorItem, _pCurProp->cursorSubItem, wText, SUBITEM_LENGTH);
	wText[_pCurProp->cursorPos] = (TCHAR)wParam;
	::WideCharToMultiByte(CP_ACP, 0, wText, -1, text, SUBITEM_LENGTH, NULL, NULL);
#else
	ListView_GetItemText(_hListCtrl, _pCurProp->cursorItem, _pCurProp->cursorSubItem, text, SUBITEM_LENGTH);
#endif

	if (_currLength > GetCurrentPos()) {
		if (FULL_SUBITEM) {
			BinHexConvert(text, SUBITEM_LENGTH);
		}
		else {
			BinHexConvert(text, (_currLength % _pCurProp->bits) * FACTOR);
		}
		/* calculate correct position */
		SciSubClassWrp::execute(SCI_SETTARGETSTART, posBeg);
		SciSubClassWrp::execute(SCI_SETTARGETEND, posBeg + 1);

		/* update text */
		SciSubClassWrp::execute(SCI_REPLACETARGET, 1, (LPARAM)&text[textPos]);
		SciSubClassWrp::execute(SCI_SETSEL, posBeg, posBeg);
	}
	else {
		memset(&text[_pCurProp->cursorPos + 1], 0x20, FACTOR);
		BinHexConvert(text, ((_currLength % _pCurProp->bits) + 1) * FACTOR);

		SciSubClassWrp::execute(SCI_SETCURRENTPOS, posBeg);
		SciSubClassWrp::execute(SCI_ADDTEXT, 1, (LPARAM)&text[textPos]);
	}

	/* set position */
	_onChar = TRUE;
	_pCurProp->cursorPos++;
	if ((INT)_pCurProp->cursorPos >= SUBITEM_LENGTH) {
		::SendMessage(_hListCtrl, WM_KEYDOWN, VK_RIGHT, 0);
	}
	_onChar = FALSE;

	InvalidateList();
	return FALSE;
	}


void HexEdit::DrawItemText(HDC hDc, DWORD item, INT subItem)
{
	RECT		rc{};
	RECT		rcText;
	SIZE		size;
	TCHAR		text[65]{};
	RECT		rcCursor;

	/* get list informations */
	ListView_GetItemText(_hListCtrl, item, subItem, text, 65);
	ListView_GetSubItemRect(_hListCtrl, item, subItem, LVIR_BOUNDS, &rc);

	/* calculate text begin position */
	::GetTextExtentPoint32(hDc, text, SUBITEM_LENGTH, &size);
	rcText = rc;
	rcText.left += (rc.right - rc.left - size.cx) / 2;
	rcCursor = rcText;
	rcCursor.right = rcCursor.left;

	/* draw normal text */
	DrawPartOfItemText(hDc, rc, rcText, text, 0, SUBITEM_LENGTH, eSelItem::HEX_ITEM_MIDDLE, eSelType::HEX_COLOR_REG);

	/* draw compare highlight */
	if (_pCurProp->pCmpResult != NULL)
	{
		eSelItem	sel = eSelItem::HEX_ITEM_MIDDLE;
		INT			pos = (item * _pCurProp->columns + subItem - 1) - _pCurProp->pCmpResult->offCmpCache;
		if (pos < CACHE_SIZE)
		{
			if (pos == 0) {
				if ((_pCurProp->pCmpResult->cmpCache[0] == TRUE) && (_pCurProp->pCmpResult->cmpCache[1] == FALSE)) {
					DrawPartOfItemText(hDc, rc, rcText, text, 0, SUBITEM_LENGTH, eSelItem::HEX_ITEM_MIDDLE, eSelType::HEX_COLOR_DIFF);
				}
				else if ((_pCurProp->pCmpResult->cmpCache[0] == TRUE) && (_pCurProp->pCmpResult->cmpCache[1] == TRUE)) {
					DrawPartOfItemText(hDc, rc, rcText, text, 0, SUBITEM_LENGTH, eSelItem::HEX_ITEM_FIRST, eSelType::HEX_COLOR_DIFF);
				}
			}
			else if (_pCurProp->pCmpResult->cmpCache[pos] == TRUE) {
				if ((_pCurProp->pCmpResult->cmpCache[pos - 1] == TRUE) && (_pCurProp->pCmpResult->cmpCache[pos + 1] == TRUE)) {
					if (subItem == 1) {
						sel = eSelItem::HEX_ITEM_FIRST;
					}
					else if (subItem == static_cast<INT>(_pCurProp->columns)) {
						sel = eSelItem::HEX_ITEM_LAST;
					}
					else {
						sel = eSelItem::HEX_ITEM_MIDDLE_FULL;
					}
				}
				else if ((_pCurProp->pCmpResult->cmpCache[pos - 1] == FALSE) && (_pCurProp->pCmpResult->cmpCache[pos + 1] == TRUE) && (static_cast<UINT>(subItem) != _pCurProp->columns)) {
					sel = eSelItem::HEX_ITEM_FIRST;
				}
				else if ((_pCurProp->pCmpResult->cmpCache[pos - 1] == TRUE) && (_pCurProp->pCmpResult->cmpCache[pos + 1] == FALSE) && (subItem != 1)) {
					sel = eSelItem::HEX_ITEM_LAST;
				}
				DrawPartOfItemText(hDc, rc, rcText, text, 0, SUBITEM_LENGTH, sel, eSelType::HEX_COLOR_DIFF);
			}
		}
	}

	if (_pCurProp->isSel == FALSE)
	{
		/* paint cursor */
		if (item == _pCurProp->cursorItem)
		{
			if ((_pCurProp->editType == eEdit::HEX_EDIT_HEX) && (subItem == static_cast<INT>(_pCurProp->cursorSubItem))) {
				::GetTextExtentPoint32(hDc, text, _pCurProp->cursorPos, &size);
				rcCursor.left += size.cx;
				::GetTextExtentPoint32(hDc, text, _pCurProp->cursorPos + FACTOR, &size);
				rcCursor.right += size.cx;
				DrawCursor(hDc, rcCursor);
			}
			else if ((_pCurProp->editType == eEdit::HEX_EDIT_ASCII) && ((_pCurProp->cursorPos / _pCurProp->bits) + 1 == static_cast<UINT>(subItem))) {
				::GetTextExtentPoint32(hDc, text, (_pCurProp->cursorPos % _pCurProp->bits) * FACTOR, &size);
				rcCursor.left += size.cx;
				::GetTextExtentPoint32(hDc, text, ((_pCurProp->cursorPos % _pCurProp->bits) * FACTOR) + FACTOR, &size);
				rcCursor.right += size.cx;
				DrawCursor(hDc, rcCursor, TRUE);
			}
		}
	}
	else
	{
		INT		firstCur = _pCurProp->anchorPos;
		INT		lastCur = _pCurProp->cursorPos;
		INT		firstSub = _pCurProp->anchorSubItem;
		INT		lastSub = _pCurProp->cursorSubItem;
		DWORD	firstItem = _pCurProp->anchorItem;
		DWORD	lastItem = _pCurProp->cursorItem;

		/* correct cursor sub item */
		if (firstItem > lastItem)
		{
			/* row of cursor is before row of anchor */
			firstItem = _pCurProp->cursorItem;
			lastItem = _pCurProp->anchorItem;
		}
		if (((_pCurProp->anchorSubItem > _pCurProp->cursorSubItem) && (_pCurProp->selection == eSel::HEX_SEL_BLOCK)) ||
			((_pCurProp->anchorItem > _pCurProp->cursorItem) && (_pCurProp->selection == eSel::HEX_SEL_NORM)) ||
			((_pCurProp->anchorItem == _pCurProp->cursorItem) && (_pCurProp->anchorSubItem > _pCurProp->cursorSubItem) && (_pCurProp->selection == eSel::HEX_SEL_NORM)))
		{
			firstCur = _pCurProp->cursorPos;
			lastCur = _pCurProp->anchorPos;
			firstSub = _pCurProp->cursorSubItem;
			lastSub = _pCurProp->anchorSubItem;
		}
		if ((_pCurProp->anchorSubItem == _pCurProp->cursorSubItem) &&
			(_pCurProp->anchorPos > _pCurProp->cursorPos) &&
			(((_pCurProp->anchorItem == _pCurProp->cursorItem) && (_pCurProp->selection == eSel::HEX_SEL_NORM)) ||
				(_pCurProp->selection == eSel::HEX_SEL_BLOCK)))
		{
			firstCur = _pCurProp->cursorPos;
			lastCur = _pCurProp->anchorPos;
		}

		INT			factor1 = (firstCur % _pCurProp->bits) * FACTOR;
		INT			factor2 = (lastCur % _pCurProp->bits) * FACTOR;

		switch (_pCurProp->selection)
		{
		case eSel::HEX_SEL_NORM:
		{
			if ((firstItem == item) && (lastItem == item))
			{
				/* go to eSel::HEX_SEL_BLOCK */
			}
			else
			{
				if ((firstSub == subItem) && (firstItem == item))
				{
					/* first selected item */
					if (subItem == static_cast<INT>(_pCurProp->columns)) {
						DrawPartOfItemText(hDc, rc, rcText, text, factor1, SUBITEM_LENGTH, eSelItem::HEX_ITEM_MIDDLE, eSelType::HEX_COLOR_SEL);
					}
					else {
						DrawPartOfItemText(hDc, rc, rcText, text, factor1, SUBITEM_LENGTH, eSelItem::HEX_ITEM_FIRST, eSelType::HEX_COLOR_SEL);
					}
				}
				else if ((lastSub == subItem) && (lastItem == item))
				{
					if (((lastCur % _pCurProp->bits) == 0) || (lastSub == 1)) {
						DrawPartOfItemText(hDc, rc, rcText, text, 0, factor2, eSelItem::HEX_ITEM_MIDDLE, eSelType::HEX_COLOR_SEL);
					}
					else {
						DrawPartOfItemText(hDc, rc, rcText, text, 0, factor2, eSelItem::HEX_ITEM_LAST, eSelType::HEX_COLOR_SEL);
					}
				}
				else if (((item == firstItem) && (subItem > firstSub)) ||
					((item > firstItem) && (item < lastItem)) ||
					((item == lastItem) && (subItem < lastSub)))
				{
					if ((subItem == static_cast<INT>(_pCurProp->columns)) ||
						((item == lastItem) && ((lastCur % _pCurProp->bits) == 0) && ((subItem + 1) == lastSub))) {
						if (subItem == 1) {
							DrawPartOfItemText(hDc, rc, rcText, text, 0, SUBITEM_LENGTH, eSelItem::HEX_ITEM_MIDDLE, eSelType::HEX_COLOR_SEL);
						}
						else {
							DrawPartOfItemText(hDc, rc, rcText, text, 0, SUBITEM_LENGTH, eSelItem::HEX_ITEM_LAST, eSelType::HEX_COLOR_SEL);
						}
					}
					else {
						if (subItem == 1) {
							DrawPartOfItemText(hDc, rc, rcText, text, 0, SUBITEM_LENGTH, eSelItem::HEX_ITEM_FIRST, eSelType::HEX_COLOR_SEL);
						}
						else {
							DrawPartOfItemText(hDc, rc, rcText, text, 0, SUBITEM_LENGTH, eSelItem::HEX_ITEM_MIDDLE_FULL, eSelType::HEX_COLOR_SEL);
						}
					}
				}
				break;
			}
		}
		case eSel::HEX_SEL_BLOCK:
		{
			if ((item >= firstItem) && (item <= lastItem))
			{
				if ((firstSub == subItem) && (lastSub == subItem))
				{
					/* drawn only when one item is selected */
					DrawPartOfItemText(hDc, rc, rcText, text, factor1, factor2, eSelItem::HEX_ITEM_MIDDLE, eSelType::HEX_COLOR_SEL);
				}
				else if (firstSub == subItem)
				{
					/* first subitem to be drawn */
					if ((subItem == static_cast<INT>(_pCurProp->columns)) ||
						((firstSub == lastSub - 1) && ((lastCur % _pCurProp->bits) == 0))) {
						DrawPartOfItemText(hDc, rc, rcText, text, factor1, SUBITEM_LENGTH - factor2, eSelItem::HEX_ITEM_MIDDLE, eSelType::HEX_COLOR_SEL);
					}
					else {
						DrawPartOfItemText(hDc, rc, rcText, text, factor1, SUBITEM_LENGTH, eSelItem::HEX_ITEM_FIRST, eSelType::HEX_COLOR_SEL);
					}
				}
				else if ((subItem > firstSub) && (subItem < lastSub))
				{
					/* draw subitems between first and last one */
					if (((lastCur % _pCurProp->bits) == 0) && ((subItem + 1) == lastSub)) {
						DrawPartOfItemText(hDc, rc, rcText, text, 0, SUBITEM_LENGTH, eSelItem::HEX_ITEM_LAST, eSelType::HEX_COLOR_SEL);
					}
					else {
						DrawPartOfItemText(hDc, rc, rcText, text, 0, SUBITEM_LENGTH, eSelItem::HEX_ITEM_MIDDLE_FULL, eSelType::HEX_COLOR_SEL);
					}
				}
				else if ((lastSub == subItem) && ((lastCur % _pCurProp->bits) != 0))
				{
					/* draw last element */
					DrawPartOfItemText(hDc, rc, rcText, text, 0, factor2, eSelItem::HEX_ITEM_LAST, eSelType::HEX_COLOR_SEL);
				}
			}
			break;
		}
		}

		/* paint cursor */
		if ((_pCurProp->editType == eEdit::HEX_EDIT_HEX) && (item == _pCurProp->cursorItem))
		{
			if ((_pCurProp->selection == eSel::HEX_SEL_BLOCK) || (firstItem == lastItem))
			{
				if ((static_cast<UINT>(subItem) == (_pCurProp->cursorSubItem - 1)) &&
					((_pCurProp->cursorPos % _pCurProp->bits) == 0) && (_pCurProp->anchorPos < _pCurProp->cursorPos))
				{
					/* correction to last element */
					::GetTextExtentPoint32(hDc, text, _pCurProp->bits * FACTOR, &size);
					rcCursor.left += size.cx;
					DrawCursor(hDc, rcCursor);
				}
				else if ((subItem == static_cast<INT>(_pCurProp->cursorSubItem)) &&
					(((_pCurProp->cursorPos % _pCurProp->bits) != 0) || (_pCurProp->anchorPos >= _pCurProp->cursorPos)))
				{
					/* standard cursor position */
					::GetTextExtentPoint32(hDc, text, (_pCurProp->cursorPos % _pCurProp->bits) * FACTOR, &size);
					rcCursor.left += size.cx;
					DrawCursor(hDc, rcCursor);
				}
			}
			else
			{
				if (((_pCurProp->cursorPos % _pCurProp->bits) == 0) && (static_cast<UINT>(subItem) == (_pCurProp->cursorSubItem - 1)) &&
					(_pCurProp->anchorItem < _pCurProp->cursorItem))
				{
					/* correction to last element */
					::GetTextExtentPoint32(hDc, text, _pCurProp->bits * FACTOR, &size);
					rcCursor.left += size.cx;
					DrawCursor(hDc, rcCursor);
				}
				else if ((subItem == static_cast<INT>(_pCurProp->cursorSubItem)) &&
					(((_pCurProp->cursorPos % _pCurProp->bits) != 0) || (_pCurProp->anchorItem > _pCurProp->cursorItem)))
				{
					/* standard cursor position */
					::GetTextExtentPoint32(hDc, text, (_pCurProp->cursorPos % _pCurProp->bits) * FACTOR, &size);
					rcCursor.left += size.cx;
					DrawCursor(hDc, rcCursor);
				}
			}
		}
	}
}

void HexEdit::DrawPartOfItemText(HDC hDc, RECT rc, RECT rcText, LPTSTR text, UINT beg, UINT length, eSelItem sel, eSelType type)
{
	SIZE		size = { 0 };
	UINT		diff = 0;
	COLORREF	rgbBk = 0;
	COLORREF	rgbTxt = 0;
	COLORREF	rgbTBkReg = getColor(eColorType::HEX_COLOR_REG_BK);

	switch (type)
	{
	case eSelType::HEX_COLOR_REG:
		rgbTxt = getColor(eColorType::HEX_COLOR_REG_TXT);
		break;
	case eSelType::HEX_COLOR_SEL:
		rgbBk = getColor(eColorType::HEX_COLOR_SEL_BK) ^ rgbTBkReg;
		rgbTxt = getColor(eColorType::HEX_COLOR_SEL_TXT);
		break;
	case eSelType::HEX_COLOR_DIFF:
		rgbBk = getColor(eColorType::HEX_COLOR_DIFF_BK) ^ rgbTBkReg;
		rgbTxt = getColor(eColorType::HEX_COLOR_DIFF_TXT);
		break;
	}

	/* calculate text witdth and background width */
	switch (sel)
	{
	case eSelItem::HEX_ITEM_FIRST:
	{
		/* first subitem to be drawn */
		::GetTextExtentPoint32(hDc, text, beg, &size);
		rcText.left += size.cx;
		rc.left = rcText.left;
		diff = length - beg;
		break;
	}
	case eSelItem::HEX_ITEM_MIDDLE:
	{
		/* drawn only when one item is selected */
		rcText.right = rcText.left;
		::GetTextExtentPoint32(hDc, text, beg, &size);
		rcText.left += size.cx;
		::GetTextExtentPoint32(hDc, text, length, &size);
		rcText.right += size.cx;
		rc = rcText;
		diff = length - beg;
		break;
	}
	case eSelItem::HEX_ITEM_LAST:
	{
		::GetTextExtentPoint32(hDc, text, length, &size);
		rc.right = rcText.left + size.cx;
		diff = length;
		break;
	}
	case eSelItem::HEX_ITEM_MIDDLE_FULL:
	{
		diff = length;
		break;
	}
	default:
		break;
	}

	/* draw background of text */
	if (type != eSelType::HEX_COLOR_REG)
	{
		HBRUSH	hBrush = ::CreateSolidBrush(rgbBk);
		HBRUSH	hOldBrush = (HBRUSH)::SelectObject(hDc, hBrush);
		::PatBlt(hDc, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, PATINVERT);
		::SelectObject(hDc, hOldBrush);
		::DeleteObject(hBrush);
	}

	/* draw text */
	COLORREF rgbOldTxt = ::SetTextColor(hDc, rgbTxt);
	::DrawText(hDc, &text[beg], diff, &rcText, DT_LEFT | DT_HEX_VIEW);
	::SetTextColor(hDc, rgbOldTxt);
}


















void HexEdit::SelectDump(INT iItem, INT iCursor)
{
	UINT	editMax = iItem * VIEW_ROW + iCursor + 1;

	if ((iItem != -1) && (iCursor <= static_cast<INT>(VIEW_ROW)) && (editMax <= _currLength + 1))
	{
		_pCurProp->cursorItem = iItem;
		_pCurProp->cursorSubItem = iCursor / DUMP_FIELD;
		_pCurProp->cursorPos = iCursor;
		_pCurProp->isSel = FALSE;

		if ((editMax == _currLength + 1) && (iCursor == 0) &&
			((_currLength / VIEW_ROW) == static_cast<UINT>(ListView_GetItemCount(_hListCtrl))))
		{
			INT itemCnt = ListView_GetItemCount(_hListCtrl);
			ListView_SetItemCountEx(_hListCtrl, itemCnt + 1, 0);
		}
		EnsureVisible(iItem, DUMP_FIELD);
		InvalidateList();
	}

	/* set position in scintilla */
	if (_pCurProp->isLittle == FALSE) {
		SciSubClassWrp::execute(SCI_GOTOPOS, VIEW_ROW * _pCurProp->cursorItem + _pCurProp->cursorPos);
	}
	else {
		SciSubClassWrp::execute(SCI_GOTOPOS, VIEW_ROW * _pCurProp->cursorItem + _pCurProp->cursorPos +
			(_pCurProp->cursorPos % _pCurProp->bits) +
			((_pCurProp->bits - 1) - (_pCurProp->cursorPos % _pCurProp->bits)));
	}
}


void HexEdit::OnMouseClickDump(WPARAM, LPARAM lParam)
{
	LV_HITTESTINFO	info{};

	/* get selected sub item */
	info.pt.x = LOWORD(lParam);
	info.pt.y = HIWORD(lParam);
	ListView_SubItemHitTest(_hListCtrl, &info);

	/* calc current position */
	UINT	posEnd = info.iItem * VIEW_ROW + CalcCursorPos(info);

	if (0x80 & ::GetKeyState(VK_SHIFT)) {
		UINT	posBeg = GetAnchor();
		bool	isMenu = ((0x80 & ::GetKeyState(VK_MENU)) == 0x80);
		SetSelection(posBeg, posEnd, isMenu ? eSel::HEX_SEL_BLOCK : eSel::HEX_SEL_NORM);
	}
	else {
		SetSelection(posEnd, posEnd);
	}
}

#pragma warning(push)
#pragma warning(disable: 4702) //unreachable code
BOOL HexEdit::OnKeyDownDump(WPARAM wParam, LPARAM lParam)
{
	bool	isShift = ((0x80 & ::GetKeyState(VK_SHIFT)) == 0x80);

	/* test if box should be switched into lines or culumns */
	switch (wParam)
	{
	case VK_UP:
	{
		if (_pCurProp->cursorItem != 0)
		{
			if (isShift)
			{
				SelectionKeys(wParam, lParam);
			}
			else
			{
				SelectDump(_pCurProp->cursorItem - 1, _pCurProp->cursorPos);
			}
		}
		return FALSE;
	}
	case VK_DOWN:
	{
		if (_pCurProp->cursorItem != (_currLength / _pCurProp->columns / _pCurProp->bits))
		{
			if (isShift)
			{
				SelectionKeys(wParam, lParam);
			}
			else
			{
				SelectDump(_pCurProp->cursorItem + 1, _pCurProp->cursorPos);
			}
		}
		return FALSE;
	}
	case VK_LEFT:
	{
		if (isShift)
		{
			SelectionKeys(wParam, lParam);
		}
		else if (_pCurProp->isSel == TRUE)
		{
			INT	posBeg, posEnd;
			GetSelection(&posBeg, &posEnd);
			if (posBeg < posEnd) {
				SelectDump(_pCurProp->cursorItem, _pCurProp->anchorPos);
			}
			else {
				SelectDump(_pCurProp->cursorItem, _pCurProp->cursorPos);
			}
		}
		else if (_pCurProp->cursorPos == 0)
		{
			if (_pCurProp->cursorItem != 0)
				SelectDump(_pCurProp->cursorItem - 1, VIEW_ROW - 1);
		}
		else
		{
			SelectDump(_pCurProp->cursorItem, _pCurProp->cursorPos - 1);
		}
		return FALSE;
	}
	case VK_RIGHT:
	{
		if ((_pCurProp->cursorItem * VIEW_ROW + _pCurProp->cursorPos) <= _currLength)
		{
			if (isShift && (_onChar == FALSE))
			{
				SelectionKeys(wParam, lParam);
			}
			else if (_pCurProp->isSel == TRUE)
			{
				INT	posBeg, posEnd;
				GetSelection(&posBeg, &posEnd);
				if (posBeg < posEnd) {
					SelectDump(_pCurProp->cursorItem, _pCurProp->cursorPos);
				}
				else {
					SelectDump(_pCurProp->cursorItem, _pCurProp->anchorPos);
				}
			}
			else if (_pCurProp->cursorPos < (UINT)(VIEW_ROW - 1))
			{
				SelectDump(_pCurProp->cursorItem, _pCurProp->cursorPos + 1);
			}
			else
			{
				SelectDump(_pCurProp->cursorItem + 1, 0);
			}
		}
		return FALSE;
	}
	default:
		return GlobalKeys(wParam, lParam);

	}
	return TRUE;
}
#pragma warning(pop)


BOOL HexEdit::OnCharDump(WPARAM wParam, LPARAM lParam)
{
#ifdef UNICODE
	WCHAR	wText = (WCHAR)wParam;
	::WideCharToMultiByte(CP_ACP, 0, (LPTSTR)&wText, -1, (LPSTR)&wParam, 1, NULL, NULL);
#endif

	switch (wParam)
	{
	case 0x08:		/* Back     */
	case 0x09:		/* TAB		*/
	{
		break;
	}
	case 0x10:		/* Shift	*/
	{
		return GlobalKeys(wParam, lParam);
	}
	default:
	{
		if (_pCurProp->isSel == TRUE) {
			SetSelection(GetAnchor(), GetAnchor());
		}

		/* calculate correct position */
		UINT posBeg = 0;

		if (_pCurProp->isLittle == FALSE) {
			posBeg = VIEW_ROW * _pCurProp->cursorItem + _pCurProp->cursorPos;
		}
		else {
			posBeg = VIEW_ROW * _pCurProp->cursorItem + _pCurProp->cursorPos -
				(_pCurProp->cursorPos % _pCurProp->bits) +
				((_pCurProp->bits - 1) - (_pCurProp->cursorPos % _pCurProp->bits));
		}

		if (_currLength != GetCurrentPos()) {
			/* update text */
			SciSubClassWrp::execute(SCI_SETTARGETSTART, posBeg);
			SciSubClassWrp::execute(SCI_SETTARGETEND, posBeg + 1);
			SciSubClassWrp::execute(SCI_REPLACETARGET, 1, (LPARAM)&wParam);
			SciSubClassWrp::execute(SCI_SETSEL, posBeg + 1, posBeg + 1);
		}
		else {
			/* add char */
			if (_pCurProp->isLittle == TRUE) {
				SciSubClassWrp::execute(SCI_SETCURRENTPOS, _currLength - (_currLength % _pCurProp->bits));
			}
			SciSubClassWrp::execute(SCI_ADDTEXT, 1, (LPARAM)&wParam);
		}

		_onChar = TRUE;
		_pCurProp->cursorPos++;
		if (_pCurProp->cursorPos >= (UINT)VIEW_ROW) {
			::SendMessage(_hListCtrl, WM_KEYDOWN, VK_RIGHT, 0);
		}
		_onChar = FALSE;

		InvalidateList();
		return FALSE;
	}
	}
	return TRUE;
}

void HexEdit::DrawDumpText(HDC hDc, DWORD item, INT subItem)
{
	RECT		rc{};
	TCHAR		text[129]{};
	RECT		rcCursor = { 0 };
	SIZE		size = { 0 };
	UINT		diff = VIEW_ROW;

	/* get list informations */
	ListView_GetItemText(_hListCtrl, item, subItem, text, 129);
	ListView_GetSubItemRect(_hListCtrl, item, subItem, LVIR_BOUNDS, &rc);

	/* calculate cursor start position */
	rc.left += 6;
	rcCursor = rc;
	rcCursor.right = rcCursor.left;

	/* draw normal text */
	if (static_cast<UINT>(ListView_GetItemCount(_hListCtrl)) == (item + 1)) {
		diff = _currLength - (item * VIEW_ROW);
		memset(&text[diff], 0x20, 129 - diff);
	}
	DrawPartOfDumpText(hDc, rc, text, 0, diff, eSelType::HEX_COLOR_REG);

	/* draw compare highlight */
	if (_pCurProp->pCmpResult != NULL)
	{
		INT pos = (item * _pCurProp->columns) - _pCurProp->pCmpResult->offCmpCache;
		if (pos < CACHE_SIZE)
		{
			for (UINT i = 0; i < _pCurProp->columns && (pos < (CACHE_SIZE - 1)); i++, pos++) {
				if ((_pCurProp->pCmpResult->cmpCache[pos] == TRUE) &&
					(pos < _pCurProp->pCmpResult->lenCmpCache)) {
					DrawPartOfDumpText(hDc, rc, text, i * _pCurProp->bits, _pCurProp->bits, eSelType::HEX_COLOR_DIFF);
				}
			}
		}
	}

	/* draw selection */
	if (_pCurProp->isSel == FALSE)
	{
		if (item == _pCurProp->cursorItem) {
			UINT	posCurBeg = 0;
			if (_pCurProp->editType == eEdit::HEX_EDIT_ASCII) {
				posCurBeg += _pCurProp->cursorPos;
			}
			else {
				posCurBeg += ((_pCurProp->cursorSubItem - 1) * _pCurProp->bits + _pCurProp->cursorPos / FACTOR);
			}
			::GetTextExtentPoint32(hDc, text, posCurBeg, &size);
			rcCursor.left += size.cx;
			::GetTextExtentPoint32(hDc, text, posCurBeg + 1, &size);
			rcCursor.right += size.cx;
			DrawCursor(hDc, rcCursor, _pCurProp->editType == eEdit::HEX_EDIT_HEX);
		}
	}
	else
	{
		INT		length = 0;
		INT		begText = 0;
		DWORD	firstItem = _pCurProp->anchorItem;
		DWORD	lastItem = _pCurProp->cursorItem;
		INT		firstCur = _pCurProp->anchorPos;
		INT		lastCur = _pCurProp->cursorPos - 1;

		/* change first item last item sequence */
		if (firstItem > lastItem)
		{
			firstItem = _pCurProp->cursorItem;
			lastItem = _pCurProp->anchorItem;
		}
		/* change cursor sequence */
		if (((_pCurProp->selection == eSel::HEX_SEL_BLOCK) && ((_pCurProp->anchorSubItem > _pCurProp->cursorSubItem) ||
			((_pCurProp->anchorSubItem == _pCurProp->cursorSubItem) && (_pCurProp->anchorPos > _pCurProp->cursorPos)))) ||
			(((_pCurProp->anchorItem > _pCurProp->cursorItem) && (_pCurProp->selection == eSel::HEX_SEL_NORM)) ||
				((_pCurProp->anchorPos > _pCurProp->cursorPos) && (_pCurProp->anchorItem == _pCurProp->cursorItem) && (_pCurProp->selection == eSel::HEX_SEL_NORM))))
		{
			firstCur = _pCurProp->cursorPos;
			lastCur = _pCurProp->anchorPos - 1;
		}

		if ((firstItem <= item) && (lastItem >= item))
		{
			switch (_pCurProp->selection)
			{
			case eSel::HEX_SEL_NORM:
			{
				if (firstItem == lastItem)
				{
					/* go to eSel::HEX_SEL_BLOCK */
				}
				else
				{
					if (firstItem == item)
					{
						/* calculate length and selection of string */
						length = VIEW_ROW - firstCur;
						begText = firstCur;
					}
					else if ((item > firstItem) && (item < lastItem))
					{
						/* calculate length and selection of string */
						length = VIEW_ROW;
					}
					else if (item == lastItem)
					{
						/* calculate length and selection of string */
						length = lastCur;
						length += (length < static_cast<INT>(VIEW_ROW)) ? 1 : 0;
					}
					break;
				}
			}
			case eSel::HEX_SEL_VERTICAL:
			case eSel::HEX_SEL_HORIZONTAL:
			case eSel::HEX_SEL_BLOCK:
			{
				/* calculate length and selection of string */
				length = lastCur - firstCur;
				length += (firstCur + length < static_cast<INT>(VIEW_ROW)) ? 1 : 0;
				begText = firstCur;
				break;
			}
			}

			DrawPartOfDumpText(hDc, rc, text, begText, length, eSelType::HEX_COLOR_SEL);

			/* paint cursor */
			if ((item == _pCurProp->cursorItem) && (_pCurProp->editType == eEdit::HEX_EDIT_ASCII)) {
				::GetTextExtentPoint32(hDc, text, _pCurProp->cursorPos, &size);
				rcCursor.left += size.cx;
				DrawCursor(hDc, rcCursor);
			}
		}
	}
}


void HexEdit::DrawPartOfDumpText(HDC hDc, RECT rc, LPTSTR text, UINT beg, UINT length, eSelType type)
{
	SIZE		size = { 0 };
	COLORREF	rgbBk = 0;
	COLORREF	rgbTxt = 0;
	COLORREF	rgbTBkReg = getColor(eColorType::HEX_COLOR_REG_BK);

	switch (type)
	{
	case eSelType::HEX_COLOR_REG:
		rgbTxt = getColor(eColorType::HEX_COLOR_REG_TXT);
		break;
	case eSelType::HEX_COLOR_SEL:
		rgbBk = getColor(eColorType::HEX_COLOR_SEL_BK) ^ rgbTBkReg;
		rgbTxt = getColor(eColorType::HEX_COLOR_SEL_TXT);
		break;
	case eSelType::HEX_COLOR_DIFF:
		rgbBk = getColor(eColorType::HEX_COLOR_DIFF_BK) ^ rgbTBkReg;
		rgbTxt = getColor(eColorType::HEX_COLOR_DIFF_TXT);
		break;
	}

	/* calculate text witdth */
	rc.right = rc.left;
	::GetTextExtentPoint32(hDc, text, beg, &size);
	rc.left += size.cx;
	::GetTextExtentPoint32(hDc, text, beg + length, &size);
	rc.right += size.cx;

	/* draw background of text */
	if (type != eSelType::HEX_COLOR_REG)
	{
		HBRUSH	hBrush = ::CreateSolidBrush(rgbBk);
		HBRUSH	hOldBrush = (HBRUSH)::SelectObject(hDc, hBrush);
		::PatBlt(hDc, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, PATINVERT);
		::SelectObject(hDc, hOldBrush);
		::DeleteObject(hBrush);
	}

	/* draw text */
	COLORREF rgbOldTxt = ::SetTextColor(hDc, rgbTxt);
	::DrawText(hDc, &text[beg], length, &rc, DT_LEFT | DT_HEX_VIEW);
	::SetTextColor(hDc, rgbOldTxt);
}










INT HexEdit::CalcCursorPos(LV_HITTESTINFO info)
{
	RECT			rc{};
	SIZE			size;
	TCHAR			text[128]{};
	UINT			cursorPos;

	/* get dc for calculate the font size */
	HDC				hDc = ::GetDC(_hListCtrl);

	/* for zooming */
	SelectObject(hDc, _hFont);

	/* get item informations */
	ListView_GetItemText(_hListCtrl, info.iItem, info.iSubItem, text, 128);
	ListView_GetSubItemRect(_hListCtrl, info.iItem, info.iSubItem, LVIR_BOUNDS, &rc);

	if (info.iSubItem == static_cast<int>(DUMP_FIELD))
	{
		/* left offset */
		rc.left += 6;

		/* get clicked position */
		for (cursorPos = 0; cursorPos < (UINT)VIEW_ROW; cursorPos++) {
			::GetTextExtentPoint32(hDc, &text[cursorPos], 1, &size);
			if (info.pt.x < (rc.left + (size.cx / 2))) {
				break;
			}
			rc.left += size.cx;
		}

		if (cursorPos >= (UINT)VIEW_ROW)
		{
			if (_pCurProp->isSel == TRUE) {
				cursorPos = VIEW_ROW;
			}
			else {
				cursorPos = VIEW_ROW - 1;
			}
		}
	}
	else
	{
		/* left offset */
		::GetTextExtentPoint32(hDc, text, SUBITEM_LENGTH, &size);
		rc.left += ((rc.right - rc.left) - size.cx) / 2;

		/* get clicked position */
		for (cursorPos = 0; cursorPos < (UINT)VIEW_ROW; cursorPos++) {
			::GetTextExtentPoint32(hDc, &text[cursorPos], 1, &size);
			if (info.pt.x < (rc.left + (size.cx / 2))) {
				break;
			}
			rc.left += size.cx;
		}

		/* test if  */
		if (FULL_SUBITEM)
		{
			if (cursorPos >= (UINT)SUBITEM_LENGTH)
			{
				if (_pCurProp->isSel == TRUE)
					cursorPos = SUBITEM_LENGTH;
				else
					cursorPos = SUBITEM_LENGTH - FACTOR;
			}
		}
		else
		{
			UINT	maxPos = (_currLength % _pCurProp->bits) * FACTOR;

			if (cursorPos >= maxPos)
			{
				cursorPos = (maxPos == 0) ? 0 : maxPos;
			}
			else if (_pCurProp->isSel == TRUE)
			{
				cursorPos++;
			}
		}
		if (cursorPos < 0)
		{
			cursorPos = 0;
		}

	}
	::ReleaseDC(_hListCtrl, hDc);

	return cursorPos;
}


BOOL HexEdit::GlobalKeys(WPARAM wParam, LPARAM)
{
	INT		posBeg;
	INT		posEnd;
	bool	isShift = ((0x80 & ::GetKeyState(VK_SHIFT)) == 0x80);
	eSel	selection = ((0x80 & ::GetKeyState(VK_MENU)) == 0x80 ? eSel::HEX_SEL_BLOCK : eSel::HEX_SEL_NORM);

	switch (wParam)
	{
	case VK_PRIOR:
	case VK_NEXT:
	{
		DisableCursor();

		if (isShift == false)
		{
			if (wParam == VK_NEXT)
			{
				UINT    itemCount = GetItemCount();
				_pCurProp->cursorItem += ListView_GetCountPerPage(_hListCtrl);
				if (_pCurProp->cursorItem >= itemCount)
				{
					_pCurProp->cursorItem = itemCount;
					if (GetCurrentPos() > _currLength)
						SetPosition(_currLength);
				}
			}
			else
			{
				_pCurProp->cursorItem -= ListView_GetCountPerPage(_hListCtrl);
				if ((INT)_pCurProp->cursorItem < 0) {
					_pCurProp->cursorItem = 0;
				}
			}
			SetLineVis(_pCurProp->cursorItem, eLineVis::HEX_LINE_MIDDLE);
			_pCurProp->isSel = FALSE;
			InvalidateList();
		}
		else
		{
			if (wParam == VK_NEXT)
			{
				GetSelection(&posBeg, &posEnd);
				posEnd += ListView_GetCountPerPage(_hListCtrl) * VIEW_ROW;
				if ((UINT)posEnd > _currLength)
					posEnd = _currLength;
				SetSelection(posBeg, posEnd, selection, _pCurProp->cursorSubItem == DUMP_FIELD);
			}
			else
			{
				GetSelection(&posBeg, &posEnd);
				posEnd -= ListView_GetCountPerPage(_hListCtrl) * VIEW_ROW;
				if (posEnd < 0)
					posEnd = 0;
				SetSelection(posBeg, posEnd, selection, _pCurProp->cursorSubItem == DUMP_FIELD);
			}
			SetLineVis(_pCurProp->cursorItem, eLineVis::HEX_LINE_MIDDLE);
		}
		break;
	}
	case VK_HOME:
	{
		if (isShift == false)
		{
			if (0x80 & ::GetKeyState(VK_CONTROL))
			{
				SetPosition(0);
			}
			else
			{
				if (_pCurProp->editType == eEdit::HEX_EDIT_HEX)
				{
					SelectItem(_pCurProp->cursorItem, 1);
				}
				else
					SelectDump(_pCurProp->cursorItem, 0);
			}
		}
		else
		{
			if (0x80 & ::GetKeyState(VK_CONTROL))
			{
				GetSelection(&posBeg, &posEnd);
				SetLineVis(0, eLineVis::HEX_LINE_FIRST);
				SetSelection(posBeg, 0, selection);
			}
			else
			{
				GetSelection(&posBeg, &posEnd);
				SetSelection(posBeg, _pCurProp->cursorItem * VIEW_ROW, selection, _pCurProp->anchorItem < _pCurProp->cursorItem);
			}
		}
		break;
	}
	case VK_END:
	{
		if (isShift == false)
		{
			if (0x80 & ::GetKeyState(VK_CONTROL))
			{
				SetPosition(_currLength);
			}
			else
			{
				/* set correct end position */
				if (_pCurProp->editType == eEdit::HEX_EDIT_HEX)
				{
					if ((_pCurProp->cursorItem != GetItemCount()) ||
						((_currLength % VIEW_ROW) == 0))
						SelectItem(_pCurProp->cursorItem, _pCurProp->columns, SUBITEM_LENGTH - FACTOR);
					else
						SelectItem(_pCurProp->cursorItem, (_currLength % VIEW_ROW) / _pCurProp->bits + 1, (_currLength % _pCurProp->bits) * FACTOR);
				}
				else
				{
					if ((_pCurProp->cursorItem != GetItemCount()) ||
						((_currLength % VIEW_ROW) == 0))
						SelectDump(_pCurProp->cursorItem, VIEW_ROW - 1);
					else
						SelectDump(_pCurProp->cursorItem, _currLength % VIEW_ROW);
				}
			}
		}
		else
		{
			if (0x80 & ::GetKeyState(VK_CONTROL))
			{
				GetSelection(&posBeg, &posEnd);
				SetLineVis(ListView_GetItemCount(_hListCtrl) - 1, eLineVis::HEX_LINE_LAST);
				SetSelection(posBeg, _currLength, selection, TRUE);
			}
			else
			{
				GetSelection(&posBeg, &posEnd);
				SetSelection(posBeg, (_pCurProp->cursorItem + 1) * VIEW_ROW, selection, _pCurProp->anchorItem <= _pCurProp->cursorItem);
			}
		}
		break;
	}
	case VK_BACK:
		if ((_pCurProp->editType == eEdit::HEX_EDIT_HEX) && (_pCurProp->isSel == TRUE)) {
			::SendMessage(_hListCtrl, WM_KEYDOWN, VK_LEFT, 0);
		}
		::SendMessage(_hListCtrl, WM_KEYDOWN, VK_LEFT, 0);
		break;
	default:
		return TRUE;
	}
	return FALSE;
}


void HexEdit::SelectionKeys(WPARAM wParam, LPARAM)
{
	INT		posBeg;
	INT		posEnd;
	bool	isMenu = ((0x80 & ::GetKeyState(VK_MENU)) == 0x80);
	bool	isCtrl = ((0x80 & ::GetKeyState(VK_CONTROL)) == 0x80);
	eSel	selection = (isMenu ? eSel::HEX_SEL_BLOCK : eSel::HEX_SEL_NORM);

	switch (wParam)
	{
	case VK_UP:
	{
		if (_pCurProp->cursorItem > 0)
		{
			GetSelection(&posBeg, &posEnd);
			if ((_pCurProp->anchorItem == _pCurProp->cursorItem) && (_pCurProp->cursorSubItem == DUMP_FIELD)) {
				_pCurProp->cursorSubItem = 1;
				_pCurProp->cursorPos = 0;
				UpdateListChanges();
			}
			else {
				SetSelection(posBeg, posEnd - VIEW_ROW, selection, _pCurProp->cursorSubItem == DUMP_FIELD);
			}
			SetLineVis(_pCurProp->cursorItem, eLineVis::HEX_LINE_MIDDLE);
		}
		break;
	}
	case VK_DOWN:
	{
		if (_pCurProp->cursorItem < (UINT)ListView_GetItemCount(_hListCtrl))
		{
			GetSelection(&posBeg, &posEnd);
			posEnd += VIEW_ROW;
			SetSelection(posBeg, posEnd, selection, (_pCurProp->cursorSubItem == DUMP_FIELD) ||
				(((posEnd % VIEW_ROW) == 0) && (_pCurProp->anchorItem == _pCurProp->cursorItem)));
			SetLineVis(_pCurProp->cursorItem, eLineVis::HEX_LINE_MIDDLE);
		}
		break;
	}
	case VK_LEFT:
	{
		if ((_pCurProp->cursorSubItem == 1) && (_pCurProp->cursorPos == 0) && isMenu)
		{
			/* nothing to do */
		}
		else
		{
			GetSelection(&posBeg, &posEnd);
			if (isCtrl || (_pCurProp->editType == eEdit::HEX_EDIT_ASCII)) {
				posEnd--;
				SetSelection(posBeg, posEnd, selection, ((posEnd % VIEW_ROW) == 0) && (posBeg < posEnd));
			}
			else {
				if (posEnd % _pCurProp->bits) {
					posEnd -= (posEnd % _pCurProp->bits);
				}
				else {
					posEnd -= _pCurProp->bits;
				}
				SetSelection(posBeg, posEnd, selection, ((posEnd % VIEW_ROW) == 0) && (posBeg < posEnd));
			}
			SetLineVis(_pCurProp->cursorItem, eLineVis::HEX_LINE_MIDDLE);
		}
		break;
	}
	case VK_RIGHT:
	{
		if (((_pCurProp->cursorSubItem == _pCurProp->columns) || (_pCurProp->cursorSubItem == DUMP_FIELD)) &&
			((_pCurProp->cursorPos == VIEW_ROW - 1) || (_pCurProp->cursorPos == VIEW_ROW)) && isMenu)
		{
			if (_pCurProp->cursorPos == VIEW_ROW - 1)
			{
				_pCurProp->cursorSubItem++;
				_pCurProp->cursorPos++;
				UpdateListChanges();
			}
		}
		else
		{
			GetSelection(&posBeg, &posEnd);
			if (isCtrl || (_pCurProp->editType == eEdit::HEX_EDIT_ASCII)) {
				posEnd++;
				SetSelection(posBeg, posEnd, selection, (posEnd % VIEW_ROW) == 0);
			}
			else {
				posEnd -= (posEnd % _pCurProp->bits);
				posEnd += _pCurProp->bits;
				SetSelection(posBeg, posEnd, selection, (posEnd % VIEW_ROW) == 0);
			}
			SetLineVis(_pCurProp->cursorItem, eLineVis::HEX_LINE_MIDDLE);
		}
		break;
	}
	default:
		break;
	}
}


void HexEdit::SetPosition(UINT pos, BOOL isLittle)
{
	if (isLittle == FALSE)
	{
		if (_pCurProp->editType == eEdit::HEX_EDIT_HEX) {
			SelectItem(pos / VIEW_ROW, ((pos % VIEW_ROW) / _pCurProp->bits) + 1, (pos % _pCurProp->bits) * FACTOR);
		}
		else {
			SelectDump(pos / VIEW_ROW, pos % VIEW_ROW);
		}
	}
	else
	{
		if (_pCurProp->editType == eEdit::HEX_EDIT_HEX) {
			SelectItem(pos / VIEW_ROW, ((pos % VIEW_ROW) / _pCurProp->bits) + 1, (_pCurProp->bits) - ((pos % _pCurProp->bits) - 1) * FACTOR);
		}
		else {
			SelectDump(pos / VIEW_ROW, (pos % VIEW_ROW) - 2 * (pos % _pCurProp->bits) + (_pCurProp->bits - 1));
		}
	}
}


UINT HexEdit::GetCurrentPos(void)
{
	if ((_pCurProp->editType == eEdit::HEX_EDIT_ASCII) || (_pCurProp->isSel == TRUE)) {
		if (_pCurProp->cursorSubItem == DUMP_FIELD) {
			return (_pCurProp->cursorItem + 1) * VIEW_ROW;
		}
		else {
			return _pCurProp->cursorItem * VIEW_ROW + _pCurProp->cursorPos;
		}
	}
	else {
		return _pCurProp->cursorItem * VIEW_ROW + (_pCurProp->cursorSubItem - 1) * _pCurProp->bits + _pCurProp->cursorPos / FACTOR;
	}
}


UINT HexEdit::GetAnchor(void)
{
	if ((_pCurProp->editType == eEdit::HEX_EDIT_ASCII) || (_pCurProp->isSel == TRUE)) {
		return _pCurProp->anchorItem * VIEW_ROW + _pCurProp->anchorPos;
	}
	else {
		return _pCurProp->anchorItem * VIEW_ROW + (_pCurProp->anchorSubItem - 1) * _pCurProp->bits + _pCurProp->anchorPos / FACTOR;
	}
}


void HexEdit::GetSelection(LPINT posBegin, LPINT posEnd)
{
	*posBegin = GetAnchor();
	*posEnd = GetCurrentPos();
}


void HexEdit::SetSelection(UINT posBegin, UINT posEnd, eSel selection, BOOL isEND)
{
	if ((posBegin <= _currLength) && (posEnd <= _currLength) &&
		(posBegin >= 0) && (posEnd >= 0))
	{
		UINT	modPosBegin = posBegin % VIEW_ROW;
		UINT	modPosEnd = posEnd % VIEW_ROW;

		_pCurProp->isSel = posBegin == posEnd ? FALSE : TRUE;
		_pCurProp->selection = selection;
		_pCurProp->anchorItem = posBegin / VIEW_ROW;
		_pCurProp->cursorItem = posEnd / VIEW_ROW;
		_pCurProp->anchorSubItem = (modPosBegin / _pCurProp->bits) + 1;
		_pCurProp->cursorSubItem = (modPosEnd / _pCurProp->bits) + 1;

		if ((_pCurProp->editType == eEdit::HEX_EDIT_ASCII) || (_pCurProp->isSel == TRUE)) {
			_pCurProp->anchorPos = modPosBegin;
			_pCurProp->cursorPos = modPosEnd;
		}
		else {
			_pCurProp->anchorPos = (posBegin % _pCurProp->bits) * FACTOR;
			_pCurProp->cursorPos = (posEnd % _pCurProp->bits) * FACTOR;
		}

		/* correction for corsor view */
		if ((_pCurProp->cursorPos == 0) && (isEND == TRUE)) {
			_pCurProp->cursorSubItem = DUMP_FIELD;
			_pCurProp->cursorPos = VIEW_ROW;
			_pCurProp->cursorItem--;
		}

		UpdateListChanges();
	}
}


void HexEdit::SetLineVis(UINT line, eLineVis mode)
{
	switch (mode)
	{
	case eLineVis::HEX_LINE_FIRST:
		ListView_EnsureVisible(_hListCtrl, (_currLength / _pCurProp->columns / _pCurProp->bits) - 1, FALSE);
		ListView_EnsureVisible(_hListCtrl, line, FALSE);
		break;
	case eLineVis::HEX_LINE_MIDDLE:
		ListView_EnsureVisible(_hListCtrl, line, FALSE);
		break;
	case eLineVis::HEX_LINE_LAST:
		ListView_EnsureVisible(_hListCtrl, 0, FALSE);
		ListView_EnsureVisible(_hListCtrl, line, FALSE);
		break;
	default:
		OutputDebugString(_T("Mode Unknown\n"));
		return;
	}
	GetLineVis();
}


void HexEdit::GetLineVis(void)
{
	if (_pCurProp != NULL)
	{
		if (_pCurProp->isVisible == TRUE)
		{
			_pCurProp->firstVisRow = ListView_GetTopIndex(_hListCtrl);
		}
	}
}


void HexEdit::NextBookmark(void)
{
	UINT	vecSize = (UINT)_pCurProp->vBookmarks.size();

	if (vecSize == 0)
		return;

	LONG	lAdrs = _pCurProp->vBookmarks[0].lAddress;

	_pCurProp->cursorPos = 0;
	if (_pCurProp->editType == eEdit::HEX_EDIT_HEX)
		_pCurProp->cursorSubItem = 1;

	for (UINT i = 0; i < vecSize; i++)
	{
		if (_pCurProp->vBookmarks[i].iItem > _pCurProp->cursorItem)
		{
			lAdrs = _pCurProp->vBookmarks[i].lAddress;
			break;
		}
	}
	SetSelection((UINT)lAdrs, (UINT)lAdrs);
	EnsureVisible(_pCurProp->cursorItem, _pCurProp->cursorSubItem);
	RestartCursor();
}

void HexEdit::PrevBookmark(void)
{
	UINT	vecSize = (UINT)_pCurProp->vBookmarks.size();

	if (vecSize == 0)
		return;

	LONG	lAdrs = _pCurProp->vBookmarks[vecSize - 1].lAddress;

	_pCurProp->cursorPos = 0;
	if (_pCurProp->editType == eEdit::HEX_EDIT_HEX)
		_pCurProp->cursorSubItem = 1;

	for (INT i = (INT)_pCurProp->vBookmarks.size() - 1; i >= 0; i--)
	{
		if (_pCurProp->vBookmarks[i].iItem < _pCurProp->cursorItem)
		{
			lAdrs = _pCurProp->vBookmarks[i].lAddress;
			break;
		}
	}
	SetSelection((UINT)lAdrs, (UINT)lAdrs);
	EnsureVisible(_pCurProp->cursorItem, _pCurProp->cursorSubItem);
	RestartCursor();
}

void HexEdit::ToggleBookmark(void)
{
	ToggleBookmark(_pCurProp->cursorItem);
}

void HexEdit::ToggleBookmark(UINT iItem)
{
	UINT	isChanged = FALSE;

	for (UINT i = 0; (i < _pCurProp->vBookmarks.size()) && (isChanged == FALSE); i++)
	{
		if (_pCurProp->vBookmarks[i].iItem == iItem) {
			/* if bookmark exists delete it from list */
			_pCurProp->vBookmarks.erase(_pCurProp->vBookmarks.begin() + i);
			isChanged = TRUE;
		}
		else if (_pCurProp->vBookmarks[i].iItem > iItem) {
			/* if bookmark dosn't exist on this position attach it and sort list */
			tBkMk	bm = { (LONG)(iItem * VIEW_ROW), iItem };
			_pCurProp->vBookmarks.push_back(bm);
			QuickSortRecursive(0, (INT)_pCurProp->vBookmarks.size() - 1);
			isChanged = TRUE;
		}
	}

	if (isChanged == FALSE)
	{
		/* bookmark wasn't found, attach it to list */
		tBkMk	bm = { (LONG)(iItem * VIEW_ROW), iItem };
		_pCurProp->vBookmarks.push_back(bm);
	}

	/* redraw bookmark */
	RECT	rc{};
	ListView_GetSubItemRect(_hListCtrl, iItem, 0, LVIR_BOUNDS, &rc);
	::RedrawWindow(_hListCtrl, &rc, NULL, TRUE);
}

void HexEdit::UpdateBookmarks(UINT firstAdd, INT length)
{
	if ((_pCurProp->vBookmarks.size() == 0) && (length != 0))
		return;

	RECT	rcOld = { 0 };
	RECT	rcNew = { 0 };

	for (UINT i = 0; i < _pCurProp->vBookmarks.size(); i++)
	{
		LONG	addressTest = _pCurProp->vBookmarks[i].lAddress;
		UINT	iItemTest = _pCurProp->vBookmarks[i].iItem;
		if (static_cast<UINT>(_pCurProp->vBookmarks[i].lAddress) >= firstAdd)
		{
			/* get old item position */
			ListView_GetSubItemRect(_hListCtrl, _pCurProp->vBookmarks[i].iItem, 0, LVIR_BOUNDS, &rcOld);

			if ((static_cast<UINT>(_pCurProp->vBookmarks[i].lAddress) == firstAdd) && (length < 0)) {
				/* remove bookmark if is in a delete section */
				_pCurProp->vBookmarks.erase(_pCurProp->vBookmarks.begin() + i);
				i--;
			}
			else if (static_cast<UINT>(_pCurProp->vBookmarks[i].lAddress) > firstAdd) {
				/* calculate new addresses of bookmarks behind the first address */
				_pCurProp->vBookmarks[i].lAddress += length;
				_pCurProp->vBookmarks[i].iItem = _pCurProp->vBookmarks[i].lAddress / VIEW_ROW;

				addressTest = _pCurProp->vBookmarks[i].lAddress;
				iItemTest = _pCurProp->vBookmarks[i].iItem;

				/* if some data was deleted and the changed item matches with the privious one delete it */
				if ((i != 0) && (_pCurProp->vBookmarks[i].lAddress == _pCurProp->vBookmarks[i - 1].lAddress)) {
					_pCurProp->vBookmarks.erase((_pCurProp->vBookmarks.begin() + i));
					i--;
				}

				/* redraw new item */
				ListView_GetSubItemRect(_hListCtrl, _pCurProp->vBookmarks[i].iItem, 0, LVIR_BOUNDS, &rcNew);
				::RedrawWindow(_hListCtrl, &rcNew, NULL, TRUE);
			}
			/* redraw old sub item */
			::RedrawWindow(_hListCtrl, &rcOld, NULL, TRUE);
		}
	}
}

void HexEdit::ClearBookmarks(void)
{
	if (_pCurProp->vBookmarks.size() == 0)
		return;

	/* clear list and redraw window */
	_pCurProp->vBookmarks.clear();
	::RedrawWindow(_hListCtrl, NULL, NULL, TRUE);
}

void HexEdit::CutBookmarkLines(void)
{
	if (_pCurProp->vBookmarks.size() == 0)
		return;

	HWND		hSciTgt = NULL;
	INT			deleted = 0;
	INT			offset = 0;
	INT			length = 0;
	tClipboard	clipboard{};

	/* set selection */
	clipboard.selection = eSel::HEX_SEL_BLOCK;

	/* copy data into scintilla handle (encoded if necessary) */
	hSciTgt = (HWND)::SendMessage(_hParent, NPPM_CREATESCINTILLAHANDLE, 0, NULL);

	SciSubClassWrp::execute(SCI_BEGINUNDOACTION);

	/* get length and initialize clipboard */
	clipboard.items = (UINT)_pCurProp->vBookmarks.size();
	clipboard.stride = VIEW_ROW;
	clipboard.length = clipboard.items * VIEW_ROW;
	clipboard.text = (LPSTR)new CHAR[clipboard.length + 1];
	if (clipboard.text != NULL)
	{
		/* cut and replace line with "" */
		for (UINT i = 0; i < clipboard.items; i++)
		{
			offset = _pCurProp->vBookmarks[i].iItem * VIEW_ROW - deleted;
			length = VIEW_ROW;
			if (LittleEndianChange(hSciTgt, _hParentHandle, &offset, &length) == TRUE)
			{
				::SendMessage(hSciTgt, SCI_SETSEL, 0, (LPARAM)length);
				::SendMessage(hSciTgt, SCI_GETSELTEXT, 0, (LPARAM)&clipboard.text[i * VIEW_ROW]);
				SciSubClassWrp::execute(SCI_SETSEL, offset, (LPARAM)offset + length);
				SciSubClassWrp::execute(SCI_REPLACESEL, 0, (LPARAM)"\0");
			}
			deleted += VIEW_ROW;
		}
	}
	else
	{
		::MessageBox(_hParent, _T("Couldn't create memory."), _T("Hex-Editor"), MB_OK | MB_ICONERROR);
	}

	/* destory scintilla handle */
	CleanScintillaBuf(hSciTgt);
	::SendMessage(_hParent, NPPM_DESTROYSCINTILLAHANDLE, 0, (LPARAM)hSciTgt);

	/* convert to hex if usefull */
	if (clipboard.text != NULL)
	{
		if (_pCurProp->editType == eEdit::HEX_EDIT_HEX)
		{
			tClipboard	data = clipboard;
			ChangeClipboardDataToHex(&data);
			/* store selected text in scintilla clipboard */
			SciSubClassWrp::execute(SCI_COPYTEXT, data.length + 1, (LPARAM)data.text);
			delete[] data.text;
		}
		else
		{
			/* store selected text in scintilla clipboard */
			SciSubClassWrp::execute(SCI_COPYTEXT, clipboard.length + 1, (LPARAM)clipboard.text);
		}

		/* delete old text and store to clipboard */
		delete[] g_clipboard.text;
		g_clipboard = clipboard;
	}
	SciSubClassWrp::execute(SCI_ENDUNDOACTION);

	ClearBookmarks();
}

void HexEdit::CopyBookmarkLines(void)
{
	if (_pCurProp->vBookmarks.size() == 0)
		return;

	HWND		hSciTgt = NULL;
	INT			offset = 0;
	INT			length = 0;
	tClipboard	clipboard{};

	/* set selection */
	clipboard.selection = eSel::HEX_SEL_BLOCK;

	/* copy data into scintilla handle (encoded if necessary) */
	hSciTgt = (HWND)::SendMessage(_hParent, NPPM_CREATESCINTILLAHANDLE, 0, NULL);

	/* get length and initialize clipboard */
	clipboard.items = (UINT)_pCurProp->vBookmarks.size();
	clipboard.stride = VIEW_ROW;
	clipboard.length = clipboard.items * VIEW_ROW;
	clipboard.text = (LPSTR)new CHAR[clipboard.length + 1];
	if (clipboard.text != NULL)
	{
		/* cut and replace line with "" */
		for (UINT i = 0; i < clipboard.items; i++)
		{
			offset = _pCurProp->vBookmarks[i].iItem * VIEW_ROW;
			length = VIEW_ROW;
			if (LittleEndianChange(hSciTgt, _hParentHandle, &offset, &length) == TRUE)
			{
				::SendMessage(hSciTgt, SCI_SETSEL, 0, (LPARAM)length);
				::SendMessage(hSciTgt, SCI_GETSELTEXT, 0, (LPARAM)&clipboard.text[i * VIEW_ROW]);
			}
		}
	}
	else
	{
		::MessageBox(_hParent, _T("Couldn't create memory."), _T("Hex-Editor"), MB_OK | MB_ICONERROR);
	}

	/* destory scintilla handle */
	CleanScintillaBuf(hSciTgt);
	::SendMessage(_hParent, NPPM_DESTROYSCINTILLAHANDLE, 0, (LPARAM)hSciTgt);

	/* convert to hex if usefull */
	if (clipboard.text != NULL)
	{
		if (_pCurProp->editType == eEdit::HEX_EDIT_HEX)
		{
			tClipboard	data = clipboard;
			ChangeClipboardDataToHex(&data);
			/* store selected text in scintilla clipboard */
			SciSubClassWrp::execute(SCI_COPYTEXT, data.length + 1, (LPARAM)data.text);
			delete[] data.text;
		}
		else
		{
			/* store selected text in scintilla clipboard */
			SciSubClassWrp::execute(SCI_COPYTEXT, clipboard.length + 1, (LPARAM)clipboard.text);
		}

		/* delete old text and store to clipboard */
		delete[] g_clipboard.text;
		g_clipboard = clipboard;
	}
}

void HexEdit::PasteBookmarkLines(void)
{
	if (_pCurProp->vBookmarks.size() == 0)
		return;

	HWND	hSciTgt = NULL;
	INT		posBeg = 0;
	INT		posEnd = 0;
	INT		length = 0;

	SciSubClassWrp::execute(SCI_BEGINUNDOACTION);

	if (g_clipboard.text == NULL)
	{
		/* copy data into scintilla handle (encoded if necessary) */
		hSciTgt = (HWND)::SendMessage(_hParent, NPPM_CREATESCINTILLAHANDLE, 0, (LPARAM)_hSelf);
		ScintillaMsg(hSciTgt, SCI_PASTE);

		length = (INT)::SendMessage(hSciTgt, SCI_GETLENGTH, 0, 0);
	}
	else
	{
		if (g_clipboard.stride != VIEW_ROW)
		{
			if (NLMessageBox(_hInst, _hParent, _T("MsgBox SameWidth"), MB_OK | MB_ICONERROR) == FALSE)
				MessageBox(_hSelf, _T("Clipboard info has not the same width!"), _T("Hex-Editor Error"), MB_OK | MB_ICONERROR);
			return;
		}
		else
		{
			/* copy data into scintilla handle (encoded if necessary) */
			hSciTgt = (HWND)::SendMessage(_hParent, NPPM_CREATESCINTILLAHANDLE, 0, (LPARAM)_hSelf);
			::SendMessage(hSciTgt, SCI_ADDTEXT, g_clipboard.length, (LPARAM)g_clipboard.text);
			length = g_clipboard.length;
		}
	}

	/* replace line with clipboard information */
	for (UINT i = 0; i < _pCurProp->vBookmarks.size(); i++)
	{
		/* copy into target scintilla */
		posBeg = _pCurProp->vBookmarks[i].iItem * VIEW_ROW;
		posEnd = posBeg + VIEW_ROW;

		/* no test necessary because of same stride */
		replaceLittleToBig(_hParentHandle, hSciTgt, 0, posBeg, VIEW_ROW, length);
		UpdateBookmarks(posBeg, length - VIEW_ROW);
	}

	/* destory scintilla handle */
	CleanScintillaBuf(hSciTgt);
	::SendMessage(_hParent, NPPM_DESTROYSCINTILLAHANDLE, 0, (LPARAM)hSciTgt);

	SciSubClassWrp::execute(SCI_ENDUNDOACTION);
}

void HexEdit::DeleteBookmarkLines(void)
{
	if (_pCurProp->vBookmarks.size() == 0)
		return;

	UINT	posBeg = 0;
	UINT	posEnd = 0;
	UINT	deleted = 0;

	SciSubClassWrp::execute(SCI_BEGINUNDOACTION);

	/* replace line with "" */
	for (UINT i = 0; i < _pCurProp->vBookmarks.size(); i++)
	{
		posBeg = _pCurProp->vBookmarks[i].iItem * VIEW_ROW - deleted;
		posEnd = posBeg + VIEW_ROW;
		SciSubClassWrp::execute(SCI_SETSEL, posBeg, (LPARAM)posEnd);
		SciSubClassWrp::execute(SCI_REPLACESEL, 0, (LPARAM)"\0");
		deleted += VIEW_ROW;
	}

	SciSubClassWrp::execute(SCI_ENDUNDOACTION);

	ClearBookmarks();
}


void HexEdit::SetStatusBar(void)
{
#ifdef ENABLE_FLICKERING_STATUSBAR_WITHOUT_51
	if (_pCurProp && _pCurProp->isVisible == TRUE)
	{
		TCHAR buffer[64] = { 0 };

		/* set mode */
		::SendMessage(_hParent, NPPM_SETSTATUSBAR, STATUSBAR_DOC_TYPE, (LPARAM)_T("Hex Edit View"));
		/* set doc length */
		_stprintf(buffer, _T("nb char : %d"), _currLength);
		::SendMessage(_hParent, NPPM_SETSTATUSBAR, STATUSBAR_DOC_SIZE, (LPARAM)buffer);

		/* set doc length */
		_stprintf(buffer, _T("Ln : %d    Col : %d    Sel : %d"),
			_pCurProp->cursorItem + 1,
			(GetCurrentPos() % VIEW_ROW) + 1,
			(GetCurrentPos() > GetAnchor() ? GetCurrentPos() - GetAnchor() : GetAnchor() - GetCurrentPos()));
		::SendMessage(_hParent, NPPM_SETSTATUSBAR, STATUSBAR_CUR_POS, (LPARAM)buffer);

		/* display information in which mode it is (binary or hex) */
		::SendMessage(_hParent, NPPM_SETSTATUSBAR, STATUSBAR_EOF_FORMAT, (LPARAM)(_pCurProp->isBin == FALSE ? _T("Hex") : _T("Binary")));

		/* display information in which mode it is (BigEndian or Little) */
		::SendMessage(_hParent, NPPM_SETSTATUSBAR, STATUSBAR_UNICODE_TYPE, (LPARAM)(_pCurProp->isLittle == FALSE ? _T("BigEndian") : _T("LittleEndian")));
	}
#endif
}


void HexEdit::ConvertSelNppToHEX(void)
{
	if (_pCurProp == NULL)
		return;

	extern	UINT	currentSC;
	UINT	selStart = (UINT)SciSubClassWrp::execute(SCI_GETSELECTIONSTART);
	UINT	selEnd = (UINT)SciSubClassWrp::execute(SCI_GETSELECTIONEND);
	UINT	offset = 0;

	_currLength = (UINT)SciSubClassWrp::execute(SCI_GETLENGTH, 0, 0);

	if ((_pCurProp->isLittle == FALSE) || (_pCurProp->bits == HEX_BYTE))
	{
		switch (_pCurProp->codePage)
		{
		case eNppCoding::HEX_CODE_NPP_UTF8_BOM:
		{
			::SendMessage(_nppData._nppHandle, NPPM_DECODESCI, currentSC, 0);
			::SendMessage(_nppData._nppHandle, WM_COMMAND, IDM_FORMAT_ANSI, 0);
			SetSelection(selStart + 3, selEnd + 3, eSel::HEX_SEL_NORM, (selEnd + 3) % VIEW_ROW == 0);
			break;
		}
		case eNppCoding::HEX_CODE_NPP_USCBE:
		case eNppCoding::HEX_CODE_NPP_USCLE:
		{
			UINT	posStart = 2;
			UINT	posEnd = 2;
			UINT	curPos = 0;

			while ((curPos <= selStart) || (curPos <= selEnd)) {
				BYTE byte = (BYTE)SciSubClassWrp::execute(SCI_GETCHARAT, curPos);
				if ((byte & 0x80) == 0) curPos++;
				else if ((byte & 0xF8) == 0xF0) curPos += 4;
				else if ((byte & 0xF0) == 0xE0) curPos += 3;
				else if ((byte & 0xE0) == 0xC0) curPos += 2;
				if (curPos <= selStart) posStart += 2;
				if (curPos <= selEnd) posEnd += 2;
			}
			::SendMessage(_nppData._nppHandle, NPPM_DECODESCI, currentSC, 0);
			::SendMessage(_nppData._nppHandle, WM_COMMAND, IDM_FORMAT_ANSI, 0);
			SetSelection(posStart, posEnd, eSel::HEX_SEL_NORM, posEnd % VIEW_ROW == 0);
			break;
		}
		case eNppCoding::HEX_CODE_NPP_ASCI:
		case eNppCoding::HEX_CODE_NPP_UTF8:
		default:
		{
			SetSelection(selStart, selEnd, eSel::HEX_SEL_NORM, selEnd % VIEW_ROW == 0);
			break;
		}
		}

		_pCurProp->firstVisRow = _pCurProp->cursorItem;
	}
	else
	{
		if (selStart > selEnd) {
			selStart = selEnd;
		}

		switch (_pCurProp->codePage)
		{
		case eNppCoding::HEX_CODE_NPP_UTF8_BOM:
		{
			::SendMessage(_nppData._nppHandle, NPPM_DECODESCI, currentSC, 0);
			::SendMessage(_nppData._nppHandle, WM_COMMAND, IDM_FORMAT_ANSI, 0);

			selStart += 3;
			break;
		}
		case eNppCoding::HEX_CODE_NPP_USCBE:
		case eNppCoding::HEX_CODE_NPP_USCLE:
		{
			UINT	posStart = 2;
			UINT	curPos = 0;

			while (curPos <= selStart) {
				BYTE byte = (BYTE)SciSubClassWrp::execute(SCI_GETCHARAT, curPos);
				if ((byte & 0x80) == 0) curPos++;
				else if ((byte & 0xF8) == 0xF0) curPos += 4;
				else if ((byte & 0xF0) == 0xE0) curPos += 3;
				else if ((byte & 0xE0) == 0xC0) curPos += 2;
				if (curPos <= selStart) posStart += 2;
			}
			::SendMessage(_nppData._nppHandle, NPPM_DECODESCI, currentSC, 0);
			::SendMessage(_nppData._nppHandle, WM_COMMAND, IDM_FORMAT_ANSI, 0);

			selStart = posStart;
			break;
		}
		case eNppCoding::HEX_CODE_NPP_ASCI:
		case eNppCoding::HEX_CODE_NPP_UTF8:
		default:
		{
			break;
		}
		}
		offset = selStart % _pCurProp->bits;
		selStart = (selStart - offset) + (_pCurProp->bits - offset - 1);
		SetSelection(selStart, selStart);
	}
}

void HexEdit::ConvertSelHEXToNpp(void)
{
	if (_pCurProp == NULL)
		return;

	extern	UINT	currentSC;
	INT		selStart = GetAnchor();
	INT		selEnd = GetCurrentPos();
	INT		offset = 0;

	UniMode	um = (UniMode)::SendMessage(_nppData._nppHandle, NPPM_ENCODESCI, currentSC, 0);

	if ((_pCurProp->isLittle == FALSE) || (_pCurProp->bits == HEX_BYTE))
	{
		switch (um)
		{
		case UniMode::uniUTF8:
		{
			SciSubClassWrp::execute(SCI_SETSEL,
				SciSubClassWrp::execute(SCI_POSITIONBEFORE, SciSubClassWrp::execute(SCI_POSITIONAFTER, selStart - 3)),
				SciSubClassWrp::execute(SCI_POSITIONAFTER, SciSubClassWrp::execute(SCI_POSITIONBEFORE, selEnd - 3)));
			break;
		}
		case UniMode::uni16BE:
		case UniMode::uni16LE:
		{
			UINT	posStart = 0;
			UINT	posEnd = 0;
			UINT	curPos = 0;
			UINT	addPos = 0;

			selStart = selStart / 2 - 1;
			selEnd = selEnd / 2 - 1;

			while ((selStart > 0) || (selEnd > 0)) {
				BYTE byte = (BYTE)SciSubClassWrp::execute(SCI_GETCHARAT, curPos);
				if ((byte & 0x80) == 0) curPos++, addPos = 1;
				else if ((byte & 0xF8) == 0xF0) curPos += 4, addPos = 4;
				else if ((byte & 0xF0) == 0xE0) curPos += 3, addPos = 3;
				else if ((byte & 0xE0) == 0xC0) curPos += 2, addPos = 2;
				if (selStart > 0) posStart += addPos, selStart--;
				if (selEnd > 0) posEnd += addPos, selEnd--;
			}
			SciSubClassWrp::execute(SCI_SETSEL, posStart, posEnd);
			break;
		}
		case UniMode::uni8Bit:
		case UniMode::uniCookie:
		default:
		{
			SciSubClassWrp::execute(SCI_SETSEL, selStart, selEnd);
			break;
		}
		}
	}
	else
	{
		if (selStart > selEnd) {
			selStart = selEnd;
		}

		offset = selStart % _pCurProp->bits;
		selStart = (selStart - offset) + (_pCurProp->bits - offset - 1);

		switch (um)
		{
		case UniMode::uniUTF8:
		{
			SciSubClassWrp::execute(SCI_SETSEL,
				SciSubClassWrp::execute(SCI_POSITIONBEFORE, SciSubClassWrp::execute(SCI_POSITIONAFTER, selStart - 3)),
				SciSubClassWrp::execute(SCI_POSITIONBEFORE, SciSubClassWrp::execute(SCI_POSITIONAFTER, selStart - 3)));
			break;
		}
		case UniMode::uni16BE:
		case UniMode::uni16LE:
		{
			UINT	posStart = 0;
			UINT	curPos = 0;
			UINT	addPos = 0;

			selStart = selStart / 2 - 1;

			while (selStart > 0) {
				BYTE byte = (BYTE)SciSubClassWrp::execute(SCI_GETCHARAT, curPos);
				if ((byte & 0x80) == 0) curPos++, addPos = 1;
				else if ((byte & 0xF8) == 0xF0) curPos += 4, addPos = 4;
				else if ((byte & 0xF0) == 0xE0) curPos += 3, addPos = 3;
				else if ((byte & 0xE0) == 0xC0) curPos += 2, addPos = 2;
				if (selStart > 0) posStart += addPos, selStart--;
			}
			SciSubClassWrp::execute(SCI_SETSEL, posStart, posStart);
			break;
		}
		case UniMode::uni8Bit:
		case UniMode::uniCookie:
		default:
		{
			SciSubClassWrp::execute(SCI_SETSEL, selStart, selStart);
			break;
		}
		}
	}
}
////Code taken from NPP code, and modified to match hex-editor Copy
//https://github.com/notepad-plus-plus/notepad-plus-plus/blob/master/PowerEditor/src/ScintillaComponent/ScintillaEditView.cpp
void HexEdit::getText(char* dest, size_t start, size_t end) const
{
	Sci_TextRange tr{};
	tr.chrg.cpMin = static_cast<Sci_PositionCR>(start);
	tr.chrg.cpMax = static_cast<Sci_PositionCR>(end);
	tr.lpstrText = dest;
	execute(SCI_GETTEXTRANGE, 0, reinterpret_cast<LPARAM>(&tr));
}

char* HexEdit::getWordFromRange(char* txt, size_t size, size_t pos1, size_t pos2)
{
	if (!size)
		return NULL;
	if (pos1 > pos2)
	{
		size_t tmp = pos1;
		pos1 = pos2;
		pos2 = tmp;
	}

	if (size < pos2 - pos1)
		return NULL;

	getText(txt, pos1, pos2);
	return txt;
}

bool HexEdit::expandWordSelection()
{
	pair<size_t, size_t> wordRange = getWordRange();
	if (wordRange.first != wordRange.second)
	{
		SciSubClassWrp::execute(SCI_SETSELECTIONSTART, wordRange.first);
		SciSubClassWrp::execute(SCI_SETSELECTIONEND, wordRange.second);
		return true;
	}
	return false;
}

pair<size_t, size_t> HexEdit::getWordRange()
{
	size_t caretPos = SciSubClassWrp::execute(SCI_GETCURRENTPOS, 0, 0);
	size_t startPos = SciSubClassWrp::execute(SCI_WORDSTARTPOSITION, caretPos, true);
	size_t endPos = SciSubClassWrp::execute(SCI_WORDENDPOSITION, caretPos, true);
	return pair<size_t, size_t>(startPos, endPos);
}

char* HexEdit::getSelectedText(char* txt, size_t size, bool expand)
{
	if (!size)
		return NULL;
	Sci_CharacterRange range = getSelection();
	if (range.cpMax == range.cpMin && expand)
	{
		expandWordSelection();
		range = getSelection();
	}
	if (!(static_cast<Sci_PositionCR>(size) > (range.cpMax - range.cpMin)))	//there must be atleast 1 byte left for zero terminator
	{
		range.cpMax = range.cpMin + (Sci_PositionCR)size - 1;	//keep room for zero terminator
	}
	return getWordFromRange(txt, size, range.cpMin, range.cpMax);
}

void HexEdit::PasteBinary(void)
{
	//std::lock_guard<std::mutex> lock(command_mutex);
	if (!IsClipboardFormatAvailable(CF_TEXT))
		return;

	if (!OpenClipboard(NULL))
		return;

	HGLOBAL hglb = GetClipboardData(CF_TEXT);
	if (hglb != NULL)
	{
		char* lpchar = (char*)GlobalLock(hglb);
		if (lpchar != NULL)
		{
			UINT cf_nppTextLen = RegisterClipboardFormat(CF_NPPTEXTLEN);
			if (IsClipboardFormatAvailable(cf_nppTextLen))
			{
				HGLOBAL hglbLen = GetClipboardData(cf_nppTextLen);
				if (hglbLen != NULL)
				{
					unsigned long* lpLen = (unsigned long*)GlobalLock(hglbLen);
					if (lpLen != NULL)
					{
						execute(SCI_REPLACESEL, 0, reinterpret_cast<LPARAM>(""));
						execute(SCI_ADDTEXT, *lpLen, reinterpret_cast<LPARAM>(lpchar));

						GlobalUnlock(hglbLen);
					}
				}
			}
			else
			{
				execute(SCI_REPLACESEL, 0, reinterpret_cast<LPARAM>(lpchar));
			}
			GlobalUnlock(hglb);
		}
	}
	CloseClipboard();
}
void HexEdit::CutBinary()
{
	CopyBinary();
	Delete();
}

void HexEdit::CopyBinary(void)
{

	HWND		hSciTgt = nullptr;
	INT			offset = 0;
	INT			textLen = 0;
	INT			posBeg = 0;
	INT			posEnd = 0;

	tClipboard	clipboard = { 0 };
	size_t selectionStart = execute(SCI_GETSELECTIONSTART);
	size_t selectionEnd = execute(SCI_GETSELECTIONEND);

	int32_t selectionLen = static_cast<int32_t>(selectionEnd - selectionStart);
	hSciTgt = (HWND)::SendMessage(_hParent, NPPM_CREATESCINTILLAHANDLE, 0, (LPARAM)_hSelf);

	GetSelection(&posBeg, &posEnd);
	offset = posBeg;
	textLen = CalcStride(posBeg, posEnd);

	clipboard.length = textLen;
	clipboard.text = (char*)new char[textLen + 1];
	if (clipboard.text != NULL) {
		if (LittleEndianChange(hSciTgt, _hParentHandle, &offset, &textLen) == TRUE) {
			SciSubClassWrp::execute(SCI_SETSEL, posBeg, (LPARAM)posBeg + textLen);
		}
	}

	textLen = static_cast<int32_t>(SciSubClassWrp::execute(SCI_GETSELTEXT, 0, 0)) - 1;
	if (!textLen)
		return;

	char* pBinText = new char[textLen + 1];
	getSelectedText(pBinText, textLen + 1, false);

	// Open the clipboard, and empty it.
	if (!OpenClipboard(NULL))
		return;
	EmptyClipboard();

	// Allocate a global memory object for the text.
	HGLOBAL hglbCopy = GlobalAlloc(GMEM_MOVEABLE, (textLen + 1) * sizeof(unsigned char));
	if (hglbCopy == NULL)
	{
		CloseClipboard();
		return;
	}

	// Lock the handle and copy the text to the buffer.
	unsigned char* lpucharCopy = (unsigned char*)GlobalLock(hglbCopy);
	memcpy(lpucharCopy, pBinText, textLen * sizeof(unsigned char));
	lpucharCopy[textLen] = 0;    // null character

	GlobalUnlock(hglbCopy);

	// Place the handle on the clipboard.
	SetClipboardData(CF_TEXT, hglbCopy);


	// Allocate a global memory object for the text length.
	HGLOBAL hglbLenCopy = GlobalAlloc(GMEM_MOVEABLE, sizeof(unsigned long));
	if (hglbLenCopy == NULL)
	{
		CloseClipboard();
		return;
	}

	// Lock the handle and copy the text to the buffer.
	unsigned long* lpLenCopy = (unsigned long*)GlobalLock(hglbLenCopy);
	*lpLenCopy = textLen;

	GlobalUnlock(hglbLenCopy);

	// Place the handle on the clipboard.
	UINT f = RegisterClipboardFormat(CF_NPPTEXTLEN);
	SetClipboardData(f, hglbLenCopy);

	CloseClipboard();

}
