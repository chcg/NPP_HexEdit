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



void GotoDlg::doDialog(HWND hHexEdit)
{
    if (!isCreated())
	{
        create(IDD_GOTO_DLG);
		::SendMessage(_hParent, WM_MODELESSDIALOG, MODELESSDIALOGADD, (LPARAM)_hSelf);
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
			::SetWindowLong(_hLineEdit, GWL_USERDATA, reinterpret_cast<LONG>(this));
			_hDefaultEditProc = reinterpret_cast<WNDPROC>(::SetWindowLong(_hLineEdit, GWL_WNDPROC, reinterpret_cast<LONG>(wndEditProc)));

			::SendDlgItemMessage(_hSelf, IDC_CHECK_HEX, BM_SETCHECK, (_isHex == TRUE)?BST_CHECKED:BST_UNCHECKED, 0);
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
					char	text[16];

					if (_isHex == TRUE)
					{
						tHexProp	prop;
						::SendMessage(_hParentHandle, HEXM_GETSETTINGS, 0, (LPARAM) &prop);
						::GetWindowText(_hLineEdit, text, 16);
						::SendMessage(_hParentHandle, HEXM_SETCURLINE, 0, (LPARAM)ASCIIConvert(text) / (prop.columns * prop.bits));
					}
					else
					{
						::GetWindowText(_hLineEdit, text, 16);
						::SendMessage(_hParentHandle, HEXM_SETCURLINE, 0, (LPARAM)atoi(text)-1);
					}
					display(false);
					break;
				}
				case IDC_CHECK_HEX:
				{
					if (HIWORD(wParam) == BN_CLICKED)
					{
						onCombo();
					}
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
			::SendMessage(_hParent, WM_MODELESSDIALOG, MODELESSDIALOGREMOVE, (LPARAM)_hSelf);

			/* save view mode setting */
			::WritePrivateProfileString(dlgEditor, gotoProp, (_isHex == TRUE)?"1":"0", _iniFilePath);
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


void GotoDlg::onCombo(void)
{
	UINT		newLine		= 0;
	tHexProp	prop		= {0};
	char		text[16];
	char		temp[16];

	/* get new state */
	_isHex = (::SendDlgItemMessage(_hSelf, IDC_CHECK_HEX, BM_GETCHECK, 0, 0) == BST_CHECKED)? TRUE:FALSE;

	/* update also min and max */
	UpdateDialog();

	/* change user input */
	::SendMessage(_hParentHandle, HEXM_GETSETTINGS, 0, (LPARAM)&prop);

	::GetWindowText(_hLineEdit, temp, 16);

	if (strlen(temp) != 0)
	{
		if (_isHex == TRUE)
		{
			newLine = atoi(temp);
			newLine *= prop.columns * prop.bits;
			itoa(newLine, text, 16);
			::SetWindowText(_hLineEdit, text);
		}
		else
		{
			newLine = ASCIIConvert(temp);
			newLine /= (prop.columns * prop.bits);
			itoa(newLine, text, 10);
			::SetWindowText(_hLineEdit, text);
		}
	}
}


void GotoDlg::UpdateDialog(void)
{
	UINT		curLine		= 0;
	UINT		maxLine		= 0;
	tHexProp	prop		= {0};
	char		text[16];

	::SendMessage(_hParentHandle, HEXM_GETCURLINE, 0, (LPARAM)&curLine);
	::SendMessage(_hParentHandle, HEXM_GETLINECNT, 0, (LPARAM)&maxLine);
	::SendMessage(_hParentHandle, HEXM_GETSETTINGS, 0, (LPARAM)&prop);

	if (_isHex == TRUE)
	{
		/* set current line info */
		itoa(curLine * prop.columns * prop.bits, text, 16);
		::SetDlgItemText(_hSelf, IDC_CURRLINE, text);

		/* set max line info */
		itoa((maxLine-1) * prop.columns * prop.bits, text, 16);
		::SetDlgItemText(_hSelf, IDC_LASTLINE, text);
	}
	else
	{
		::SetWindowText(::GetDlgItem(_hSelf, IDC_CURRLINE), itoa(curLine+1, text, 10));
		::SetWindowText(::GetDlgItem(_hSelf, IDC_LASTLINE), itoa(maxLine, text, 10));
	}
}


