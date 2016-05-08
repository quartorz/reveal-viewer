#pragma once

#include <functional>
#include <list>
#include <algorithm>
#include <sstream>

#include "include/cef_client.h"
#include "include/wrapper/cef_helpers.h"

#include <quote/macro.hpp>
#include <quote/cef/browser_handler.hpp>

class browser_handler :
	public CefApp,
	public CefBrowserProcessHandler,
	public CefClient,
	public CefContextMenuHandler,
	public CefDisplayHandler,
	public CefLifeSpanHandler,
	public CefLoadHandler,
	public CefKeyboardHandler,
	public CefRequestHandler
{
	IMPLEMENT_REFCOUNTING(browser_handler);

	using browser_list_type = std::list<CefRefPtr<CefBrowser>>;
	bool primary_browser_closing_ = false;

public:
	QUOTE_DEFINE_SIMPLE_PROPERTY(browser_list_type, browser_list, accessor (const_getter));

	// CefApp
	CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override
	{
		return this;
	}

#pragma region CEF handlers

	// CefClient
	CefRefPtr<CefContextMenuHandler> GetContextMenuHandler() override
	{
		return this;
	}

	CefRefPtr<CefDisplayHandler> GetDisplayHandler() override
	{
		return this;
	}

	CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override
	{
		return this;
	}

	CefRefPtr<CefLoadHandler> GetLoadHandler() override
	{
		return this;
	}

	CefRefPtr<CefKeyboardHandler> GetKeyboardHandler() override
	{
		return this;
	}

	CefRefPtr<CefRequestHandler> GetRequestHandler() override
	{
		return this;
	}

	QUOTE_DEFINE_SIMPLE_PROPERTY(
		std::function<bool(
			CefRefPtr<CefBrowser> browser,
			CefProcessId source_process,
			CefRefPtr<CefProcessMessage> message)>,
		on_process_message_received,
		accessor (setter),
		default ([](...) { return false; }));
	bool OnProcessMessageReceived(
		CefRefPtr<CefBrowser> browser,
		CefProcessId source_process,
		CefRefPtr<CefProcessMessage> message
		) override
	{
		return on_process_message_received_(browser, source_process, message);
	}

	// CefBrowserProcessHandler
	QUOTE_DEFINE_SIMPLE_PROPERTY(
		std::function<void(void)>,
		on_context_initialized,
		accessor (setter),
		default ([]() { }));
	void OnContextInitialized() override
	{
		on_context_initialized_();
	}

	// CefContextMenuHandler
	QUOTE_DEFINE_SIMPLE_PROPERTY(
		std::function<
			void(
				CefRefPtr<CefBrowser> browser,
				CefRefPtr<CefFrame> frame,
				CefRefPtr<CefContextMenuParams> params,
				CefRefPtr<CefMenuModel> model)>,
		on_before_context_menu,
		accessor (setter),
		default ([](...) { }));
	void OnBeforeContextMenu(
		CefRefPtr<CefBrowser> browser,
		CefRefPtr<CefFrame> frame,
		CefRefPtr<CefContextMenuParams> params,
		CefRefPtr<CefMenuModel> model
		) override
	{
		on_before_context_menu_(browser, frame, params, model);
	}

	QUOTE_DEFINE_SIMPLE_PROPERTY(
		std::function<
			bool(
				CefRefPtr<CefBrowser> browser,
				CefRefPtr<CefFrame> frame,
				CefRefPtr<CefContextMenuParams> params,
				CefRefPtr<CefMenuModel> model,
				CefRefPtr<CefRunContextMenuCallback> callback)>,
		run_context_menu,
		accessor (setter),
		default ([](...) { return false; }));
	bool RunContextMenu(
		CefRefPtr<CefBrowser> browser,
		CefRefPtr<CefFrame> frame,
		CefRefPtr<CefContextMenuParams> params,
		CefRefPtr<CefMenuModel> model,
		CefRefPtr<CefRunContextMenuCallback> callback
		) override
	{
		return run_context_menu_(browser, frame, params, model, callback);
	}

	QUOTE_DEFINE_SIMPLE_PROPERTY(
		std::function<
			bool(
				CefRefPtr<CefBrowser> browser,
				CefRefPtr<CefFrame> frame,
				CefRefPtr<CefContextMenuParams> params,
				int command_id,
				EventFlags event_flags)>,
		on_context_menu_command,
		accessor (setter),
		default ([](...) { return false; }));
	bool OnContextMenuCommand(
		CefRefPtr<CefBrowser> browser,
		CefRefPtr<CefFrame> frame,
		CefRefPtr<CefContextMenuParams> params,
		int command_id,
		EventFlags event_flags
		) override
	{
		return on_context_menu_command_(browser, frame, params, command_id, event_flags);
	}

	QUOTE_DEFINE_SIMPLE_PROPERTY(
		std::function<
			void(
				CefRefPtr<CefBrowser> browser,
				CefRefPtr<CefFrame> frame)>,
		on_context_menu_dismissed,
		accessor (setter),
		default ([](...) { }));
	void OnContextMenuDismissed(
		CefRefPtr<CefBrowser> browser,
		CefRefPtr<CefFrame> frame
		) override
	{
		on_context_menu_dismissed_(browser, frame);
	}

	// CefDisplayHandler
	QUOTE_DEFINE_SIMPLE_PROPERTY(
		std::function<
			bool(
				CefRefPtr<CefBrowser> browser,
				const CefString& title)>,
		on_title_change,
		accessor (setter),
		default ([](...) { return false; }));
	void OnTitleChange(
		CefRefPtr<CefBrowser> browser,
		const CefString& title
		) override
	{
		if (!on_title_change_(browser, title)) {
			auto hwnd = browser->GetHost()->GetWindowHandle();
			auto hparent = ::GetAncestor(hwnd, GA_PARENT);

			if (hparent != nullptr) {
				::SetWindowTextW(hparent, title.c_str());
			} else {
				::SetWindowTextW(hwnd, title.c_str());
			}
		}
	}

	// CefLifeSpanHandler
	QUOTE_DEFINE_SIMPLE_PROPERTY(
		std::function<
			bool(
				CefRefPtr<CefBrowser> browser,
				CefRefPtr<CefFrame> frame,
				const CefString& target_url,
				const CefString& target_frame_name,
				CefLifeSpanHandler::WindowOpenDisposition target_disposition,
				bool user_gesture,
				const CefPopupFeatures& popupFeatures,
				CefWindowInfo& windowInfo,
				CefRefPtr<CefClient>& client,
				CefBrowserSettings& settings,
				bool* no_javascript_access)>,
		on_before_popup,
		accessor (setter),
		default ([](...) { return false; }));
	bool OnBeforePopup(
		CefRefPtr<CefBrowser> browser,
		CefRefPtr<CefFrame> frame,
		const CefString& target_url,
		const CefString& target_frame_name,
		CefLifeSpanHandler::WindowOpenDisposition target_disposition,
		bool user_gesture,
		const CefPopupFeatures& popupFeatures,
		CefWindowInfo& windowInfo,
		CefRefPtr<CefClient>& client,
		CefBrowserSettings& settings,
		bool* no_javascript_access
		) override
	{
		return on_before_popup_(
			browser,
			frame,
			target_url,
			target_frame_name,
			target_disposition,
			user_gesture,
			popupFeatures,
			windowInfo,
			client,
			settings,
			no_javascript_access);
	}

	QUOTE_DEFINE_SIMPLE_PROPERTY(
		std::function<bool(CefRefPtr<CefBrowser> browser)>,
		on_after_created,
		accessor (setter),
		default ([](...) { return false; }));
	void OnAfterCreated(CefRefPtr<CefBrowser> browser) override
	{
		CEF_REQUIRE_UI_THREAD();

		if (!on_after_created_(browser)) {
			browser_list_.push_back(browser);
		}
	}

	QUOTE_DEFINE_SIMPLE_PROPERTY(
		std::function<bool(CefRefPtr<CefBrowser> browser)>,
		do_close,
		accessor (setter),
		default ([](...) { return false; }));
	bool DoClose(CefRefPtr<CefBrowser> browser) override
	{
		CEF_REQUIRE_UI_THREAD();

		if (!do_close_(browser)) {
			// Assume that browser_list_.front() is the primary browser
			// which is the same as main_window::primary_browser_.

			if (browser_list_.front()->IsSame(browser)) {
				primary_browser_closing_ = true;
			}
		}

		return false;
	}

	QUOTE_DEFINE_SIMPLE_PROPERTY(
		std::function<bool(CefRefPtr<CefBrowser> browser)>,
		on_before_close,
		accessor (setter),
		default ([](...) { return false; }));
	void OnBeforeClose(CefRefPtr<CefBrowser> browser) override
	{
		CEF_REQUIRE_UI_THREAD();

		if (!on_before_close_(browser)) {
			auto end = browser_list_.end();
			auto iter = std::find_if(browser_list_.begin(), end, [&](auto &b) {
				return b->IsSame(browser);
			});

			if (iter != end)
				browser_list_.erase(iter);
		}
	}

	// CefLoadHandler
	QUOTE_DEFINE_SIMPLE_PROPERTY(
		std::function<
			bool(
				CefRefPtr<CefBrowser> browser,
				CefRefPtr<CefFrame> frame,
				ErrorCode errorCode,
				const CefString& errorText,
				const CefString& failedUrl)>,
		on_load_error,
		accessor (setter),
		default ([](...) { return false; }));
	void OnLoadError(
		CefRefPtr<CefBrowser> browser,
		CefRefPtr<CefFrame> frame,
		ErrorCode errorCode,
		const CefString& errorText,
		const CefString& failedUrl
		) override
	{
		CEF_REQUIRE_UI_THREAD();

		if (!on_load_error_(browser, frame, errorCode, errorText, failedUrl)) {
			if (errorCode == ERR_ABORTED)
				return;

			std::stringstream ss;

			ss << std::string(errorText) << '(' << errorCode << ')';

			frame->LoadString(ss.str(), failedUrl);
		}
	}

	// CefKeyboardHandler
	QUOTE_DEFINE_SIMPLE_PROPERTY(
		std::function<
			bool(
				CefRefPtr<CefBrowser> browser,
				const CefKeyEvent& event,
				CefEventHandle os_event,
				bool* is_keyboard_shortcut)>,
		on_pre_key_event,
		accessor (setter),
		default ([](...) { return false; }));
	bool OnPreKeyEvent(
		CefRefPtr<CefBrowser> browser,
		const CefKeyEvent& event,
		CefEventHandle os_event,
		bool* is_keyboard_shortcut
		) override
	{
		return on_pre_key_event_(browser, event, os_event, is_keyboard_shortcut);
	}

	QUOTE_DEFINE_SIMPLE_PROPERTY(
		std::function<
			bool(
				CefRefPtr<CefBrowser> browser,
				const CefKeyEvent& event,
				CefEventHandle os_event)>,
		on_key_event,
		accessor (setter),
		default ([](...) { return false; }));
	bool OnKeyEvent(
		CefRefPtr<CefBrowser> browser,
		const CefKeyEvent& event,
		CefEventHandle os_event
		) override
	{
		return on_key_event_(browser, event, os_event);
	}

	// CefRequestHandler
	QUOTE_DEFINE_SIMPLE_PROPERTY(
		std::function<
			bool(
				CefRefPtr<CefBrowser> browser,
				CefRefPtr<CefFrame> frame,
				CefRefPtr<CefRequest> request,
				bool is_redirect)>,
		on_before_browse,
		accessor (setter),
		default ([](...) { return false; }));
	bool OnBeforeBrowse(
		CefRefPtr<CefBrowser> browser,
		CefRefPtr<CefFrame> frame,
		CefRefPtr<CefRequest> request,
		bool is_redirect
		) override
	{
		return on_before_browse_(browser, frame, request, is_redirect);
	}

	QUOTE_DEFINE_SIMPLE_PROPERTY(
		std::function<
			bool(
				CefRefPtr<CefBrowser> browser,
				CefRefPtr<CefFrame> frame,
				const CefString& target_url,
				CefRequestHandler::WindowOpenDisposition target_disposition,
				bool user_gesture)>,
		on_open_url_from_tab,
		accessor (setter),
		default ([](...) { return false; }));
	bool OnOpenURLFromTab(
		CefRefPtr<CefBrowser> browser,
		CefRefPtr<CefFrame> frame,
		const CefString& target_url,
		CefRequestHandler::WindowOpenDisposition target_disposition,
		bool user_gesture
		) override
	{
		return on_open_url_from_tab_(
			browser, frame, target_url, target_disposition, user_gesture);
	}

#pragma endregion

#pragma region other methods

	bool IsPrimaryBrowserClosing() const
	{
		return primary_browser_closing_;
	}

	void CloseAllBrowsers(bool force_close)
	{
		for (auto &b : browser_list_) {
			b->GetHost()->CloseBrowser(force_close);
		}
	}

#pragma endregion
};
