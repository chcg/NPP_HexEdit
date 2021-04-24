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

#ifndef OPTION_DEFINE_H
#define OPTION_DEFINE_H

#include "StaticDialog.h"
#include "Hex.h"
#include "HexResource.h"
#include "ColorCombo.h"



class OptionDlg : public StaticDialog
{

public:
	OptionDlg() : StaticDialog()
		, _pProp(nullptr)
	{};

	void init(HINSTANCE hInst, NppData nppData)
	{
		_nppData = nppData;
		Window::init(hInst, nppData._nppHandle);
	};

	UINT doDialog(tProp *prop);

	virtual void destroy() override {};


protected:
	INT_PTR CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam) override;

	void TabUpdate(void);
	void SetParams(void);
	BOOL GetParams(void);

private:
	/* Handles */
	NppData			_nppData;
	HWND			_HSource;

	ColorCombo		_ColCmbRegTxt;
	ColorCombo		_ColCmbRegBk;
	ColorCombo		_ColCmbSelTxt;
	ColorCombo		_ColCmbSelBk;
	ColorCombo		_ColCmbDiffTxt;
	ColorCombo		_ColCmbDiffBk;
	ColorCombo		_ColCmbBkMkTxt;
	ColorCombo		_ColCmbBkMkBk;
	ColorCombo		_ColCmbCurLine;

	tProp*			_pProp;
};



#endif // OPTION_DEFINE_H
