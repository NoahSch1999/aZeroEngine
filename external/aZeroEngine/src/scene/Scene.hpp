#pragma once
#include "SceneProxy.hpp"
#include "misc/HelperFunctions.hpp"

namespace aZero
{
	namespace Scene
	{
		class SceneManager;

		using SceneID = uint32_t;
		constexpr SceneID InvalidSceneID = std::numeric_limits<SceneID>::max();

		class Scene
		{
		public:
			Scene() = default;
			Scene(SceneID ID, const std::string& name);

			void RenameEntity(ECS::Entity entity, const std::string& newName);

			ECS::Entity CreateEntity();

			void DeleteEntity(const std::string& name);

			void DeleteEntity(ECS::Entity entity);

			void UpdateRenderState(ECS::Entity entity);

			std::optional<ECS::Entity> GetEntity(const std::string& name) { return m_Entities.count(name) == 1 ? m_Entities.at(name) : std::optional<ECS::Entity>{}; }
			const std::unordered_map<std::string, ECS::Entity>& GetEntities() const { return m_Entities; }
			
			// TODO: Change how a scene's entities are modified
			ECS::ComponentManagerDecl& GetComponentManager() { return m_ComponentManager; }
			const ECS::ComponentManagerDecl& GetComponentManager() const { return m_ComponentManager; }

		private:
			friend SceneManager;

			SceneID m_ID = InvalidSceneID;
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
		};
	}
}