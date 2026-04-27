#pragma once
#include <memory>
#include "renderer/Renderer.hpp"
#include "renderer/WireframeRenderer.hpp"
#include "scene/Scene.hpp"
#include "aZeroAudio.hpp"
#include "physics/PhysicsEngine.hpp"

namespace aZero
{
	class Engine : public NonCopyable
	{
	public:
		Engine(uint32_t bufferCount);
		Engine(Engine&&) noexcept = default;
		Engine& operator=(Engine&&) noexcept = default;
		~Engine();

		bool TryBeginFrame();
		void EndFrame();

		void DrawColliders(const ECS::CameraComponent& camera, Rendering::RenderTarget& rtv, Rendering::DepthStencilTarget& dsv) // TODO: Move to renderer
		{
			m_PhysicsEngine->GetColliderRenderer().Render(*m_Renderer.get(), camera, rtv, dsv);
		}

		Scene::SceneNew CreateScene()
		{
			return Scene::SceneNew(*m_PhysicsEngine.get());
		}

		IDxcCompilerX& GetCompiler() const { return *m_Compiler.Get(); }
		ID3D12DeviceX* GetDevice() const { return m_Device.Get(); }

		Rendering::Renderer& GetRenderer() const { return *m_Renderer.get(); }
		Audio::AudioEngine& GetAudioEngine() const { return *m_AudioEngine.get(); }
		Physics::PhysicsEngine& GetPhysicsEngine() const { return *m_PhysicsEngine.get(); }

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
		std::unique_ptr<Physics::PhysicsEngine> m_PhysicsEngine;
		//
	};
}