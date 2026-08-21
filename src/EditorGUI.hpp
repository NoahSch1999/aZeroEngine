#pragma once
#include "Engine.hpp"
#include <aZeroInput.hpp>
#include "ImguiInclude.hpp"
#include "Statistics.hpp"
#include <psapi.h>
#include <format>

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

		struct MaterialEditing
		{
			std::string CurrentlyEditedMaterial;
			std::string SelectedAlbedo = "", SelectedNormalMap = "", SelectedMetallicRoughnessMap = "";
		};

		struct DialogueSettings
		{
			bool ShowSceneEditor = false;
			bool ShowMaterialEditor = false;
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
		{
			SYSTEM_INFO sysInfo;
			FILETIME ftime, fsys, fuser;

			GetSystemInfo(&sysInfo);
			numProcessors = sysInfo.dwNumberOfProcessors;

			GetSystemTimeAsFileTime(&ftime);
			memcpy(&lastCPU, &ftime, sizeof(FILETIME));

			self = GetCurrentProcess();
			GetProcessTimes(self, &ftime, &ftime, &fsys, &fuser);
			memcpy(&lastSysCPU, &fsys, sizeof(FILETIME));
			memcpy(&lastUserCPU, &fuser, sizeof(FILETIME));

			Microsoft::WRL::ComPtr<IDXGIFactory1> factory;
			CreateDXGIFactory1(IID_PPV_ARGS(&factory));

			Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter1;
			factory->EnumAdapters1(0, adapter1.GetAddressOf());

			adapter1.As(&m_Adapter);
		}

		double GetUsage_CPU() {
			FILETIME ftime, fsys, fuser;
			ULARGE_INTEGER now, sys, user;
			double percent;

			GetSystemTimeAsFileTime(&ftime);
			memcpy(&now, &ftime, sizeof(FILETIME));

			GetProcessTimes(self, &ftime, &ftime, &fsys, &fuser);
			memcpy(&sys, &fsys, sizeof(FILETIME));
			memcpy(&user, &fuser, sizeof(FILETIME));
			percent = (sys.QuadPart - lastSysCPU.QuadPart) +
				(user.QuadPart - lastUserCPU.QuadPart);
			percent /= (now.QuadPart - lastCPU.QuadPart);
			percent /= numProcessors;
			lastCPU = now;
			lastUserCPU = user;
			lastSysCPU = sys;

			return percent * 100;
		}

		void Update(Scene::Scene& scene, Rendering::Renderer& renderer, Statistics& stats)
		{
			if (m_ShowEditorGUI)
			{
				ImGui::Begin("Dialogue Settings");
				ImGui::Checkbox("Show editor settings", &m_DialogueSettings.ShowEditorSettings);
				ImGui::Checkbox("Show render settings", &m_DialogueSettings.ShowRenderSettings);
				ImGui::Checkbox("Show debug settings", &m_DialogueSettings.ShowDebugSettings);
				ImGui::Checkbox("Show scene editor", &m_DialogueSettings.ShowSceneEditor);
				ImGui::Checkbox("Show material editor", &m_DialogueSettings.ShowMaterialEditor);
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

				if (m_DialogueSettings.ShowMaterialEditor)
				{
					this->ShowMaterialEditor(renderer);
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

					if (ImGui::DragFloat3("##Position", &pComp->x, 0.1f))
					{
						if (Component::Rigidbody* rigidbody = e.try_get_mut<Component::Rigidbody>(); rigidbody != nullptr)
						{
							rigidbody->GetBody().SetPosition(Math::Convert(DXM::Vector3(pComp->x, pComp->y, pComp->z)), JPH::EActivation::Activate);
						}

						if (e.has<Component::Static>())
						{
							shouldUpdateStatics = true;
						}
					}
				}

				if (Component::Rotation* pComp = e.try_get_mut<Component::Rotation>(); pComp != nullptr)
				{
					ImGui::TextColored(
						ImVec4(1.0f, 0.f, 0.f, 1.0f),
						"Rotation (deg)"
					);

					DXM::Vector3 rotationDegrees = Math::PositiveZero({ Math::ToDegree(pComp->x), Math::ToDegree(pComp->y), Math::ToDegree(pComp->z) });

					if (ImGui::DragFloat3("##Rotation", &rotationDegrees.x, 0.1f))
					{
						pComp->x = Math::ToRadian(rotationDegrees.x);
						pComp->y = Math::ToRadian(rotationDegrees.y);
						pComp->z = Math::ToRadian(rotationDegrees.z);

						if (Component::Rigidbody* rigidbody = e.try_get_mut<Component::Rigidbody>(); rigidbody != nullptr)
						{
							rigidbody->GetBody().SetRotation(Math::Convert(DXM::Quaternion::CreateFromYawPitchRoll(pComp->x, pComp->y, pComp->z)), JPH::EActivation::Activate);
						}

						if (e.has<Component::Static>())
						{
							shouldUpdateStatics = true;
						}
					}
				}

				if (Component::Scale* pComp = e.try_get_mut<Component::Scale>(); pComp != nullptr)
				{
					ImGui::TextColored(
						ImVec4(0.2f, 0.6f, 1.0f, 1.0f),
						"Scale"
					);

					if (ImGui::DragFloat3("##Scale", &pComp->x, 0.1f))
					{
						if (e.has<Component::Static>())
						{
							shouldUpdateStatics = true;
						}
					}
				}
			}

			{
				if (Component::Mesh* pComp = e.try_get_mut<Component::Mesh>(); pComp != nullptr)
				{
					ImGui::SeparatorText("Mesh");

					Asset::Mesh* meshAsset = nullptr;

					{
						auto& assetContainer = m_diAssetManager->GetContainer<Asset::Mesh>();
						for (auto& asset : assetContainer)
						{
							if (asset.second->GetRenderRef().m_MeshletGlobalOffset == pComp->m_MeshID)
							{
								meshAsset = asset.second.get();
								break;
							}
						}

						if (!meshAsset) {
							meshAsset = m_diAssetManager->Get<Asset::Mesh>("Fallback");
						}
					}

					if (meshAsset)
					{
						ImGui::Text(("Mesh: " + meshAsset->GetCachedData().Name).c_str());
						ImGui::SameLine();

						if (ImGui::BeginCombo("##MeshSelected", meshAsset->GetCachedData().Name.c_str()))
						{
							std::string meshSelected = meshAsset->GetCachedData().Name;
							auto& assetContainer = m_diAssetManager->GetContainer<Asset::Mesh>();
							for (const auto& [name, asset] : assetContainer)
							{
								const bool is_selected = (meshSelected == name);
								if (ImGui::Selectable(name.c_str(), is_selected))
								{
									meshSelected = name;
									pComp->SetMesh(*asset, *m_diAssetManager->Get<Asset::Material>("Fallback"));
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

						ImGui::Text(("Submesh Count: " + std::to_string(pComp->m_NumSubmeshes)).c_str());

						if (pComp->m_NumSubmeshes > 0)
						{
							ImGui::SeparatorText("Submesh");

							ImGui::SetNextItemWidth(50.f);

							{
								std::vector<std::string> submeshOptions(pComp->m_NumSubmeshes); uint32_t counter = 0;
								std::for_each(submeshOptions.begin(), submeshOptions.end(), [&](std::string& x) { x = std::to_string(counter); counter++; });
								aZero::ImGui_Wrapper::ComboBox(m_SceneHierarchyEditing.SelectedSubmesh, "Submesh", submeshOptions, [](uint32_t index) {});
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

			{
				auto calcAttenuation = [](float x, float falloffStart, float falloffEnd) {
					return std::clamp((falloffEnd - x) / (falloffEnd - falloffStart), 0.f, 1.f);
				};

				if (Component::PointLight* pComp = e.try_get_mut<Component::PointLight>(); pComp != nullptr)
				{
					ImGui::SeparatorText("Point Light");
					ImGuiColorEditFlags flags = ImGuiColorEditFlags_::ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_::ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float | ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_DisplayHex;
					
					if (ImGui::ColorEdit3("Color##Point", (float*)&pComp->color, flags))
					{
						if (e.has<Component::Static>())
						{
							shouldUpdateStatics = true;
						}
					}
					if (ImGui::DragFloat("Intensity##Point", (float*)&pComp->intensity, 0.1f, 0.f, std::numeric_limits<float>::max()))
					{
						if (e.has<Component::Static>())
						{
							shouldUpdateStatics = true;
						}
					}
					if (ImGui::DragFloat("Falloff Start##Point", (float*)&pComp->falloffStart, 0.1f, 0.f, pComp->falloffEnd))
					{
						if (e.has<Component::Static>())
						{
							shouldUpdateStatics = true;
						}
					}
					if (ImGui::DragFloat("Falloff End##Point", (float*)&pComp->falloffEnd, 0.1f, pComp->falloffStart, std::numeric_limits<float>::max()))
					{
						if (e.has<Component::Static>())
						{
							shouldUpdateStatics = true;
						}
					}

					constexpr uint32_t sampleCount = 200u;
					std::vector<float> samples(sampleCount);
					float maxX = pComp->falloffEnd * 1.2f;

					for (uint32_t i = 0; i < samples.size(); i++)
					{
						float x = maxX * i / (sampleCount - 1u);
						samples[i] = calcAttenuation(
							x,
							pComp->falloffStart,
							pComp->falloffEnd
						);
					}
					ImGui::PlotLines("Falloff##Point", samples.data(), samples.size());
				}

				if (Component::SpotLight* pComp = e.try_get_mut<Component::SpotLight>(); pComp != nullptr)
				{
					ImGui::SeparatorText("Spot Light");
					ImGuiColorEditFlags flags = ImGuiColorEditFlags_::ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_::ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float | ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_DisplayHex;
					DXM::Vector3 temp = pComp->color;
					if (ImGui::ColorEdit3("Color##Spot", (float*)&pComp->color, flags))
					{
						if (e.has<Component::Static>())
						{
							shouldUpdateStatics = true;
						}
					}

					if (ImGui::DragFloat("Intensity##Spot", (float*)&pComp->intensity, 0.1f, 0.f, std::numeric_limits<float>::max()))
					{
						if (e.has<Component::Static>())
						{
							shouldUpdateStatics = true;
						}
					}

					if (ImGui::DragFloat("Power##Spot", (float*)&pComp->spotPower, 0.1f, 0.f, std::numeric_limits<float>::max()))
					{
						if (e.has<Component::Static>())
						{
							shouldUpdateStatics = true;
						}
					}

					if (ImGui::DragFloat("Falloff Start##Spot", (float*)&pComp->falloffStart, 0.1f, 0.f, pComp->falloffEnd))
					{
						if (e.has<Component::Static>())
						{
							shouldUpdateStatics = true;
						}
					}

					if (ImGui::DragFloat("Falloff End##Spot", (float*)&pComp->falloffEnd, 0.1f, pComp->falloffStart, std::numeric_limits<float>::max()))
					{
						if (e.has<Component::Static>())
						{
							shouldUpdateStatics = true;
						}
					}

					constexpr uint32_t sampleCount = 200u;
					std::vector<float> samples(sampleCount);
					float maxX = pComp->falloffEnd * 1.2f;

					for (uint32_t i = 0; i < samples.size(); i++)
					{
						float x = maxX * i / (sampleCount - 1u);
						samples[i] = calcAttenuation(
							x,
							pComp->falloffStart,
							pComp->falloffEnd
						);
					}
					ImGui::PlotLines("Falloff##Spot", samples.data(), samples.size());
				}

				if (Component::DirectionalLight* pComp = e.try_get_mut<Component::DirectionalLight>(); pComp != nullptr)
				{
					ImGui::SeparatorText("Directional Light");
					ImGuiColorEditFlags flags = ImGuiColorEditFlags_::ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_::ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float | ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_DisplayHex;
					DXM::Vector3 temp = pComp->color;
					if (ImGui::ColorEdit3("Color##Directional", (float*)&pComp->color, flags))
					{
						if (e.has<Component::Static>())
						{
							shouldUpdateStatics = true;
						}
					}
					if (ImGui::DragFloat("Intensity##Directional", (float*)&pComp->intensity, 0.1f, 0.f, std::numeric_limits<float>::max()))
					{
						if (e.has<Component::Static>())
						{
							shouldUpdateStatics = true;
						}
					}
				}
			}

			{
				if (Component::Camera* pComp = e.try_get_mut<Component::Camera>(); pComp != nullptr)
				{
					ImGui::SeparatorText("Camera##Camera");

					bool isOrtographics = Component::Camera::EProjectionType::Ortographic == pComp->ProjectionType;

					ImGui::BeginDisabled(isOrtographics);

					float fov = pComp->Fov;
					if (ImGui::DragFloat("Fov##Camera", &fov, 0.1f, 0.1f, 360.f))
					{
						if (!DirectX::XMScalarNearEqual(fov, 0.0f, 0.00001f * 2.0f)) // To ensure that we don't input invalid values
						{
							pComp->Fov = fov;
						}

						if (e.has<Component::Static>())
						{
							// Maybe do something in the future if static
						}
					}

					ImGui::EndDisabled();

					float nearPlane = pComp->Near;
					float farPlane = pComp->Far;
					if (ImGui::DragFloat("Near Plane##Camera", &nearPlane, 0.1f, 0.1f, farPlane))
					{
						if (!DirectX::XMScalarNearEqual(farPlane, nearPlane, 0.00001f))
						{
							pComp->Near = nearPlane;
						}

						if (e.has<Component::Static>())
						{

						}
					}

					if (ImGui::DragFloat("Far Plane##Camera", &farPlane, 0.1f, nearPlane + 0.1f, std::numeric_limits<float>::max()))
					{
						if (!DirectX::XMScalarNearEqual(farPlane, nearPlane, 0.00001f))
						{
							pComp->Far = farPlane;
						}

						if (e.has<Component::Static>())
						{

						}
					}

					if (ImGui::DragInt2("Top Left##Camera", &pComp->Viewport.TopX, 0.1f, 0.f, 8000.f))
					{
						if (e.has<Component::Static>())
						{

						}
					}

					int32_t viewportDims[2] = { pComp->Viewport.Width, pComp->Viewport.Height };
					if (ImGui::DragInt2("Width / Height##Camera", &viewportDims[0], 0.1f, 0.f, 8000.f))
					{
						if (!DirectX::XMScalarNearEqual(viewportDims[0] / viewportDims[1], 0.0f, 0.00001f)) // To ensure that we don't input invalid values
						{
							pComp->Viewport.Width = viewportDims[0];
							pComp->Viewport.Height = viewportDims[1];
						}

						if (e.has<Component::Static>())
						{

						}
					}

					if (ImGui::Checkbox("Active##Camera", &pComp->Active))
					{
						if (e.has<Component::Static>())
						{

						}
					}

					if (ImGui::Checkbox("Ortographic##Camera", &isOrtographics))
					{
						if (isOrtographics)
						{
							pComp->ProjectionType = Component::Camera::EProjectionType::Ortographic;
						}
						else
						{
							pComp->ProjectionType = Component::Camera::EProjectionType::Perspective;
						}

						if (e.has<Component::Static>())
						{

						}
					}

					if (ImGui::DragInt("Layer##Camera", &pComp->Layer, 1.f))
					{
						if (e.has<Component::Static>())
						{

						}
					}

					ImGui::Text("Render target format: %s", pComp->Rtv != nullptr ? aZero::RenderAPI::Format_To_String(aZero::RenderAPI::FromDX_Format(pComp->Rtv->GetTexture().GetResource()->GetDesc().Format)).c_str() : "None");
					ImGui::Text("Depth stencil target format: %s", pComp->Dsv != nullptr ? aZero::RenderAPI::Format_To_String(aZero::RenderAPI::FromDX_Format(pComp->Dsv->GetTexture().GetResource()->GetDesc().Format)).c_str() : "None");
				}
			}

			{
				if (Component::Rigidbody* pComp = e.try_get_mut<Component::Rigidbody>(); pComp != nullptr)
				{
					ImGui::SeparatorText("Rigidbody");

					// todo Impl

				}

				if (Component::Triggerbody* pComp = e.try_get_mut<Component::Triggerbody>(); pComp != nullptr)
				{
					ImGui::SeparatorText("Collider");

					// todo Impl
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

			ImGui::SeparatorText("Timing");

			ImGui::Text(("FPS: " + std::to_string(stats.FPS.Value)).c_str());
			ImGui::Text(("Scene Render (ms): " + std::to_string(stats.GetRenderStat(Statistics::ERenderStat::Scene))).c_str());
			ImGui::Text(("Scene Wireframe (ms): " + std::to_string(stats.GetRenderStat(Statistics::ERenderStat::Wireframe))).c_str());
			ImGui::Text(("Editor GUI (ms): " + std::to_string(stats.GetRenderStat(Statistics::ERenderStat::EditorGUI))).c_str());
			ImGui::Text(("Resolve SwapChain (ms): " + std::to_string(stats.GetRenderStat(Statistics::ERenderStat::ResolveSwapChain))).c_str());
			ImGui::Text(("Present (ms): " + std::to_string(stats.GetRenderStat(Statistics::ERenderStat::Present))).c_str());

			ImGui::SeparatorText("Memory");

			{
				MEMORYSTATUSEX memInfo;
				memInfo.dwLength = sizeof(MEMORYSTATUSEX);
				GlobalMemoryStatusEx(&memInfo);
				PROCESS_MEMORY_COUNTERS_EX pmc;
				GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
				double physMemUsedByProcess = pmc.WorkingSetSize / (1024.0 * 1024 * 1024);
				double availablePhysRamGB =
					(double)memInfo.ullAvailPhys / (1024.0 * 1024 * 1024) + physMemUsedByProcess;

				std::string buf = std::format("{:.3f}/{:.3f} (gb)",
					physMemUsedByProcess,
					availablePhysRamGB);

				ImGui::ProgressBar(physMemUsedByProcess / availablePhysRamGB, ImVec2(0.f, 0.f), buf.c_str());
				ImGui::SetItemTooltip("How much ram the process uses out of available ram.");
				ImGui::SameLine();
				ImGui::Text("Physical Ram");
			}

			{
				DXGI_QUERY_VIDEO_MEMORY_INFO info = {};

				HRESULT hr = m_Adapter->QueryVideoMemoryInfo(
					0,
					DXGI_MEMORY_SEGMENT_GROUP_LOCAL,
					&info);

				if (SUCCEEDED(hr))
				{
					double vramUsed = info.CurrentUsage / 1024.0 / 1024.0 / 1024.0;
					double totalVram = info.Budget / 1024.0 / 1024.0 / 1024.0;

					std::string buf = std::format("{:.3f}/{:.3f} (gb)",
						vramUsed,
						totalVram);

					ImGui::ProgressBar(vramUsed / totalVram, ImVec2(0.f, 0.f), buf.c_str());
					ImGui::SetItemTooltip("How much vram the process uses out of total vram.");
					ImGui::SameLine();
					ImGui::Text("Video Ram");
				}
			}

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

		void ShowMaterialEditor(Rendering::Renderer& renderer)
		{
			ImGui::Begin("Material Editor");

			{
				//auto& materials = m_diAssetManager->GetContainer<Asset::Material>();

				//if (ImGui::BeginListBox("Materials"))
				//{
				//	for (auto& [name, mat] : materials)
				//	{

				//	}
				//	for (int n = 0; n < IM_COUNTOF(items); n++)
				//	{
				//		const bool is_selected = (item_selected_idx == n);
				//		if (ImGui::Selectable(items[n], is_selected))
				//			item_selected_idx = n;

				//		if (item_highlight && ImGui::IsItemHovered())
				//			item_highlighted_idx = n;

				//		// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
				//		if (is_selected)
				//			ImGui::SetItemDefaultFocus();
				//	}
				//	ImGui::EndListBox();
				//}

				
			}

			if (!m_MaterialEditing.CurrentlyEditedMaterial.empty())
			{
				Asset::Material* m = m_diAssetManager->Get<Asset::Material>(m_MaterialEditing.CurrentlyEditedMaterial);
				if (m)
				{
					bool updateMaterial = false;
					auto& textureContainer = m_diAssetManager->GetContainer<Asset::Texture>();

					ImGui::Text(("Name: " + m->GetCachedData().Name).c_str());

					if (!m->GetAlbedoPtr())
					{
						m->SetAlbedo(m_diAssetManager->Get<Asset::Texture>("Fallback"));
						updateMaterial = true;
					}

					if (ImGui::BeginCombo("Albedo", m->GetAlbedoPtr()->GetCachedData().Name.c_str()))
					{
						for (auto& [name, tex] : textureContainer)
						{
							const bool is_selected = m_MaterialEditing.SelectedAlbedo == name;
							if (ImGui::Selectable(name.c_str(), is_selected)) {
								m_MaterialEditing.SelectedAlbedo = name;
								m->SetAlbedo(tex.get());
								updateMaterial = true;
								break;
							}

							// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
							if (is_selected) {
								ImGui::SetItemDefaultFocus();
							}
						}
						ImGui::EndCombo();
					}

					if (m->GetNormalMapPtr())
					{
						if (ImGui::BeginCombo("Normal Map", m->GetNormalMapPtr()->GetCachedData().Name.c_str()))
						{
							for (auto& [name, tex] : textureContainer)
							{
								const bool is_selected = m_MaterialEditing.SelectedNormalMap == name;
								if (ImGui::Selectable(name.c_str(), is_selected)) {
									m_MaterialEditing.SelectedNormalMap = name;
									m->SetNormalMap(tex.get());
									updateMaterial = true;
									break;
								}

								// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
								if (is_selected) {
									ImGui::SetItemDefaultFocus();
								}
							}
							ImGui::EndCombo();
						}

						ImGui::SameLine();
						if (ImGui::Button("Clear")) 
						{
							m->SetNormalMap(nullptr);
							updateMaterial = true;
						}
					}
					else
					{
						if (ImGui::Button("Add normal map"))
						{
							m->SetNormalMap(m_diAssetManager->Get<Asset::Texture>("FallbackNormalMap"));
							updateMaterial = true;
						}
					}
					
					if (m->GetMetallicRoughnessTexturePtr())
					{
						if (ImGui::BeginCombo("Metallic/Roughness Map", m->GetMetallicRoughnessTexturePtr()->GetCachedData().Name.c_str()))
						{
							for (auto& [name, tex] : textureContainer)
							{
								const bool is_selected = m_MaterialEditing.SelectedMetallicRoughnessMap == name;
								if (ImGui::Selectable(name.c_str(), is_selected)) {
									m_MaterialEditing.SelectedMetallicRoughnessMap = name;
									m->SetMetallicRoughnessTexture(tex.get());
									updateMaterial = true;
									break;
								}

								// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
								if (is_selected) {
									ImGui::SetItemDefaultFocus();
								}
							}
							ImGui::EndCombo();
						}

						ImGui::SameLine();
						if (ImGui::Button("Clear"))
						{
							m->SetMetallicRoughnessTexture(nullptr);
							updateMaterial = true;
						}
					}
					else
					{
						float metallicFactor = m->GetCachedData().Info.MetallicFactor;
						if (ImGui::SliderFloat("Metallic Factor", &metallicFactor, 0.f, 1.f))
						{
							m->SetMetallicFactor(metallicFactor);
							updateMaterial = true;
						}

						float roughnessFactor = m->GetCachedData().Info.RoughnessFactor;
						if (ImGui::SliderFloat("Roughness Factor", &roughnessFactor, 0.f, 1.f))
						{
							m->SetRoughnessFactor(roughnessFactor);
							updateMaterial = true;
						}

						if (ImGui::Button("Add metallic/roughness map"))
						{
							m->SetMetallicRoughnessTexture(m_diAssetManager->Get<Asset::Texture>("FallbackMetallicRoughnessMap"));
							updateMaterial = true;
						}
					}

					if (updateMaterial)
					{
						renderer.RegisterOrUpdateAsset(*m);
					}
				}
			}
			ImGui::End();
		}

		bool m_EnableFlecsExplorer = false;
		bool m_ShowRigidbodyColliders = false;
		bool m_ShowTriggerbodyColliders = false;
		bool m_ShowMeshBounds = false;
		bool m_ShowGrid = true;
		bool m_PhysicsCamera = false;
		bool m_Show_demo_window = false;

		ULARGE_INTEGER lastCPU, lastSysCPU, lastUserCPU;
		int numProcessors;
		HANDLE self;
		Microsoft::WRL::ComPtr<IDXGIAdapter3> m_Adapter;

		Rendering::WireframeRenderer* m_diWireframeRenderer;
		Asset::AssetManager<std::string>* m_diAssetManager;
		SceneHierarchyEditing m_SceneHierarchyEditing;
		MaterialEditing m_MaterialEditing;
		DialogueSettings m_DialogueSettings;
	};
}