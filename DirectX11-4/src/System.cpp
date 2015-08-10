#include "System.h"

namespace {
	MSG msg = {};
	int Frame_Per_Second = sys::Default_FPS;


	int curTime = 0, prevTime = timeGetTime();
}


namespace sys {
	

	bool Update() {
		// ƒƒCƒ“ƒ‹[ƒv
		while (msg.message != WM_QUIT) {
			if (::PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
				::TranslateMessage(&msg);
				::DispatchMessage(&msg);
			} else {

				curTime = timeGetTime();
				if (curTime - prevTime >= 1000 / Frame_Per_Second) {
					prevTime = curTime;
					return true;
				}
				Sleep(10);
			}
		}

		return false;
	}

	void SetFPS(int fps) {
		Frame_Per_Second =  fps;
	}

	MSG GetLastMessage() {
		return msg;
	}
}