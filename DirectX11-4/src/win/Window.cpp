#include "Window.h"

#include <stdexcept>

static ATOM static_RegisterClass(const TCHAR * classname, WNDPROC wndProc, HICON hIcon = nullptr, HCURSOR hCursor = nullptr) {
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = wndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = GetModuleHandle(NULL);
	wcex.hIcon = hIcon;
	wcex.hIconSm = nullptr;
	wcex.hCursor = hCursor;
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = classname;

	return RegisterClassEx(&wcex);
}

static HWND static_CreateWindow(const TCHAR * classname, const TCHAR * title, RECT rc, DWORD windowStyle, WNDPROC wndProc) {
	AdjustWindowRect(&rc, windowStyle, FALSE);

	auto hWnd = CreateWindow(
		classname,
		title,
		windowStyle,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		rc.right - rc.left,
		rc.bottom - rc.top,
		NULL,
		NULL,
		GetModuleHandle(NULL),
		NULL
	);

	if (hWnd == nullptr) throw std::runtime_error("CreateWindow‚ÉŽ¸”s.");
	return hWnd;
}

namespace win {
	Window::Window(
		const TCHAR * classname, const TCHAR * title, RECT rc, DWORD windowStyle, WNDPROC wndProc
	) {
		m_classname = std::move(std::unique_ptr<TCHAR[]>(new TCHAR[lstrlen(classname) + 1]));
		lstrcpy(m_classname.get(), classname);

		static_RegisterClass(classname, wndProc);
		m_hWnd = static_CreateWindow(classname, title, rc, windowStyle, wndProc);

		ShowWindow(m_hWnd, SW_SHOW);
		UpdateWindow(m_hWnd);
	}

	Window::~Window() {
		::UnregisterClass(m_classname.get(), GetModuleHandle(NULL));
	}
}