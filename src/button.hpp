#pragma once

#include <quote/direct2d/flat_button.hpp>
#include <quote/direct2d/font.hpp>

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

class toggle_button : public quote::direct2d::flat_button {
	bool pushed_ = false;

public:
	QUOTE_DEFINE_SIMPLE_PROPERTY(
		std::function<void(bool)>,
		callback,
		accessor (setter),
		default ([](...) {}));
	QUOTE_DEFINE_SIMPLE_PROPERTY(
		std::function<void(state)>,
		state_changed,
		accessor (setter),
		default ([](...) {}));

	toggle_button()
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
		pushed_ = !pushed_;
		callback_(pushed_);
		set_state(pushed_ ? state::push : state::none);
	}

	void set_state(state s) override
	{
		state_changed_(s);

		if (pushed_ && s == state::none) {
			quote::direct2d::flat_button::set_state(state::push);
		} else {
			quote::direct2d::flat_button::set_state(s);
		}
	}
};
