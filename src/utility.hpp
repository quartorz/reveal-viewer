#pragma once

#include <Windows.h>

#include "include/cef_base.h"

#include <boost/filesystem/path.hpp>

inline bool copy_to_clipboard(HWND howner, CefString const &s)
{
	auto hmem = ::GlobalAlloc(
		GHND | GMEM_SHARE,
		(s.size() + 1) * sizeof(wchar_t));

	if (hmem == nullptr) {
		return false;
	}

	auto dest = reinterpret_cast<wchar_t*>(::GlobalLock(hmem));

	if (dest == nullptr) {
		::GlobalFree(hmem);
		return false;
	}

	if (::wcscpy_s(dest, s.size() + 1, s.c_str()) != 0) {
		::GlobalUnlock(hmem);
		::GlobalFree(hmem);
		return false;
	}

	if (::GlobalUnlock(hmem) == FALSE && ::GetLastError() != 0) {
		::GlobalFree(hmem);
		return false;
	}

	if (::OpenClipboard(howner) == FALSE) {
		::GlobalFree(hmem);
		return false;
	}

	if (::EmptyClipboard() == FALSE) {
		::GlobalFree(hmem);
		::CloseClipboard();
		return false;
	}

	if (::SetClipboardData(CF_UNICODETEXT, hmem) == nullptr) {
		::GlobalFree(hmem);
		::CloseClipboard();
		return false;
	}

	return ::CloseClipboard() == TRUE;
}

void set_taskbar_zorder(HWND after)
{
	auto htaskbar = ::FindWindowW(L"Shell_TrayWnd", nullptr);
	::SetWindowPos(htaskbar, after, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

boost::filesystem::path get_exe_path()
{
	wchar_t exe_name[MAX_PATH];

	::GetModuleFileNameW(nullptr, exe_name, MAX_PATH);

	return exe_name;
}
