#include "WindowProcedure.h"

namespace win{
	typedef std::tuple<HWND, UINT, UINT, LONG> Environment;
	static Environment static_environment;

	void NoticeEnvironment(std::function<LRESULT(HWND, UINT, UINT, LONG)> observer) {
		observer(
			std::get<0>(static_environment),
			std::get<1>(static_environment),
			std::get<2>(static_environment),
			std::get<3>(static_environment)
		);
	}


	LRESULT CALLBACK DefaultProcedure(HWND hWnd, UINT msg, UINT wParam, LONG lParam) {
		static_environment = std::make_tuple(hWnd, msg, wParam, lParam);

		HRESULT hr = S_OK;

		switch (msg) {
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;

		case WM_KEYDOWN:
			switch (wParam) {
			case VK_ESCAPE:
				PostMessage(hWnd, WM_CLOSE, 0, 0);
				break;
			}
			break;
		}

		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
}