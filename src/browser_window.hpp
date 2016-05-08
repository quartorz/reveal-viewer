#pragma once

#include <quote/win32/window.hpp>
#include <quote/win32/creator.hpp>
#include <quote/win32/procs.hpp>
#include <quote/win32/object_processor.hpp>

#include <quote/direct2d/traits.hpp>
#include <quote/direct2d/painter.hpp>
#include <quote/direct2d/integrated_manager.hpp>
#include <quote/direct2d/flat_button.hpp>

#include <quote/cef/browser_handler.hpp>
#include <quote/cef/print_to_pdf.hpp>

#include <quote/macro.hpp>

#include <functional>

template <typename Derived>
struct quit_on_close {
	bool WindowProc(Derived &window, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT &lresult)
	{
		if (msg == WM_DESTROY) {
			if (window.is_primary_window()) {
				::PostQuitMessage(0);
			}
		}

		return true;
	}
};

template <typename Derived>
struct on_subwindow_close {
	bool WindowProc(Derived &window, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT &lresult)
	{
		if (msg == WM_APP) {
			window.on_subwindow_close(static_cast<int>(wParam));
		}

		return true;
	}
};

class browser_window :
	public quote::win32::window<
		browser_window,
		quote::win32::resizer<browser_window>,
		quote::win32::on_close<browser_window>,
		quote::win32::keyboard<browser_window>,
		quote::win32::focus<browser_window>,
		quote::win32::object_processor<browser_window, quote::direct2d::traits>,
		quote::direct2d::painter<browser_window>,
		::quit_on_close<browser_window>,
		on_subwindow_close<browser_window>
	>,
	public quote::win32::creator<browser_window>,
	public quote::direct2d::integrated_manager<browser_window>
{
	using quote_manager = quote::direct2d::integrated_manager<browser_window>;

	// window state
	RECT window_rect_ = {};
	bool is_full_screen_ = false;
	LONG_PTR window_style_;

	bool is_closing_ = false;

	bool show_navigate_buttons_ = false;

	struct button : quote::direct2d::flat_button {
		QUOTE_DEFINE_SIMPLE_PROPERTY(
			std::function<void()>,
			callback,
			accessor (setter),
			default ([]() {}));
		QUOTE_DEFINE_SIMPLE_PROPERTY(
			std::function<void(state)>,
			state_changed,
			accessor (setter),
			default ([](...) {}));

		button()
		{
			get_font().use_custom_renderer(false);
			get_font().set_weight(quote::direct2d::font_weight::light);
			set_text_size(15.0f);
			set_color(button::state::none, { 250, 250, 250 });
			set_color(button::state::hover, { 255, 255, 255 });
			set_color(button::state::push, { 200, 200, 200 });
			set_text_color(button::state::none, { 128, 128, 128 });
			set_text_color(button::state::hover, { 200, 200, 200 });
			set_text_color(button::state::push, { 255, 255, 255, 150 });
		}

		void on_push() override
		{
			callback_();
		}

		void set_state(state s) override
		{
			state_changed_(s);
			quote::direct2d::flat_button::set_state(s);
		}
	};

	button back_button_, forward_button_, overview_button_;

public:
	QUOTE_DEFINE_SIMPLE_PROPERTY(
		HWND,
		hwnd_browser,
		accessor (all),
		default (nullptr));
	QUOTE_DEFINE_SIMPLE_PROPERTY(
		CefRefPtr<CefBrowser>,
		browser,
		accessor (all));
	QUOTE_DEFINE_SIMPLE_PROPERTY(
		bool,
		is_primary_window,
		accessor (all),
		default (false));
	QUOTE_DEFINE_SIMPLE_PROPERTY(
		HWND,
		hwnd_primary,
		accessor (all),
		default (nullptr));

	static const wchar_t *get_class_name()
	{
		return L"reveal-viewer browser_window";
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

			auto hmonitor = ::MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
			MONITORINFO mi;

			mi.cbSize = sizeof(MONITORINFO);
			::GetMonitorInfo(hmonitor, &mi);

			::SetWindowPos(
				hwnd, nullptr,
				mi.rcMonitor.left, mi.rcMonitor.top,
				mi.rcMonitor.right - mi.rcMonitor.left,
				mi.rcMonitor.bottom - mi.rcMonitor.top,
				SWP_NOZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
		}

		is_full_screen_ = !is_full_screen_;
	}

	void toggle_navigate_buttons()
	{
		show_navigate_buttons_ = !show_navigate_buttons_;

		int width, height;
		std::tie(width, height) = this->get_size();

		if (show_navigate_buttons_) {
			::SetWindowPos(hwnd_browser_, nullptr, 0, 40, width, height - 40, SWP_NOZORDER);
		} else {
			::SetWindowPos(hwnd_browser_, nullptr, 0, 0, width, height, SWP_NOZORDER);
		}
	}

	void draw(const quote::direct2d::paint_params &pp)
	{
		pp.clear({ 255, 255, 255 });
		quote_manager::draw(pp);
	}

#pragma region quote window initializer and uninitializer

	virtual bool initialize()
	{
		this->register_object(&back_button_);
		back_button_.set_text(L"Back");
		back_button_.set_size({ 110.0f, 40.0f });
		back_button_.callback([&]() {
			if (browser_ != nullptr) {
				browser_->GoBack();
			}
		});
		back_button_.state_changed([&](button::state s) {
			this->repaint();
		});

		this->register_object(&forward_button_);
		forward_button_.set_text(L"Forward");
		forward_button_.set_size({ 110.0f, 40.0f });
		forward_button_.set_position({ 110.0f, 0.0f });
		forward_button_.callback([&]() {
			if (browser_ != nullptr) {
				browser_->GoForward();
			}
		});
		forward_button_.state_changed([&](button::state s) {
			this->repaint();
		});

		this->register_object(&overview_button_);
		overview_button_.set_text(L"Overview");
		overview_button_.set_size({ 110.0f, 40.0f });
		overview_button_.set_position({ 220.0f, 0.0f });
		overview_button_.callback([&]() {
			if (browser_ != nullptr) {
				CefKeyEvent event;
				event.windows_key_code = L'O';
				browser_->GetHost()->SendKeyEvent(event);
			}
		});
		overview_button_.state_changed([&](button::state s) {
			this->repaint();
		});

		return true;
	}

	virtual void uninitialize()
	{
		this->unregister_object(&back_button_);
		this->unregister_object(&forward_button_);
		this->unregister_object(&overview_button_);

		if (!is_primary_window_) {
			::SetWindowLongPtrW(this->get_hwnd(), 0, 0);
			::PostMessageW(hwnd_primary_, WM_APP, browser_->GetIdentifier(), 0);
		}

		return;
	}

#pragma endregion

#pragma region quote window handlers

	void on_size(int width, int height)
	{
		if (hwnd_browser_) {
			if (show_navigate_buttons_) {
				::SetWindowPos(
					hwnd_browser_,
					nullptr,
					0,
					40,
					width,
					height - 40,
					SWP_NOZORDER | SWP_NOMOVE);
			} else {
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
	}

	// ウィンドウの閉じ方についてはCefLifeSpanHandler::DoClose()のドキュメントを参照
	virtual bool on_close()
	{
		if (browser_ != nullptr && !is_closing_) {
			browser_->GetHost()->CloseBrowser(false);
			is_closing_ = true;
			return false;
		} else {
			return true;
		}
	}

	void on_key_down(unsigned int keycode)
	{
		if (keycode == VK_F11) {
			toggle_full_screen();
		} else if (keycode == L'R' && ::GetKeyState(VK_CONTROL) < 0) {
			browser_->Reload();
		}
	}

	void on_key_up(unsigned int keycode)
	{
	}

	void on_get_focus(HWND hwnd)
	{
		::SetFocus(hwnd_browser_);
	}

	void on_lost_focus(HWND hwnd)
	{
	}

	virtual void on_subwindow_close(int browser_id)
	{
	}

#pragma endregion

};
