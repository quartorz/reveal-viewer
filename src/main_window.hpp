#pragma once

#include <quote/win32/window.hpp>
#include <quote/win32/creator.hpp>
#include <quote/win32/procs.hpp>

#include <quote/cef/browser_handler.hpp>
#include <quote/cef/print_to_pdf.hpp>

#include <thread>

#include "include/wrapper/cef_closure_task.h"

#include "msr.hpp"
#include "browser_handler.hpp"
#include "utility.hpp"

class main_window :
	public quote::win32::window<
		main_window,
		quote::win32::quit_on_close<main_window>,
		quote::win32::resizer<main_window>,
		quote::win32::on_close<main_window>,
		quote::win32::keyboard<main_window>
	>,
	public quote::win32::creator<main_window>
{
	// micro server for reveal.js
	boost::asio::io_service io_service_;
	msr::tcp_server server_ = { io_service_, 0 };
	std::thread server_thread_;

	// chromium
	CefRefPtr<browser_handler> browser_handler_;
	HWND hwnd_browser_ = nullptr;
	CefRefPtr<CefBrowser> primary_browser_;

	// window state
	RECT window_rect_ = {};
	bool is_full_screen_ = false;
	LONG_PTR window_style_;

	// menu command IDs
	enum class menu_command : int {
		PRINT_TO_PDF,
		COPY_URL,
		OPEN_LINK
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

	void toggle_full_screen()
	{
		auto hwnd = this->get_hwnd();

		if (is_full_screen_) {
			window_style_ = (window_style_ & ~WS_POPUPWINDOW) | WS_OVERLAPPEDWINDOW;

			::SetWindowLongPtrW(hwnd, GWL_STYLE, window_style_);

			::SetWindowPos(
				hwnd,
				nullptr,
				window_rect_.left,
				window_rect_.top,
				window_rect_.right - window_rect_.left,
				window_rect_.bottom - window_rect_.top,
				SWP_NOZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
		} else {
			::GetWindowRect(hwnd, &window_rect_);

			window_style_ = ::GetWindowLongPtrW(hwnd, GWL_STYLE);
			window_style_ = (window_style_ & ~WS_OVERLAPPEDWINDOW) | WS_POPUPWINDOW;

			::SetWindowLongPtrW(hwnd, GWL_STYLE, window_style_);

			auto width = ::GetSystemMetrics(SM_CXSCREEN);
			auto height = ::GetSystemMetrics(SM_CYSCREEN);

			::SetWindowPos(
				hwnd, nullptr, 0, 0, width, height,
				SWP_NOZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
		}

		is_full_screen_ = !is_full_screen_;
	}

#pragma region quote window initializer and uninitializer

	bool initialize()
	{
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

			primary_browser_ = CefBrowserHost::CreateBrowserSync(
				window_info, browser_handler_.get(), L"http://localhost:" + std::to_wstring(port),
				settings, nullptr);

			if (primary_browser_.get() == nullptr) {
				return false;
			}

			hwnd_browser_ = primary_browser_->GetHost()->GetWindowHandle();
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
					true);
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

			if ((params->GetTypeFlags() & CM_TYPEFLAG_LINK) != 0) {
				model->AddItem(static_cast<int>(menu_command::OPEN_LINK), L"&Open Link");
			}

			model->AddItem(static_cast<int>(menu_command::COPY_URL), L"&Copy URL");
			model->AddSeparator();
			model->AddItem(static_cast<int>(menu_command::PRINT_TO_PDF), L"&Print to PDF");
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

			if (id == menu_command::PRINT_TO_PDF) {
				CefWindowInfo window_info;

				window_info.SetAsPopup(this->get_hwnd(), L"PDF");

				CefBrowserSettings settings;

				browser->GetHost()->CreateBrowserSync(
					window_info,
					browser_handler_.get(),
					L"http://localhost:" + std::to_wstring(server_.get_port()) + L"/?print-pdf",
					settings,
					nullptr);

				return true;
			} else if (id == menu_command::COPY_URL) {
				if ((params->GetTypeFlags() & CM_TYPEFLAG_LINK) != 0) {
					::copy_to_clipboard(this->get_hwnd(), params->GetLinkUrl());
				} else {
					::copy_to_clipboard(this->get_hwnd(), params->GetPageUrl());
				}
				return true;
			} else if (id == menu_command::OPEN_LINK) {
				auto url = params->GetLinkUrl();
				::ShellExecuteW(this->get_hwnd(), L"open", url.c_str(), nullptr, nullptr, SW_SHOW);
				return true;
			}

			return false;
		});

		browser_handler_->on_key_event([&](
			CefRefPtr<CefBrowser> &browser,
			const CefKeyEvent &event,
			CefEventHandle os_event
			)
		{
			if (browser->IsSame(primary_browser_)) {
				if (event.type == KEYEVENT_RAWKEYDOWN && event.windows_key_code == VK_F11) {
					this->toggle_full_screen();
				}
			}

			if (event.type == KEYEVENT_RAWKEYDOWN
				&& event.windows_key_code == L'R'
				&& ::GetKeyState(VK_CONTROL) < 0) {
				browser->Reload();
			}

			return false;
		});

		return true;
	}

	void uninitialize()
	{
		io_service_.stop();
		server_.stop();

		if (server_thread_.joinable()) {
			while (server_thread_.get_id() != std::thread::id()) {
				::Sleep(1);
			}
		}

		hwnd_browser_ = nullptr;
		primary_browser_ = nullptr;

		if (browser_handler_ != nullptr) {
			browser_handler_->CloseAllBrowsers(false);

			// clear browser handler callbacks

			browser_handler_->on_process_message_received(nullptr);
			browser_handler_->on_before_context_menu(nullptr);
			browser_handler_->on_context_menu_command(nullptr);
			browser_handler_->on_key_event(nullptr);
		}
	}

#pragma endregion

#pragma region quote window handlers

	void on_size(int width, int height)
	{
		if (hwnd_browser_) {
			::SetWindowPos(
				hwnd_browser_,
				nullptr,
				0,
				0,
				width,
				height,
				SWP_NOZORDER | SWP_NOMOVE);
		}
	}

	// ウィンドウの閉じ方についてはCefLifeSpanHandler::DoClose()のドキュメントを参照
	bool on_close()
	{
		if (browser_handler_->IsPrimaryBrowserClosing()) {
			return true;
		}
		else if (primary_browser_ != nullptr) {
			primary_browser_->GetHost()->CloseBrowser(false);
			return false;
		}
		else {
			return true;
		}
	}

	void on_key_down(unsigned int keycode)
	{
		if (keycode == VK_F11) {
			toggle_full_screen();
		}
		else if (keycode == L'R' && ::GetKeyState(VK_CONTROL) < 0) {
			primary_browser_->Reload();
		}
	}

	void on_key_up(unsigned int keycode)
	{
	}

#pragma endregion
};
