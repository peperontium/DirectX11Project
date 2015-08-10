#pragma once

#include <WinUser.h>

namespace sys {

	const int Default_FPS = 30;

	//!	更新処理を行う時にtrueが返る。アプリケーション終了の場合はfalseが返る。
	bool Update();

	void SetFPS(int fps);

	MSG GetLastMessage();

}