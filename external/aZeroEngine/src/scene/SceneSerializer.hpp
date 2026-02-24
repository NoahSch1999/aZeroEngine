#pragma once
#include <fstream>

#include "misc/Yaml_Operator_Overloads.hpp"
#include "Scene.hpp"

namespace aZero
{
	namespace Scene
	{
		class SceneSerializer
		{
		private:
			static void SerializeEntity(const ECS::ComponentManagerDecl& componentManager, YAML::Emitter& out, const std::string& name, ECS::Entity entity)
			{
				out << YAML::BeginMap;
				out << YAML::Key << "Name" << YAML::Value << name;

				// Serialize each component
				if (const ECS::TransformComponent* tf = componentManager.GetComponent<ECS::TransformComponent>(entity))
				{
					out << YAML::Key << "TransformComponent";

					out << YAML::BeginMap;
					out << YAML::Key << "Translation" << YAML::Value << tf->GetTransform().Translation();
					out << YAML::Key << "Rotation" << YAML::Value << tf->GetTransform().ToEuler();
					out << YAML::Key << "Scale" << YAML::Value << DXM::Vector3(tf->GetTransform().m[0][0], tf->GetTransform().m[1][1], tf->GetTransform().m[2][2]);
					out << YAML::EndMap;

					if (const ECS::StaticMeshComponent* mesh = componentManager.GetComponent<ECS::StaticMeshComponent>(entity))
					{
						out << YAML::Key << "StaticMeshComponent";
						out << YAML::BeginMap;
						out << YAML::Key << "StaticMeshInfo" << YAML::Value << "todo Add mesh and material information";
						out << YAML::EndMap;
					}

					if (const ECS::CameraComponent* camera = componentManager.GetComponent<ECS::CameraComponent>(entity))
					{
						out << YAML::Key << "CameraComponent";
						out << YAML::BeginMap;
						out << YAML::Key << "Info" << YAML::Value << "todo Add info";
						out << YAML::EndMap;
					}

					if (const ECS::DirectionalLightComponent* dl = componentManager.GetComponent<ECS::DirectionalLightComponent>(entity))
					{
						out << YAML::Key << "DirectionalLightComponent";
						out << YAML::BeginMap;
						out << YAML::Key << "Info" << YAML::Value << "todo Add info";
						out << YAML::EndMap;
					}

					if (const ECS::PointLightComponent* pl = componentManager.GetComponent<ECS::PointLightComponent>(entity))
					{
						out << YAML::Key << "PointLightComponent";
						out << YAML::BeginMap;
						out << YAML::Key << "Info" << YAML::Value << "todo Add info";
						out << YAML::EndMap;
					}

					if (const ECS::SpotLightComponent* sl = componentManager.GetComponent<ECS::SpotLightComponent>(entity))
					{
						out << YAML::Key << "SpotLightComponent";
						out << YAML::BeginMap;
						out << YAML::Key << "Info" << YAML::Value << "todo Add info";
						out << YAML::EndMap;
					}
				}

				out << YAML::EndMap;
			}

		public:
			static bool Serialize(const Scene& scene, const std::string& path)
			{
				std::ofstream outFile(path);
				if (outFile.is_open())
				{
					// Early-out if file with name already exists

					// Serialize...
					ECS::ComponentManagerDecl& componentManager = scene.GetComponentManager();
					auto entities = scene.GetEntities();
					YAML::Emitter out;

					out << YAML::BeginMap;

					out << YAML::Key << "Scene";
					out << YAML::Value << scene.m_Name;

					out << YAML::Key << "Entities";
					out << YAML::Value << YAML::BeginSeq;
					for (const auto& [name, entity] : entities)
					{
						SerializeEntity(componentManager, out, name, entity);
					}
					out << YAML::EndSeq;

					outFile << out.c_str();
					outFile.close();
				}

				return true;
			}

			static std::optional<Scene> Deserialize(const std::string& path)
			{
				Scene newScene;
				// Serialize...

				//
				return newScene;
			}
		};
	}
}