#include "NppDarkMode.h"
#include "PluginInterface.h"

extern NppData nppData;

static constexpr COLORREF HEXRGB(DWORD rrggbb) {
	// from 0xRRGGBB like natural #RRGGBB
	// to the little-endian 0xBBGGRR
	return
		((rrggbb & 0xFF0000) >> 16) |
		((rrggbb & 0x00FF00)) |
		((rrggbb & 0x0000FF) << 16);
}


namespace NppDarkMode
{
	bool isEnabled()
	{
		return ::SendMessage(nppData._nppHandle, NPPM_ISDARKMODEENABLED, 0, 0);
	}

	HBRUSH getDarkerBackgroundBrush()
	{
		return ::CreateSolidBrush(HEXRGB(0x202020));
	}

	COLORREF getTextColor()
	{
		return HEXRGB(0xE0E0E0);
	}

	COLORREF getDarkerTextColor()
	{
		return HEXRGB(0xC0C0C0);
	}

	COLORREF getLinkTextColor()
	{
		return HEXRGB(0xFFFF00);
	}

	COLORREF getDarkerBackgroundColor()
	{
		return HEXRGB(0x202020);
	}

	void setDarkTitleBar(HWND /*hwnd*/)
	{
	}

	bool isWindows10()
	{
		return false;
	}

	LRESULT onCtlColorDarker(HDC hdc)
	{
		if (!NppDarkMode::isEnabled())
		{
			return FALSE;
		}

		::SetTextColor(hdc, NppDarkMode::getTextColor());
		::SetBkColor(hdc, NppDarkMode::getDarkerBackgroundColor());
		return reinterpret_cast<LRESULT>(NppDarkMode::getDarkerBackgroundBrush());
	}

}
