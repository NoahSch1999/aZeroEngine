#pragma once
#include <memory>
#include "renderer/Renderer.hpp"
#include "scene/Scene.hpp"
#include "aZeroAudio.hpp"

namespace aZero
{
	class Engine : public NonCopyable
	{
	public:
		Engine(uint32_t bufferCount);
		Engine(Engine&&) noexcept = default;
		Engine& operator=(Engine&&) noexcept = default;
		~Engine();

		IDxcCompilerX& GetCompiler() const { return *m_Compiler.Get(); }
		ID3D12DeviceX* GetDevice() const { return m_Device.Get(); }

		Rendering::Renderer& GetRenderer() const { return *m_Renderer.get(); }
		Audio::AudioEngine& GetAudioEngine() const { return *m_AudioEngine.get(); }

		// TODO: Replace with a better file system/handling implementation (perhaps a project file or something reads the path)
		const std::string& GetProjectDirectory() const { return m_ProjectDirectory; }

	private:
		Microsoft::WRL::ComPtr<ID3D12DeviceX> m_Device;
		Microsoft::WRL::ComPtr<IDxcCompilerX> m_Compiler;

		// TODO: Replace with a better file system/handling implementation (perhaps a project file or something reads the path)
		std::string m_ProjectDirectory;

		// API Interfaces
		std::unique_ptr<Rendering::Renderer> m_Renderer;
		std::unique_ptr<Audio::AudioEngine> m_AudioEngine;
		//
	};
}