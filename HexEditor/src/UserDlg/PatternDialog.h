//this file is part of Hex Edit Plugin for Notepad++
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

#ifndef PATTERN_DEFINE_H
#define PATTERN_DEFINE_H

#include "Hex.h"
#include "HexResource.h"
#include "StaticDialog.h"
#include "MultiTypeCombo.h"



class PatternDlg : public StaticDialog
{

public:
	PatternDlg() : StaticDialog()
	{};

	void init(HINSTANCE hInst, NppData nppData)
	{
		_nppData = nppData;
		Window::init(hInst, nppData._nppHandle);
	};

	void patternReplace(HWND hHexEdit);
	void insertColumns(HWND hHexEdit);

	virtual void destroy() {
		/* deregister this dialog */
		display(false);
		::SendMessage(_hParent, NPPM_MODELESSDIALOG, MODELESSDIALOGREMOVE, (LPARAM)_hSelf);
	};


protected:
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);

	void doDialog(HWND hHexEdit);
	BOOL onInsert(void);
	BOOL onReplace(void);


private:
	/* Handles */
	NppData				_nppData;
	HWND				_hParentHandle = nullptr;
	WNDPROC				_hDefaultEditProc = nullptr;

	tComboInfo			_pattern{};
	MultiTypeCombo*		_pCombo = nullptr;

	BOOL				_isReplace = FALSE;
	TCHAR				_txtCaption[64]{};
};



#endif // PATTERN_DEFINE_H
