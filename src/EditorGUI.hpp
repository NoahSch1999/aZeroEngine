#pragma once
#include "Engine.hpp"
#include <aZeroInput.hpp>
#include "ImguiInclude.hpp"
#include <chrono>

namespace aZero::Editor::GUI
{
	class EditorGUI
	{
	public:
		EditorGUI() = default;
		EditorGUI(Input::DeviceManager& deviceManager,
			Rendering::WireframeRenderer& wireframeRenderer
		)
			:m_diWireframeRenderer(&wireframeRenderer) { }

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

				if (ImGui::Checkbox("Enable physics camera", &m_PhysicsCamera))
				{
					flecs::entity ent = scene.GetEntityWorld().lookup("EditorCamera");
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
							std::cout << "Camera collided with body: " << scene.GetEntityWorld().entity(scene.GetBodyID_To_EntityID_Map().at(body.GetBodyID().GetIndexAndSequenceNumber())).name() << "\n";
						});
					}
					else
					{
						ent.remove<Component::Rigidbody>();
					}
				}

				m_EnableFlecsExplorer = scene.GetEntityWorld().has<flecs::Rest>();
				if (ImGui::Checkbox("Enable flecs explorer", &m_EnableFlecsExplorer)) {
					if (m_EnableFlecsExplorer) {
						// on
						scene.GetEntityWorld().set<flecs::Rest>({});
					}
					else {
						// off
						scene.GetEntityWorld().remove<flecs::Rest>();
					}
				}

				ImGui::Checkbox("Show rigidbody colliders", &m_ShowRigidbodyColliders);
				ImGui::Checkbox("Show triggerbody colliders", &m_ShowTriggerbodyColliders);
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
	};
}