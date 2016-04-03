#pragma once

#include "include/cef_base.h"
#include "include/cef_v8.h"

#include <quote/cef/print_to_pdf.hpp>

class v8_handler :
	public CefV8Handler
{
	IMPLEMENT_REFCOUNTING(v8_handler);

	CefRefPtr<CefBrowser> browser_;

public:
	v8_handler(CefRefPtr<CefBrowser> browser)
		: browser_(browser)
	{
	}

	bool Execute(
		const CefString& name,
		CefRefPtr<CefV8Value> object,
		const CefV8ValueList& arguments,
		CefRefPtr<CefV8Value>& retval,
		CefString& exception
	) override
	{
		if (name == "printToPdf") {
			quote::cef::print_to_pdf(
				browser_,
				[](CefRefPtr<CefBrowser> browser, const CefString& path, bool ok) {
					auto message = L"File \"" + path.ToWString()
						+ (ok ? L"\" saved successfully." : L"\" failed to save.");
					auto frame = browser->GetMainFrame();

					frame->ExecuteJavaScript(L"alert('" + message + +L"'); window.close();", frame->GetURL(), 0);
				},
				true);

			return true;
		}

		return false;
	}
};
