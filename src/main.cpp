#include <WinSock2.h>

#include <quote/quote.hpp>
#include <quote/win32/message_loop.hpp>
#include <quote/cef/prepare.hpp>
#include <quote/cef/wrap.hpp>
#include <quote/cef/process_type.hpp>

#include "main_window.hpp"

#include "browser_handler.hpp"
#include "renderer_handler.hpp"
#include "other_handler.hpp"

# pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")

int run()
{
	int exit_code;
	void *sandbox_info = nullptr;

	CefRefPtr<CefApp> app;

	auto process_type = quote::cef::process_type();

	if (process_type == PID_BROWSER) {
		app = new browser_handler;
	}
	else if (process_type == PID_RENDERER) {
		app = new renderer_handler;
	}
	else {
		app = new other_handler;
	}

	if (quote::cef::prepare(app.get(), exit_code, sandbox_info)) {
		return exit_code;
	}

	if (!main_window::register_class())
		return 0;

	return quote::cef::wrap(
		app.get(),
		sandbox_info,
		[&]() {
			main_window w(static_cast<browser_handler*>(app.get()));

			if (!w.create(nullptr, L""))
				return 0;

			w.show();

			int code = quote::win32::message_loop(
				quote::cef::message_handler()
			);

			app = nullptr;

			return code;
		}
	);
}

QUOTE_DEFINE_MAIN
