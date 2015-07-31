/*
this file is part of HexEdit Plugin for Notepad++
Copyright (C)2006 Jens Lorenz <jens.plugin.npp@gmx.de>

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

#ifndef MULTITYPECOMBO_H
#define MULTITYPECOMBO_H

#include "Window.h"
#include "Hex.h"
#include <vector>
using namespace std;


typedef enum
{
	HEX_CODE_HEX = 0,
	HEX_CODE_ASCI,
	HEX_CODE_UNI,
	HEX_CODE_MAX
} eCodingType;

const TCHAR strCode[HEX_CODE_MAX][16] =
{
	_T("Hexadecimal"),
	_T("ANSI String"),
	_T("Unicode String")
};

#if(WINVER <= 0x0400)
struct COMBOBOXINFO 
{
    int cbSize;
    RECT rcItem;
    RECT rcButton;
    DWORD stateButton;
    HWND hwndCombo;
    HWND hwndItem;
    HWND hwndList; 
};
#endif 

typedef enum
{
	NPP_CP_ASCI = 0,
	NPP_CP_UTF8,
	NPP_CP_USCLE,
	NPP_CP_USCBE
} eNppCP;

typedef union {
	struct {
		tComboInfo	comboInfo;
		eNppCoding	codePage;
	};
	struct {
		int 		length;
		char		text[COMBO_STR_MAX];
		eNppCoding	codePage;
	};
} tEncComboInfo;



#define	CB_GETCOMBOBOXINFO	0x0164

class MultiTypeCombo : public Window
{
public :
	MultiTypeCombo();
    ~MultiTypeCombo () {};
	virtual void init(HWND hNpp, HWND hCombo);
	virtual void destroy() {
		DestroyWindow(_hSelf);
	};

	void addText(tComboInfo info);
	void setText(tComboInfo info);
	void getText(tComboInfo* info);
	eCodingType setCodingType(eCodingType code);
	void setDocCodePage(eNppCoding codepage);
	void convertBaseCoding(void);

private:
	void changeCoding(void);
	BOOL setComboText(tComboInfo info, UINT message = CB_ADDSTRING);
	BOOL setComboText(tEncComboInfo info, UINT message = CB_ADDSTRING);
	void getComboText(char* str);
	void selectComboText(tEncComboInfo info);
	void decode(tComboInfo* info, eCodingType type);
	void encode(tComboInfo* info, eCodingType type);
	void encode(tEncComboInfo* info, eCodingType type);

private :
	HWND					_hNpp;
	HWND					_hCombo;
    WNDPROC					_hDefaultComboProc;

	tComboInfo				_currData;
	vector<tEncComboInfo>	_comboItems;
	eCodingType				_currDataType;

	eNppCoding				_docCodePage;

	/* Subclassing combo boxes */
	LRESULT runProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK wndDefaultProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
		return (((MultiTypeCombo *)(::GetWindowLong(hwnd, GWL_USERDATA)))->runProc(hwnd, Message, wParam, lParam));
	};
};

#endif // MULTITYPECOMBO_H
