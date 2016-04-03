#pragma once

#include <quote/win32/subclass_window.hpp>
#include <quote/win32/procs.hpp>

#include "include/cef_base.h"

class primary_browser :
	public quote::win32::subclass_window<
		primary_browser
	>
{
	CefRefPtr<CefBrowser> browser_;

public:
	void set_browser(CefRefPtr<CefBrowser> &b)
	{
		browser_ = b;
	}
};
