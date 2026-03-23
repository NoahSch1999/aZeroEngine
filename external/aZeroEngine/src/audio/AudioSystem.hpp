//#pragma once
//#include <stdexcept>
//#include <optional>
//#include "AudioSource.hpp"
//
//// https://miniaud.io/docs/manual/#Introduction
//namespace aZero::Audio {
//	class AudioSystem {
//	public:
//		AudioSystem() {
//			ma_result result = ma_engine_init(NULL, &m_Engine);
//			if (result != MA_SUCCESS) {
//				throw std::runtime_error("Failed to create audio engine");
//			}
//
//			m_Manager = ma_engine_get_resource_manager(&m_Engine);
//			m_Device = ma_engine_get_device(&m_Engine);
//		}
//
//		~AudioSystem() {
//			ma_engine_uninit(&m_Engine);
//		}
//
//		std::optional<AudioSource> LoadAudio(std::string_view path) { // TODO: Cache loaded so we dont need to read from file every time
//			AudioSource source;
//			if (source.Load(m_Engine, path))
//				return source;
//			return {};
//		}
//
//		ma_engine m_Engine;
//	private:
//
//		void Move(AudioSystem& other) {
//			m_Engine = other.m_Engine;
//			m_Device = other.m_Device;
//			other.m_Device = nullptr;
//			m_Manager = other.m_Manager;
//			other.m_Manager = nullptr;
//		}
//
//		ma_device* m_Device = nullptr;
//		ma_resource_manager* m_Manager = nullptr;
//	};
//}