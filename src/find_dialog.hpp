#pragma once

#include <Windows.h>

#include <quote/win32/window.hpp>
#include <quote/win32/creator.hpp>
#include <quote/win32/procs.hpp>

#include <quote/direct2d/traits.hpp>
#include <quote/direct2d/painter.hpp>
#include <quote/direct2d/integrated_manager.hpp>
#include <quote/direct2d/flat_button.hpp>

#include <quote/macro.hpp>

#include "include/cef_client.h"

#include "button.hpp"

class find_dialog :
	public quote::win32::window<
		find_dialog,
		quote::win32::resizer<find_dialog>,
		quote::win32::object_processor<find_dialog, quote::direct2d::traits>,
		quote::direct2d::painter<find_dialog>
	>,
	public quote::win32::creator<find_dialog>,
	public quote::direct2d::integrated_manager<find_dialog>
{
	HWND hwnd_editbox_;

	button next_, prev_;
	toggle_button case_;
	bool case_pushed_ = false;

	std::wstring text_last_;
	bool find_next_ = false;
	bool case_last_ = false;

	std::wstring get_find_text()
	{
		auto length = ::GetWindowTextLengthW(hwnd_editbox_);

		if (length == 0) {
			return L"";
		}

		std::unique_ptr<wchar_t[]> buf(new wchar_t[length + 1]);

		::GetWindowTextW(hwnd_editbox_, buf.get(), length + 1);

		return buf.get();
	}

	void do_find(bool forward)
	{
		std::wstring text = get_find_text();

		auto host = browser_->GetHost();

		if (case_pushed_ != case_last_ || text != text_last_) {
			if (!text.empty()) {
				host->StopFinding(true);
				find_next_ = false;
			}
			case_last_ = case_pushed_;
			text_last_ = text;
		}

		host->Find(0, text, forward, case_last_, find_next_);

		if (!find_next_) {
			find_next_ = true;
		}
	}

public:
	QUOTE_DEFINE_SIMPLE_PROPERTY(
		CefRefPtr<CefBrowser>,
		browser,
		accessor (all),
		default (nullptr)
	);

	static const wchar_t *get_class_name()
	{
		return L"reveal-viewer find_dialog";
	}

	bool initialize()
	{
		hwnd_editbox_ = ::CreateWindowW(
			WC_EDITW,
			L"",
			WS_CHILD | WS_BORDER,
			0, 0,
			0, 0,
			this->get_hwnd(),
			nullptr,
			::GetModuleHandleW(nullptr),
			nullptr
		);

		if (hwnd_editbox_ == nullptr) {
			return false;
		}

		::ShowWindow(hwnd_editbox_, SW_SHOW);

		this->register_object(&case_);
		case_.set_text(L"Match Case");
		case_.state_changed(std::bind(&find_dialog::repaint, this));
		case_.callback([&](bool pushed) {
			case_pushed_ = pushed;
		});

		this->register_object(&prev_);
		prev_.set_text(L"Prev");
		prev_.state_changed(std::bind(&find_dialog::repaint, this));
		prev_.callback(std::bind(&find_dialog::do_find, this, false));

		this->register_object(&next_);
		next_.set_text(L"Next");
		next_.state_changed(std::bind(&find_dialog::repaint, this));
		next_.callback(std::bind(&find_dialog::do_find, this, true));

		int w, h;

		std::tie(w, h) = this->get_size();
		relayout(w, h);
 
		return true;
	}

	void uninitialize()
	{
		this->unregister_object(&case_);
		this->unregister_object(&prev_);
		this->unregister_object(&next_);

		browser_->GetHost()->StopFinding(false);
	}

	void on_size(int w, int h)
	{
		relayout(w, h);
	}

	void relayout(int w, int h)
	{
		::SetWindowPos(
			hwnd_editbox_, nullptr,
			0, 0, w - 100, h,
			SWP_NOACTIVATE | SWP_NOZORDER);
		case_.set_position({ w - 100, 0 });
		case_.set_size({ 100, h / 2 });
		prev_.set_position({ w - 100, h / 2 });
		prev_.set_size({ 50, h / 2 });
		next_.set_position({ w - 50, h / 2 });
		next_.set_size({ 50, h / 2 });
	}
};
