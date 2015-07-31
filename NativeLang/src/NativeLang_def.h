/**
 * @author		Jens Lorenz		<jens.plugin.npp@gmx.de>
 * @version		1.1
 * @date		2007
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * @note		This is the interface to NativeLang.dll plugin.\n
 *				You need only to include this file into your project and\n
 *				call the particular function to change language of dialogs\n
 *				combo boxes, Notepad++ menu, menues, list headers.\n
 *				Or get only the new text. Only one language can be supported\n
 *				at once. The translations are stored in NativeLang.ini\n
 *				in the plugins config folder. NativeLang.ini needs is \n
 *				formated in USC2-little endian.\n
 */

#ifndef NATIVE_LANG_DEF_H
#define NATIVE_LANG_DEF_H

#include "PluginInterface.h"
#include <shlwapi.h>

#define NATIVE_LANG_NAME "NativeLang.dll"

typedef enum eNatLangMsg {
	NPP_NATLANG_CHANGEDLG,			/*!< enum change complete dialog info			*/
	NPP_NATLANG_CHANGENPPMENU,		/*!< enum change plugins menu of Notepad++		*/
	NPP_NATLANG_CHANGEMENU,			/*!< enum change another menu					*/
	NPP_NATLANG_CHANGEHEADER,		/*!< enum change header elements				*/
	NPP_NATLANG_CHANGECOMBO,		/*!< enum change combo elements					*/
	NPP_NATLANG_GETTEXTA,			/*!< enum get text via key name					*/
	NPP_NATLANG_GETTEXTW			/*!< enum get text via key name (wide char)		*/
} eNatLangMsg;

/** This struct is only necessary for the comunication between this interface 
 *	and NativeLang plugin
 */
typedef struct tNatLangInfo {
	HANDLE		hCtrl;				/*!< HWND or HMENU								*/
	LPCSTR		pszCtrl;			/*!< section name								*/
	LRESULT		lRes;				/*!< result value								*/
	WPARAM		wParam;				/*!< additional information						*/
	LPARAM		lParam;				/*!< additional information						*/
} tNatLangInfo;



/**
 * @fn			void NLChangeDialog(HINSTANCE hInst, HWND hNpp, HWND hWnd, LPCSTR pszSection)
 * @brief		Changes the dialog language. Call function in e.g. WM_INITDIALOG.
 *
 * @param[in]	hInst			The instance handle of dialog.
 * @param[in]	hNpp			The window handle of Notepad++.
 * @param[in]	hWnd			The window handle of the dialog box.
 * @param[in]	pszSection		The section name in NativeLang.ini file.
 * @retval		void			Nothing
 *
 * @details		Add in NativeLang.ini a section like the following:\n
 *				\n
 *				['plugin_file_name' 'section_name']\n
 *				'resource_value'="new name"\n
 *				...\n
 *
 * @note		'&' sign for key selection supported, e.g. "&Cancel process"
 */
static void NLChangeDialog(HINSTANCE hInst, HWND hNpp, HWND hWnd, LPCSTR pszSection)
{
	CHAR		szPath[MAX_PATH];
	::GetModuleFileNameA((HMODULE)hInst, szPath, MAX_PATH);

	tNatLangInfo		nli;
	nli.hCtrl			= hWnd;
	nli.pszCtrl			= pszSection;

	CommunicationInfo	ci;
	ci.srcModuleName	= PathFindFileNameA(szPath);
	ci.internalMsg		= NPP_NATLANG_CHANGEDLG;
	ci.info				= &nli;
	::SendMessage(hNpp, NPPM_MSGTOPLUGIN, (WPARAM)NATIVE_LANG_NAME, (LPARAM)&ci);
}

/**
 * @fn			void NLChangeNppMenu(HINSTANCE hInst, HWND hNpp, LPCSTR pszPluginName, FuncItem* funcItem, UINT nbFunc)
 * @brief		Changes Notepad++ plugins menu. Call function on notification NPPN_TBMODIFICATION.
 *
 * @param[in]	hInst			The instance handle of dialog.
 * @param[in]	hNpp			The window handle of Notepad++.
 * @param[in]	pszPluginName	The name of plugin that was given to Notepad++ on by function getName().
 * @param[in]	funcItem		The funcItem struct that was given to Notepad++ by function getFuncsArray().
 * @param[in]	nbFunc			The size of the funcItem struct that was given to Notepad++ by function getFuncsArray().
 * @retval		void			Nothing
 *
 * @details		Add in NativeLang.ini a section like the following:\n
 *				\n
 *				['plugin_file_name' NppMenu]\n
 *				Main="new plugin name in Menu->Plugins"\n
 *				0="name of entry 0"\n
 *				...\n
 *
 * @note		'&' sign for key selection supported, e.g. "&Explorer..."
 */
static void NLChangeNppMenu(HINSTANCE hInst, HWND hNpp, LPCSTR pszPluginName, FuncItem* funcItem, UINT nbFunc)
{
	CHAR		szPath[MAX_PATH];
	::GetModuleFileNameA((HMODULE)hInst, szPath, MAX_PATH);

	tNatLangInfo		nli;
	nli.hCtrl			= NULL;
	nli.pszCtrl			= pszPluginName;
	nli.wParam			= nbFunc;
	nli.lParam			= (LPARAM)funcItem;

	CommunicationInfo	ci;
	ci.srcModuleName	= PathFindFileNameA(szPath);
	ci.internalMsg		= NPP_NATLANG_CHANGENPPMENU;
	ci.info				= &nli;
	::SendMessage(hNpp, NPPM_MSGTOPLUGIN, (WPARAM)NATIVE_LANG_NAME, (LPARAM)&ci);
}

/**
 * @fn			BOOL NLChangeMenu(HINSTANCE hInst, HWND hNpp, HMENU hMenu, LPCSTR pszMenu, UINT mf_ByComPos)
 * @brief		Changes given plugin menu. Call function before ::TrackPopupMenu().
 *
 * @param[in]	hInst			The instance handle of dialog.
 * @param[in]	hNpp			The window handle of Notepad++.
 * @param[in]	hMenu			The menu handle of the plugin menu.
 * @param[in]	pszMenu			The section name in NativeLang.ini file.
 * @param[in]	mf_ByComPos		Use here MF_BYCOMMAND or MF_BYPOSITION.
 * @retval		BOOL			FALSE if mf_ByComPos is not correct set.
 *
 * @details		Add in NativeLang.ini a section like the following:\n
 *				\n
 *				['plugin_file_name' 'section name']\n
 *				menu_id_or_pos="new name"\n
 *				...\n
 *
 * @note		'&' sign for key selection supported, e.g. "&Open.."
 */
static BOOL NLChangeMenu(HINSTANCE hInst, HWND hNpp, HMENU hMenu, LPCSTR pszMenu, UINT mf_ByComPos)
{
	CHAR		szPath[MAX_PATH];
	::GetModuleFileNameA((HMODULE)hInst, szPath, MAX_PATH);

	tNatLangInfo		nli;
	nli.hCtrl			= hMenu;
	nli.pszCtrl			= pszMenu;
	nli.lRes			= 0;
	nli.wParam			= mf_ByComPos;

	CommunicationInfo	ci;
	ci.srcModuleName	= PathFindFileNameA(szPath);
	ci.internalMsg		= NPP_NATLANG_CHANGEMENU;
	ci.info				= &nli;
	::SendMessage(hNpp, NPPM_MSGTOPLUGIN, (WPARAM)NATIVE_LANG_NAME, (LPARAM)&ci);

	return nli.lRes;
}

/**
 * @fn			void NLChangeHeader(HINSTANCE hInst, HWND hNpp, HWND hHeader, LPCSTR pszSection)
 * @brief		Change automaticaly the header text. Call function in e.g. WM_INITDIALOG.
 *
 * @param[in]	hInst			The instance handle of dialog.
 * @param[in]	hNpp			The window handle of Notepad++.
 * @param[in]	hHeader			The handle of header.
 * @param[in]	pszSection		The section name in NativeLang.ini file.
 * @retval		void			Nothing
 *
 * @details		Add in NativeLang.ini a section like the following:\n
 *				\n
 *				['plugin_file_name' 'section name']\n
 *				orig_name="new name"\n
 *				...\n
 */
static void NLChangeHeader(HINSTANCE hInst, HWND hNpp, HWND hHeader, LPCSTR pszSection)
{
	CHAR		szPath[MAX_PATH];
	::GetModuleFileNameA((HMODULE)hInst, szPath, MAX_PATH);

	tNatLangInfo		nli;
	nli.hCtrl			= hHeader;
	nli.pszCtrl			= pszSection;

	CommunicationInfo	ci;
	ci.srcModuleName	= PathFindFileNameA(szPath);
	ci.internalMsg		= NPP_NATLANG_CHANGEHEADER;
	ci.info				= &nli;
	::SendMessage(hNpp, NPPM_MSGTOPLUGIN, (WPARAM)NATIVE_LANG_NAME, (LPARAM)&ci);
}

/**
 * @fn			void NLChangeCombo(HINSTANCE hInst, HWND hNpp, HWND hCombo, LPCSTR pszSection, UINT count)
 * @brief		Change automaticaly the combo text. Call function in e.g. WM_INITDIALOG.
 *
 * @param[in]	hInst			The instance handle of dialog.
 * @param[in]	hNpp			The window handle of Notepad++.
 * @param[in]	hCombo			The handle of combo box.
 * @param[in]	pszSection		The section name in NativeLang.ini file.
 * @param[in]	count			The count of combo elements.
 * @retval		void			Nothing
 *
 * @details		Add in NativeLang.ini a section like the following:\n
 *				\n
 *				['plugin_file_name' 'section name']\n
 *				pos_in_combo="new name"\n
 *				...\n
 */
static void NLChangeCombo(HINSTANCE hInst, HWND hNpp, HWND hCombo, LPCSTR pszSection, UINT count)
{
	CHAR		szPath[MAX_PATH];
	::GetModuleFileNameA((HMODULE)hInst, szPath, MAX_PATH);

	tNatLangInfo		nli;
	nli.hCtrl			= hCombo;
	nli.pszCtrl			= pszSection;
	nli.wParam			= count;

	CommunicationInfo	ci;
	ci.srcModuleName	= PathFindFileNameA(szPath);
	ci.internalMsg		= NPP_NATLANG_CHANGECOMBO;
	ci.info				= &nli;
	::SendMessage(hNpp, NPPM_MSGTOPLUGIN, (WPARAM)NATIVE_LANG_NAME, (LPARAM)&ci);
}

/** Use NLGetText to get the correct function and text format.\n\n
 *	If precompiler flag _UNICODE is set NLGetTextW is used.
 */
#ifdef _UNICODE
#define NLGetText	NLGetTextW		
#else
#define NLGetText	NLGetTextA
#endif

/**
 * @fn			UINT NLGetTextA(HINSTANCE hInst, HWND hNpp, LPCSTR pszKey, LPSTR pszText, UINT length)
 * @brief		ANSI version to get a language specific text by a key.
 *
 * @param[in]	hInst			The instance handle of dialog.
 * @param[in]	hNpp			The window handle of Notepad++.
 * @param[in]	pszKey			The key name in NativeLang.ini file.
 * @param[out]	pszText			The buffer to fill.
 * @param[in]	length			The maximum length of text buffer.
 * @retval		UINT			The size of the copied chars into pszText.
 *
 * @details		Add in NativeLang.ini a section like the following:\n
 *				\n
 *				['plugin_file_name' Text]\n
 *				key_name="text"\n
 *				...\n
 *
 * @note		Format tags are supported. Use % instead of \, e.g. %t %s %d %n
 */
static UINT NLGetTextA(HINSTANCE hInst, HWND hNpp, LPCSTR pszKey, LPSTR pszText, UINT length)
{
	CHAR		szPath[MAX_PATH];
	::GetModuleFileNameA((HMODULE)hInst, szPath, MAX_PATH);

	tNatLangInfo		nli;
	nli.hCtrl			= NULL;
	nli.pszCtrl			= pszKey;
	nli.lRes			= 0;
	nli.wParam			= length;
	nli.lParam			= (LPARAM)&pszText;

	CommunicationInfo	ci;
	ci.srcModuleName	= PathFindFileNameA(szPath);
	ci.internalMsg		= NPP_NATLANG_GETTEXTA;
	ci.info				= &nli;
	::SendMessage(hNpp, NPPM_MSGTOPLUGIN, (WPARAM)NATIVE_LANG_NAME, (LPARAM)&ci);

	return (UINT)nli.lRes;
}

/**
 * @fn			UINT NLGetTextW(HINSTANCE hInst, HWND hNpp, LPCSTR pszKey, LPWSTR pszText, UINT length)
 * @brief		UNICODE version to get a language specific text by a key.
 *
 * @param[in]	hInst			The instance handle of dialog.
 * @param[in]	hNpp			The window handle of Notepad++.
 * @param[in]	pszKey			The key name in NativeLang.ini file.
 * @param[out]	pszText			The buffer to fill.
 * @param[in]	length			The maximum length of text buffer.
 * @retval		UINT			The size of the copied chars into pszText.
 *
 * @details		Add in NativeLang.ini a section like the following:\n
 *				\n
 *				['plugin_file_name' Text]\n
 *				key_name="text"\n
 *				...\n
 *
 * @note		Format tags are supported. Use % instead of \, e.g. %t %s %d %n
 */
static UINT NLGetTextW(HINSTANCE hInst, HWND hNpp, LPCSTR pszKey, LPWSTR pszText, UINT length)
{
	CHAR		szPath[MAX_PATH];
	::GetModuleFileNameA((HMODULE)hInst, szPath, MAX_PATH);

	tNatLangInfo		nli;
	nli.hCtrl			= NULL;
	nli.pszCtrl			= pszKey;
	nli.wParam			= length;
	nli.lRes			= 0;
	nli.lParam			= (LPARAM)&pszText;

	CommunicationInfo	ci;
	ci.srcModuleName	= PathFindFileNameA(szPath);
	ci.internalMsg		= NPP_NATLANG_GETTEXTW;
	ci.info				= &nli;
	::SendMessage(hNpp, NPPM_MSGTOPLUGIN, (WPARAM)NATIVE_LANG_NAME, (LPARAM)&ci);

	return (UINT)nli.lRes;
}


/**
 * @fn			BOOL NLMessageBox(HINSTANCE hInst, HWND hNpp, LPCSTR pszKey, UINT uType)
 * @brief		Show message box identified by a key.
 *
 * @param[in]	hInst			The instance handle of dialog.
 * @param[in]	hNpp			The window handle of Notepad++.
 * @param[in]	pszKey			The key name in NativeLang.ini file.
 * @param[in]	uType			The buffer to fill.
 * @retval		INT				The message box was shown.
 *
 * @details		Add in NativeLang.ini a section like the following:\n
 *				\n
 *				['plugin_file_name' Text]\n
 *				key_name="text%tcaption%t"\n
 *				...\n
 */
static INT NLMessageBox(HINSTANCE hInst, HWND hNpp, LPCSTR pszKey, UINT uType)
{
	LPTSTR	wPtr = NULL;
	TCHAR	text[MAX_PATH]	= {0};
	if (NLGetText(hInst, hNpp, pszKey, text, MAX_PATH)) {
#ifdef _UNICODE
		wPtr = wcstok(text, _T("\t"));
		wPtr = wcstok(NULL, _T("\t"));
#else
		wPtr = strtok(text, "\t");
		wPtr = strtok(NULL, "\t");
#endif
		return ::MessageBox(hNpp, text, wPtr, uType);
	}
	return FALSE;
}



#endif	/* NATIVE_LANG_DEF_H */

