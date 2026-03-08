#pragma once
#include "include/input/SDLInput.hpp"

namespace aZero
{
	class TestKeyboard : public Input::Keyboard {
	public:
		TestKeyboard()
			:Keyboard() {
		}

		void OnKeyUp(SDL_Keycode keyCode) final override {
			std::cout << "KEY UP\n";
		}

		void OnKeyDown(SDL_Keycode keyCode) final override {
			std::cout << "KEY DOWN\n";
		}

		void OnConnect(const SDL_Event& event) final override {
			std::cout << "KEYBOARD CONNECTED\n";
		}

	private:
		void ProcessEventImpl(const SDL_Event& event) final override {}
	};
}