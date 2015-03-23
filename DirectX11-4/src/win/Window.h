#pragma once
/*!
@file Window.h
@brief ウィンドウ生成関連
@author takamoto
@date 14/10/15
*/

#include <Windows.h>
#include <memory>
#include <tchar.h>

/*!
@namespace win
@brief Windows独自実装に関するクラスや関数を含む名前空間
*/
namespace win {

	/*!
	@class Window
	*/
	class Window {
	private:
		HWND m_hWnd;
		std::unique_ptr<TCHAR[]> m_classname;
	public:
		Window(const TCHAR * classname, const TCHAR * title, RECT rc, DWORD windowStyle, WNDPROC wndProc);
		~Window();

		HWND getHWnd() const { return m_hWnd; };

		Window(const Window &) = delete;
		Window & operator=(const Window &) = delete;
	};
}