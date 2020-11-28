//this file is part of Hex Editor Plugin for Notepad++
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

#ifndef COMPARE_DEFINE_H
#define COMPARE_DEFINE_H

#include "StaticDialog.h"
#include "Hex.h"
#include "HexDialog.h"
#include "HexResource.h"



class CompareDlg : public StaticDialog
{

public:
	CompareDlg() : StaticDialog()
		, _pHexEdit1(nullptr)
		, _pHexEdit2(nullptr)
		, _currentSC(MAIN_VIEW)
	{};

	void init(HINSTANCE hInst, NppData nppData)
	{
		_nppData = nppData;
		Window::init(hInst, nppData._nppHandle);
	};

	UINT doDialog(HexEdit *pHexEdit1, HexEdit *pHexEdit2, UINT currentSC);

	virtual void destroy() {};

protected:
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);

	NppData			_nppData;
	HexEdit*		_pHexEdit1;
	HexEdit*		_pHexEdit2;

	/* Get client orientation */
	RECT			_rcEdit1;
	RECT			_rcEdit2;
	UINT			_currentSC;
};



#endif // COMPARE_DEFINE_H
