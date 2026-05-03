#pragma once
#include "aZeroEngine/Engine.hpp"
#include "aZeroInput.hpp"
#include "ImguiInclude.hpp"

namespace aZero::Editor::GUI
{
	class EditorGUI
	{
	public:
		EditorGUI(Input::DeviceManager& deviceManager,
			// Dependency injections
			Rendering::WireframeRenderer& wireframeRenderer
		)
			:m_diWireframeRenderer(&wireframeRenderer)
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
			if (m_ShowEditorGUI)
			{
				// Debug settings
				ImGui::Begin("Debug");

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
				scene.AddDebugDrawArguments(*m_diWireframeRenderer, m_ShowColliders, m_ShowMeshBounds);
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
			RenderAPI::CommandList& cmdList = renderer.GetCurrentContext().m_DirectCmdList;
			auto rtvHandle = rtv.GetCpuHandle();

			ImGui::Render();

			std::array<ID3D12DescriptorHeap*, 1> heaps{ renderer.GetResourceHeap().Get() };
			cmdList->SetDescriptorHeaps(heaps.size(), &heaps[0]);
			cmdList->OMSetRenderTargets(1, &rtvHandle, false, nullptr);

			ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), renderer.GetCurrentContext().m_DirectCmdList.Get());

			renderer.GetGraphicsCommandQueue().ExecuteCommandList(renderer.GetCurrentContext().m_DirectCmdList, true);

			aZero::ImGui_Wrapper::HandleMultiViewport();
		}

	private:
		bool m_ShowEditorGUI = true;
		bool m_ShowColliders = false;
		bool m_ShowMeshBounds = false;
		bool m_ShowGrid = true;
		bool m_Show_demo_window = false;
		Input::KeyboardListener m_KeyboardListener;
		Rendering::WireframeRenderer* m_diWireframeRenderer;
	};
}