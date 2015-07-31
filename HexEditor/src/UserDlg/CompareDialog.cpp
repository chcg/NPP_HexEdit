//this file is part of Hex Editor Plugin for Notepad++
//Copyright (C)2008 Jens Lorenz <jens.plugin.npp@gmx.de>
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

#include "CompareDialog.h"


UINT CompareDlg::doDialog(HexEdit *pHexEdit1, HexEdit *pHexEdit2, UINT currentSC)
{
	_currentSC	= currentSC;
	_pHexEdit1	= pHexEdit1;
	_pHexEdit2	= pHexEdit2;
	return (UINT)::DialogBoxParam(_hInst, MAKEINTRESOURCE(IDD_COMPARE_DLG), _hParent,  (DLGPROC)dlgProc, (LPARAM)this);
}

BOOL CALLBACK CompareDlg::run_dlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message) 
	{
		case WM_INITDIALOG:
		{
			goToCenter();

			_pHexEdit1->getWindowRect(_rcEdit1);
			_pHexEdit2->getWindowRect(_rcEdit2);
			_isUpDown = (_rcEdit1.left == _rcEdit2.left ? TRUE : FALSE);

			if (_isUpDown == FALSE) {
				/* hide the unneccesary elements */
				::ShowWindow(::GetDlgItem(_hSelf, IDC_STATIC_COMPARE_TB), SW_HIDE);
				::ShowWindow(::GetDlgItem(_hSelf, IDC_BUTTON_TOP), SW_HIDE);
				::ShowWindow(::GetDlgItem(_hSelf, IDC_BUTTON_BOTTOM), SW_HIDE);

				/* focus the correct button */
				if (_rcEdit1.left < _rcEdit2.left) {
					if (_currentSC == MAIN_VIEW) {
						::SetFocus(::GetDlgItem(_hSelf, IDC_BUTTON_LEFT));
					} else {
						::SetFocus(::GetDlgItem(_hSelf, IDC_BUTTON_RIGHT));
					}
				} else {
					if (_currentSC == SUB_VIEW) {
						::SetFocus(::GetDlgItem(_hSelf, IDC_BUTTON_LEFT));
					} else {
						::SetFocus(::GetDlgItem(_hSelf, IDC_BUTTON_RIGHT));
					}
				}
			} else {
				/* hide the unneccesary elements */
				::ShowWindow(::GetDlgItem(_hSelf, IDC_STATIC_COMPARE_LR), SW_HIDE);
				::ShowWindow(::GetDlgItem(_hSelf, IDC_BUTTON_LEFT), SW_HIDE);
				::ShowWindow(::GetDlgItem(_hSelf, IDC_BUTTON_RIGHT), SW_HIDE);

				/* focus the correct button */
				if (_rcEdit1.top < _rcEdit2.top) {
					if (_currentSC == MAIN_VIEW) {
						::SetFocus(::GetDlgItem(_hSelf, IDC_BUTTON_TOP));
					} else {
						::SetFocus(::GetDlgItem(_hSelf, IDC_BUTTON_BOTTOM));
					}
				} else {
					if (_currentSC == SUB_VIEW) {
						::SetFocus(::GetDlgItem(_hSelf, IDC_BUTTON_TOP));
					} else {
						::SetFocus(::GetDlgItem(_hSelf, IDC_BUTTON_BOTTOM));
					}
				}
			}

			/* change language */
			NLChangeDialog(_hInst, _nppData._nppHandle, _hSelf, _T("CompDialog"));
			break;
		}
		case WM_COMMAND : 
		{
			BOOL	owEdit2			= FALSE;

			switch (LOWORD(wParam))
			{
				case IDCANCEL:
                    ::EndDialog(_hSelf, IDCANCEL);
					return FALSE;

				case IDC_BUTTON_TOP:
					if (_rcEdit1.top < _rcEdit2.top)
						owEdit2 = TRUE;
					break;

				case IDC_BUTTON_BOTTOM:
					if (_rcEdit2.top < _rcEdit1.top)
						owEdit2 = TRUE;
					break;

				case IDC_BUTTON_LEFT:
					if (_rcEdit1.left < _rcEdit2.left)
						owEdit2 = TRUE;
					break;

				case IDC_BUTTON_RIGHT:
					if (_rcEdit2.left < _rcEdit1.left)
						owEdit2 = TRUE;
					break;

				default:
					break;
			}

			tHexProp hexProp1 = _pHexEdit1->GetHexProp();
			tHexProp hexProp2 = _pHexEdit2->GetHexProp();

			/* set settings in on of the hex edit */
			if (owEdit2 == TRUE)
			{
				tHexProp hexProp = hexProp2;
				hexProp.bits	= hexProp1.bits;
				hexProp.columns	= hexProp1.columns;
				hexProp.isBin	= hexProp1.isBin;
				_pHexEdit2->SetHexProp(hexProp);
			}
			else
			{
				tHexProp hexProp = hexProp1;
				hexProp.bits	= hexProp2.bits;
				hexProp.columns	= hexProp2.columns;
				hexProp.isBin	= hexProp2.isBin;
				_pHexEdit1->SetHexProp(hexProp);
			}
            ::EndDialog(_hSelf, LOWORD(wParam));
			break;
		}
		default:
			break;
	}
	return FALSE;
}
