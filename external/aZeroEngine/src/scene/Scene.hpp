#pragma once
#include <string>
#include <span>
#include <fstream>

#include "ecs/aZeroECS.hpp"
#include "graphics_api/resource_type/LinearBuffer.hpp"
#include "misc/SparseMappedVector.hpp"
#include "misc/Yaml_Operator_Overloads.hpp"

namespace aZero
{
	namespace Scene
	{
		class SceneManager;
		class Scene;

		using SceneID = uint32_t;

		class SceneProxy
		{
		public:
			friend Scene;
			struct StaticMesh
			{
				DXM::Matrix m_Transform;
				uint32_t m_MeshIndex;
				uint32_t m_MaterialIndex;
				DirectX::BoundingSphere m_BoundingSphere;
				uint32_t m_NumVertices;
			};

			struct Camera
			{
				DirectX::BoundingFrustum m_Frustrum;
				DXM::Matrix m_ViewProjectionMatrix;
				D3D12_VIEWPORT m_Viewport;
				D3D12_RECT m_ScizzorRect;
				bool m_IsActive;
			};

			struct DirectionalLight
			{
				DXM::Vector3 m_Direction;
				DXM::Vector3 m_Color;
				float m_Intensity;
			};

			struct PointLight
			{
				DXM::Vector3 m_Position;
				DXM::Vector3 m_Color;
				float m_FalloffFactor;
				float m_Intensity;
			};

			struct SpotLight
			{
				DXM::Vector3 m_Direction;
				DXM::Vector3 m_Position;
				DXM::Vector3 m_Color;
				float m_ConeRadius;
				float m_Range;
				float m_Intensity;
			};

		private:
		public:
			/*
			NOTE:
				If we move to a fully gpu-driven rendering pipeline this data need to have a mirrored version in vram.
				We can probably skip having mirrored versions of the cameras since we probably can upload them per-frame.
				In best case we can simply change the logic of adding a new entry in the SparseMappedVector to include gpu-upload of the data too.
					Then, instead of the renderer owning the instance buffers etc, we can have the renderer fetch the buffers from the scene proxy and set them.
			*/
			DataStructures::SparseMappedVector<ECS::EntityID, StaticMesh> m_StaticMeshes;
			DataStructures::SparseMappedVector<ECS::EntityID, Camera> m_Cameras;
			DataStructures::SparseMappedVector<ECS::EntityID, DirectionalLight> m_DirectionalLights;
			DataStructures::SparseMappedVector<ECS::EntityID, PointLight> m_PointLights;
			DataStructures::SparseMappedVector<ECS::EntityID, SpotLight> m_SpotLights;

			SceneProxy() = default;

			void Init()
			{

			}
		};

		class SceneSerializer;

		class Scene
		{
		private:
			friend SceneSerializer;
			friend SceneManager;

			SceneID m_ID;
			SceneProxy m_Proxy;
			std::string m_Name;
			std::unordered_map<std::string, ECS::Entity> m_Entities;
			std::unordered_map<ECS::EntityID, std::string> m_Entity_To_Name;
			ECS::EntityManager m_EntityManager;
			ECS::ComponentManagerDecl m_ComponentManager;

			std::string GenerateEntityName()
			{
				static uint32_t nameDummy = 0;
				return "Entity_" + std::to_string(nameDummy++);
			}

			bool IsNameTaken(const std::string& Name)
			{
				return m_Entities.count(Name) == 1;
			}

			Scene() = default;

			Scene(SceneID ID, const std::string& Name)
			{
				m_ID = ID;
				m_Name = Name;

				constexpr uint32_t MaxEntities = 1000;
				m_ComponentManager.GetComponentArray<ECS::TransformComponent>().Init(MaxEntities);
				m_ComponentManager.GetComponentArray<ECS::StaticMeshComponent>().Init(MaxEntities);
				m_ComponentManager.GetComponentArray<ECS::DirectionalLightComponent>().Init(MaxEntities);
				m_ComponentManager.GetComponentArray<ECS::PointLightComponent>().Init(MaxEntities);
				m_ComponentManager.GetComponentArray<ECS::SpotLightComponent>().Init(MaxEntities);
				m_ComponentManager.GetComponentArray<ECS::CameraComponent>().Init(MaxEntities);
			}
		public:
			const SceneProxy& GetProxy() const { return m_Proxy; }

			void RenameEntity(ECS::Entity entity, const std::string& newName)
			{
				std::string entityName = Helper::HandleNameCollision(newName, m_Entities);
				m_Entities[entityName] = entity;
				m_Entities.erase(m_Entity_To_Name.at(entity.GetID()));
				m_Entity_To_Name[entity.GetID()] = entityName;
			}

			ECS::Entity CreateEntity()
			{
				ECS::Entity entity = m_EntityManager.CreateEntity();
				std::string newName = Helper::HandleNameCollision(this->GenerateEntityName(), m_Entities);
				m_Entities[newName] = entity;
				m_Entity_To_Name[entity.GetID()] = newName;
				m_ComponentManager.AddComponent(entity, ECS::TransformComponent());
				return entity;
			}

			void DeleteEntity(const std::string& Name)
			{
				if (m_Entities.count(Name))
				{
					this->DeleteEntity(m_Entities.at(Name));
				}
			}

			void DeleteEntity(ECS::Entity entity)
			{
				if (m_Entity_To_Name.count(entity.GetID()))
				{
					std::apply([&entity](auto& ... args)
						{
							(args.RemoveComponent(entity), ...);
						}, m_ComponentManager.GetComponentTuple()
							);

					this->UpdateRenderState(entity);

					m_Entities.erase(m_Entity_To_Name.at(entity.GetID()));
					m_Entity_To_Name.erase(entity.GetID());
				}
			}

			void UpdateRenderState(ECS::Entity entity)
			{
				ECS::EntityID id = entity.GetID();
				if (ECS::TransformComponent* tf = m_ComponentManager.GetComponent<ECS::TransformComponent>(entity))
				{
					ECS::StaticMeshComponent* mesh = m_ComponentManager.GetComponent<ECS::StaticMeshComponent>(entity);

					auto meshRef = mesh->m_MeshReference;
					auto materialRef = mesh->m_MaterialReference;

					if (meshRef.IsValid() && materialRef.IsValid()
						&& meshRef.GetAsset()->GetRenderID() != std::numeric_limits<Asset::RenderID>::max()
						&& materialRef.GetAsset()->GetRenderID() != std::numeric_limits<Asset::RenderID>::max())
					{
						auto meshAsset = meshRef.GetAsset();
						auto materialAsset = materialRef.GetAsset();

						if (meshAsset->GetRenderID() != std::numeric_limits<Asset::RenderID>::max()
							&& materialAsset->GetRenderID() != std::numeric_limits<Asset::RenderID>::max())
						{
							const DXM::Matrix tfMatrix = tf->GetTransform();

							SceneProxy::StaticMesh meshProxy;
							meshProxy.m_NumVertices = meshAsset->m_VertexData.Indices.size();
							meshProxy.m_Transform = tfMatrix;

							const float XAxisScale = tfMatrix.m[0][0];
							const float YAxisScale = tfMatrix.m[1][1];
							const float ZAxisScale = tfMatrix.m[2][2];
							const float MaxScaleAxis = std::max(abs(XAxisScale), std::max(abs(YAxisScale), abs(ZAxisScale)));
							meshProxy.m_BoundingSphere = DirectX::BoundingSphere(
								{ tfMatrix.Translation().x, tfMatrix.Translation().y, tfMatrix.Translation().z },
								meshAsset->m_BoundingSphereRadius * MaxScaleAxis
							);
							meshProxy.m_MeshIndex = meshAsset->GetRenderID();
							meshProxy.m_MaterialIndex = materialAsset->GetRenderID();

							// TODO: Maybe have a separate set of meshes for all of them with transparency enabled (similar to how skeletal meshes etc might work)?
							// I think the component should control the transparency setting
							//		This is because it's gonna be a problem if the material changes but the mesh is in the array of non-transparent meshes...
							//			How do we structure it????

							m_Proxy.m_StaticMeshes.AddOrUpdate(id, std::move(meshProxy));
						}
						else
						{
							m_Proxy.m_StaticMeshes.Remove(id);
						}
					}
					else
					{
						m_Proxy.m_StaticMeshes.Remove(id);
					}

					if (ECS::CameraComponent* camera = m_ComponentManager.GetComponent<ECS::CameraComponent>(entity))
					{
						if (camera->m_IsActive)
						{
							SceneProxy::Camera cameraProxy;
							cameraProxy.m_Frustrum = DirectX::BoundingFrustum(camera->GetProjectionMatrix(), true);
							cameraProxy.m_ViewProjectionMatrix = camera->GetViewMatrix() * camera->GetProjectionMatrix();
							cameraProxy.m_Viewport = camera->GetViewport();
							cameraProxy.m_ScizzorRect = camera->GetScizzorRect();
							m_Proxy.m_Cameras.AddOrUpdate(id, std::move(cameraProxy));
						}
						else
						{
							m_Proxy.m_Cameras.Remove(id);
						}
					}
					else
					{
						m_Proxy.m_Cameras.Remove(id);
					}

					if (ECS::DirectionalLightComponent* dl = m_ComponentManager.GetComponent<ECS::DirectionalLightComponent>(entity))
					{
						SceneProxy::DirectionalLight dlProxy;
						dlProxy.m_Direction = dl->GetData().Direction;
						dlProxy.m_Color = dl->GetData().Color;
						dlProxy.m_Intensity = dl->GetData().Intensity;
						m_Proxy.m_DirectionalLights.AddOrUpdate(id, std::move(dlProxy));
					}
					else
					{
						m_Proxy.m_DirectionalLights.Remove(id);
					}

					if(ECS::PointLightComponent* pl = m_ComponentManager.GetComponent<ECS::PointLightComponent>(entity))
					{
						SceneProxy::PointLight plProxy;
						plProxy.m_Position = pl->GetData().Position;
						plProxy.m_Color = pl->GetData().Color;
						plProxy.m_Intensity = pl->GetData().Intensity;
						plProxy.m_FalloffFactor = pl->GetData().FalloffFactor;
						m_Proxy.m_PointLights.AddOrUpdate(id, std::move(plProxy));
					}
					else
					{
						m_Proxy.m_PointLights.Remove(id);
					}

					if (ECS::SpotLightComponent* sl = m_ComponentManager.GetComponent<ECS::SpotLightComponent>(entity))
					{
						SceneProxy::SpotLight slProxy;
						slProxy.m_Direction = sl->GetData().Direction;
						slProxy.m_Color = sl->GetData().Color;
						slProxy.m_Intensity = sl->GetData().Intensity;
						slProxy.m_ConeRadius = sl->GetData().CutoffAngle;
						slProxy.m_Position = sl->GetData().Position;
						slProxy.m_Range = sl->GetData().Range;
						m_Proxy.m_SpotLights.AddOrUpdate(id, std::move(slProxy));
					}
					else
					{
						m_Proxy.m_SpotLights.Remove(id);
					}
				}
				else
				{
					m_Proxy.m_StaticMeshes.Remove(id);
					m_Proxy.m_Cameras.Remove(id);
					m_Proxy.m_DirectionalLights.Remove(id);
					m_Proxy.m_PointLights.Remove(id);
					m_Proxy.m_SpotLights.Remove(id);
				}
			}

			std::optional<ECS::Entity> GetEntity(const std::string& Tag) 
			{
				return m_Entities.count(Tag) == 1 ? m_Entities.at(Tag) : std::optional<ECS::Entity>{};
			}

			ECS::ComponentManagerDecl& GetComponentManager() { return m_ComponentManager; }
		};


		class SceneSerializer
		{
		private:
			static void SerializeEntity( Scene& scene, YAML::Emitter& out, const std::string& name, ECS::Entity entity)
			{
				out << YAML::BeginMap;
				out << YAML::Key << "Name" << YAML::Value << name;

				// Serialize each component
				if ( ECS::TransformComponent* const tf = scene.m_ComponentManager.GetComponent<ECS::TransformComponent>(entity))
				{
					out << YAML::Key << "TransformComponent";

					out << YAML::BeginMap;
					out << YAML::Key << "Translation" << YAML::Value << tf->GetTransform().Translation();
					out << YAML::Key << "Rotation" << YAML::Value << tf->GetTransform().ToEuler();
					out << YAML::Key << "Scale" << YAML::Value << DXM::Vector3(tf->GetTransform().m[0][0], tf->GetTransform().m[1][1], tf->GetTransform().m[2][2]);
					out << YAML::EndMap;

					if ( ECS::StaticMeshComponent* mesh = scene.m_ComponentManager.GetComponent<ECS::StaticMeshComponent>(entity))
					{
						out << YAML::Key << "StaticMeshComponent";
						out << YAML::BeginMap;
						out << YAML::Key << "StaticMeshInfo" << YAML::Value << "TODO: Add mesh and material information";
						out << YAML::EndMap;
					}

					if ( ECS::CameraComponent* camera = scene.m_ComponentManager.GetComponent<ECS::CameraComponent>(entity))
					{
						out << YAML::Key << "CameraComponent";
						out << YAML::BeginMap;
						out << YAML::Key << "Info" << YAML::Value << "TODO: Add info";
						out << YAML::EndMap;
					}

					if ( ECS::DirectionalLightComponent* dl = scene.m_ComponentManager.GetComponent<ECS::DirectionalLightComponent>(entity))
					{
						out << YAML::Key << "DirectionalLightComponent";
						out << YAML::BeginMap;
						out << YAML::Key << "Info" << YAML::Value << "TODO: Add info";
						out << YAML::EndMap;
					}

					if ( ECS::PointLightComponent* pl = scene.m_ComponentManager.GetComponent<ECS::PointLightComponent>(entity))
					{
						out << YAML::Key << "PointLightComponent";
						out << YAML::BeginMap;
						out << YAML::Key << "Info" << YAML::Value << "TODO: Add info";
						out << YAML::EndMap;
					}

					if ( ECS::SpotLightComponent* sl = scene.m_ComponentManager.GetComponent<ECS::SpotLightComponent>(entity))
					{
						out << YAML::Key << "SpotLightComponent";
						out << YAML::BeginMap;
						out << YAML::Key << "Info" << YAML::Value << "TODO: Add info";
						out << YAML::EndMap;
					}
				}

				out << YAML::EndMap;
			}

		public:
			static bool Serialize( Scene& scene, const std::string& path)
			{
				std::ofstream outFile(path);
				if (outFile.is_open())
				{
					// Early-out if file with name already exists

					// Serialize...
					YAML::Emitter out;

					out << YAML::BeginMap;

					out << YAML::Key << "Scene";
					out << YAML::Value << scene.m_Name;

					out << YAML::Key << "Entities";
					out << YAML::Value << YAML::BeginSeq;
					for (const auto& [name, entity] : scene.m_Entities)
					{
						SerializeEntity(scene, out, name, entity);
					}
					out << YAML::EndSeq;

					outFile << out.c_str();
					outFile.close();
				}

				return true;
			}

			static std::optional<Scene> Deserialize(const std::string& Path)
			{
				Scene newScene;
				// Serialize...

				//
				return newScene;
			}
		};
	}
}