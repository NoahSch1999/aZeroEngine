#pragma once
#include <stdexcept>
#include "miniaudio.h"
#include "graphics_api/D3D12Include.hpp"

namespace aZero::Audio {
	class AudioSource {
	public:
		AudioSource() = default;
		AudioSource(ma_engine& engine, std::string_view path) {
			this->Init(engine, path);
		}

		~AudioSource() {
			this->Destroy();
		}

		AudioSource(const AudioSource&) = delete;
		AudioSource& operator=(const AudioSource&) = delete;

		AudioSource(AudioSource&& other) noexcept {
			this->Move(other);
		}

		AudioSource& operator=(AudioSource&& other) noexcept {
			if(this != &other)
				this->Move(other);
			return *this;
		}

		void Play() {
			ma_sound_start(m_Sound);
		}

		bool Load(ma_engine& engine, std::string_view path) {
			return this->Init(engine, path);
		}

	private:
		bool Init(ma_engine& engine, std::string_view path)
		{
			ma_result result;

			this->Destroy();
			m_Sound = new ma_sound;

			result = ma_sound_init_from_file(&engine, path.data(), 0, NULL, NULL, m_Sound);
			if (result != MA_SUCCESS) {
				return false;
			}
			return true;
		}

		void Destroy()
		{
			if (m_Sound) {
				ma_sound_uninit(m_Sound);
				delete m_Sound;
			}
		}

		void Move(AudioSource& other) {
			m_Sound = other.m_Sound;
			other.m_Sound = nullptr;
			m_Path = other.m_Path;
			other.m_Path = "";
		}

		ma_sound* m_Sound = nullptr;
		std::string m_Path;
	};
}