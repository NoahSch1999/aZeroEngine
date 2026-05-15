#pragma once
#include "Engine.hpp"
#include "aZeroInput.hpp"
#include "ImguiInclude.hpp"
#include <chrono>

namespace aZero::Editor::GUI
{
	class EditorGUI
	{
	public:
		EditorGUI(Input::DeviceManager& deviceManager,
			// Dependency injections
			Rendering::WireframeRenderer& wireframeRenderer,
			Asset::AssetManager& assetManager
		)
			:m_diWireframeRenderer(&wireframeRenderer), m_diAssetManager(&assetManager)
		{
			m_KeyboardListener = deviceManager.ListenKeyboard(
				{
					[this](const SDL_Event& event, Input::Keyboard& keyboard) {
						if (event.type == SDL_EVENT_KEY_DOWN)
						{
							if (event.key.key == SDLK_ESCAPE)
							{
								this->m_ShowEditorGUI = !this->m_ShowEditorGUI;
							}
							if (event.key.key == SDLK_C)
							{
								this->m_ShowColliders = !this->m_ShowColliders;
							}
							if (event.key.key == SDLK_M)
							{
								this->m_ShowMeshBounds = !this->m_ShowMeshBounds;
							}
						}
					},
					[](const SDL_Event& event, Input::Keyboard& keyboard) {
						
					}
				}
			);
		}

		void Update(Scene::Scene& scene)
		{
			m_FrameCount++;
			auto now = std::chrono::high_resolution_clock::now();
			float elapsed =
				std::chrono::duration<float>(now - m_LastTime).count();

			if (elapsed >= 1.0f)
			{
				m_FPS = m_FrameCount / elapsed;
				m_FrameCount = 0;
				m_LastTime = now;
			}

			if (m_ShowEditorGUI)
			{
				ImGui::Text(std::to_string(m_FPS).c_str());

				// Debug settings
				ImGui::Begin("Debug");

				if(ImGui::Button("TEST EVENT"))
				{
					auto ent = scene.GetEntityWorld().lookup("Mesh");
					scene.UnregisterFromPhysics(ent);
					ent.destruct();
				}

				if (ImGui::Checkbox("Enable physics camera", &m_PhysicsCamera))
				{
					flecs::entity ent = scene.GetEntityWorld().lookup("Camera");
					if (m_PhysicsCamera)
					{
						JPH::BoxShapeSettings boxShape(JPH::Vec3(1, 1, 1));
						JPH::BodyCreationSettings boxSettings(boxShape.Create().Get(), JPH::RVec3(0.0, 100.0, 0.0), JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic, aZero::Physics::Layers::DYNAMIC);
						boxSettings.mOverrideMassProperties = JPH::EOverrideMassProperties::MassAndInertiaProvided;
						boxSettings.mMassPropertiesOverride.mMass = 1.0f;

						ent.set(Component::Rigidbody());
						Component::Rigidbody& rb = ent.get_mut<Component::Rigidbody>();
						rb.SetCreationSettings(boxSettings);
						scene.RegisterToPhysics(ent);
						rb.GetBody().SetPosition(Math::Convert(DXM::Vector3(ent.get<Component::Position>())), JPH::EActivation::Activate);
						rb.SetOnContactAdded([&scene](Physics::Body& body, const JPH::ContactManifold& cm, const JPH::ContactSettings& cs) {
							std::cout << "Camera collided with body: " << scene.GetEntityWorld().entity(scene.GetBodyID_To_EntityID_Map().at(body.GetBodyID())).name() << "\n";
						});
					}
					else
					{
						scene.UnregisterFromPhysics(ent);
						ent.remove<Component::Rigidbody>();
					}
				}

				ImGui::Checkbox("Show colliders", &m_ShowColliders);
				ImGui::Checkbox("Show mesh bounds", &m_ShowMeshBounds);
				ImGui::Checkbox("Show demo window", &m_Show_demo_window);
				if(m_Show_demo_window)
					ImGui::ShowDemoWindow(&m_Show_demo_window);

				ImGui::End();

				// General ditor settings
				ImGui::Begin("Editor");

				ImGui::Checkbox("Show world grid", &m_ShowGrid);

				ImGui::End();
			}

			if (m_ShowColliders || m_ShowMeshBounds) {
				scene.AddDebugDrawArguments(*m_diAssetManager, *m_diWireframeRenderer, m_ShowColliders, m_ShowMeshBounds);
			}

			if (m_ShowGrid)
			{
				constexpr int gridOffset = 5;
				constexpr int gridDimensions = 200;
				Rendering::WireframeShape::LineShape gridLines;
				for (int i = -gridDimensions; i < gridDimensions; i += gridOffset)
				{
					gridLines.m_Lines.emplace_back(Rendering::WireframeShape::Line(DXM::Vector3(-gridDimensions, 0, i), DXM::Vector3(gridDimensions, 0, i), DXM::Vector3(70, 70, 70)));
					gridLines.m_Lines.emplace_back(Rendering::WireframeShape::Line(DXM::Vector3(i, 0, -gridDimensions), DXM::Vector3(i, 0, gridDimensions), DXM::Vector3(70, 70, 70)));
				}

				m_diWireframeRenderer->AddShape(gridLines);
			}
		}

		void Render(Rendering::Renderer& renderer, Rendering::RenderTarget& rtv)
		{
			if (m_ShowEditorGUI)
			{
				RenderAPI::CommandList& cmdList = renderer.GetCurrentContext().m_DirectCmdList;
				auto rtvHandle = rtv.GetCpuHandle();

				ImGui::Render();

				std::array<ID3D12DescriptorHeap*, 1> heaps{ renderer.GetResourceHeap().Get() };
				cmdList->SetDescriptorHeaps(heaps.size(), &heaps[0]);
				cmdList->OMSetRenderTargets(1, &rtvHandle, false, nullptr);

				ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), renderer.GetCurrentContext().m_DirectCmdList.Get());

				renderer.GetGraphicsCommandQueue().ExecuteCommandList(renderer.GetCurrentContext().m_DirectCmdList, true);

			}
			else
			{
				aZero::ImGui_Wrapper::EndFrame();
			}
			aZero::ImGui_Wrapper::HandleMultiViewport();
		}

	private:
		uint64_t m_FrameCount = 0;
		float m_FPS = 0.f;
		std::chrono::high_resolution_clock::time_point m_LastTime =
			std::chrono::high_resolution_clock::now();

		bool m_ShowEditorGUI = true;
		bool m_ShowColliders = false;
		bool m_ShowMeshBounds = false;
		bool m_ShowGrid = true;
		bool m_PhysicsCamera = false;
		bool m_Show_demo_window = false;
		Input::KeyboardListener m_KeyboardListener;
		Rendering::WireframeRenderer* m_diWireframeRenderer;
		Asset::AssetManager* m_diAssetManager;
	};
}