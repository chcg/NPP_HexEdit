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

#include "ColumnDialog.h"
#include "PluginInterface.h"


UINT ColumnDlg::doDialogColumn(UINT column)
{
	_isColumn = TRUE;
	_column = column;
	return (UINT)::DialogBoxParam(_hInst, MAKEINTRESOURCE(IDD_COLUMN_DLG), _hParent,  (DLGPROC)dlgProc, (LPARAM)this);
}

UINT ColumnDlg::doDialogAddWidth(UINT width)
{
	_isColumn = FALSE;
	_width = width;
	return (UINT)::DialogBoxParam(_hInst, MAKEINTRESOURCE(IDD_COLUMN_DLG), _hParent,  (DLGPROC)dlgProc, (LPARAM)this);
}


BOOL CALLBACK ColumnDlg::run_dlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	TCHAR	text[16];

	switch (Message) 
	{
		case WM_INITDIALOG:
		{
			goToCenter();

			if (_isColumn == TRUE) {
				::SetWindowText(::GetDlgItem(_hSelf, IDC_COLUMN_EDIT), _itot(_column, text, 10));
				NLChangeDialog(_hInst, _nppData._nppHandle, _hSelf, _T("ColumnCount"));
			} else {
				::SetWindowText(::GetDlgItem(_hSelf, IDC_COLUMN_EDIT), _itot(_width, text, 10));
				::SetWindowText(_hSelf, _T("Address Width"));
				NLChangeDialog(_hInst, _nppData._nppHandle, _hSelf, _T("AddressWidth"));
			}
			break;
		}
		case WM_COMMAND : 
		{
			switch (wParam)
			{
				case IDCANCEL:
					if (_isColumn == TRUE) {
						::EndDialog(_hSelf, _column);
					} else {
						::EndDialog(_hSelf, _width);
					}
					return TRUE;
				case IDOK:
					::GetWindowText(::GetDlgItem(_hSelf, IDC_COLUMN_EDIT), text, 16);
					::EndDialog(_hSelf, _ttoi(text));
					return TRUE;
				default:
					return FALSE;
			}
		}
		default:
			break;
	}
	return FALSE;
}
