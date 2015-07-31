/*
this file is part of notepad++
Copyright (C)2003 Don HO < donho@altern.org >

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "ToolTip.h"
#include "SysMsg.h"



LRESULT CALLBACK dlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);


void ToolTip::init(HINSTANCE hInst, HWND hParent)
{
	if (_hSelf == NULL)
	{
		Window::init(hInst, hParent);

		_hSelf = CreateWindowEx( WS_EX_TOOLWINDOW | WS_EX_TOPMOST, TITLETIP_CLASSNAME, NULL, WS_BORDER | WS_POPUP,
		   0, 0, 0, 0, NULL, NULL, NULL, NULL );
		if (!_hSelf)
		{
			systemMessage(_T("System Err"));
			throw int(6969);
		}
    
		::SetWindowLongPtr(_hSelf, GWL_USERDATA, reinterpret_cast<LONG>(this));
		_defaultProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtr(_hSelf, GWL_WNDPROC, reinterpret_cast<LONG>(staticWinProc)));
	}
}


void ToolTip::Show(RECT rectTitle, string strTitle, int iXOff, int iWidthOff)
{
	/* If titletip is already displayed, don't do anything. */
	if( ::IsWindowVisible(_hSelf))
		return;

	if( strTitle.size() == 0 )
		return;

	/* Determine the width of the text */
	ClientToScreen(_hParent, &rectTitle);

	/* Select window standard font */
	HDC hDc	= ::GetDC(_hSelf);
	::SelectObject(hDc,GetStockObject(ANSI_VAR_FONT));

	/* Calculate box size for font length */
	SIZE	size;
	RECT	rectDisplay	= rectTitle;

	::GetTextExtentPoint(hDc, strTitle.c_str(), (int)strTitle.size(), &size);
	rectDisplay.left   += iXOff + iWidthOff;
	rectDisplay.right   = rectDisplay.left + size.cx + 3;

	/* Calc new position if box is outside the screen */
	if (rectDisplay.right > GetSystemMetrics(SM_CXSCREEN))
	{
		rectDisplay.left -= rectDisplay.right - GetSystemMetrics(SM_CXSCREEN);
		rectDisplay.right = GetSystemMetrics(SM_CXSCREEN);
	}

	/* Offset for tooltip. It should displayed under the cursor */
	if ((rectDisplay.bottom + 40) < GetSystemMetrics(SM_CYSCREEN))
	{
		rectDisplay.top	   += 40;
		rectDisplay.bottom += 40;
	}
	else
	{
		rectDisplay.top	   -= 10;
		rectDisplay.bottom -= 10;
	}

	/* Show the titletip */
	SetWindowPos( _hSelf, NULL, rectDisplay.left, rectDisplay.top, 
				  rectDisplay.right-rectDisplay.left, rectDisplay.bottom-rectDisplay.top, 
				  SWP_SHOWWINDOW|SWP_NOACTIVATE );
	::SetBkMode(hDc, TRANSPARENT );
	::TextOut(hDc, 0, (rectTitle.bottom-rectTitle.top)/2-7, strTitle.c_str(), (int)strTitle.size());

	::ReleaseDC(_hSelf, hDc);
}


LRESULT ToolTip::runProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	return ::CallWindowProc(_defaultProc, _hSelf, message, wParam, lParam);
}