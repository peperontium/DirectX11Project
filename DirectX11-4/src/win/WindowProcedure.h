#pragma once
/*!
@file WindowProcedure.h
@brief ウィンドウプロシージャ関連
@author takamoto
@date 14/10/15
*/
#include <functional>
#include <Windows.h>

/*!
@namespace win
@brief Windows独自実装に関するクラスや関数を含む名前空間
*/
namespace win{
	void NoticeEnvironment(std::function<LRESULT(HWND, UINT, UINT, LONG)> observer);
	LRESULT CALLBACK DefaultProcedure(HWND hWnd, UINT msg, UINT wParam, LONG lParam);
}

