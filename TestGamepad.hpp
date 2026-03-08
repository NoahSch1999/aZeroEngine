#pragma once
#include "include/input/SDLInput.hpp"

namespace aZero
{
	class TestGamepad : public Input::Gamepad {
	public:
		TestGamepad() = default;
		explicit TestGamepad(SDL_JoystickID id)
			:Gamepad(id) {
		}
		TestGamepad(TestGamepad&&) = default;
		TestGamepad& operator=(TestGamepad&&) = default;

		void OnConnect() final { std::cout << "\n\nCONNECTED GAMEPAD GUID: " << this->GetSDLGUID() << " with name: " << this->GetName() << "\n"; }
		void OnDisconnect() final { std::cout << "\n\nDISCONNECTED GAMEPAD GUID: " << this->GetSDLGUID() << " with name: " << this->GetName() << "\n"; }

	private:
		void OnButtonUp(const SDL_GamepadButtonEvent& event) final {}
		void OnButtonDown(const SDL_GamepadButtonEvent& event) final {
			if (event.button == SDL_GamepadButton::SDL_GAMEPAD_BUTTON_NORTH) {
				this->SetLED(0, 255, 0);
			}
			if (event.button == SDL_GamepadButton::SDL_GAMEPAD_BUTTON_SOUTH) {
				this->SetLED(0, 0, 255);
			}
			if (event.button == SDL_GamepadButton::SDL_GAMEPAD_BUTTON_WEST) {
				this->SetLED(255, 0, 255);
			}
			if (event.button == SDL_GamepadButton::SDL_GAMEPAD_BUTTON_EAST) {
				this->SetLED(255, 0, 0);
			}
			if (event.button == SDL_GamepadButton::SDL_GAMEPAD_BUTTON_START) {
				this->Disconnect();
				//this->Disconnect();
			}
		}
		void OnAxisInput(const SDL_GamepadAxisEvent& event) final {
			if (event.axis == SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) {
				this->StartRumbleController(0, event.value * 2, 2000);
			}
			else if (event.axis == SDL_GAMEPAD_AXIS_LEFT_TRIGGER) {
				this->StartRumbleController(0, event.value * 2, 2000);
			}
		}
		void OnTouchInput(const SDL_GamepadTouchpadEvent& event) final {}
	};
}