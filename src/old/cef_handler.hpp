#pragma once

#include "include/cef_client.h"
#include "include/wrapper/cef_helpers.h"

#include <list>
#include <algorithm>
#include <sstream>

template <typename Owner>
class cef_handler :
	public CefClient,
	public CefContextMenuHandler,
	public CefDisplayHandler,
	public CefLifeSpanHandler,
	public CefLoadHandler,
	public CefKeyboardHandler
{
	IMPLEMENT_REFCOUNTING(cef_handler<Owner>);

	using browser_list_type = std::list<CefRefPtr<CefBrowser>>;

	Owner &owner_;

	browser_list_type browser_list_;
	bool primary_browser_closing_ = false;

public:
	cef_handler(Owner &owner)
		: owner_(owner)
	{
	}

	browser_list_type &get_browser_list()
	{
		return browser_list_;
	}

	browser_list_type const &get_browser_list() const
	{
		return browser_list_;
	}

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

	// CefContextMenuHandler
	void OnBeforeContextMenu(
		CefRefPtr<CefBrowser> browser,
		CefRefPtr<CefFrame> frame,
		CefRefPtr<CefContextMenuParams> params,
		CefRefPtr<CefMenuModel> model
	) override
	{
		owner_.on_cef_before_context_menu(browser, frame, params, model);
	}

	bool RunContextMenu(
		CefRefPtr<CefBrowser> browser,
		CefRefPtr<CefFrame> frame,
		CefRefPtr<CefContextMenuParams> params,
		CefRefPtr<CefMenuModel> model,
		CefRefPtr<CefRunContextMenuCallback> callback
	) override
	{
		return owner_.cef_run_context_menu(browser, frame, params, model, callback);
	}

	bool OnContextMenuCommand(
		CefRefPtr<CefBrowser> browser,
		CefRefPtr<CefFrame> frame,
		CefRefPtr<CefContextMenuParams> params,
		int command_id,
		EventFlags event_flags
	) override
	{
		return owner_.on_cef_context_menu_command(browser, frame, params, command_id, event_flags);
	}

	void OnContextMenuDismissed(
		CefRefPtr<CefBrowser> browser,
		CefRefPtr<CefFrame> frame
	) override
	{
		owner_.on_cef_context_menu_dismissed(browser, frame);
	}

	// CefDisplayHandler
	void OnTitleChange(
		CefRefPtr<CefBrowser> browser,
		const CefString& title
	) override
	{
		if (!owner_.on_cef_title_change(browser, title)) {
			auto hwnd = browser->GetHost()->GetWindowHandle();

			if (browser_list_.front()->IsSame(browser)) {
				auto hparent = ::GetAncestor(hwnd, GA_PARENT);
				if (hparent != nullptr) {
					::SetWindowTextW(hparent, title.c_str());
				}
			} else {
				::SetWindowTextW(hwnd, title.c_str());
			}
		}
	}

	// CefLifeSpanHandler
	void OnAfterCreated(CefRefPtr<CefBrowser> browser) override
	{
		CEF_REQUIRE_UI_THREAD();

		if (!owner_.on_cef_after_created(browser)) {
			browser_list_.push_back(browser);
		}
	}

	bool DoClose(CefRefPtr<CefBrowser> browser) override
	{
		CEF_REQUIRE_UI_THREAD();

		if (!owner_.cef_do_close(browser)) {
			// Assume that browser_list_.front() is the primary browser
			// which is the same as main_window::primary_browser_.

			if (browser_list_.front()->IsSame(browser)) {
				primary_browser_closing_ = true;
			}
		}

		return false;
	}

	void OnBeforeClose(CefRefPtr<CefBrowser> browser) override
	{
		CEF_REQUIRE_UI_THREAD();

		if (!owner_.on_cef_before_close(browser)) {
			auto end = browser_list_.end();
			auto iter = std::find_if(browser_list_.begin(), end, [&](auto &b) {
				return b->IsSame(browser);
			});

			if (iter != end)
				browser_list_.erase(iter);
		}
	}

	// CefLoadHandler
	void OnLoadError(
		CefRefPtr<CefBrowser> browser,
		CefRefPtr<CefFrame> frame,
		ErrorCode errorCode,
		const CefString& errorText,
		const CefString& failedUrl
	) override
	{
		CEF_REQUIRE_UI_THREAD();

		if (!owner_.on_cef_load_error(browser, frame, errorCode, errorText, failedUrl)) {
			if (errorCode == ERR_ABORTED)
				return;

			std::stringstream ss;

			ss << std::string(errorText) << '(' << errorCode << ')';

			frame->LoadString(ss.str(), failedUrl);
		}
	}

	// CefKeyboardHandler
	bool OnPreKeyEvent(
		CefRefPtr<CefBrowser> browser,
		const CefKeyEvent& event,
		CefEventHandle os_event,
		bool* is_keyboard_shortcut
	) override
	{
		return owner_.on_cef_pre_key_event(browser, event, os_event, is_keyboard_shortcut);
	}

	bool OnKeyEvent(
		CefRefPtr<CefBrowser> browser,
		const CefKeyEvent& event,
		CefEventHandle os_event
	) override
	{
		return owner_.on_cef_key_event(browser, event, os_event);
	}

	void CloseAllBrowsers(bool force_close)
	{
		for (auto &b : browser_list_) {
			b->GetHost()->CloseBrowser(force_close);
		}
	}

	bool IsPrimaryBrowserClosing() const
	{
		return primary_browser_closing_;
	}

	virtual bool OnProcessMessageReceived(CefRefPtr<CefBrowser> browser,
		CefProcessId source_process,
		CefRefPtr<CefProcessMessage> message) {
		MessageBoxW(0, L"message received", L"", 0);
		return false;
	}
};
