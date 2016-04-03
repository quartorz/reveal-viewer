#pragma once

#include "include/cef_base.h"
#include "include/cef_app.h"
#include "include/cef_v8.h"

#include <quote/macro.hpp>

#include "v8_handler.hpp"

class renderer_handler :
	public CefApp,
	public CefRenderProcessHandler
{
	IMPLEMENT_REFCOUNTING(renderer_handler);

public:
	CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() override
	{
		return this;
	}

	void OnContextCreated(
		CefRefPtr<CefBrowser> browser,
		CefRefPtr<CefFrame> frame,
		CefRefPtr<CefV8Context> context
		) override
	{
		CefRefPtr<v8_handler> handler = new v8_handler(browser);
		auto function = CefV8Value::CreateFunction(L"printToPdf", handler);

		context->GetGlobal()->SetValue(
			L"printToPdf",
			function,
			V8_PROPERTY_ATTRIBUTE_NONE);
	}


};
