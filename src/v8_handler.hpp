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
		if (name == L"printToPdf") {
			browser_->SendProcessMessage(PID_BROWSER, CefProcessMessage::Create(L"RevealViewer.PrintToPdf"));
			return true;
		} else if (name == L"printToPdfFinished") {
			browser_->SendProcessMessage(PID_BROWSER, CefProcessMessage::Create(L"RevealViewer.CloseWindow"));
			return true;
		} else if (name == L"openFile") {
			auto msg = CefProcessMessage::Create(L"RevealViewer.OpenFile");
			msg->GetArgumentList()->SetString(0, arguments[0]->GetStringValue());
			browser_->SendProcessMessage(PID_BROWSER, msg);
			return true;
		}

		return false;
	}
};
