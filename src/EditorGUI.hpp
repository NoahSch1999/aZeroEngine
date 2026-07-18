#pragma once
#include "Engine.hpp"
#include <aZeroInput.hpp>
#include "ImguiInclude.hpp"
#include <chrono>

#include "misc/Flecs_Helpers.hpp"

namespace aZero::Editor::GUI
{
	class EditorGUI
	{
		struct SceneHierarchyEditing
		{
			flecs::entity LastSelectedEntity;
			bool RefreshEntityData = false;
		};

		struct DialogueSettings
		{
			bool ShowSceneEditor = false;
			bool ShowStats = false;
			bool ShowDebugSettings = false;
			bool ShowMisc = false;
			bool ShowEditorSettings = false;
			bool ShowRenderSettings = false;
		};

	public:
		EditorGUI() = default;
		EditorGUI(Input::DeviceManager& deviceManager,
			Rendering::WireframeRenderer& wireframeRenderer
		)
			:m_diWireframeRenderer(&wireframeRenderer) { }

		

		void Update(Scene::Scene& scene, Rendering::Renderer& renderer)
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

			ImGui::Begin("Dialogue Settings");
			ImGui::Checkbox("Show editor settings", &m_DialogueSettings.ShowEditorSettings);
			ImGui::Checkbox("Show render settings", &m_DialogueSettings.ShowRenderSettings);
			ImGui::Checkbox("Show debug settings", &m_DialogueSettings.ShowDebugSettings);
			ImGui::Checkbox("Show scene editor", &m_DialogueSettings.ShowSceneEditor);
			ImGui::Checkbox("Show stats", &m_DialogueSettings.ShowStats);
			ImGui::Checkbox("Show misc", &m_DialogueSettings.ShowMisc);
			ImGui::End();

			if (m_DialogueSettings.ShowStats)
			{
				this->ShowStats();
			}

			if (m_DialogueSettings.ShowSceneEditor)
			{
				this->ShowEditor(scene);
			}

			if (m_DialogueSettings.ShowRenderSettings)
			{
				this->ShowRenderSettings(renderer);
			}

			if (m_DialogueSettings.ShowDebugSettings)
			{
				this->ShowDebugSettings(scene, renderer);
			}

			if (m_DialogueSettings.ShowEditorSettings)
			{
				this->ShowEditorSettings();
			}

			if (m_DialogueSettings.ShowMisc)
			{
				this->ShowMisc(scene);
			}
		}

		void Render(Rendering::Renderer& renderer, Rendering::RenderTarget& rtv)
		{
			if (m_ShowEditorGUI)
			{
				RenderAPI::CommandList& cmdList = renderer.GetCurrentContext().GetCommandList();
				auto rtvHandle = rtv.GetCpuHandle();

				ImGui::Render();

				std::array<ID3D12DescriptorHeap*, 1> heaps{ renderer.GetResourceHeap().Get() };
				cmdList->SetDescriptorHeaps(heaps.size(), &heaps[0]);
				cmdList->OMSetRenderTargets(1, &rtvHandle, false, nullptr);

				ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList.Get());

				renderer.GetGraphicsCommandQueue().ExecuteCommandList(cmdList, true);

			}
			else
			{
				aZero::ImGui_Wrapper::EndFrame();
			}
			aZero::ImGui_Wrapper::HandleMultiViewport();
		}
		bool m_ShowEditorGUI = true;

	private:

		void ShowEditor(Scene::Scene& scene)
		{
			this->ShowSceneHierarchy(scene);
			this->ShowEntityEditor(scene);
		}

		void ShowRenderSettings(Rendering::Renderer& renderer)
		{
			ImGui::Begin("Render settings");

			auto renderSettings = renderer.GetRenderSettings();
			if (ImGui::Checkbox("Enable depth prepass", &renderSettings.EnableDepthPrepass))
			{
				renderer.ToggleDepthPrepass(renderSettings.EnableDepthPrepass);
			}

			auto currentRenderMode = Rendering::Renderer::RenderSettings::RenderModeToString(renderer.GetRenderSettings().RenderMode);

			if (ImGui::BeginCombo("Render Mode", currentRenderMode.c_str()))
			{
				std::vector<std::string> modes(Rendering::Renderer::RenderSettings::ERenderMode::RENDER_MODE_COUNT);
				for (uint32_t i = 0; i < Rendering::Renderer::RenderSettings::ERenderMode::RENDER_MODE_COUNT; i++)
				{
					modes[i] = Rendering::Renderer::RenderSettings::RenderModeToString(static_cast<Rendering::Renderer::RenderSettings::ERenderMode>(i));
				}

				for (uint32_t i = 0; i < modes.size(); i++)
				{
					const bool is_selected = (renderer.GetRenderSettings().RenderMode == static_cast<Rendering::Renderer::RenderSettings::ERenderMode>(i));
					if (ImGui::Selectable(modes[i].c_str(), is_selected)) {
						renderer.SetRenderMode(static_cast<Rendering::Renderer::RenderSettings::ERenderMode>(i));
					}

					// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
					if (is_selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}

			ImGui::End();
		}

		void ShowSceneHierarchyRecursive(flecs::entity e)
		{
			ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
			bool isOpen = ImGui::TreeNodeEx(e.name(), flags);

			if (ImGui::IsItemClicked())
			{
				std::cout << "Opened: " << e.name() << "\n";
				if (e != m_SceneHierarchyEditing.LastSelectedEntity)
				{
					m_SceneHierarchyEditing.RefreshEntityData = true;
					m_SceneHierarchyEditing.LastSelectedEntity = e;
				}
			}

			if (isOpen)
			{
				e.children([&](flecs::entity child) {
					this->ShowSceneHierarchyRecursive(child);
				});
				ImGui::TreePop();
			}
		}

		void ShowSceneHierarchy(Scene::Scene& scene)
		{
			ImGui::Begin("Scene Hierarchy");
			scene.GetRootEntityQuery().each([this](flecs::entity e) {
				this->ShowSceneHierarchyRecursive(e);
			});
			ImGui::End();
		}

		void ShowEntityEditor(Scene::Scene& scene)
		{
			ImGui::Begin("Entity Editor");

			if (m_SceneHierarchyEditing.LastSelectedEntity.is_valid())
			{
				ImGui::Text("Name: %s", m_SceneHierarchyEditing.LastSelectedEntity.name().c_str());
			}

			ImGui::End();
		}

		void ShowEditorSettings()
		{
			ImGui::Begin("Editor Setting");
			
			ImGui::Checkbox("Show world grid", &m_ShowGrid);

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

			ImGui::End();
		}

		void ShowStats()
		{
			ImGui::Begin("Statistics");

			ImGui::Text(("FPS: " + std::to_string(m_FPS)).c_str());

			ImGui::End();
		}

		void ShowDebugSettings(Scene::Scene& scene, Rendering::Renderer& renderer)
		{
			ImGui::Begin("Debug Settings");

			ImGui::Checkbox("Show rigidbody colliders", &m_ShowRigidbodyColliders);
			ImGui::Checkbox("Show triggerbody colliders", &m_ShowTriggerbodyColliders);
			ImGui::Checkbox("Show mesh bounds", &m_ShowMeshBounds);

			m_EnableFlecsExplorer = scene.GetWorld().has<flecs::Rest>();
			if (ImGui::Checkbox("Enable flecs explorer", &m_EnableFlecsExplorer)) {
				if (m_EnableFlecsExplorer) {
					// on
					scene.GetWorld().set<flecs::Rest>({});
				}
				else {
					// off
					scene.GetWorld().remove<flecs::Rest>();
				}
			}

			if (m_ShowRigidbodyColliders)
			{
				if (scene.HasPhysics())
				{
					scene.GetRigidbodyQuery().each([this](Component::Rigidbody& rigidBody, Component::Position& position, Component::Rotation& rotation) {
						auto lock = rigidBody.GetBody().LockForRead();
						if (lock.Succeeded())
						{
							auto& body = lock.GetBody();
							auto bounds = body.GetWorldSpaceBounds();
							auto* shape = body.GetShape();
							if (body.GetShape()->GetSubType() == JPH::EShapeSubType::Box)
							{
								const JPH::BoxShape* boxShape = static_cast<const JPH::BoxShape*>(body.GetShape());
								this->m_diWireframeRenderer->AddShape(Rendering::WireframeShape::OBB(DXM::Vector3(0, 1, 0), Math::Convert(body.GetPosition()), Math::Convert(body.GetRotation()), Math::Convert(boxShape->GetHalfExtent())));
							}
						}
						});
				}
			}

			if (m_ShowTriggerbodyColliders)
			{
				if (scene.HasPhysics())
				{
					scene.GetTriggerbodyQuery().each([this](Component::Triggerbody& triggerbody, Component::Position& position) {
						auto lock = triggerbody.GetBody().LockForRead();
						if (lock.Succeeded())
						{
							auto& body = lock.GetBody();
							auto bounds = body.GetWorldSpaceBounds();
							auto* shape = body.GetShape();
							if (body.GetShape()->GetSubType() == JPH::EShapeSubType::Box)
							{
								const JPH::BoxShape* boxShape = static_cast<const JPH::BoxShape*>(body.GetShape());
								this->m_diWireframeRenderer->AddShape(Rendering::WireframeShape::OBB(DXM::Vector3(0, 1, 0), Math::Convert(body.GetPosition()), Math::Convert(body.GetRotation()), Math::Convert(boxShape->GetHalfExtent())));
							}
						}
						});
				}
			}

			if (m_ShowMeshBounds)
			{
				// todo Fix since the performance is horrendous
				//scene.GetStaticMeshQuery().each(
				//	[this](flecs::entity entity, const Component::Mesh& mesh, const Component::Position& position, const Component::Rotation& rotation, const Component::Scale& scale) {
				//		for (const auto& submesh : mesh.m_Submeshes)
				//		{
				//			Rendering::WireframeShape::Sphere sphere(DXM::Vector3(0, 0, 1), DXM::Vector3::Transform(submesh.m_Bounds.Center, DXM::Matrix::CreateTranslation(position)), submesh.m_Bounds.Radius, 5); // todo Handle scale
				//			this->m_diWireframeRenderer->AddShape(sphere);
				//		}
				//	}
				//);

				//scene.GetDynamicMeshQuery().each(
				//	[this](flecs::entity entity, const Component::Mesh& mesh, const Component::Position& position, const  Component::Rotation& rotation, const Component::Scale& scale) {
				//		for (const auto& submesh : mesh.m_Submeshes)
				//		{
				//			Rendering::WireframeShape::Sphere sphere(DXM::Vector3(0, 0, 1), DXM::Vector3::Transform(submesh.m_Bounds.Center, DXM::Matrix::CreateTranslation(position)), submesh.m_Bounds.Radius, 5); // todo Handle scale
				//			this->m_diWireframeRenderer->AddShape(sphere);
				//		}
				//	}
				//);
			}

			ImGui::End();
		}

		void ShowMisc(Scene::Scene& scene)
		{
			ImGui::Begin("Misc");

			if (ImGui::Checkbox("Enable physics camera", &m_PhysicsCamera))
			{
				flecs::entity ent = scene.GetWorld().lookup("EditorCamera");
				if (m_PhysicsCamera)
				{
					JPH::BoxShapeSettings boxShape(JPH::Vec3(1, 1, 1));
					JPH::BodyCreationSettings boxSettings(boxShape.Create().Get(), JPH::RVec3(0.0, 100.0, 0.0), JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic, aZero::Physics::Layers::DYNAMIC);
					boxSettings.mOverrideMassProperties = JPH::EOverrideMassProperties::MassAndInertiaProvided;
					boxSettings.mMassPropertiesOverride.mMass = 1.0f;

					Component::Rigidbody rb(boxSettings);

					ent.set<Component::Rigidbody>(rb);
					ent.get_mut<Component::Rigidbody>().GetBody().SetPosition(Math::Convert(DXM::Vector3(ent.get<Component::Position>())), JPH::EActivation::Activate);
					ent.get_mut<Component::Rigidbody>().SetOnContactAdded([&scene](Physics::Body& body, const JPH::ContactManifold& cm, const JPH::ContactSettings& cs) {
						std::cout << "Camera collided with body: " << scene.GetWorld().entity(scene.GetBodyID_To_EntityID_Map().at(body.GetBodyID().GetIndexAndSequenceNumber())).name() << "\n";
						});
				}
				else
				{
					ent.remove<Component::Rigidbody>();
				}
			}

			ImGui::Checkbox("Show demo window", &m_Show_demo_window);
			if (m_Show_demo_window)
				ImGui::ShowDemoWindow(&m_Show_demo_window);

			ImGui::End();
		}

		uint64_t m_FrameCount = 0;
		float m_FPS = 0.f;
		std::chrono::high_resolution_clock::time_point m_LastTime =
			std::chrono::high_resolution_clock::now();

		bool m_EnableFlecsExplorer = false;
		bool m_ShowRigidbodyColliders = false;
		bool m_ShowTriggerbodyColliders = false;
		bool m_ShowMeshBounds = false;
		bool m_ShowGrid = true;
		bool m_PhysicsCamera = false;
		bool m_Show_demo_window = false;
		Rendering::WireframeRenderer* m_diWireframeRenderer;
		SceneHierarchyEditing m_SceneHierarchyEditing;
		DialogueSettings m_DialogueSettings;
	};
}