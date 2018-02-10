//this file is part of plugins common
//Copyright (C)2008 Loon Chew Yeo & Jens Lorenz
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

#ifndef SCI_SUB_CLASS_WRP_H
#define SCI_SUB_CLASS_WRP_H

#include "Scintilla.h"


class SciSubClassWrp
{
public:
	HWND hWnd;

	SciSubClassWrp()
		:
		hWnd(0)
		,SciCallWndProc(0)
		,SciFunc(0)
		,SciPtr(0)
		,OrigSciWndProc(0)
	{}

	void Init( HWND hSci, WNDPROC NewWndProc )
	{
		hWnd = hSci;
		SubclassScintillaWndProc( NewWndProc );
		SciFunc = (SciFnDirect)::SendMessage(hSci, SCI_GETDIRECTFUNCTION, 0, 0);
		SciPtr = (sptr_t)::SendMessage( hSci, SCI_GETDIRECTPOINTER, 0, 0);
	}

	void CleanUp()
	{
		if (OrigSciWndProc != 0)
		{
			if ( ::IsWindowUnicode( hWnd ) )
			{
				SetWindowLongPtrW( hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(OrigSciWndProc) );
			}
			else
			{
				SetWindowLongPtrA( hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(OrigSciWndProc) );
			}
		}
		OrigSciWndProc = 0;
	}

	LRESULT execute(UINT Msg, WPARAM wParam=0, LPARAM lParam=0) const
	{
		return SciFunc(SciPtr, Msg, wParam, lParam);
	};

	LRESULT CALLBACK CallScintillaWndProc( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
	{
		return SciCallWndProc( OrigSciWndProc, hwnd, msg, wp, lp );
	}

private:
	LRESULT (WINAPI *SciCallWndProc) (WNDPROC,HWND,UINT,WPARAM,LPARAM);
	SciFnDirect SciFunc;
	sptr_t SciPtr;
	WNDPROC OrigSciWndProc;

	void SubclassScintillaWndProc( WNDPROC NewWndProc )
	{
		if ( ::IsWindowUnicode( hWnd ) )
		{
			SciCallWndProc = CallWindowProcW;
			OrigSciWndProc = (WNDPROC) SetWindowLongPtrW( hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(NewWndProc) );
		}
		else
		{
			SciCallWndProc = CallWindowProcA;
			OrigSciWndProc = (WNDPROC) SetWindowLongPtrA( hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(NewWndProc) );
		}
	}
};

#endif
