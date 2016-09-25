#pragma once

#include <quote/win32/window.hpp>
#include <quote/win32/creator.hpp>
#include <quote/win32/procs.hpp>

#include <quote/cef/browser_handler.hpp>
#include <quote/cef/print_to_pdf.hpp>
#include <quote/cef/make_string_visitor.hpp>

#include <thread>
#include <unordered_map>

#include "include/wrapper/cef_closure_task.h"

#include "msr.hpp"
#include "browser_handler.hpp"
#include "utility.hpp"

#include "browser_window.hpp"

class main_window : public browser_window
{
	// micro server for reveal.js
	boost::asio::io_service io_service_;
	msr::tcp_server server_ = { io_service_, 0 };
	std::thread server_thread_;

	CefRefPtr<browser_handler> browser_handler_;
	std::unordered_map<int, browser_window*> other_windows_;

	// menu command IDs
	enum class menu_command : int {
		COPY,
		CUT,
		PASTE,
		GO_BACK,
		GO_FORWARD,
		RELOAD,
		PRINT_TO_PDF,
		SAVE_PAGE,
		COPY_URL,
		OPEN_LINK,
		OPEN_LINK_IN_NEW_WINDOW,
		TOGGLE_NAVIGATE,
	};

public:
	static const wchar_t *get_class_name()
	{
		return L"reveal-viewer main_window";
	}

	main_window(CefRefPtr<browser_handler> handler)
		: browser_handler_(handler)
	{
	}

	browser_window *find_window(const CefRefPtr<CefBrowser> &browser)
	{
		if (browser->IsSame(browser_)) {
			return this;
		}

		auto iter = other_windows_.find(browser->GetIdentifier());

		if (iter != other_windows_.end()) {
			return iter->second;
		}

		return nullptr;
	}

#pragma region quote window initializer and uninitializer

	bool initialize() override
	{
		is_primary_window(true);

		if (!browser_window::initialize()) {
			return false;
		}

		try {
			wchar_t exe_path[MAX_PATH];

			::GetModuleFileNameW(nullptr, exe_path, MAX_PATH);

			auto server_root = boost::filesystem::path(exe_path).remove_filename() / "slide";

			if (!server_.start(server_root.string()))
				return false;

			server_thread_ = std::thread([&]() {
				io_service_.run();
				server_thread_.detach();
			});

			LONG width, height;
			std::tie(width, height) = this->get_size();

			unsigned short port = server_.get_port();

			CefWindowInfo window_info;
			window_info.SetAsChild(this->get_hwnd(), { 0, 0, width, height });

			CefBrowserSettings settings;

			CefString standard, sans_serif, serif, fixed;

			standard.Attach(&settings.standard_font_family, false);
			sans_serif.Attach(&settings.sans_serif_font_family, false);
			serif.Attach(&settings.serif_font_family, false);
			fixed.Attach(&settings.fixed_font_family, false);

			standard = "MS Gothic";
			sans_serif = "MS Gothic";
			serif = "MS Mincho";
			fixed = "MS P Gothic";

			browser_ = CefBrowserHost::CreateBrowserSync(
				window_info, browser_handler_.get(), L"http://localhost:" + std::to_wstring(port),
				settings, nullptr);

			if (browser_.get() == nullptr) {
				return false;
			}

			hwnd_browser_ = browser_->GetHost()->GetWindowHandle();
		} catch (std::exception &) {
			return false;
		}

		// set browser handler callbacks

		browser_handler_->on_process_message_received([](
			CefRefPtr<CefBrowser> browser,
			CefProcessId source_process,
			CefRefPtr<CefProcessMessage> message
			)
		{
			if (message->GetName() == L"RevealViewer.PrintToPdf") {
				quote::cef::print_to_pdf(
					browser,
					[](CefRefPtr<CefBrowser> browser, const CefString& path, bool ok) {
						auto message = L"File \"" + path.ToWString()
							+ (ok ? L"\" saved successfully." : L"\" failed to save.");
						auto frame = browser->GetMainFrame();

						frame->ExecuteJavaScript(L"alert('" + message + +L"'); window.close();", frame->GetURL(), 0);
					},
					true,
					[](CefRefPtr<CefBrowser> browser) {
						auto frame = browser->GetMainFrame();
						frame->ExecuteJavaScript(L"window.close();", frame->GetURL(), 0);
					});
			}

			return false;
		});

		browser_handler_->on_before_context_menu([&](
			CefRefPtr<CefBrowser> &browser,
			CefRefPtr<CefFrame> &frame,
			CefRefPtr<CefContextMenuParams> &params,
			CefRefPtr<CefMenuModel> &model
			)
		{
			model->Clear();

			if ((params->GetTypeFlags() & CM_TYPEFLAG_SELECTION) != 0) {
				model->AddItem(static_cast<int>(menu_command::COPY), L"&Copy");

				if ((params->GetTypeFlags() & CM_TYPEFLAG_EDITABLE) != 0) {
					model->AddItem(static_cast<int>(menu_command::CUT), L"Cu&t");
					model->AddItem(static_cast<int>(menu_command::PASTE), L"&Paste");
				}

				model->AddSeparator();
			} else if ((params->GetTypeFlags() & CM_TYPEFLAG_EDITABLE) != 0) {
				model->AddItem(static_cast<int>(menu_command::PASTE), L"&Paste");
				model->AddSeparator();
			}

			model->AddItem(static_cast<int>(menu_command::GO_BACK), L"Go &Back");
			model->AddItem(static_cast<int>(menu_command::GO_FORWARD), L"Go &Forward");
			model->AddItem(static_cast<int>(menu_command::RELOAD), L"&Reload");

			if ((params->GetTypeFlags() & CM_TYPEFLAG_LINK) != 0) {
				model->AddSeparator();
				model->AddItem(static_cast<int>(menu_command::OPEN_LINK), L"&Open Link");
				model->AddItem(static_cast<int>(menu_command::OPEN_LINK_IN_NEW_WINDOW), L"Open Link in New &Window");
			}

			model->AddItem(static_cast<int>(menu_command::COPY_URL), L"Copy &URL");
			model->AddSeparator();
			model->AddItem(static_cast<int>(menu_command::PRINT_TO_PDF), L"&Print to PDF");
			model->AddItem(static_cast<int>(menu_command::SAVE_PAGE), L"&Save Page");
			model->AddItem(static_cast<int>(menu_command::TOGGLE_NAVIGATE), L"&Toggle Navigate Buttons");

			if (!browser->CanGoBack()) {
				model->SetEnabled(static_cast<int>(menu_command::GO_BACK), false);
			}

			if (!browser->CanGoForward()) {
				model->SetEnabled(static_cast<int>(menu_command::GO_FORWARD), false);
			}
		});

		browser_handler_->on_context_menu_command([&](
			CefRefPtr<CefBrowser> &browser,
			CefRefPtr<CefFrame> &frame,
			CefRefPtr<CefContextMenuParams> params,
			int command_id,
			CefContextMenuHandler::EventFlags event_flags
			)
		{
			auto id = static_cast<menu_command>(command_id);
			auto window = find_window(browser);

			if (window == nullptr) {
				return false;
			}

			if (id == menu_command::COPY) {
				frame->Copy();
			} else if (id == menu_command::CUT) {
				frame->Cut();
			} else if (id == menu_command::PASTE) {
				frame->Paste();
			} else if (id == menu_command::GO_BACK) {
				browser->GoBack();
			} else if (id == menu_command::GO_FORWARD) {
				browser->GoForward();
			} else if (id == menu_command::RELOAD) {
				browser->Reload();
			} else if (id == menu_command::PRINT_TO_PDF) {
				CefWindowInfo window_info;
				window_info.SetAsPopup(window->get_hwnd(), L"PDF");

				CefBrowserSettings settings;

				auto frame = browser->GetMainFrame();

				browser->GetHost()->CreateBrowserSync(
					window_info,
					browser_handler_.get(),
					frame->GetURL().ToWString() + L"?print-pdf",
					settings,
					nullptr);

				return true;
			} else if (id == menu_command::SAVE_PAGE) {
				auto frame = browser->GetMainFrame();

				frame->GetSource(quote::cef::make_string_visitor([&](const CefString &s) {
					MessageBoxW(get_hwnd(), s.c_str(), L"", 0);
				}));
			} else if (id == menu_command::COPY_URL) {
				if ((params->GetTypeFlags() & CM_TYPEFLAG_LINK) != 0) {
					::copy_to_clipboard(window->get_hwnd(), params->GetLinkUrl());
				} else {
					::copy_to_clipboard(window->get_hwnd(), params->GetPageUrl());
				}
				return true;
			} else if (id == menu_command::OPEN_LINK) {
				auto url = params->GetLinkUrl();
				::ShellExecuteW(window->get_hwnd(), L"open", url.c_str(), nullptr, nullptr, SW_SHOW);
				return true;
			} else if (id == menu_command::OPEN_LINK_IN_NEW_WINDOW) {
				CefWindowInfo window_info;
				window_info.SetAsPopup(window->get_hwnd(), L"");

				CefBrowserSettings settings;

				auto url = params->GetLinkUrl();
				browser->GetHost()->CreateBrowser(
					window_info,
					browser_handler_.get(),
					url,
					settings,
					nullptr);
				return true;
			} else if (id == menu_command::TOGGLE_NAVIGATE) {
				window->toggle_navigate_buttons();
			}

			return false;
		});

		browser_handler_->on_key_event([&](
			CefRefPtr<CefBrowser> &browser,
			const CefKeyEvent &event,
			CefEventHandle os_event
			)
		{
			if (event.type == KEYEVENT_RAWKEYDOWN) {
				browser_window *window = find_window(browser);

				if (window != nullptr) {
					window->on_key_down(event.windows_key_code);
				}
			}

			return false;
		});

		browser_handler_->on_after_created([&](CefRefPtr<CefBrowser> browser) {
			auto window = new browser_window();
			auto hwnd = browser->GetHost()->GetWindowHandle();

			if (!window->create()) {
				browser->GetHost()->CloseBrowser(true);
				return true;
			}

			RECT rc;

			::GetClientRect(hwnd, &rc);

			auto style = ::GetWindowLongPtrW(hwnd, GWL_STYLE);

			style &= ~WS_OVERLAPPEDWINDOW;
			style |= WS_CHILDWINDOW;

			::SetWindowLongPtrW(hwnd, GWL_STYLE, style);
			::SetParent(hwnd, window->get_hwnd());
			::SetWindowPos(hwnd, nullptr, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE);

			window->hwnd_browser(hwnd);
			window->browser(browser);
			window->hwnd_primary(this->get_hwnd());
			window->set_size(rc.right - rc.left, rc.bottom - rc.top);
			window->show();

			other_windows_.insert({ browser->GetIdentifier(), window });

			return false;
		});

		return true;
	}

	void uninitialize() override
	{
		io_service_.stop();
		server_.stop();

		if (server_thread_.joinable()) {
			while (server_thread_.get_id() != std::thread::id()) {
				::Sleep(1);
			}
		}

		hwnd_browser_ = nullptr;
		browser_ = nullptr;

		for (auto &w : other_windows_) {
			delete w.second;
		}

		browser_window::uninitialize();
	}

#pragma endregion

#pragma region quote window handlers

	// ウィンドウの閉じ方についてはCefLifeSpanHandler::DoClose()のドキュメントを参照
	bool on_close() override
	{
		if (!browser_handler_->IsPrimaryBrowserClosing()) {
			auto status = ::MessageBoxW(
				this->get_hwnd(),
				L"プログラムを終了しますか？",
				L"確認",
				MB_YESNO);

			if (status == IDNO) {
				return false;
			}
		}

		for (auto &w : other_windows_) {
			::DestroyWindow(w.second->get_hwnd());
		}

		return browser_window::on_close();
	}

	void on_subwindow_close(int browser_id) override
	{
		auto iter = other_windows_.find(browser_id);

		if (iter != other_windows_.end()) {
			delete iter->second;
			other_windows_.erase(iter);
		}
	}

#pragma endregion
};
