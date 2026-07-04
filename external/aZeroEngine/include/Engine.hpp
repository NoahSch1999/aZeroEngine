#pragma once
#include <memory>
#include "renderer/Renderer.hpp"
#include "renderer/WireframeRenderer.hpp"
#include "scene/Scene.hpp"
#include "aZeroAudio.hpp"
#include "physics/PhysicsEngine.hpp"
#include "assets/AssetTemplateImpl.hpp"

namespace aZero
{
	class Engine : public NonCopyable
	{
	public:
		Engine(std::string projectRootDirectory, uint32_t bufferCount);
		Engine(Engine&&) noexcept = default;
		Engine& operator=(Engine&&) noexcept = default;
		~Engine();

		IDxcCompilerX& GetCompiler() const { return *m_Compiler.Get(); }
		ID3D12DeviceX* GetDevice() const { return m_Device.Get(); }

		std::string GetProjectRootDirectory() { return m_ProjectRootDirectory; }

		Rendering::Renderer& GetRenderer() const { return *m_Renderer.get(); }
		Audio::AudioEngine& GetAudioEngine() const { return *m_AudioEngine.get(); }
		Physics::PhysicsEngine& GetPhysicsEngine() const { return *m_PhysicsEngine.get(); }
		Asset::AssetManager<std::string>& GetAssetManager_NEW() const { return *m_AssetManager_NEW.get(); }

	private:
		Microsoft::WRL::ComPtr<ID3D12DeviceX> m_Device;
		Microsoft::WRL::ComPtr<IDxcCompilerX> m_Compiler;

		std::string m_ProjectRootDirectory;

		// API Interfaces
		std::unique_ptr<Rendering::Renderer> m_Renderer;
		std::unique_ptr<Audio::AudioEngine> m_AudioEngine;
		std::unique_ptr<Physics::PhysicsEngine> m_PhysicsEngine;
		std::unique_ptr<Asset::AssetManager<std::string>> m_AssetManager_NEW;
		//
	};
}