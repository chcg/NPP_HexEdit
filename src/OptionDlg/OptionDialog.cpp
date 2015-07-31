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

#include "OptionDialog.h"
#include "PluginInterface.h"
#include <Commctrl.h>
#include <shlobj.h>



static char*	szTabNames[11] = {
	"View",
	"Extensions"
};

typedef enum {
	PROP_VIEW = 0,
	PROP_EXT
} ePropStr;



UINT OptionDlg::doDialog(tProp *prop)
{
	_pProp = prop;
	return (UINT)::DialogBoxParam(_hInst, MAKEINTRESOURCE(IDD_OPTION_DLG), _hParent,  (DLGPROC)dlgProc, (LPARAM)this);
}


BOOL CALLBACK OptionDlg::run_dlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message) 
	{
		case WM_INITDIALOG:
		{
			TCITEM		item;
			item.mask		= TCIF_TEXT;

			goToCenter();
			
			item.pszText	= szTabNames[PROP_VIEW];
			item.cchTextMax	= (int)strlen(szTabNames[PROP_VIEW]);
			::SendDlgItemMessage(_hSelf, IDC_TAB_PROP, TCM_INSERTITEM, PROP_VIEW, (LPARAM)&item);
			item.pszText	= szTabNames[PROP_EXT];
			item.cchTextMax	= (int)strlen(szTabNames[PROP_EXT]);
			::SendDlgItemMessage(_hSelf, IDC_TAB_PROP, TCM_INSERTITEM, PROP_EXT, (LPARAM)&item);
			TabUpdate();
			SetParams();
			break;
		}
		case WM_COMMAND : 
		{
			switch (LOWORD(wParam))
			{
				case IDCANCEL:
					::EndDialog(_hSelf, IDCANCEL);
					return TRUE;
				case IDOK:
				{
					if (GetParams() == FALSE)
					{
						return FALSE;
					}
					::EndDialog(_hSelf, IDOK);
					return TRUE;
				}
				default:
					return FALSE;
			}
			break;
		}
		case WM_NOTIFY:
		{
			NMHDR	nmhdr = *((LPNMHDR)lParam);

			switch (nmhdr.idFrom)
			{
				case IDC_TAB_PROP:
				{
					if (nmhdr.code == TCN_SELCHANGE)
					{
						TabUpdate();
						return TRUE;
					}
					break;
				}
				default:
					break;
			}
			break;
		}
		default:
			break;
	}
	return FALSE;
}


void OptionDlg::TabUpdate(void)
{
	INT		iSel	= (INT)::SendDlgItemMessage(_hSelf, IDC_TAB_PROP, TCM_GETCURSEL, 0, 0);

	BOOL	bView	= (iSel == PROP_VIEW? SW_SHOW:SW_HIDE);
	BOOL	bExt	= (iSel == PROP_EXT ? SW_SHOW:SW_HIDE);

	/* set tab visibility state of "View" */
	::ShowWindow(::GetDlgItem(_hSelf, IDC_RADIO_8),			bView);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_RADIO_16),		bView);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_RADIO_32),		bView);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_RADIO_64),		bView);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_RADIO_BIG),		bView);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_RADIO_LITTLE),	bView);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_RADIO_HEX),		bView);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_RADIO_BIN),		bView);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_COLUMN_EDIT),		bView);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_STATIC_COLUMN),	bView);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_CHECK_CLM),		bView);

	/* set tab visibility state of "Extension" */
	::ShowWindow(::GetDlgItem(_hSelf, IDC_STATIC_NOTE),		bExt);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_STATIC_EXAMPLE),	bExt);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_EDIT_EXTLIST),	bExt);

	if (bExt == TRUE)
	{
		::SetFocus(::GetDlgItem(_hSelf, IDC_EDIT_EXTLIST));
	}
}


void OptionDlg::SetParams(void)
{
	char		text[16];
	extern char	pszExtensions[256];

	::SetWindowText(::GetDlgItem(_hSelf, IDC_COLUMN_EDIT), itoa(_pProp->hex.columns, text, 10));
	switch (_pProp->hex.bits)
	{
		case HEX_BYTE:
			::SendDlgItemMessage(_hSelf, IDC_RADIO_8, BM_SETCHECK, BST_CHECKED, 0); 
			break;
		case HEX_WORD:
			::SendDlgItemMessage(_hSelf, IDC_RADIO_16, BM_SETCHECK, BST_CHECKED, 0); 
			break;
		case HEX_DWORD:
			::SendDlgItemMessage(_hSelf, IDC_RADIO_32, BM_SETCHECK, BST_CHECKED, 0); 
			break;
		case HEX_LONG:
			::SendDlgItemMessage(_hSelf, IDC_RADIO_64, BM_SETCHECK, BST_CHECKED, 0); 
			break;
		default:
			break;
	}
	if (_pProp->hex.isLittle == FALSE)
	{
		::SendDlgItemMessage(_hSelf, IDC_RADIO_BIG, BM_SETCHECK, BST_CHECKED, 0); 
	}
	else
	{
		::SendDlgItemMessage(_hSelf, IDC_RADIO_LITTLE, BM_SETCHECK, BST_CHECKED, 0); 
	}
	if (_pProp->hex.isBin == FALSE)
	{
		::SendDlgItemMessage(_hSelf, IDC_RADIO_HEX, BM_SETCHECK, BST_CHECKED, 0); 
	}
	else
	{
		::SendDlgItemMessage(_hSelf, IDC_RADIO_BIN, BM_SETCHECK, BST_CHECKED, 0); 
	}

	/* check CLM */
	if (_pProp->isCapital == TRUE)
	{
		::SendDlgItemMessage(_hSelf, IDC_CHECK_CLM, BM_SETCHECK, BST_CHECKED, 0); 
	}

	/* set extension list */
	::SetWindowText(::GetDlgItem(_hSelf, IDC_EDIT_EXTLIST), pszExtensions);
}


BOOL OptionDlg::GetParams(void)
{
	char		text[16];
	extern char	pszExtensions[256];
	UINT		col		= 0;
	UINT		bits	= 0;
	BOOL		bRet	= FALSE;

	/* get user define of capital letter mode (CLM) */
	if (::SendDlgItemMessage(_hSelf, IDC_CHECK_CLM, BM_GETCHECK, 0, 0) == BST_CHECKED)
		_pProp->isCapital = true;
	else
		_pProp->isCapital = false;

	/* get column count */
	::GetWindowText(::GetDlgItem(_hSelf, IDC_COLUMN_EDIT), text, 16);
	col = atoi(text);

	/* get bit alignment */
	if (::SendDlgItemMessage(_hSelf, IDC_RADIO_8, BM_GETCHECK, 0, 0) == BST_CHECKED)
		bits = HEX_BYTE;
	else if (::SendDlgItemMessage(_hSelf, IDC_RADIO_16, BM_GETCHECK, 0, 0) == BST_CHECKED)
		bits = HEX_WORD;
	else if (::SendDlgItemMessage(_hSelf, IDC_RADIO_32, BM_GETCHECK, 0, 0) == BST_CHECKED)
		bits = HEX_DWORD;
	else
		bits = HEX_LONG;

	/* test if values are possible */
	if ((col > 0) && (col <= (128 / bits)))
	{
		_pProp->hex.columns = col;

		_pProp->hex.bits    = bits;
		

		/* get endian */
		if (::SendDlgItemMessage(_hSelf, IDC_RADIO_BIG, BM_GETCHECK, 0, 0) == BST_CHECKED)
			_pProp->hex.isLittle = FALSE;
		else
			_pProp->hex.isLittle = TRUE;

		/* get display form */
		if (::SendDlgItemMessage(_hSelf, IDC_RADIO_HEX, BM_GETCHECK, 0, 0) == BST_CHECKED)
			_pProp->hex.isBin    = FALSE;
		else
			_pProp->hex.isBin    = TRUE;

		bRet = TRUE;
	}
	else
	{
		MessageBox(_hSelf, "Max column count can be 1 till 128 bytes in a row.", "HexEdit", MB_OK|MB_ICONERROR);
	}

	/* get extensions */
	if (bRet == TRUE)
	{
		::GetWindowText(::GetDlgItem(_hSelf, IDC_EDIT_EXTLIST), pszExtensions, 256);
	}

	return bRet;
}




