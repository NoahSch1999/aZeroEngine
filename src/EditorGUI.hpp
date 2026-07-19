#pragma once
#include "Engine.hpp"
#include <aZeroInput.hpp>
#include "ImguiInclude.hpp"
#include "Statistics.hpp"

#include "misc/Flecs_Helpers.hpp"

namespace aZero::Editor::GUI
{
	class EditorGUI
	{
		struct SceneHierarchyEditing
		{
			flecs::entity LastSelectedEntity;
			bool RefreshEntityData = false;
			uint32_t SelectedSubmesh = 0;
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
			Rendering::WireframeRenderer& wireframeRenderer,
			Asset::AssetManager<std::string>& assetManager
		)
			:m_diWireframeRenderer(&wireframeRenderer),
			m_diAssetManager(&assetManager)
		{ }

		

		void Update(Scene::Scene& scene, Rendering::Renderer& renderer, Statistics& stats)
		{
			if (m_ShowEditorGUI)
			{
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
					this->ShowStats(stats);
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

			// todo Fix crash if a viewport is open but m_ShowEditorGUI is false
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
				if (e != m_SceneHierarchyEditing.LastSelectedEntity)
				{
					m_SceneHierarchyEditing.LastSelectedEntity = e;
					m_SceneHierarchyEditing.SelectedSubmesh = 0;
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

		void ShowEntityComponents(flecs::entity e, Scene::Scene& scene)
		{
			bool shouldUpdateStatics = false;
			{
				ImGui::SeparatorText("Entity Settings");
				ImGui::Text("Name: %s", m_SceneHierarchyEditing.LastSelectedEntity.name().c_str());
				std::array<std::string, 2> entityTypesStr = { "Static", "Dynamic" };
				std::string staticType = e.has<Component::Static>() ? entityTypesStr[0] : entityTypesStr[1];

				if (ImGui::BeginCombo("Entity Update Type", staticType.c_str())) {
					for (uint32_t i = 0; i < entityTypesStr.size(); i++) {
						const bool is_selected = entityTypesStr[i] == staticType;
						if (ImGui::Selectable(entityTypesStr[i].c_str(), is_selected)) {
							if (e.has<Component::Static>() && entityTypesStr[i] == "Dynamic") {
								e.remove<Component::Static>();
								shouldUpdateStatics = true;
							}
							else if (!e.has<Component::Static>() && entityTypesStr[i] == "Static") {
								e.add<Component::Static>();
								shouldUpdateStatics = true;
							}
						}
						if (is_selected)
							ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}
			}

			{
				ImGui::SeparatorText("Transform");

				if (Component::Position* pComp = e.try_get_mut<Component::Position>(); pComp != nullptr)
				{
					ImGui::TextColored(
						ImVec4(0.f, 1.f, 0.f, 1.0f),
						"Position"
					);

					if (ImGui::DragFloat3("##Position", &pComp->x, 0.1f) && e.has<Component::Static>())
					{
						shouldUpdateStatics = true;
					}
				}

				if (Component::Rotation* pComp = e.try_get_mut<Component::Rotation>(); pComp != nullptr)
				{
					ImGui::TextColored(
						ImVec4(1.0f, 0.f, 0.f, 1.0f),
						"Rotation"
					);

					if (ImGui::DragFloat3("##Rotation", &pComp->x, 0.1f) && e.has<Component::Static>())
					{
						shouldUpdateStatics = true;
					}
				}

				if (Component::Scale* pComp = e.try_get_mut<Component::Scale>(); pComp != nullptr)
				{
					ImGui::TextColored(
						ImVec4(0.2f, 0.6f, 1.0f, 1.0f),
						"Scale"
					);

					if (ImGui::DragFloat3("##Scale", &pComp->x, 0.1f) && e.has<Component::Static>())
					{
						shouldUpdateStatics = true;
					}
				}
			}

			{
				if (Component::Mesh* pComp = e.try_get_mut<Component::Mesh>(); pComp != nullptr)
				{
					ImGui::SeparatorText("Mesh");
					/*ImGui::TextColored(
						ImVec4(0.2f, 0.6f, 1.0f, 1.0f),
						"Scale"
					);
					ImGui::DragFloat3("##Scale", &pComp->x, 0.1f);*/
					

					const Asset::Mesh* meshAsset = nullptr;

					{
						const auto& assetContainer = m_diAssetManager->GetContainer<Asset::Mesh>();
						for (const auto& asset : assetContainer)
						{
							if (asset.second->GetRenderRef().m_MeshletGlobalOffset == pComp->m_MeshID)
							{
								meshAsset = asset.second.get();
								break;
							}
						}
					}

					if (meshAsset)
					{
						ImGui::Text(("Name: " + meshAsset->GetCachedData().Name).c_str());
						ImGui::Text(("Submesh Count: " + std::to_string(pComp->m_NumSubmeshes)).c_str());

						if (pComp->m_NumSubmeshes > 0)
						{
							ImGui::SeparatorText("Submesh");

							ImGui::SetNextItemWidth(50.f);
							if (ImGui::BeginCombo("Submesh", std::to_string(m_SceneHierarchyEditing.SelectedSubmesh).c_str()))
							{
								for (uint32_t i = 0; i < pComp->m_NumSubmeshes; i++) 
								{
									const bool is_selected = (m_SceneHierarchyEditing.SelectedSubmesh == i);
									if (ImGui::Selectable(std::to_string(i).c_str(), is_selected))
										m_SceneHierarchyEditing.SelectedSubmesh = i;

									// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
									if (is_selected)
										ImGui::SetItemDefaultFocus();
								}
								ImGui::EndCombo();
							}

							if (m_SceneHierarchyEditing.SelectedSubmesh < pComp->m_NumSubmeshes)
							{
								ImGui::BeginGroup();

								ImGui::Text(("Meshlet Count: " + std::to_string(pComp->m_Submeshes[m_SceneHierarchyEditing.SelectedSubmesh].MeshletCount)).c_str());
								ImGui::Text(("Bounding Radius: " + std::to_string(pComp->m_Submeshes[m_SceneHierarchyEditing.SelectedSubmesh].m_Bounds.Radius)).c_str());

								const auto& assetContainer = m_diAssetManager->GetContainer<Asset::Material>();
								Asset::Material* materialAsset = nullptr;
								for (const auto& asset : assetContainer)
								{
									if (asset.second->GetRenderRef().MaterialIndex == pComp->m_Submeshes[m_SceneHierarchyEditing.SelectedSubmesh].m_MaterialID)
									{
										materialAsset = asset.second.get();
										break;
									}
								}

								if (!materialAsset) {
									materialAsset = m_diAssetManager->Get<Asset::Material>("Fallback");
								}

								ImGui::Text("Material:");
								ImGui::SameLine();
								if (ImGui::BeginCombo("##MaterialSelect", materialAsset->GetCachedData().Name.c_str()))
								{
									std::string materialSelected = materialAsset->GetCachedData().Name;
									for (const auto& [name, asset] : assetContainer)
									{
										const bool is_selected = (materialSelected == name);
										if (ImGui::Selectable(name.c_str(), is_selected))
										{
											materialSelected = name;
											pComp->SetMaterial(m_SceneHierarchyEditing.SelectedSubmesh, *asset);
											if (e.has<Component::Static>()) {
												shouldUpdateStatics = true;
											}
										}

										// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
										if (is_selected)
											ImGui::SetItemDefaultFocus();
									}
									ImGui::EndCombo();
								}

								/*
								materialAsset->GetAlbedoPtr();
										materialAsset->GetNormalMapPtr();
										materialAsset->GetMetallicRoughnessTexturePtr();
										materialAsset->GetCachedData().Name;
										materialAsset->GetCachedData().Info.MetallicFactor;
										materialAsset->GetCachedData().Info.RoughnessFactor;
										materialAsset->GetCachedData().FilePath;
								*/

								ImGui::EndGroup();
							}
						}
					}
					else
					{
						ImGui::TextColored(
							ImVec4(1.f, 0.f, 0.f, 1.0f),
							"ERROR - Mesh component references an invalid mesh asset!"
						);
					}
				}
			}

			if (shouldUpdateStatics)
			{
				scene.MarkStaticMeshesDirty();
				scene.MarkStaticLightsDirty();
			}
		}

		void ShowEntityEditor(Scene::Scene& scene)
		{
			ImGui::Begin("Entity Editor");

			if (m_SceneHierarchyEditing.LastSelectedEntity.is_valid())
			{
				this->ShowEntityComponents(m_SceneHierarchyEditing.LastSelectedEntity, scene);
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

		void ShowStats(Statistics& stats)
		{
			ImGui::Begin("Statistics");

			ImGui::Text(("FPS: " + std::to_string(stats.FPS.Value)).c_str());
			ImGui::Text(("Scene Render (ms): " + std::to_string(stats.GetRenderStat(Statistics::ERenderStat::Scene))).c_str());
			ImGui::Text(("Scene Wireframe (ms): " + std::to_string(stats.GetRenderStat(Statistics::ERenderStat::Wireframe))).c_str());
			ImGui::Text(("Editor GUI (ms): " + std::to_string(stats.GetRenderStat(Statistics::ERenderStat::EditorGUI))).c_str());
			ImGui::Text(("Resolve SwapChain (ms): " + std::to_string(stats.GetRenderStat(Statistics::ERenderStat::ResolveSwapChain))).c_str());
			ImGui::Text(("Present (ms): " + std::to_string(stats.GetRenderStat(Statistics::ERenderStat::Present))).c_str());

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

		

		bool m_EnableFlecsExplorer = false;
		bool m_ShowRigidbodyColliders = false;
		bool m_ShowTriggerbodyColliders = false;
		bool m_ShowMeshBounds = false;
		bool m_ShowGrid = true;
		bool m_PhysicsCamera = false;
		bool m_Show_demo_window = false;
		Rendering::WireframeRenderer* m_diWireframeRenderer;
		Asset::AssetManager<std::string>* m_diAssetManager;
		SceneHierarchyEditing m_SceneHierarchyEditing;
		DialogueSettings m_DialogueSettings;
	};
}