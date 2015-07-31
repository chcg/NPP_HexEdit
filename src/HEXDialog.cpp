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

#include "PluginInterface.h"
#include "HEXDialog.h"
#include "ColumnDialog.h"
#include "HEXResource.h"
#include "Scintilla.h"
#include "resource.h"

#include <windows.h>
#include <commctrl.h>


#define FACTOR			((_pCurProp->isBin == TRUE)?8:2)
#define	SUBITEM_LENGTH	(_pCurProp->bits * FACTOR)
#define FULL_SUBITEM	((_pCurProp->cursorItem * VIEW_ROW + (_pCurProp->cursorSubItem * _pCurProp->bits)) <= _currLength)
#define SELECTION_COLOR	(::CreateSolidBrush(RGB(0x88,0x88,0xff)))
#define DUMP_FIELD		(_pCurProp->columns + 1)


extern char	hexMask[256][3];



HexEdit::HexEdit(void)
{
	_hListCtrl			= NULL;
	_pCurProp			= NULL;
	_hDefaultParentProc	= NULL;
	_iFontSize			= 16;
	_isCurOn			= FALSE;
	_openDoc			= -1;
	_isLBtnDown			= FALSE;
	_isRBtnDown			= FALSE;
	_isWheel			= FALSE;
}

HexEdit::~HexEdit(void)
{
}


void HexEdit::init(HINSTANCE hInst, NppData nppData, LPCTSTR iniFilePath)
{
	_nppData = nppData;
	Window::init(hInst, nppData._nppHandle);
	strcpy(_iniFilePath, iniFilePath);

    if (!isCreated())
	{
        create(IDD_HEX_DLG);
//		::SendMessage(_hParent, NPPM_MODELESSDIALOG, MODELESSDIALOGADD, (LPARAM)_hSelf);
	}
}


BOOL CALLBACK HexEdit::run_dlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message) 
	{
		case WM_INITDIALOG:
		{
			_hListCtrl	= ::GetDlgItem(_hSelf, IDC_HEX_LIST);
			SetFontSize(_iFontSize);

			/* intial subclassing for key mapping */
			::SetWindowLong(_hListCtrl, GWL_USERDATA, reinterpret_cast<LONG>(this));
			_hDefaultListProc = reinterpret_cast<WNDPROC>(::SetWindowLong(_hListCtrl, GWL_WNDPROC, reinterpret_cast<LONG>(wndListProc)));
			ListView_SetExtendedListViewStyleEx(_hListCtrl, LVS_EX_ONECLICKACTIVATE, LVS_EX_ONECLICKACTIVATE);
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
					_isCurOn = TRUE;
					::SetTimer(_hListCtrl, IDC_HEX_CURSORTIMER, 500, cursorFunc);
					::SetFocus(_hListCtrl);
				}

				/* check menu and tb icon */
				checkMenu(_pCurProp->isVisible);
			}
			else
			{
				::KillTimer(_hListCtrl, IDC_HEX_CURSORTIMER);
				_isCurOn = FALSE;
			}
			ListView_RedrawItems(_hListCtrl, _pCurProp->cursorItem, _pCurProp->cursorItem);
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
			::SetWindowPos(::GetDlgItem(_hSelf, IDC_HEX_LIST), NULL, 
						   rc.left, rc.top, rc.right, rc.bottom, 
						   SWP_NOZORDER | SWP_SHOWWINDOW);
			break;
		}
		case WM_NOTIFY:
		{
			switch (((LPNMHDR)lParam)->code)
			{
				case LVN_GETDISPINFO:
				{
					static char text[129] = "\0";
					LV_ITEM &lvItem = reinterpret_cast<LV_DISPINFO*>((LV_DISPINFO FAR *)lParam)->item;

					if (lvItem.mask & LVIF_TEXT)
					{
						ReadArrayToList(text, lvItem.iItem ,lvItem.iSubItem);
						lvItem.pszText = text;
					}
					return TRUE;
				}
				case NM_CUSTOMDRAW:
				{
					LPNMLVCUSTOMDRAW lpCD = (LPNMLVCUSTOMDRAW)lParam;

					switch (lpCD->nmcd.dwDrawStage)
					{
						case CDDS_PREPAINT:
							SetWindowLong(_hSelf, DWL_MSGRESULT, (LONG)(CDRF_NOTIFYITEMDRAW));
							return TRUE;

						case CDDS_ITEMPREPAINT:
							SetWindowLong(_hSelf, DWL_MSGRESULT, (LONG)(CDRF_NOTIFYSUBITEMDRAW));
							return TRUE;

						case CDDS_ITEMPREPAINT | CDDS_SUBITEM:
						{
							UINT	iItem	 = _pCurProp->anchorItem;

							if ((_pCurProp->isSel == FALSE) && (lpCD->nmcd.dwItemSpec == iItem))
							{
								UINT	iSubItem = _pCurProp->anchorSubItem;

								if ((lpCD->iSubItem >= 1) && (lpCD->iSubItem <= _pCurProp->columns))
								{
									/* draw text */
									DrawItemText(lpCD->nmcd.hdc, (DWORD)lpCD->nmcd.dwItemSpec, lpCD->iSubItem);
									SetWindowLong(_hSelf, DWL_MSGRESULT, (LONG)(CDRF_SKIPDEFAULT));
									return TRUE;
								}
								if (lpCD->iSubItem == DUMP_FIELD)
								{
									/* draw text */
									DrawDumpText(lpCD->nmcd.hdc, (DWORD)lpCD->nmcd.dwItemSpec, lpCD->iSubItem);
									SetWindowLong(_hSelf, DWL_MSGRESULT, (LONG)(CDRF_SKIPDEFAULT));
									return TRUE;
								}
							}
							else if (_pCurProp->isSel == TRUE)
							{
								UINT	firstItem	= 0;
								UINT	lastItem	= 0;

								if (_pCurProp->anchorItem <= _pCurProp->cursorItem)
								{
									firstItem	= _pCurProp->anchorItem;
									lastItem	= _pCurProp->cursorItem;
								}
								else
								{
									firstItem	= _pCurProp->cursorItem;
									lastItem	= _pCurProp->anchorItem;
								}

								if ((lpCD->iSubItem >= 1) && (lpCD->iSubItem <= _pCurProp->columns) &&
									(lpCD->nmcd.dwItemSpec >= firstItem) && (lpCD->nmcd.dwItemSpec <= lastItem))
								{
									/* draw text */
									DrawItemText(lpCD->nmcd.hdc, (DWORD)lpCD->nmcd.dwItemSpec, lpCD->iSubItem);
									SetWindowLong(_hSelf, DWL_MSGRESULT, (LONG)(CDRF_SKIPDEFAULT));
									return TRUE;
								}
								if ((lpCD->iSubItem == DUMP_FIELD) && 
									(lpCD->nmcd.dwItemSpec >= firstItem) && (lpCD->nmcd.dwItemSpec <= lastItem))
								{
									/* draw text */
									DrawDumpText(lpCD->nmcd.hdc, (DWORD)lpCD->nmcd.dwItemSpec, lpCD->iSubItem);
									SetWindowLong(_hSelf, DWL_MSGRESULT, (LONG)(CDRF_SKIPDEFAULT));
									return TRUE;
								}
							}
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
			break;
		}
		case WM_TIMER:
		{
			switch (wParam)
			{
				case IDC_HEX_REDOUNDO:
				{
					/* stop timer */
					::KillTimer(_hSelf, IDC_HEX_REDOUNDO);

					/* set cursor position */
					SetPosition(_posRedoUndo);
					break;
				}
				case IDC_HEX_PASTE:
				{
					/* stop timer */
					::KillTimer(_hSelf, IDC_HEX_PASTE);

					SetPosition(ScintillaMsg(SCI_GETCURRENTPOS));
					break;
				}
			}
			break;
		}
		case WM_DESTROY :
		{
			_hexProp.clear();
			break;
		}
		case HEXM_SETSEL :
		{
			SetSelection((INT)wParam, (INT)lParam);
			break;
		}
		case HEXM_GETSEL :
		{
			GetSelection((INT*)wParam, (INT*)lParam);
			break;
		}
		case HEXM_SETPOS :
		{
			SetPosition((INT)lParam, _pCurProp->isLittle);
			break;
		}
		case HEXM_GETPOS :
		{
			*((INT*)lParam) = GetCurrentPos();
			break;
		}
		case HEXM_ENSURE_VISIBLE :
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
			case WM_SIZING:
			case WM_MOVING:
			case WM_ENTERSIZEMOVE:
			case WM_EXITSIZEMOVE:
			{
				MoveView();
				break;
			}
			case WM_MOUSEWHEEL:
			{
				if (_pCurProp->isVisible == TRUE)
				{
					if (wParam & MK_CONTROL)
					{
						if ((short)HIWORD(wParam) >= WHEEL_DELTA)
						{
							ZoomIn();
						}
						else
						{
							ZoomOut();
						}
						return FALSE;
					}
					else
					{
						::SendMessage(_hListCtrl, Message, wParam, lParam);
					}
				}
				break;
			}
			case WM_CHAR:
			case WM_KEYDOWN:
			case WM_SYSKEYDOWN:
			{
				if (_pCurProp->isVisible == TRUE)
				{
					::SetFocus(_hListCtrl);
					::SendMessage(_hListCtrl, Message, wParam, lParam);
					return FALSE;
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

	return ::CallWindowProc(_hDefaultParentProc, hwnd, Message, wParam, lParam);
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
				break;
			}
			case WM_LBUTTONDOWN:
			{
				LV_HITTESTINFO	info;

				/* get selected sub item */
				info.pt.x = LOWORD(lParam);
				info.pt.y = HIWORD(lParam);
				ListView_SubItemHitTest(_hListCtrl, &info);

				if (info.iSubItem == 0)
					return FALSE;

				/* change edit type */
				_pCurProp->editType = (info.iSubItem == (_pCurProp->columns+1))? HEX_EDIT_ASCII : HEX_EDIT_HEX;

				if (!0x80 & ::GetKeyState(VK_SHIFT))
				{
					/* keep sure that selection is off */
					_pCurProp->isSel	= FALSE;
				}

				/* goto position in parent view */
				if (_pCurProp->editType == HEX_EDIT_HEX)
				{
					OnMouseClickItem(wParam, lParam);
				}
				else
				{
					OnMouseClickDump(wParam, lParam);
				}
				InvalidateNotepad();

				/* store last position */
				_x = LOWORD(lParam);
				_y = HIWORD(lParam);
				_isLBtnDown = TRUE;
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
					POINT	pt = {LOWORD(lParam), HIWORD(lParam)};
					::ClientToScreen(_hParentHandle, &pt);
					TrackMenu(pt);
				}

				_isRBtnDown = FALSE;
				_isWheel	= FALSE;
				break;
			}
			case WM_MOUSEWHEEL:
			{
				if (_isRBtnDown == TRUE)
				{
					_isWheel = TRUE;
					return TRUE;
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
							_pCurProp->selection = HEX_SEL_BLOCK;
						}
						else
						{
							_pCurProp->selection = HEX_SEL_NORM;
						}
						_pCurProp->isSel = TRUE;

						if (_pCurProp->editType == HEX_EDIT_HEX)
						{
							/* correct the cursor position */
							_pCurProp->anchorPos   = (_pCurProp->anchorSubItem-1) * _pCurProp->bits + _pCurProp->anchorPos / FACTOR;
						}
						else
						{
							/* correct column */
							_pCurProp->anchorSubItem = _pCurProp->anchorPos / _pCurProp->bits + 1;
						}
					}
					
					LV_HITTESTINFO	info;

					::GetCursorPos(&info.pt);
					::ScreenToClient(_hListCtrl, &info.pt);
					ListView_SubItemHitTest(_hListCtrl, &info);

					/* test if last row is empty */
					if ((_currLength == ((ListView_GetItemCount(_hListCtrl) - 1) * VIEW_ROW)) &&
						((info.iItem + 1) == ListView_GetItemCount(_hListCtrl)))
						return FALSE;

					/* store row */
					_pCurProp->cursorItem	= info.iItem;

					if (info.iSubItem <= 0)
						return FALSE;

					if (info.iSubItem == DUMP_FIELD)
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
						_pCurProp->cursorSubItem   = _pCurProp->cursorPos / _pCurProp->bits + 1;
					}
					else
					{
						/* calculate cursor */
						UINT	cursorPos = CalcCursorPos(info) / FACTOR;

						_pCurProp->cursorPos = (info.iSubItem-1) * _pCurProp->bits + cursorPos;

						/* calculate column */
						_pCurProp->cursorSubItem   = info.iSubItem;

						if (cursorPos >= (UINT)_pCurProp->bits)
						{
							/* offset */
							_pCurProp->cursorSubItem++;
						}

						if ((ListView_GetItemCount(_hListCtrl) == (info.iItem + 1)) && 
							(_currLength < (_pCurProp->cursorItem * VIEW_ROW + _pCurProp->cursorPos + 1)))
						{
							_pCurProp->cursorPos = (_currLength % VIEW_ROW);
							_pCurProp->cursorSubItem   = ((_currLength % VIEW_ROW)/_pCurProp->bits) + 1;
						}
					}

					/* update list only on changes (prevent flickering) */
					if ((_oldPaintItem != _pCurProp->cursorItem) || (_oldPaintSubItem != _pCurProp->cursorSubItem) ||
						(_oldPaintCurPos != _pCurProp->cursorPos))
					{
						ListView_Update(_hListCtrl, 0);
						_oldPaintItem	 = _pCurProp->cursorItem;
						_oldPaintSubItem = _pCurProp->cursorSubItem;
						_oldPaintCurPos  = _pCurProp->cursorPos;

						/* select also in parent view */
						ScintillaMsg(SCI_SETSEL, _pCurProp->anchorItem * VIEW_ROW + _pCurProp->anchorPos,
												 _pCurProp->cursorItem * VIEW_ROW + _pCurProp->cursorPos);
						InvalidateNotepad();
						return TRUE;
					}
				}
				break;
			}
			case WM_SYSKEYDOWN:
				// ::CallWindowProc(_hDefaultListProc, hwnd, Message, wParam, lParam);
			case WM_KEYDOWN:
			{
				LPARAM	ret = 0;

				if (_pCurProp->editType == HEX_EDIT_HEX)
				{
					return OnKeyDownItem(wParam, lParam);
				}
				else
				{
					return OnKeyDownDump(wParam, lParam);
				}
			}
			case WM_CHAR:
			{
				if (0x80 & ::GetKeyState(VK_CONTROL))
				{
					switch (wParam)
					{
						case 0x01:
							SetSelection(0, _currLength);
							break;
						case 0x18:
							Cut();
							break;
						case 0x03:
							Copy();
							break;
						case 0x16:
							Paste();
							break;
						case 0x19:
							::SendMessage(_hParent, WM_COMMAND, IDM_EDIT_REDO, 0);
							break;
						case 0x1A:
							::SendMessage(_hParent, WM_COMMAND, IDM_EDIT_UNDO, 0);
							break;
						default:
							break;
					}
				}
				else
				{
					if (_pCurProp->editType == HEX_EDIT_HEX)
					{
						return OnCharItem(wParam, lParam);
					}
					else
					{
						OnCharDump(wParam, lParam);
					}
				}
				return TRUE;
			}
			case WM_ERASEBKGND:
			{
				HBRUSH	hbrush	= ::CreateSolidBrush(RGB(0xFF,0xFF,0xFF));

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
				ListView_GetSubItemRect(_hListCtrl, 0, _pCurProp->columns+1, LVIR_BOUNDS, &rc);
				rcList.left = rc.right;
				::FillRect((HDC)wParam, &rcList, hbrush);

				/* fill bottom rect if is missed */
				rcList.right = rcList.left;
				rcList.left  = right;
				ListView_GetItemRect(_hListCtrl, ListView_GetItemCount(_hListCtrl)-1, &rc, LVIR_BOUNDS);
				rcList.top = rc.bottom;
				::FillRect((HDC)wParam, &rcList, hbrush);

				::DeleteObject(hbrush);
				return FALSE;
			}
			case WM_GETDLGCODE:
			{
				MSG		*msg = (MSG*)lParam;

				if (msg != NULL)
				{
					if ((msg->message == WM_KEYDOWN) && (msg->wParam == VK_TAB))
					{
						if (0x80 & GetKeyState(VK_CONTROL))
						{
							if (0x80 & GetKeyState(VK_SHIFT))
							{
								::SendMessage(_hParent, WM_COMMAND, IDC_PREV_DOC, 0);
							}
							else
							{
								::SendMessage(_hParent, WM_COMMAND, IDC_NEXT_DOC, 0);
							}
						}
						else
						{
							UINT pos = GetCurrentPos();
							_pCurProp->editType = ((_pCurProp->editType == HEX_EDIT_ASCII)?HEX_EDIT_HEX:HEX_EDIT_ASCII);
							SetPosition(pos);
						}
						return TRUE;
					}
				}
				break;
			}
			default:
				break;
		}
	}

	return ::CallWindowProc(_hDefaultListProc, hwnd, Message, wParam, lParam);
}


void HexEdit::UpdateDocs(const char** pFiles, UINT numFiles, INT openDoc)
{
	/* update current visible line */
	GetLineVis();

	vector<tHexProp>	tmpList;

	/* attach (un)known files */
	for (size_t i = 0; i < numFiles; i++)
	{
		BOOL isCopy = FALSE;

		for (size_t j = 0; j < _hexProp.size(); j++)
		{
			if (strcmp(pFiles[i], _hexProp[j].pszFileName) == 0)
			{
				tmpList.push_back(_hexProp[j]);
				isCopy = TRUE;
			}
		}

		if (isCopy == FALSE)
		{
			/* attach new file */
			tHexProp	prop = getProp();

			strcpy(prop.pszFileName, pFiles[i]);
			prop.isModified		= FALSE;
			prop.firstVisRow	= 0;
			prop.editType		= HEX_EDIT_HEX;
			prop.isSel			= FALSE;
			prop.selection		= HEX_SEL_NORM;

			/* test if extension of file is registered */
			prop.isVisible		= IsExtensionRegistered(pFiles[i]);

			tmpList.push_back(prop);
		}
	}

	/* copy new list into current list */
	_hexProp = tmpList;

	if (_openDoc != openDoc)
	{
		/* store current open document */
		_openDoc = openDoc;

		if (openDoc != -1)
		{
			/* set the current file attributes */
			_pCurProp = &_hexProp[_openDoc];
		}
		else
		{
			/* hide hex editor */
			::KillTimer(_hListCtrl, IDC_HEX_CURSORTIMER);
			display(false);

			_pCurProp = NULL;
		}
	}
	else if (tmpList.size() != 0)
	{
		/* set the current file attributes */
		_pCurProp = &_hexProp[openDoc];
	}
	else
	{
		_pCurProp = NULL;
	}

	/* update views */
	if (openDoc != -1)
		doDialog();
}


void HexEdit::doDialog(BOOL toggle)
{
	if (_pCurProp == NULL)
		return;

	/* toggle view if user requested */
	if (toggle == TRUE)
	{
		extern UINT	currentSC;

		_pCurProp->isVisible ^= TRUE;

		/* get modification state */
		BOOL isModified = (BOOL)ScintillaMsg(SCI_GETMODIFY);
		BOOL isModifiedBefore = _pCurProp->isModified;

		if (_pCurProp->isVisible == TRUE)
		{
			tHexProp	prop = getProp();

			_pCurProp->columns			= prop.columns;
			_pCurProp->bits				= prop.bits;
			_pCurProp->isBin			= prop.isBin;
			_pCurProp->isLittle			= prop.isLittle;
			_pCurProp->isSel			= FALSE;
			_pCurProp->anchorItem		= 0;
			_pCurProp->anchorSubItem	= 0;
			_pCurProp->anchorPos		= 0;
			_pCurProp->cursorItem		= 0;
			_pCurProp->cursorSubItem	= 1;
			_pCurProp->cursorPos		= 0;

			::SendMessage(_nppData._nppHandle, NPPM_DECODE_SCI, currentSC, 0);
		}
		else
			::SendMessage(_nppData._nppHandle, NPPM_ENCODE_SCI, currentSC, 0);
		
		if ((isModified == FALSE) && (isModifiedBefore == FALSE))
			ScintillaMsg(SCI_SETSAVEPOINT);
		else
			_pCurProp->isModified = TRUE;

		InvalidateNotepad();
	}

	/* update the header */
	UpdateHeader();

	/* set window position and display informations */
	MoveView();

	/* set focus */
	ActivateWindow();

	/* set coding entries gray */
	GrayEncoding();

	/* check menu and tb icon */
	checkMenu(_pCurProp->isVisible);

	/* show or hide window */
	display(_pCurProp->isVisible == TRUE);
}


void HexEdit::MoveView(void)
{
	if (_pCurProp == NULL)
		return;

	if (_pCurProp->isVisible == TRUE)
	{
		RECT	rc;
		
		::GetWindowRect(_hParentHandle, &rc);
		ScreenToClient(_nppData._nppHandle, &rc);
		::SetWindowPos(_hSelf, _hParentHandle, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, 0L);
		::ShowWindow(_hParentHandle, SW_HIDE);

		/* Update header */
		::RedrawWindow(_hListCtrl, NULL, NULL, RDW_INVALIDATE);
	}
	else if ((_openDoc == -1) && (_hParentHandle == _nppData._scintillaSecondHandle))
	{
		::ShowWindow(_hParentHandle, SW_HIDE);
		::ShowWindow(_hSelf, SW_HIDE);
	}
	else
	{
		::ShowWindow(_hParentHandle, SW_SHOW);
		::ShowWindow(_hSelf, SW_HIDE);
	}
}


void HexEdit::UpdateHeader(void)
{
	if (_pCurProp == NULL)
		return;

	if (_pCurProp->isVisible)
	{
		_currLength = (UINT)::SendMessage(_hParentHandle, SCI_GETLENGTH, 0, 0);

		/* get current font width */
		SIZE	size;
		HDC		hDc = ::GetDC(_hListCtrl);
        SelectObject(hDc, _hFont);
        GetTextExtentPoint32(hDc, "0", 1, &size);
		::ReleaseDC(_hSelf, hDc);
		
		/* update header now */
		UINT	fontWidth = size.cx;
		char	temp[16];
		temp[0] = '+';

		while (ListView_DeleteColumn(_hListCtrl, 0));

		LVCOLUMN ColSetupTermin = {LVCF_FMT | LVCF_WIDTH | LVCF_TEXT, LVCFMT_LEFT, (fontWidth<<4) + 20, "Address", 7, 0};
		ListView_InsertColumn(_hListCtrl, 0, &ColSetupTermin);

		for (SHORT i = 0; i < _pCurProp->columns; i++)
		{
			itoa(i * _pCurProp->bits, &temp[1], 16);
			ColSetupTermin.mask			= LVCF_TEXT | LVCF_FMT | LVCF_WIDTH;
			ColSetupTermin.fmt			= LVCFMT_CENTER;
			ColSetupTermin.pszText		= temp;
			ColSetupTermin.cchTextMax	= 6;
			ColSetupTermin.cx			= ((_pCurProp->isBin)? fontWidth<<3:fontWidth<<1) * _pCurProp->bits + 20;
			ListView_InsertColumn(_hListCtrl, i + 1, &ColSetupTermin);
		}
		
		ColSetupTermin.mask			= LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
		ColSetupTermin.fmt			= LVCFMT_LEFT;
		ColSetupTermin.cx			= fontWidth * _pCurProp->columns * _pCurProp->bits + 20;
		ColSetupTermin.pszText		= "Dump";
		ColSetupTermin.cchTextMax	= 4;
		ListView_InsertColumn(_hListCtrl, _pCurProp->columns + 2, &ColSetupTermin);

		/* create line rows */
		UINT	items = _currLength;
		if (items)
		{
			items = items / _pCurProp->columns / _pCurProp->bits;
			ListView_SetItemCountEx(_hListCtrl, ((_currLength%VIEW_ROW)?items+1:items), LVSICF_NOSCROLL);
		}
		else
		{
			ListView_SetItemCountEx(_hListCtrl, 1, LVSICF_NOSCROLL);
		}

		/* set list view to old position */
		SetLineVis(_pCurProp->firstVisRow, HEX_LINE_FIRST);
	}
}


void HexEdit::Copy(void)
{
	if (_pCurProp == NULL)
		return;

	if (_pCurProp->isSel == TRUE)
	{
		HWND		hSCI;
		INT			posBeg;
		INT			posEnd;
		tClipboard	clipboard;

		/* store selection */
		clipboard.selection = _pCurProp->selection;

		/* copy data into scintilla handle (encoded if necessary) */
		hSCI = (HWND)::SendMessage(_hParent, NPPM_CREATESCINTILLAHANDLE, 0, (LPARAM)_hSelf);
		LittleEndianChange(hSCI, _hParentHandle);
		
		/* get text */
		switch (_pCurProp->selection)
		{
			case HEX_SEL_NORM:
			{
				GetSelection(&posBeg, &posEnd);
				clipboard.length = CalcStride(posBeg, posEnd);
				clipboard.text = (char*)new char[clipboard.length+1];
				::SendMessage(hSCI, SCI_SETSEL, posBeg, (LPARAM)posEnd);
				::SendMessage(hSCI, SCI_GETSELTEXT, 0, (LPARAM)clipboard.text);
				break;
			}
			case HEX_SEL_BLOCK:
			{
				/* get columns and count */
				UINT	first	= _pCurProp->anchorItem;
				UINT	last	= _pCurProp->cursorItem;
				
				if (_pCurProp->anchorItem == _pCurProp->cursorItem)
				{
					GetSelection(&posBeg, &posEnd);
					clipboard.length = CalcStride(posBeg, posEnd);
					clipboard.text = (char*)new char[clipboard.length+1];
					::SendMessage(hSCI, SCI_SETSEL, posBeg, (LPARAM)posEnd);
					::SendMessage(hSCI, SCI_GETSELTEXT, 0, (LPARAM)clipboard.text);
					break;
				}
				else if (_pCurProp->anchorItem > _pCurProp->cursorItem)
				{
					first = _pCurProp->cursorItem;
					last  = _pCurProp->anchorItem;
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
				clipboard.length = clipboard.stride*(last-first+1);
				clipboard.text = (char*)new char[clipboard.length+1];

				posEnd = posBeg + clipboard.stride;
				for (UINT i = 0; i < clipboard.items; i++)
				{
					::SendMessage(hSCI, SCI_SETSEL, posBeg, posBeg + clipboard.stride);
					::SendMessage(hSCI, SCI_GETSELTEXT, 0, (LPARAM)&clipboard.text[i*clipboard.stride]);
					posBeg += VIEW_ROW;
				}
				break;
			}
			default:
				break;
		}

		/* destory scintilla handle */
		::SendMessage(hSCI, SCI_UNDO, 0, 0);
		::SendMessage(_hParent, NPPM_DESTROYSCINTILLAHANDLE, 0, (LPARAM)hSCI);
		
		/* convert to hex if usefull */
		if (_pCurProp->editType == HEX_EDIT_HEX)
		{
			tClipboard	data = clipboard;
			ChangeClipboardDataToHex(&data);
			/* store selected text in scintilla clipboard */
			::SendMessage(_hParentHandle, SCI_COPYTEXT, data.length+1, (LPARAM)data.text);
			delete [] data.text;
		}
		else
		{
			/* store selected text in scintilla clipboard */
			::SendMessage(_hParentHandle, SCI_COPYTEXT, clipboard.length+1, (LPARAM)clipboard.text);
		}

		/* delete old text and store to clipboard */
		if (g_clipboard.text != NULL)
			delete [] g_clipboard.text;
		g_clipboard = clipboard;
	}
}


void HexEdit::Cut(void)
{
	if (_pCurProp == NULL)
		return;

	if (_pCurProp->isSel == TRUE)
	{
		HWND		hSCI;
		INT			posBeg;
		INT			posEnd;
		tClipboard	clipboard;


		ScintillaMsg(SCI_BEGINUNDOACTION);

		/* store selection */
		clipboard.selection = _pCurProp->selection;

		/* copy data into scintilla handle (encoded if necessary) */
		hSCI = (HWND)::SendMessage(_hParent, NPPM_CREATESCINTILLAHANDLE, 0, (LPARAM)_hSelf);
		LittleEndianChange(hSCI, _hParentHandle);
		
		/* get text */
		switch (_pCurProp->selection)
		{
			case HEX_SEL_NORM:
			{
				GetSelection(&posBeg, &posEnd);
				clipboard.length = CalcStride(posBeg, posEnd);
				clipboard.text = (char*)new char[clipboard.length+1];
				::SendMessage(hSCI, SCI_SETSEL, posBeg, posEnd);
				::SendMessage(hSCI, SCI_GETSELTEXT, 0, (LPARAM)clipboard.text);
				ScintillaMsg(SCI_TARGETFROMSELECTION);
				ScintillaMsg(SCI_REPLACETARGET, 0, NULL);
				SetPosition(posBeg > posEnd ? posEnd:posBeg);
				break;
			}
			case HEX_SEL_BLOCK:
			{
				/* get columns and count */
				UINT	first	= _pCurProp->anchorItem;
				UINT	last	= _pCurProp->cursorItem;
				
				if (_pCurProp->anchorItem == _pCurProp->cursorItem)
				{
					GetSelection(&posBeg, &posEnd);
					clipboard.length = CalcStride(posBeg, posEnd);
					clipboard.text = (char*)new char[clipboard.length+1];
					::SendMessage(hSCI, SCI_SETSEL, posBeg, posEnd);
					::SendMessage(hSCI, SCI_GETSELTEXT, 0, (LPARAM)clipboard.text);
					ScintillaMsg(SCI_TARGETFROMSELECTION);
					ScintillaMsg(SCI_REPLACETARGET, 0, NULL);
					SetPosition(posBeg > posEnd ? posEnd:posBeg);
					break;
				}
				else if (_pCurProp->anchorItem > _pCurProp->cursorItem)
				{
					first = _pCurProp->cursorItem;
					last  = _pCurProp->anchorItem;
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
				clipboard.length = clipboard.stride*(last-first+1);
				clipboard.text = (char*)new char[clipboard.length+1];

				INT	tempBeg = posBeg;
				posEnd = posBeg + clipboard.stride;
				for (UINT i = 0; i < clipboard.items; i++)
				{
					::SendMessage(hSCI, SCI_SETSEL, posBeg, posBeg + clipboard.stride);
					::SendMessage(hSCI, SCI_GETSELTEXT, 0, (LPARAM)&clipboard.text[i*clipboard.stride]);
					::SendMessage(hSCI, SCI_TARGETFROMSELECTION, 0, 0);
					::SendMessage(hSCI, SCI_REPLACETARGET, 0, NULL);
					ScintillaMsg(SCI_TARGETFROMSELECTION);
					ScintillaMsg(SCI_REPLACETARGET, 0, NULL);
					posBeg += VIEW_ROW - clipboard.stride;
				}
				SetPosition(tempBeg);
				break;
			}
			default:
				break;
		}
		
		/* destory scintilla handle */
		::SendMessage(hSCI, SCI_UNDO, 0, 0);
		::SendMessage(_hParent, NPPM_DESTROYSCINTILLAHANDLE, 0, (LPARAM)hSCI);

		/* convert to hex if usefull */
		if (_pCurProp->editType == HEX_EDIT_HEX)
		{
			tClipboard	data = clipboard;
			ChangeClipboardDataToHex(&data);
			/* store selected text in scintilla clipboard */
			::SendMessage(_hParentHandle, SCI_COPYTEXT, data.length+1, (LPARAM)data.text);
			delete [] data.text;
		}
		else
		{
			/* store selected text in scintilla clipboard */
			::SendMessage(_hParentHandle, SCI_COPYTEXT, clipboard.length+1, (LPARAM)clipboard.text);
		}

		/* delete old text and store to clipboard */
		if (g_clipboard.text != NULL)
			delete [] g_clipboard.text;
		g_clipboard = clipboard;

		ScintillaMsg(SCI_ENDUNDOACTION);
	}
}


void HexEdit::Paste(void)
{
	if (_pCurProp == NULL)
		return;

	ScintillaMsg(SCI_BEGINUNDOACTION);

	if (g_clipboard.text == NULL)
	{
		/* copy data into scintilla handle (encoded if necessary) */
		HWND hSCI = (HWND)::SendMessage(_hParent, NPPM_CREATESCINTILLAHANDLE, 0, (LPARAM)_hSelf);
		ScintillaMsg(hSCI, SCI_PASTE);

		UINT	length = ScintillaMsg(SCI_GETTEXTLENGTH);

		/* test if first chars are digits only */
		ScintillaMsg(hSCI, SCI_SETSEARCHFLAGS, SCFIND_REGEXP | SCFIND_POSIX);
		ScintillaMsg(hSCI, SCI_SETTARGETSTART, 0);
		ScintillaMsg(hSCI, SCI_SETTARGETEND, length);

		UINT posFind = ScintillaMsg(hSCI, SCI_SEARCHINTARGET, 14, (LPARAM)"^[0-9a-fA-F]+ ");
		if (posFind == 0)
		{
			/* if test again and extract informations */
			UINT	lineCnt		= ScintillaMsg(hSCI, SCI_GETLINECOUNT);
			UINT	charPerLine	= ScintillaMsg(hSCI, SCI_LINELENGTH, 0);
			char*	buffer		= (char*)new char[charPerLine+1];
			char*   target		= (char*)new char[charPerLine*lineCnt+1];
			char*	p_buffer	= NULL;
			char*	p_target	= target;
			bool	isOk		= TRUE;
			INT		expLine		= 0;

			while ((posFind != -1) && (isOk))
			{
				INT		posBeg  = ScintillaMsg(hSCI, SCI_GETTARGETSTART);
				INT		posEnd  = ScintillaMsg(hSCI, SCI_GETTARGETEND);
				INT		size	= posEnd - posBeg;
				INT		curLine = ScintillaMsg(hSCI, SCI_LINEFROMPOSITION, posBeg);
				ScintillaGetText(hSCI, buffer, posBeg, posEnd-1);
				buffer[size-1] = 0;

				if ((ASCIIConvert(buffer) / 0x10) == expLine)
				{
					UINT	curLength	= (curLine != lineCnt-1?charPerLine-3:length%charPerLine);
					ScintillaGetText(hSCI, buffer, posBeg, posBeg+curLength);

					/* get chars */
					UINT	i		= 0;
					char*	p_end	= buffer + curLength;
					p_buffer		= &buffer[size];

					while (i < (UINT)(p_end-p_buffer))
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

					ScintillaMsg(hSCI, SCI_SETTARGETSTART, posEnd);
					ScintillaMsg(hSCI, SCI_SETTARGETEND, length);
					posFind = ScintillaMsg(hSCI, SCI_SEARCHINTARGET, 14, (LPARAM)"^[0-9a-fA-F]+ ");

					/* increment expected values */
					expLine++;
				}
				else
				{
					isOk = FALSE;
					DEBUG_VAL_INFO(buffer, ASCIIConvert(buffer)/0x10);
				}
			}
			delete buffer;

			/* copy into target scintilla */
			if (isOk == TRUE)
			{
				UINT	posBeg	= GetAnchor();
				UINT	posEnd	= GetCurrentPos();

				if (posBeg > posEnd)
					posBeg = posEnd;
				
				ScintillaMsg(_hParentHandle, SCI_SETSEL, posBeg, posEnd);
				ScintillaMsg(_hParentHandle, SCI_TARGETFROMSELECTION);
				ScintillaMsg(_hParentHandle, SCI_REPLACETARGET, p_target - target, (LPARAM)target);
			}
			delete target;
		}
		else
		{
			ScintillaMsg(SCI_PASTE);
		}

		/* destory scintilla handle */
		::SendMessage(hSCI, SCI_UNDO, 0, 0);
		::SendMessage(_hParent, NPPM_DESTROYSCINTILLAHANDLE, 0, (LPARAM)hSCI);
	}	
	else
	{
		HWND	hSCI	= NULL;
		UINT	posBeg	= GetAnchor();
		UINT	posEnd	= GetCurrentPos();

		if (posBeg > posEnd)
			posBeg = posEnd;

		/* copy data into scintilla handle (encoded if necessary) */
		hSCI = (HWND)::SendMessage(_hParent, NPPM_CREATESCINTILLAHANDLE, 0, (LPARAM)_hSelf);
		LittleEndianChange(hSCI, _hParentHandle);
		
		switch (g_clipboard.selection)
		{
			case HEX_SEL_NORM:
			{
				::SendMessage(hSCI, SCI_SETSEL, posBeg, posEnd);
				::SendMessage(hSCI, SCI_TARGETFROMSELECTION, 0, 0);
				::SendMessage(hSCI, SCI_REPLACETARGET, g_clipboard.length, (LPARAM)g_clipboard.text);
				if (E_OK == replaceLittleToBig(hSCI, posBeg, posEnd-posBeg, g_clipboard.length))
				{
					SetPosition(posBeg + g_clipboard.length, _pCurProp->isLittle);
				}
				break;
			}
			case HEX_SEL_BLOCK:
			{
				/* set start and end of block */
				if (_pCurProp->isSel == TRUE)
				{
					UINT	stride	= 0;
					UINT	items	= 0;
					UINT	first	= _pCurProp->anchorItem;
					UINT	last	= _pCurProp->cursorItem;

					if (_pCurProp->anchorItem > _pCurProp->cursorItem)
					{
						first = _pCurProp->cursorItem;
						last  = _pCurProp->anchorItem;
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
						MessageBox(_hSelf, "Box must have the same size!", "HexEdit", MB_OK);
						ScintillaMsg(SCI_ENDUNDOACTION);
						return;
					}
				}

				posEnd = posBeg + g_clipboard.stride;
				for (UINT i = 0; i < g_clipboard.items; i++)
				{
					ScintillaMsg(SCI_SETSEL, posBeg, posEnd);
					ScintillaMsg(SCI_TARGETFROMSELECTION);
					ScintillaMsg(SCI_REPLACETARGET, g_clipboard.stride, (LPARAM)&g_clipboard.text[i*g_clipboard.stride]);
					posBeg += VIEW_ROW;
					posEnd += VIEW_ROW;
				}

				ScintillaMsg(SCI_SETSEL, posEnd - VIEW_ROW, posEnd - VIEW_ROW);
				break;
			}
			default:
				break;
		}
		/* destory scintilla handle */
		::SendMessage(hSCI, SCI_UNDO, 0, 0);
		::SendMessage(_hParent, NPPM_DESTROYSCINTILLAHANDLE, 0, (LPARAM)hSCI);

	}
	ScintillaMsg(SCI_ENDUNDOACTION);
	::SetTimer(_hSelf, IDC_HEX_PASTE, 25, NULL);
}


void HexEdit::ZoomIn(void)
{
	if (_iFontSize < 26)
	{
		_iFontSize += 2;

		if (SetFontSize(_iFontSize) == FALSE)
			DEBUG("Could not change font");

		GetLineVis();
		UpdateHeader();
	}
}


void HexEdit::ZoomOut(void)
{
	if (_iFontSize > 2)
	{
		_iFontSize -= 2;

		if (SetFontSize(_iFontSize) == FALSE)
			DEBUG("Could not change font");

		GetLineVis();
		UpdateHeader();
	}
}


void HexEdit::ZoomRestore(void)
{
	_iFontSize = 16;

	if (SetFontSize(_iFontSize) == FALSE)
		DEBUG("Could not change font");

	GetLineVis();
	UpdateHeader();
}


void HexEdit::ReadArrayToList(LPTSTR text, INT iItem, INT iSubItem)
{
	/* create addresses */
	if (iSubItem == 0)
	{
		if (getCLM())
		{
			sprintf(text, "%016X", iItem * _pCurProp->columns * _pCurProp->bits);
		}
		else
		{
			sprintf(text, "%016x", iItem * _pCurProp->columns * _pCurProp->bits);
		}
	}
	/* create dump */
	else if (iSubItem == DUMP_FIELD)
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
				memset(&text[(_currLength - posBeg)*FACTOR], 0x20, SUBITEM_LENGTH);
				text[SUBITEM_LENGTH] = 0;
			}
		}
		else
		{
			strcpy(text, " ");
		}
	}
}


void HexEdit::AddressConvert(LPTSTR text, INT length)
{
	char *temp  = (char*)new char[length];
	memcpy(temp, text, length);

	if (_pCurProp->isLittle == TRUE)
	{
		if (_pCurProp->isBin)
		{
			strcpy(text, binMask[(UCHAR)temp[--length]]);
			for (INT i = length-1; i >= 0; --i)
			{
				strcat(text, binMask[(UCHAR)temp[i]]);
			}
		}
		else
		{
			strcpy(text, hexMask[(UCHAR)temp[--length]]);
			for (INT i = length-1; i >= 0; --i)
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
	delete [] temp;
}

void HexEdit::DumpConvert(LPTSTR text, UINT length)
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
		char *temp  = (char*)new char[length];
		char *pText	= text;

		/* i must be unsigned */
		for (UINT i = 0; i < length; i++)
		{
			temp[i] = ascii[(UCHAR)text[i]];
		}

		UINT offset = length % _pCurProp->bits;
		UINT max	= length / _pCurProp->bits + 1;

		for (i = 1; i <= max; i++)
		{
			if (i == max)
			{
				for (UINT j = 1; j <= offset; j++)
				{
					*pText = temp[length-j];
					pText++;
				}
			}
			else
			{
				for (SHORT j = 1; j <= _pCurProp->bits; j++)
				{
					*pText = temp[_pCurProp->bits*i-j];
					pText++;
				}
			}
		}
		*pText = NULL;
		delete [] temp;
	}
}


void HexEdit::BinHexConvert(char *text, INT length)
{
	char *temp = (char*)new char[length+1];
	memcpy(temp, text, length);

	if (_pCurProp->isBin == FALSE)
	{
		for (INT i = length - 1; i >= 0; --i)
		{
			text[i] = 0;
			for (INT j = 0; j < 2; j++)
			{
				text[i] <<= 4;
				text[i] |= decMask[temp[2*i+j]];
			}
		}
	}
	else
	{
		char*	pText = temp;
		for (INT i = 0; i < length; i++ )
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
			text[i] = temp[length-1-i];
		}
	}
    text[length] = 0;
	delete [] temp;
}


void HexEdit::TrackMenu(POINT pt)
{
	HMENU	hMenu		= ::CreatePopupMenu();
	HMENU	hSubMenu	= ::CreatePopupMenu();
	BOOL	isChanged	= TRUE;
	UINT	cursorPos	= GetCurrentPos();
	UINT	anchorPos	= GetAnchor();
	UINT	oldColumns  = _pCurProp->columns;
	UINT	oldBits     = _pCurProp->bits;
	BOOL	oldBin		= _pCurProp->isBin;
	BOOL	oldLittle   = _pCurProp->isLittle;
	

	/* set bit decoding */
	::AppendMenu(hSubMenu, MF_STRING | ((_pCurProp->bits == HEX_BYTE)?MF_CHECKED:0), 6, "8-Bit");
	::AppendMenu(hSubMenu, MF_STRING | ((_pCurProp->bits == HEX_WORD)?MF_CHECKED:0), 7, "16-Bit");
	::AppendMenu(hSubMenu, MF_STRING | ((_pCurProp->bits == HEX_DWORD)?MF_CHECKED:0), 8, "32-Bit");
	::AppendMenu(hSubMenu, MF_STRING | ((_pCurProp->bits == HEX_LONG)?MF_CHECKED:0), 9, "64-Bit");
	::AppendMenu(hSubMenu, MF_SEPARATOR, 0, "-----------------");
	/* set binary decoding */
	::AppendMenu(hSubMenu, MF_STRING, 10, _pCurProp->isBin == TRUE ? "to Hex":"to Binary");
	/* change between big- and little-endian */
	if (_pCurProp->bits > HEX_BYTE)
	{
		::AppendMenu(hSubMenu, MF_STRING, 11, _pCurProp->isLittle == TRUE? "to BigEndian":"to LittleEndian");
	}

	::AppendMenu(hMenu, MF_STRING | ((!_pCurProp->isSel)?MF_GRAYED:0), 1, "Cut");
	::AppendMenu(hMenu, MF_STRING | ((!_pCurProp->isSel)?MF_GRAYED:0), 2, "Copy");
	::AppendMenu(hMenu, MF_STRING | ((!ScintillaMsg(SCI_CANPASTE))?MF_GRAYED:0), 3, "Paste");
	::AppendMenu(hMenu, MF_STRING | ((!_pCurProp->isSel)?MF_GRAYED:0), 4, "Delete");
	::AppendMenu(hMenu, MF_SEPARATOR, 0, "-----------------");
	::AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT_PTR)hSubMenu, "View in");
	::AppendMenu(hMenu, MF_SEPARATOR, 0, "-----------------");
	/* set columns */
	::AppendMenu(hMenu, MF_STRING, 5, "Columns...");



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
			if (_pCurProp->selection == HEX_SEL_BLOCK)
			{
				ScintillaMsg(SCI_BEGINUNDOACTION);
				OnDeleteBlock();
				ScintillaMsg(SCI_ENDUNDOACTION);
			}
			else
			{
				ScintillaMsg(SCI_TARGETFROMSELECTION);
				ScintillaMsg(SCI_REPLACETARGET, 0, NULL);
			}
			break;
		}
		case 5:
		{
			ColumnDlg	dlg;
			dlg.init(_hInst, _nppData);
			UINT	val = dlg.doDialog(_pCurProp->columns);
			if ((val > 0) && (val <= (128 / (UINT)_pCurProp->bits)))
			{
				_pCurProp->columns = val;
			}
			else
			{
				MessageBox(_nppData._nppHandle, "Max column count can be 1 till 128 bytes in a row.", "HexEdit", MB_OK|MB_ICONERROR);
				isChanged = FALSE;
			}
			break;
		}
		case 6:
		{
			_pCurProp->firstVisRow	= 0;
			_pCurProp->columns		= VIEW_ROW;
			_pCurProp->bits			= HEX_BYTE;
			break;
		}
		case 7:
		{
			_pCurProp->firstVisRow	= 0;
			if (VIEW_ROW >= HEX_WORD)
				_pCurProp->columns	= VIEW_ROW / HEX_WORD;
			else
				_pCurProp->columns	= 1;
			_pCurProp->bits			= HEX_WORD;
			break;
		}
		case 8:
		{
			_pCurProp->firstVisRow	= 0;
			if (VIEW_ROW >= HEX_DWORD)
				_pCurProp->columns	= VIEW_ROW / HEX_DWORD;
			else
				_pCurProp->columns	= 1;
			_pCurProp->bits			= HEX_DWORD;
			break;
		}
		case 9:
		{
			_pCurProp->firstVisRow	= 0;
			if (VIEW_ROW >= HEX_LONG)
				_pCurProp->columns	= VIEW_ROW / HEX_LONG;
			else
				_pCurProp->columns	= 1;
			_pCurProp->bits			= HEX_LONG;
			break;
		}
		case 10:
		{
			_pCurProp->isBin	^= TRUE;
			break;
		}
		case 11:
		{
			_pCurProp->isLittle	^= TRUE;
			break;
		}
		default:
			isChanged			= FALSE;
			break;
	}

	if (isChanged == TRUE)
	{
		/* correct selection or cursor position */
		GetLineVis();
		UINT	firstVisRow = _pCurProp->firstVisRow;
		firstVisRow = ((firstVisRow * oldColumns) / _pCurProp->columns);
		if ((_pCurProp->bits == HEX_BYTE) && (_pCurProp->isLittle == TRUE))
		{
			_pCurProp->isLittle = FALSE;
		}
		UpdateHeader();

		if (_pCurProp->isSel == FALSE)
		{
			SetPosition(cursorPos);
		}
		else
		{
			if (oldLittle == _pCurProp->isLittle)
			{
				SetSelection(anchorPos, cursorPos, _pCurProp->selection);
			}
			else
			{
				_pCurProp->editType = HEX_EDIT_HEX;
				_pCurProp->isSel    = FALSE;
				SelectItem(_pCurProp->cursorItem, _pCurProp->cursorSubItem);
			}
		}

		SetLineVis(firstVisRow, HEX_LINE_FIRST);
		_pCurProp->firstVisRow = firstVisRow;
	}

	::DestroyMenu(hMenu);
	::DestroyMenu(hSubMenu);
}


void HexEdit::OnDeleteBlock(void)
{
	HWND		hSCI;
	INT			count  = 0;
	INT			posBeg = 0;
	INT			posEnd = 0;
	UINT		lines  = 0;

	/* test if something is selected */
	GetSelection(&posBeg, &posEnd);
	if (posBeg == posEnd)
	{
		::MessageBox(_hSelf, "Select something in the text!", "HexEdit", MB_OK);
		return;
	}

	if (posBeg > posEnd)
		posBeg = posEnd;

	/* copy data into scintilla handle (encoded if necessary) */
	hSCI = (HWND)::SendMessage(_hParent, NPPM_CREATESCINTILLAHANDLE, 0, (LPARAM)_hSelf);
	LittleEndianChange(hSCI, getCurrentHScintilla());

	/* get params of Hex Edit */
	lines = GetItemCount();

	if (_pCurProp->selection == HEX_SEL_BLOCK)
	{
		/* get horizontal and vertical gap size */
		count = abs(_pCurProp->anchorPos  - _pCurProp->cursorPos);
		lines = abs(_pCurProp->anchorItem - _pCurProp->cursorItem);

		posEnd = posBeg + count;

		/* replace block with "" */
		for (UINT i = 0; i <= lines; i++)
		{
			ScintillaMsg(hSCI, SCI_SETSEL, posBeg, posEnd);
			ScintillaMsg(hSCI, SCI_TARGETFROMSELECTION);
			ScintillaMsg(hSCI, SCI_REPLACETARGET, -1, (LPARAM)"\0");
			if (E_OK != replaceLittleToBig(hSCI, posBeg, count, 0))
			{
				LITTLE_DELETE_ERROR;
				
				/* free allocated space */
				::SendMessage(hSCI, SCI_UNDO, 0, 0);
				::SendMessage(_hParent, NPPM_DESTROYSCINTILLAHANDLE, 0, (LPARAM)hSCI);
				return;
			}

			posBeg += (_pCurProp->bits * _pCurProp->columns) - count;
			posEnd += posBeg + count;
		}
	}
	/* set cursor position */
	SetPosition(posEnd - count, _pCurProp->isLittle);
	
	/* free allocated space */
	::SendMessage(hSCI, SCI_UNDO, 0, 0);
	::SendMessage(_hParent, NPPM_DESTROYSCINTILLAHANDLE, 0, (LPARAM)hSCI);
}
















void HexEdit::SelectItem(UINT iItem, UINT iSubItem, INT iCursor)
{
	UINT	editMax = iItem * VIEW_ROW + iSubItem * _pCurProp->bits;

	if ((iItem != -1) && (iSubItem >= 1) && (editMax <= (_currLength + _pCurProp->bits)))
	{
		/* set cursor position and update previous and new row */
		_pCurProp->cursorItem		= iItem;
		_pCurProp->cursorSubItem	= iSubItem;
		_pCurProp->cursorPos		= iCursor;
		_pCurProp->isSel			= FALSE;

		if ((editMax == _currLength + _pCurProp->bits) && (_pCurProp->cursorSubItem == 1) && 
			((_currLength / VIEW_ROW) == ListView_GetItemCount(_hListCtrl)))
		{
			UINT itemCnt = ListView_GetItemCount(_hListCtrl);
			ListView_SetItemCountEx(_hListCtrl, itemCnt + 1, 0);
		}
		ListView_EnsureVisible(_hListCtrl, iItem, TRUE);
		InvalidateList();
	}

	/* set position in scintilla */
	if (_pCurProp->isLittle == FALSE)
	{
		ScintillaMsg(SCI_GOTOPOS, VIEW_ROW * _pCurProp->cursorItem + (_pCurProp->cursorSubItem-1) * _pCurProp->bits + _pCurProp->cursorPos / FACTOR);
	}
	else if (FULL_SUBITEM)
	{
		ScintillaMsg(SCI_GOTOPOS, VIEW_ROW * _pCurProp->cursorItem + _pCurProp->cursorSubItem * _pCurProp->bits - _pCurProp->cursorPos / FACTOR - 1);
	}
	else
	{
		ScintillaMsg(SCI_GOTOPOS, VIEW_ROW * _pCurProp->cursorItem + (_pCurProp->cursorSubItem-1) * _pCurProp->bits + _currLength % _pCurProp->bits - _pCurProp->cursorPos / FACTOR - 1);
	}
}


void HexEdit::OnMouseClickItem(WPARAM wParam, LPARAM lParam)
{
	LV_HITTESTINFO	info;

	/* get selected sub item */
	info.pt.x = LOWORD(lParam);
	info.pt.y = HIWORD(lParam);
	ListView_SubItemHitTest(_hListCtrl, &info);

	if (0x80 & ::GetKeyState(VK_SHIFT))
	{
		UINT	posBeg	= GetAnchor();
		bool	isMenu	= ((0x80 & ::GetKeyState(VK_MENU)) == 0x80);

		SetSelection(posBeg, info.iItem * VIEW_ROW + (info.iSubItem - 1) * _pCurProp->bits + CalcCursorPos(info) / FACTOR, isMenu ? HEX_SEL_BLOCK:HEX_SEL_NORM);
	}
	else
	{
		/* set cursor if the same as before */
		UINT	cursor = 0;
		if ((_pCurProp->cursorItem == info.iItem) && (_pCurProp->cursorSubItem == info.iSubItem) &&
			(_pCurProp->editType == HEX_EDIT_HEX))
		{
			cursor = CalcCursorPos(info);
		}

		SelectItem(info.iItem, info.iSubItem, cursor);
	}
}


BOOL HexEdit::OnKeyDownItem(WPARAM wParam, LPARAM lParam)
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
			if (_pCurProp->cursorItem != (_currLength/_pCurProp->columns/_pCurProp->bits))
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
			else if (_pCurProp->cursorSubItem == 1)
			{
				if (_pCurProp->cursorItem != 0)
					SelectItem(_pCurProp->cursorItem - 1, _pCurProp->columns);
			}
			else
				SelectItem(_pCurProp->cursorItem, _pCurProp->cursorSubItem - 1);
			return FALSE;
		}
		case VK_RIGHT:
		{
			if (isShift)
			{
				SelectionKeys(wParam, lParam);
			}
			else if (FULL_SUBITEM)
			{
				if (_pCurProp->cursorSubItem < (UINT)_pCurProp->columns)
					SelectItem(_pCurProp->cursorItem, _pCurProp->cursorSubItem + 1);
				else
					SelectItem(_pCurProp->cursorItem + 1, 1);
			}
			return FALSE;
		}
		default:
			GlobalKeys(wParam, lParam);
			break;
	}

	return TRUE;
}


BOOL HexEdit::OnCharItem(WPARAM wParam, LPARAM lParam)
{
	char	text[64];
	UINT	posBeg	= 0;
	UINT	textPos = 0;
	DWORD	start	= 0;
	DWORD	end		= 0;
	BOOL	newLine	= FALSE;

	/* test if correct char is pressed */
	if (_pCurProp->isBin == FALSE)
	{
		if (((wParam < 0x30) || (wParam > 0x66)) ||
			((wParam > 0x39) && (wParam < 0x41)) || 
			((wParam > 0x46) && (wParam < 0x61)))
			return FALSE;
	}
	else
	{
		if ((wParam != 0x30) && (wParam != 0x31))
			return FALSE;
	}

	if (_pCurProp->isLittle == FALSE)
	{
		textPos = _pCurProp->cursorPos / FACTOR;
		posBeg  = _pCurProp->cursorItem * VIEW_ROW + (_pCurProp->cursorSubItem-1) * _pCurProp->bits + textPos;
	}
	else if (FULL_SUBITEM)
	{
		textPos = (_pCurProp->bits-1) - _pCurProp->cursorPos / FACTOR;
		posBeg  = _pCurProp->cursorItem * VIEW_ROW + (_pCurProp->cursorSubItem-1) * _pCurProp->bits + textPos;
	}
	else
	{
		textPos = 0;
		posBeg  = _pCurProp->cursorItem * VIEW_ROW + (_pCurProp->cursorSubItem-1) * _pCurProp->bits;
	}

	/* prepare text */
	ListView_GetItemText(_hListCtrl, _pCurProp->cursorItem, _pCurProp->cursorSubItem, text, SUBITEM_LENGTH);
	text[_pCurProp->cursorPos] = (char)wParam;

	if (_currLength > GetCurrentPos())
	{
		if (FULL_SUBITEM)
			BinHexConvert(text, SUBITEM_LENGTH);
		else
			BinHexConvert(text, (_currLength % _pCurProp->bits) * FACTOR);

		/* calculate correct position */
		ScintillaMsg(_hParentHandle, SCI_SETTARGETSTART, posBeg);
		ScintillaMsg(_hParentHandle, SCI_SETTARGETEND, posBeg + 1);

		/* update text */
		ScintillaMsg(_hParentHandle, SCI_REPLACETARGET, 1, (LPARAM)&text[textPos]);
		ScintillaMsg(_hParentHandle, SCI_SETSEL, posBeg, posBeg);
	}
	else
	{
		memset(&text[_pCurProp->cursorPos+1], 0x20, FACTOR);
		BinHexConvert(text, ((_currLength % _pCurProp->bits) + 1) * FACTOR);

		ScintillaMsg(_hParentHandle, SCI_SETCURRENTPOS, posBeg);
		ScintillaMsg(_hParentHandle, SCI_ADDTEXT, 1, (LPARAM)&text[textPos]);
	}

	/* set position */
	_pCurProp->cursorPos++;
	if ((INT)_pCurProp->cursorPos >= SUBITEM_LENGTH)
	{
		::SendMessage(_hListCtrl, WM_KEYDOWN, VK_RIGHT, 0);
	}

	InvalidateList();
	return TRUE;
}


void HexEdit::DrawItemText(HDC hDc, DWORD item, INT subItem)
{
	RECT		rc;
	RECT		rcText;
	SIZE		size;
	char		text[64];
	RECT		rcCursor;
	HBRUSH		bgbrush	 = ::CreateSolidBrush(RGB(0xFF,0xFF,0xFF));

	/* get list informations */
	ListView_GetItemText(_hListCtrl, item, subItem, text, 64);
	ListView_GetSubItemRect(_hListCtrl, item, subItem, LVIR_BOUNDS, &rc);

	/* delete background */
	::SetBkMode(hDc, TRANSPARENT);
	::FillRect(hDc, &rc, bgbrush);

	/* calculate text position */
	::GetTextExtentPoint(hDc, text, SUBITEM_LENGTH, &size);
	size.cx		= (rc.right  - rc.left - size.cx) / 2;
	rcText		= rc;
	rcText.left += size.cx;
	rcCursor	= rcText;
	::GetTextExtentPoint(hDc, "0", 1, &size);

	if ((item * VIEW_ROW + subItem * _pCurProp->bits) > _currLength)
	{
		if ((item * VIEW_ROW + subItem * _pCurProp->bits) <= (_currLength + _pCurProp->bits))
		{
			/* clear text */
			memset(&text[(_currLength%_pCurProp->bits)*FACTOR], 0x20, SUBITEM_LENGTH);
		}
		else
		{
			/* clear text */	
			memset(text, 0x20, SUBITEM_LENGTH);
		}
	}

	::SetTextColor(hDc, RGB(0x00,0x00,0x00));
	::TextOut(hDc, rcText.left, rcText.top, text, SUBITEM_LENGTH);

	if (_pCurProp->isSel == FALSE)
	{
		/* paint cursor */
		if ((subItem == _pCurProp->cursorSubItem) && (_pCurProp->editType == HEX_EDIT_HEX) && (_isCurOn == TRUE))
		{
			HBRUSH		hbrush	 = ::CreateSolidBrush(RGB(0x00,0x00,0x00));
			rcCursor.left  += (size.cx * _pCurProp->cursorPos);
			rcCursor.right  = rcCursor.left + 1;
			::FillRect(hDc, &rcCursor, hbrush);
			::DeleteObject(hbrush);
		}
	}
	else
	{
		INT		firstCur  = _pCurProp->anchorPos;
		INT		lastCur   = _pCurProp->cursorPos;
		INT		firstSub  = _pCurProp->anchorSubItem;
		INT		lastSub   = _pCurProp->cursorSubItem;
		DWORD	firstItem = _pCurProp->anchorItem;
		DWORD	lastItem  = _pCurProp->cursorItem;

		/* correct cursor sub item */
		if (firstItem > lastItem)
		{
			firstItem = _pCurProp->cursorItem;
			lastItem  = _pCurProp->anchorItem;
		}
		if (((_pCurProp->anchorSubItem > _pCurProp->cursorSubItem) && (_pCurProp->selection == HEX_SEL_BLOCK)) ||
			((_pCurProp->anchorItem > _pCurProp->cursorItem) && (_pCurProp->selection == HEX_SEL_NORM)) ||
			((_pCurProp->anchorItem == _pCurProp->cursorItem) && (_pCurProp->anchorSubItem > _pCurProp->cursorSubItem) && (_pCurProp->selection == HEX_SEL_NORM)))
		{
			firstCur = _pCurProp->cursorPos;
			lastCur  = _pCurProp->anchorPos;
			firstSub = _pCurProp->cursorSubItem;
			lastSub  = _pCurProp->anchorSubItem;
		}
		if ((_pCurProp->anchorSubItem == _pCurProp->cursorSubItem) && 
			(_pCurProp->anchorPos > _pCurProp->cursorPos) && 
			(((_pCurProp->anchorItem == _pCurProp->cursorItem) && (_pCurProp->selection == HEX_SEL_NORM)) ||
			 (_pCurProp->selection == HEX_SEL_BLOCK)))
		{
			firstCur = _pCurProp->cursorPos;
			lastCur  = _pCurProp->anchorPos;
		}

		HBRUSH		hbrush	 = SELECTION_COLOR;
		INT			factor1  = (firstCur % _pCurProp->bits) * FACTOR;
		INT			factor2  = (lastCur  % _pCurProp->bits) * FACTOR;

		/* set text color to white and calculate cursor position */
		::SetTextColor(hDc, RGB(0xFF,0xFF,0xFF));

		switch (_pCurProp->selection)
		{
			case HEX_SEL_NORM:
			{
				if ((firstItem == item) && (lastItem == item))
				{
					/* go to HEX_SEL_BLOCK */
				}
				else
				{
					if ((firstSub == subItem) && (firstItem == item))
					{
						rcText.left += (size.cx * factor1);
						if (subItem == _pCurProp->columns)
						{
							rcText.right = rcText.left + (size.cx * (SUBITEM_LENGTH - factor1));
						}
						::FillRect(hDc, &rcText, hbrush);
						::TextOut(hDc, rcText.left, rcText.top, &text[factor1], SUBITEM_LENGTH-factor1);
					}
					else if (((item == firstItem) && (subItem > firstSub)) ||
							((item > firstItem) && (item < lastItem)) ||
							((item == lastItem) && (subItem < lastSub)))
					{
						if (subItem == 1)
						{
							::FillRect(hDc, &rcText, hbrush);
							::TextOut(hDc, rcText.left, rcText.top, text, SUBITEM_LENGTH);
						}
						else if (subItem == _pCurProp->columns)
						{
							rc.right = rcText.left + (size.cx * SUBITEM_LENGTH);
							::FillRect(hDc, &rc, hbrush);
							::TextOut(hDc, rcText.left, rcText.top, text, SUBITEM_LENGTH);
						}
						else
						{
							::FillRect(hDc, &rc, hbrush);
							::TextOut(hDc, rcText.left, rcText.top, text, SUBITEM_LENGTH);
						}
					}
					else if ((lastSub == subItem) && (lastItem == item))
					{
						if (subItem == 1)
						{
							rc.left  = rcText.left;
						}
						rc.right = rcText.left + (size.cx * factor2);
						::FillRect(hDc, &rc, hbrush);
						::TextOut(hDc, rcText.left, rcText.top, text, factor2);
					}
					break;
				}
			}
			case HEX_SEL_BLOCK:
			{
				if ((firstSub == subItem) && (lastSub == subItem))
				{
					rcText.left  += (size.cx * factor1);
					rcText.right = rcText.left + (size.cx * (factor2-factor1));
					::FillRect(hDc, &rcText, hbrush);
					::TextOut(hDc, rcText.left, rcText.top, &text[factor1], factor2-factor1);
				}
				else if (firstSub == subItem)
				{
					rcText.left  += (size.cx * factor1);
					if (subItem == _pCurProp->columns)
					{
						rcText.right = rcText.left + (size.cx * SUBITEM_LENGTH);
					}
					::FillRect(hDc, &rcText, hbrush);
					::TextOut(hDc, rcText.left, rcText.top, &text[factor1], SUBITEM_LENGTH-factor1);
				}
				else if ((subItem > firstSub) && (subItem < lastSub))
				{
					if ((_pCurProp->cursorSubItem == DUMP_FIELD) && (subItem == _pCurProp->columns))
						rc.right = rcText.left + (size.cx * SUBITEM_LENGTH);
					::FillRect(hDc, &rc, hbrush);
					::TextOut(hDc, rcText.left, rcText.top, text, SUBITEM_LENGTH);
				}
				else if (lastSub == subItem)
				{
					rc.right = rcText.left + (size.cx * factor2);
					::FillRect(hDc, &rc, hbrush);
					::TextOut(hDc, rcText.left, rcText.top, text, factor2);
				}
				break;
			}
		}
		::DeleteObject(hbrush);

		/* paint cursor */
		if ((_pCurProp->editType == HEX_EDIT_HEX) && (item == _pCurProp->cursorItem) && (_isCurOn == TRUE) &&
			(subItem == _pCurProp->cursorSubItem))
		{
			hbrush = ::CreateSolidBrush(RGB(0x00,0x00,0x00));
			rcCursor.left  += (size.cx * (_pCurProp->cursorPos % _pCurProp->bits) * FACTOR);
			rcCursor.right  = rcCursor.left + 1;
			::FillRect(hDc, &rcCursor, hbrush);
			::DeleteObject(hbrush);
		}
	}

	::DeleteObject(bgbrush);
}



















void HexEdit::SelectDump(INT iItem, INT iCursor)
{
	UINT	editMax = iItem * VIEW_ROW + iCursor + 1;

	if ((iItem != -1) && (iCursor <= VIEW_ROW) && (editMax <= _currLength + 1))
	{
		_pCurProp->cursorItem		= iItem;
		_pCurProp->cursorSubItem	= iCursor / DUMP_FIELD;
		_pCurProp->cursorPos		= iCursor;
		_pCurProp->isSel			= FALSE;

		if ((editMax == _currLength + 1) && (iCursor == 0) &&
			((_currLength / VIEW_ROW) == ListView_GetItemCount(_hListCtrl)))
		{
			INT itemCnt = ListView_GetItemCount(_hListCtrl);
			ListView_SetItemCountEx(_hListCtrl, itemCnt + 1, 0);
		}
		ListView_EnsureVisible(_hListCtrl, iItem, TRUE);
		InvalidateList();
	}

	/* set position in scintilla */
	if (_pCurProp->isLittle == FALSE)
		ScintillaMsg(SCI_GOTOPOS, VIEW_ROW * _pCurProp->cursorItem + _pCurProp->cursorPos);
	else
		ScintillaMsg(SCI_GOTOPOS, VIEW_ROW * _pCurProp->cursorItem + _pCurProp->cursorPos + 
								  (_pCurProp->cursorPos % _pCurProp->bits) + 
								  ((_pCurProp->bits-1) - (_pCurProp->cursorPos % _pCurProp->bits)));
}


void HexEdit::OnMouseClickDump(WPARAM wParam, LPARAM lParam)
{
	LV_HITTESTINFO	info;

	/* get selected sub item */
	info.pt.x = LOWORD(lParam);
	info.pt.y = HIWORD(lParam);
	ListView_SubItemHitTest(_hListCtrl, &info);

	if (0x80 & ::GetKeyState(VK_SHIFT))
	{
		UINT	posBeg		= GetAnchor();
		bool	isMenu		= ((0x80 & ::GetKeyState(VK_MENU)) == 0x80);

		SetSelection(posBeg, info.iItem * VIEW_ROW + CalcCursorPos(info), isMenu ? HEX_SEL_BLOCK:HEX_SEL_NORM);
	}
	else
	{
		SelectDump(info.iItem, CalcCursorPos(info));
	}
}


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
			if (_pCurProp->cursorItem != (_currLength/_pCurProp->columns/_pCurProp->bits))
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
			else if (_pCurProp->cursorPos == 0)
			{
				if (_pCurProp->cursorItem != 0)
					SelectDump(_pCurProp->cursorItem - 1, VIEW_ROW-1);
			}
			else
				SelectDump(_pCurProp->cursorItem, _pCurProp->cursorPos - 1);
			return FALSE;
		}
		case VK_RIGHT:
		{
			if ((_pCurProp->cursorItem * VIEW_ROW + _pCurProp->cursorPos) <= _currLength)
			{
				if (isShift)
				{
					SelectionKeys(wParam, lParam);
				}
				else if (_pCurProp->cursorPos < (UINT)(VIEW_ROW - 1))
					SelectDump(_pCurProp->cursorItem, _pCurProp->cursorPos + 1);
				else
					SelectDump(_pCurProp->cursorItem + 1, 0);
			}
			return FALSE;
		}
		default:
			GlobalKeys(wParam, lParam);
			break;
	}
	return TRUE;
}


void HexEdit::OnCharDump(WPARAM wParam, LPARAM lParam)
{
	switch (wParam)
	{
		case 0x09:		/* TAB */
		{
			GlobalKeys(wParam, lParam);
			break;
		}
		default:
		{
			/* calculate correct position */
			UINT posBeg = 0;

			if (_pCurProp->isLittle == FALSE)
			{
				posBeg = VIEW_ROW * _pCurProp->cursorItem + _pCurProp->cursorPos;
			}
			else
			{
				posBeg = VIEW_ROW * _pCurProp->cursorItem + _pCurProp->cursorPos - 
					(_pCurProp->cursorPos % _pCurProp->bits) + 
					((_pCurProp->bits-1) - (_pCurProp->cursorPos % _pCurProp->bits));
			}

			if (_currLength != GetCurrentPos())
			{
				/* update text */
				ScintillaMsg(_hParentHandle, SCI_SETTARGETSTART, posBeg);
				ScintillaMsg(_hParentHandle, SCI_SETTARGETEND, posBeg + 1);
				ScintillaMsg(_hParentHandle, SCI_REPLACETARGET, 1, (LPARAM)&wParam);
				ScintillaMsg(_hParentHandle, SCI_SETSEL, posBeg+1, posBeg+1);
			}
			else
			{
				/* add char */
				if (_pCurProp->isLittle == TRUE)
				{
					ScintillaMsg(_hParentHandle, SCI_SETCURRENTPOS, _currLength - (_currLength%_pCurProp->bits));
				}
				ScintillaMsg(_hParentHandle, SCI_ADDTEXT, 1, (LPARAM)&wParam);
			}

			_pCurProp->cursorPos++;
			if (_pCurProp->cursorPos >= (UINT)VIEW_ROW)
			{
				::SendMessage(_hListCtrl, WM_KEYDOWN, VK_RIGHT, 0);
			}

			InvalidateList();
		}
	}
}


void HexEdit::DrawDumpText(HDC hDc, DWORD item, INT subItem)
{
	RECT		rc;
	SIZE		size;
	char		text[129];
	RECT		rcCursor;
	HBRUSH		bgbrush	 = ::CreateSolidBrush(RGB(0xFF,0xFF,0xFF));

	/* get list informations */
	ListView_GetItemText(_hListCtrl, item, subItem, text, 129);
	ListView_GetSubItemRect(_hListCtrl, item, subItem, LVIR_BOUNDS, &rc);

	/* paint background */
	::SetBkMode(hDc, TRANSPARENT);
	::FillRect(hDc, &rc, bgbrush);

	/* calculate cursor */
	rc.left += 6;
	rcCursor = rc;
	::GetTextExtentPoint(hDc, "0", 1, &size);

	/* draw text */
	::SetTextColor(hDc, RGB(0x00,0x00,0x00));
	if (ListView_GetItemCount(_hListCtrl) != (item + 1))
	{
		::TextOut(hDc, rc.left, rc.top, text, VIEW_ROW);
	}
	else
	{
		UINT	diff = _currLength - (item * VIEW_ROW);
		::TextOut(hDc, rc.left, rc.top, text, diff);
		text[diff] = 0x20;
	}

	if (_pCurProp->isSel == FALSE)
	{
		if ((_pCurProp->editType == HEX_EDIT_ASCII) && (_isCurOn == TRUE))
		{
			HBRUSH		hbrush	 = ::CreateSolidBrush(RGB(0x00,0x00,0x00));
			rcCursor.left  += (size.cx * _pCurProp->cursorPos);
			rcCursor.right  = rcCursor.left + 1;
			::FillRect(hDc, &rcCursor, hbrush);
			::DeleteObject(hbrush);
		}
	}
	else
	{
		INT		length    = 0;
		INT		begText	  = 0;
		DWORD	firstItem = _pCurProp->anchorItem;
		DWORD	lastItem  = _pCurProp->cursorItem;
		INT		firstCur  = _pCurProp->anchorPos;
		INT		lastCur   = _pCurProp->cursorPos - 1;

		/* change first item last item sequence */
		if (firstItem > lastItem)
		{
			firstItem = _pCurProp->cursorItem;
			lastItem  = _pCurProp->anchorItem;
		}
		/* change cursor sequence */
		if (((_pCurProp->selection == HEX_SEL_BLOCK) && ((_pCurProp->anchorSubItem > _pCurProp->cursorSubItem) ||
			((_pCurProp->anchorSubItem == _pCurProp->cursorSubItem) && (_pCurProp->anchorPos > _pCurProp->cursorPos)))) ||
			(((_pCurProp->anchorItem > _pCurProp->cursorItem) && (_pCurProp->selection == HEX_SEL_NORM)) ||
			 ((_pCurProp->anchorPos > _pCurProp->cursorPos) && (_pCurProp->anchorItem == _pCurProp->cursorItem) && (_pCurProp->selection == HEX_SEL_NORM))))
		{
			firstCur = _pCurProp->cursorPos;
			lastCur  = _pCurProp->anchorPos - 1;
		}

		switch (_pCurProp->selection)
		{
			case HEX_SEL_NORM:
			{
				if (firstItem == lastItem)
				{
					/* go to HEX_SEL_BLOCK */
				}
				else
				{
					if (firstItem == item)
					{
						/* calculate length and selection of string */
						length		= VIEW_ROW - firstCur;
						rc.left		+= (size.cx * firstCur);
						rc.right	= rc.left + (size.cx * length);
						begText		= firstCur;
					}
					else if ((item > firstItem) && (item < lastItem))
					{
						/* calculate length and selection of string */
						length		= VIEW_ROW;
						rc.right	= rc.left + (size.cx * length);
					}
					else if (item == lastItem)
					{
						/* calculate length and selection of string */
						length		= lastCur;
						length		+= (length < VIEW_ROW)? 1:0;
						rc.right	= rc.left + (size.cx * length);
					}
					break;
				}
			}
			case HEX_SEL_BLOCK:
			{
				/* calculate length and selection of string */
				length		= lastCur-firstCur;
				length		+= (firstCur+length < VIEW_ROW)? 1:0;
				rc.left		+= (size.cx * firstCur);
				rc.right	= rc.left + (size.cx * length);
				begText		= firstCur;
				break;
			}
		}

		/* draw text and background */
		HBRUSH	hbrush  = SELECTION_COLOR;

		::FillRect(hDc, &rc, hbrush);
		::SetTextColor(hDc, RGB(0xFF,0xFF,0xFF));
		::TextOut(hDc, rc.left, rc.top, &text[begText], length);
		::DeleteObject(hbrush);


		/* paint cursor */
		if ((item == _pCurProp->cursorItem) && (_pCurProp->editType == HEX_EDIT_ASCII) && (_isCurOn == TRUE))
		{
			hbrush = ::CreateSolidBrush(RGB(0x00,0x00,0x00));
			rcCursor.left  += (size.cx * _pCurProp->cursorPos);
			rcCursor.right  = rcCursor.left + 1;
			::FillRect(hDc, &rcCursor, hbrush);
			::DeleteObject(hbrush);
		}
	}
	::DeleteObject(bgbrush);
}










INT HexEdit::CalcCursorPos(LV_HITTESTINFO info)
{
	RECT			rc;
	SIZE			size;
	char			text[64];
	UINT			cursorPos;

	/* get dc for calculate the font size */
	HDC				hDc = ::GetDC(_hListCtrl);

	/* for zooming */
	SelectObject(hDc, _hFont);

	/* get item informations */
	ListView_GetItemText(_hListCtrl, info.iItem, info.iSubItem, text, 64);
	ListView_GetSubItemRect(_hListCtrl, info.iItem, info.iSubItem, LVIR_BOUNDS, &rc);

	if (info.iSubItem == DUMP_FIELD)
	{
		/* left offset */
		::GetTextExtentPoint(hDc, text, VIEW_ROW, &size);
		rc.left += 6;

		/* get clicked position */
		::GetTextExtentPoint(hDc, "0", 1, &size);
		cursorPos = (info.pt.x - rc.left + size.cx / 2) / size.cx;

		if (cursorPos >= (UINT)VIEW_ROW)
		{
			if (_pCurProp->isSel == TRUE)
				cursorPos = VIEW_ROW;
			else
				cursorPos = VIEW_ROW - 1;
		}
	}
	else
	{
		/* left offset */
		::GetTextExtentPoint(hDc, text, SUBITEM_LENGTH, &size);
		size.cx = ((rc.right - rc.left) - size.cx) / 2;
		rc.left   += size.cx;
		::GetTextExtentPoint(hDc, "0", 1, &size);

		cursorPos = (info.pt.x - rc.left + size.cx / 2) / size.cx;

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
				cursorPos = (maxPos == 0)?0:maxPos;
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
	::ReleaseDC(_hSelf, hDc);
	
	return cursorPos;
}


void HexEdit::GlobalKeys(WPARAM wParam, LPARAM lParam)
{
	INT		posBeg;
	INT		posEnd;
	bool	isShift   = ((0x80 & ::GetKeyState(VK_SHIFT)) == 0x80);
	eSel	selection = ((0x80 & ::GetKeyState(VK_MENU)) == 0x80 ? HEX_SEL_BLOCK:HEX_SEL_NORM);

	switch (wParam)
	{
		case VK_PRIOR:
		case VK_NEXT:
			if (isShift == false)
			{
				if (wParam == VK_NEXT)
				{
					_pCurProp->cursorItem += ListView_GetCountPerPage(_hListCtrl);
					if (_pCurProp->cursorItem >= (UINT)ListView_GetItemCount(_hListCtrl))
					{
						_pCurProp->cursorItem = ListView_GetItemCount(_hListCtrl) - 1;
						if (GetCurrentPos() > _currLength)
							SetPosition(_currLength);
					}
				}
				else
				{
					_pCurProp->cursorItem -= ListView_GetCountPerPage(_hListCtrl);
					if (_pCurProp->cursorItem < 0)
						_pCurProp->cursorItem = 0;
				}
				SetLineVis(_pCurProp->cursorItem, HEX_LINE_MIDDLE);
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
					SetSelection(posBeg, posEnd, selection);
				}
				else
				{
					GetSelection(&posBeg, &posEnd);
					posEnd -= ListView_GetCountPerPage(_hListCtrl) * VIEW_ROW;
					if (posEnd < 0)
						posEnd = 0;
					SetSelection(posBeg, posEnd, selection);
				}
				SetLineVis(_pCurProp->cursorItem, HEX_LINE_MIDDLE);
				_pCurProp->isSel = TRUE;
			}
			break;
		case VK_HOME:
			if (isShift == false)
			{
				if (0x80 & ::GetKeyState(VK_CONTROL))
				{
					SetPosition(0);
				}
				else
				{
					if (_pCurProp->editType == HEX_EDIT_HEX)
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
					SetSelection(posBeg, 0, selection);
					SetLineVis(0, HEX_LINE_FIRST);
				}
				else
				{
					GetSelection(&posBeg, &posEnd);
					SetSelection(posBeg, _pCurProp->cursorItem * VIEW_ROW, selection);
				}
			}
			break;
		case VK_END:
			if (isShift == false)
			{
				if (0x80 & ::GetKeyState(VK_CONTROL))
				{
					SetPosition(_currLength);
				}
				else
				{
					/* set correct end position */
					if (_pCurProp->editType == HEX_EDIT_HEX)
					{
						if ((_pCurProp->cursorItem != ListView_GetItemCount(_hListCtrl) - 1) ||
							((_currLength % VIEW_ROW) == 0))
							SelectItem(_pCurProp->cursorItem, _pCurProp->columns, SUBITEM_LENGTH-FACTOR);
						else
							SelectItem(_pCurProp->cursorItem, (_currLength % VIEW_ROW) / _pCurProp->bits + 1, (_currLength % _pCurProp->bits) * FACTOR);
					}
					else
					{
						if ((_pCurProp->cursorItem != ListView_GetItemCount(_hListCtrl) - 1) ||
							((_currLength % VIEW_ROW) == 0))
							SelectDump(_pCurProp->cursorItem, VIEW_ROW-1);
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
					SetSelection(posBeg, _currLength, selection, TRUE);
					SetLineVis(ListView_GetItemCount(_hListCtrl)-1, HEX_LINE_LAST);
				}
				else
				{
					GetSelection(&posBeg, &posEnd);
					SetSelection(posBeg, (_pCurProp->cursorItem+1) * VIEW_ROW, selection, TRUE);
				}
			}
			break;
		case VK_TAB:
			/* nothing happens here */
			break;
		case VK_ADD:
			if (0x80 & ::GetKeyState(VK_CONTROL))
				::SendMessage(_hParent, WM_COMMAND, IDM_VIEW_ZOOMIN, 0);
			break;
		case VK_SUBTRACT:
			if (0x80 & ::GetKeyState(VK_CONTROL))
				::SendMessage(_hParent, WM_COMMAND, IDM_VIEW_ZOOMOUT, 0);
			break;
		case VK_DIVIDE:
			if (0x80 & ::GetKeyState(VK_CONTROL))
				::SendMessage(_hParent, WM_COMMAND, IDM_VIEW_ZOOMRESTORE, 0);
			break;
		case VK_BACK:
			::SendMessage(_hListCtrl, WM_KEYDOWN, VK_LEFT, 0);
			break;
		default:
			if ((0x80 & ::GetKeyState(VK_CONTROL)) || (0x80 & ::GetKeyState(VK_SHIFT)))
			{
				SetFocusNpp(_hParentHandle);
				if ((0x80 & ::GetKeyState(VK_SHIFT)) && (0x80 & ::GetKeyState(VK_MENU)) && (0x80 & ::GetKeyState('H')))
				{
					toggleHexEdit();
					SetFocusNpp(_hParentHandle);
				}
			}
			break;
	}
}


void HexEdit::SelectionKeys(WPARAM wParam, LPARAM lParam)
{
	INT		posBeg;
	INT		posEnd;
	bool	isMenu  = ((0x80 & ::GetKeyState(VK_MENU)) == 0x80);
	eSel	selection = (isMenu ? HEX_SEL_BLOCK:HEX_SEL_NORM);

	switch (wParam)
	{
		case VK_UP:
		{
			if (_pCurProp->cursorPos == VIEW_ROW)
			{
				if (_pCurProp->cursorItem != 0)
				{
					_pCurProp->cursorItem--;
					ListView_Update(_hListCtrl, 0);
					SetStatusBar();
				}
			}
			else
			{
				GetSelection(&posBeg, &posEnd);
				SetSelection(posBeg, posEnd - VIEW_ROW, selection);
			}
			break;
		}
		case VK_DOWN:
		{
			if (_pCurProp->cursorPos == VIEW_ROW)
			{
				if (_pCurProp->cursorItem != ListView_GetItemCount(_hListCtrl))
				{
					_pCurProp->cursorItem++;
					ListView_Update(_hListCtrl, 0);
					SetStatusBar();
				}
			}
			else
			{
				GetSelection(&posBeg, &posEnd);
				SetSelection(posBeg, posEnd + VIEW_ROW, selection);
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
				SetSelection(posBeg, posEnd - 1, selection);
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
					ListView_Update(_hListCtrl, 0);					
					SetStatusBar();
				}
			}
			else if (_pCurProp->cursorPos == VIEW_ROW)
			{
				GetSelection(&posBeg, &posEnd);
				SetSelection(posBeg, posEnd, selection);
			}
			else
			{
				GetSelection(&posBeg, &posEnd);
				SetSelection(posBeg, posEnd + 1, selection);
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
		if (_pCurProp->editType == HEX_EDIT_HEX)
		{
			SelectItem(pos / VIEW_ROW,((pos % VIEW_ROW)/_pCurProp->bits)+1, (pos % _pCurProp->bits) * FACTOR);
		}
		else
		{
			SelectDump(pos / VIEW_ROW, pos % VIEW_ROW);
		}
	}
	else
	{
		if (_pCurProp->editType == HEX_EDIT_HEX)
		{
			SelectItem(pos / VIEW_ROW,((pos % VIEW_ROW)/_pCurProp->bits)+1, (_pCurProp->bits) - ((pos % _pCurProp->bits)-1) * FACTOR);
		}
		else
		{
			SelectDump(pos / VIEW_ROW, (pos % VIEW_ROW) - 2*(pos % _pCurProp->bits) + (_pCurProp->bits - 1));
		}
	}
}


UINT HexEdit::GetCurrentPos(void)
{
	UINT	pos = 0;

	if (_pCurProp->isSel == FALSE)
	{
		if (_pCurProp->editType == HEX_EDIT_HEX)
		{
			pos = _pCurProp->cursorItem * VIEW_ROW + (_pCurProp->cursorSubItem-1) * _pCurProp->bits + _pCurProp->cursorPos / FACTOR;
		}
		else
		{	
			pos = _pCurProp->cursorItem * VIEW_ROW + _pCurProp->cursorPos;
		}
	}
	else
	{
		pos = _pCurProp->cursorItem * VIEW_ROW + _pCurProp->cursorPos;
	}

	return pos;
}


UINT HexEdit::GetAnchor(void)
{
	UINT	pos = 0;

	if (_pCurProp->isSel == FALSE)
	{
		pos = GetCurrentPos();
	}
	else
	{
		pos = _pCurProp->anchorItem * VIEW_ROW + _pCurProp->anchorPos;
	}

	return pos;
}


void HexEdit::GetSelection(LPINT posBegin, LPINT posEnd)
{
	*posBegin = GetAnchor();
	*posEnd   = GetCurrentPos();
}


void HexEdit::SetSelection(UINT posBegin, UINT posEnd, eSel selection, BOOL isEND)
{
	if ((posBegin <= _currLength) && (posEnd <= _currLength) &&
		(posBegin >= 0)           && (posEnd >= 0)) 
	{
		_pCurProp->anchorItem		= posBegin / VIEW_ROW;
		_pCurProp->anchorSubItem	= ((posBegin % VIEW_ROW)/_pCurProp->bits) + 1;
		_pCurProp->anchorPos		= posBegin % VIEW_ROW;
		_pCurProp->cursorItem		= posEnd / VIEW_ROW;
		_pCurProp->cursorSubItem	= ((posEnd % VIEW_ROW)/_pCurProp->bits) + 1;
		_pCurProp->cursorPos		= posEnd % VIEW_ROW;
		_pCurProp->isSel			= TRUE;
		_pCurProp->selection		= selection;

		if ((_pCurProp->cursorPos == 0) && (isEND == TRUE))
		{
			_pCurProp->cursorSubItem = DUMP_FIELD;
			_pCurProp->cursorPos = VIEW_ROW;
			_pCurProp->cursorItem--;
		}

		/* select also in parent view */
		ScintillaMsg(SCI_SETSEL, _pCurProp->anchorItem * VIEW_ROW + _pCurProp->anchorPos,
								 _pCurProp->cursorItem * VIEW_ROW + _pCurProp->cursorPos);
		InvalidateNotepad();
	}

	ListView_Update(_hListCtrl, 0);
}


void HexEdit::SetLineVis(UINT line, eLineVis mode)
{
	switch (mode)
	{
		case HEX_LINE_FIRST:
			ListView_EnsureVisible(_hListCtrl, (_currLength/_pCurProp->columns/_pCurProp->bits) - 1, FALSE);
			ListView_EnsureVisible(_hListCtrl, line, FALSE);
			break;
		case HEX_LINE_MIDDLE:
			ListView_EnsureVisible(_hListCtrl, line, TRUE);
			break;
		case HEX_LINE_LAST:
			ListView_EnsureVisible(_hListCtrl, 0, FALSE);
			ListView_EnsureVisible(_hListCtrl, line, FALSE);
			break;
		default:
			DEBUG("Mode Unknown");
			break;
	}
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


void HexEdit::GrayEncoding(void)
{
	HMENU	hMenu = ::GetMenu(_hParent);

	for (UINT i = IDM_FORMAT_ANSI; i <= IDM_FORMAT_AS_UTF_8; i++)
	{
		::EnableMenuItem(hMenu, i, MF_BYCOMMAND | (_pCurProp->isVisible?MF_GRAYED:MF_ENABLED));
	}
}


void HexEdit::SetStatusBar(void)
{
	if (_pCurProp->isVisible == TRUE)
	{
		char buffer[64];

		/* set mode */
		::SendMessage(_hParent, NPPM_SETSTATUSBAR, STATUSBAR_DOC_TYPE, (LPARAM)"Hex Edit View");
		/* set doc length */
		sprintf(buffer, "nb char : %d", _currLength);
		::SendMessage(_hParent, NPPM_SETSTATUSBAR, STATUSBAR_DOC_SIZE, (LPARAM)buffer);

		/* set doc length */
		sprintf(buffer, "Ln : %d    Col : %d    Sel : %d", 
			_pCurProp->cursorItem + 1, 
			(GetCurrentPos() % VIEW_ROW) + 1,
			(GetCurrentPos() > GetAnchor() ? GetCurrentPos()-GetAnchor() : GetAnchor()-GetCurrentPos()));
		::SendMessage(_hParent, NPPM_SETSTATUSBAR, STATUSBAR_CUR_POS, (LPARAM)buffer);

		/* display information in which mode it is (binary or hex) */
		::SendMessage(_hParent, NPPM_SETSTATUSBAR, STATUSBAR_EOF_FORMAT, (LPARAM)(_pCurProp->isBin == FALSE?"Hex":"Binary"));

		/* display information in which mode it is (BigEndian or Little) */
		::SendMessage(_hParent, NPPM_SETSTATUSBAR, STATUSBAR_UNICODE_TYPE, (LPARAM)(_pCurProp->isLittle == FALSE?"BigEndian":"LittleEndian"));
	}
}


