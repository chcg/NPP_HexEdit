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

#include "GotoDialog.h"
#include "tables.h"


#define LINE_OFFSET	1

void GotoDlg::doDialog(HWND hHexEdit)
{
    if (!isCreated())
	{
        create(IDD_GOTO_DLG);
		::SendMessage(_hParent, NPPM_MODELESSDIALOG, MODELESSDIALOGADD, (LPARAM)_hSelf);
		goToCenter();
	}

	_hParentHandle = hHexEdit;

	UpdateDialog();
	display();
	::SetFocus(::GetDlgItem(_hSelf, IDC_EDIT_GOTO));
}


BOOL CALLBACK GotoDlg::run_dlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message) 
	{
		case WM_INITDIALOG:
		{
			/* initialize hex view */
			_isHex   = ::GetPrivateProfileInt(dlgEditor, gotoProp, 0, _iniFilePath);

			_hLineEdit = ::GetDlgItem(_hSelf, IDC_EDIT_GOTO);

			/* intial subclassing */
			::SetWindowLongPtr(_hLineEdit, GWL_USERDATA, reinterpret_cast<LONG>(this));
			_hDefaultEditProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtr(_hLineEdit, GWL_WNDPROC, reinterpret_cast<LONG>(wndEditProc)));

			::SendDlgItemMessage(_hSelf, IDC_CHECK_LINE, BM_SETCHECK, (_isHex == TRUE)?BST_UNCHECKED:BST_CHECKED, 0);
			::SendDlgItemMessage(_hSelf, IDC_RADIO_ADDRESS, BM_SETCHECK, BST_CHECKED, 0);

			/* change language */
			NLChangeDialog(_hInst, _nppData._nppHandle, _hSelf, _T("Goto"));

			break;
		}
		case WM_ACTIVATE :
        {
			UpdateDialog();
            break;
        }
		case WM_COMMAND : 
		{
			switch (LOWORD(wParam))
			{
				case IDCANCEL:
					display(false);
					break;
				case IDOK:
				{
					CHAR	text[16];
					UINT	iPos = 0;
					UINT	iMax = 0;

					if (_isHex == TRUE)
					{
						if (_isOff == TRUE) {
							::SendMessage(_hParentHandle, HEXM_GETPOS, 0, (LPARAM)&iPos);
						}

						::GetWindowTextA(_hLineEdit, text, 16);
						iPos += ASCIIConvert(text);

						::SendMessage(_hParentHandle, HEXM_GETLENGTH, 0, (LPARAM)&iMax);
						iMax--;

						if (iPos > iMax) {
							iPos = iMax;
						}
						::SendMessage(_hParentHandle, HEXM_SETSEL, iPos, (LPARAM)iPos);
					}
					else
					{
						if (_isOff == TRUE) {
							::SendMessage(_hParentHandle, HEXM_GETCURLINE, 0, (LPARAM)&iPos);
							iPos += LINE_OFFSET;
						}

						::GetWindowTextA(_hLineEdit, text, 16);
						iPos += atoi(text) - LINE_OFFSET;

						::SendMessage(_hParentHandle, HEXM_GETLINECNT, 0, (LPARAM)&iMax);

						if (iPos > iMax) {
							iPos = iMax;
						}
						::SendMessage(_hParentHandle, HEXM_SETCURLINE, 0, (LPARAM)iPos);
					}
					display(false);
					break;
				}
				case IDC_CHECK_LINE:
				{
					if (HIWORD(wParam) == BN_CLICKED)
					{
						calcAddress();
					}
					break;
				}
				case IDC_RADIO_ADDRESS:
				case IDC_RADIO_OFFSET:
				{
					calcAddress();
					break;
				}
				default:
					break;
			}
			break;
		}
		case WM_DESTROY :
		{
			/* deregister this dialog */
			::SendMessage(_hParent, NPPM_MODELESSDIALOG, MODELESSDIALOGREMOVE, (LPARAM)_hSelf);

			/* save view mode setting */
			::WritePrivateProfileString(dlgEditor, gotoProp, (_isHex == TRUE)?_T("1"):_T("0"), _iniFilePath);
			break;
		}
		default:
			break;
	}
	return FALSE;
}


LRESULT GotoDlg::runProcEdit(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message) 
	{
		case WM_CHAR:
		{
			if ((((wParam < 0x30) || (wParam > 0x66)) ||
				((wParam > 0x39) && (wParam < 0x41)) || 
				((wParam > 0x46) && (wParam < 0x61))) &&
				 (wParam != 0x08) && (_isHex == TRUE))
			{
				return TRUE;
			}
			else if (((wParam < 0x30) || (wParam > 0x39)) && 
					  (wParam != 0x08) && (_isHex == FALSE))
			{
				return TRUE;
			}
			break;
		}
		default:
			break;
	}

	return ::CallWindowProc(_hDefaultEditProc, hwnd, Message, wParam, lParam);
}


void GotoDlg::calcAddress(void)
{
	UINT		newPos		= 0;
	tHexProp	prop;
	CHAR		text[17];
	CHAR		temp[17];

	/* get new states */
	_isHex = (::SendDlgItemMessage(_hSelf, IDC_CHECK_LINE, BM_GETCHECK, 0, 0) == BST_CHECKED)? FALSE:TRUE;
	_isOff = (::SendDlgItemMessage(_hSelf, IDC_RADIO_OFFSET, BM_GETCHECK, 0, 0) == BST_CHECKED)? TRUE:FALSE;

	/* update also min and max */
	UpdateDialog();

	/* change user input */
	::SendMessage(_hParentHandle, HEXM_GETSETTINGS, 0, (LPARAM)&prop);

	::GetWindowTextA(_hLineEdit, temp, 17);

	if (strlen(temp) != 0)
	{
		if (_isOff == TRUE)
		{
			if (_isHex == TRUE)
			{
				newPos = atoi(temp) * prop.columns * prop.bits;
				sprintf(text, "%x", newPos);
				::SetWindowTextA(_hLineEdit, text);
			}
			else
			{
				newPos = ASCIIConvert(temp);
				newPos /= (prop.columns * prop.bits);
				sprintf(text, "%d", newPos);
				::SetWindowTextA(_hLineEdit, text);
			}
		}
		else
		{
			if (_isHex == TRUE)
			{
				newPos = atoi(temp) * prop.columns * prop.bits;
				sprintf(text, "%x", newPos);
				::SetWindowTextA(_hLineEdit, text);
			}
			else
			{
				newPos = ASCIIConvert(temp);
				newPos /= (prop.columns * prop.bits);
				sprintf(text, "%d", newPos + 1);
				::SetWindowTextA(_hLineEdit, text);
			}
		}
	}
}


void GotoDlg::UpdateDialog(void)
{
	tHexProp	prop;
	CHAR		text[17];

	::SendMessage(_hParentHandle, HEXM_GETSETTINGS, 0, (LPARAM)&prop);

	if (_isHex == TRUE)
	{
		UINT		curPos		= 0;
		UINT		maxPos		= 0;
		::SendMessage(_hParentHandle, HEXM_GETPOS, 0, (LPARAM)&curPos);
		::SendMessage(_hParentHandle, HEXM_GETLENGTH, 0, (LPARAM)&maxPos);
		maxPos--;

		/* set current line info */
		sprintf(text, "%08X", curPos);
		::SetDlgItemTextA(_hSelf, IDC_CURRLINE, text);

		/* set max possible position */
		if (_isOff == TRUE) {
			sprintf(text, "%08X", maxPos - curPos);
		} else {
			sprintf(text, "%08X", maxPos);
		}
		::SetDlgItemTextA(_hSelf, IDC_LASTLINE, text);
	}
	else
	{
		UINT		curLine		= 0;
		UINT		maxLine		= 0;
		::SendMessage(_hParentHandle, HEXM_GETCURLINE, 0, (LPARAM)&curLine);
		::SendMessage(_hParentHandle, HEXM_GETLINECNT, 0, (LPARAM)&maxLine);

		/* set current line info */
		::SetWindowTextA(::GetDlgItem(_hSelf, IDC_CURRLINE), itoa(curLine + LINE_OFFSET, text, 10));

		/* set max possible position */
		if (_isOff == TRUE) {
			::SetWindowTextA(::GetDlgItem(_hSelf, IDC_LASTLINE), itoa(maxLine - curLine, text, 10));
		} else {
			::SetWindowTextA(::GetDlgItem(_hSelf, IDC_LASTLINE), itoa(maxLine + LINE_OFFSET, text, 10));
		}
	}
}


