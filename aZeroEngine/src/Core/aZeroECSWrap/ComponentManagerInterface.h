#pragma once
#include "Core/Renderer/Renderer.h"

namespace aZero
{
	class Engine;

	class ComponentManagerInterface
	{
		friend Engine;
	private:
		ECS::ComponentManagerDecl* m_ComponentManager = nullptr;

		void Init(ECS::ComponentManagerDecl& InComponentManager)
		{
			m_ComponentManager = &InComponentManager;
		}

	public:
		ComponentManagerInterface() = default;

		template<typename ComponentType>
		void AddComponent(ECS::Entity& Ent, const ComponentType& Component)
		{
			m_ComponentManager->AddComponent(Ent, Component);
			aZero::gRenderer.RegisterRenderComponent<ComponentType>(Ent);
		}

		template<typename ComponentType>
		void AddComponent(ECS::Entity& Ent, ComponentType&& Component)
		{
			m_ComponentManager->AddComponent(Ent, std::forward<ComponentType>(Component));
			aZero::gRenderer.RegisterRenderComponent<ComponentType>(Ent);
		}

		template<typename ComponentType>
		void RemoveComponent(ECS::Entity& Ent)
		{
			m_ComponentManager->RemoveComponent<ComponentType>(Ent);
			aZero::gRenderer.UnregisterRenderComponent<ComponentType>(Ent);
		}

		template<typename ComponentType>
		void UpdateRenderComponent(const ECS::Entity& Ent)
		{
			if (m_ComponentManager->HasComponent<ComponentType>(Ent))
			{
				const ComponentType& Component = *m_ComponentManager->GetComponentArray<ComponentType>().GetComponent(Ent);
				aZero::gRenderer.UpdateRenderComponent(Ent, Component);
			}
		}

		template<typename ComponentType>
		ComponentType& GetComponent(const ECS::Entity& Ent)
		{
			return *m_ComponentManager->GetComponentArray<ComponentType>().GetComponent(Ent);
		}

		template<typename ComponentType>
		ECS::ComponentArray<ComponentType>& GetComponentArray()
		{
			return m_ComponentManager->GetComponentArray<ComponentType>();
		}

		template<typename ComponentType>
		const ECS::ComponentArray<ComponentType>& GetComponentArray() const
		{
			return m_ComponentManager->GetComponentArray<ComponentType>();
		}

		template<typename ComponentType>
		bool HasComponent(const ECS::Entity& Ent) const
		{
			return m_ComponentManager->HasComponent<ComponentType>(Ent);
		}
	};

	extern ComponentManagerInterface gComponentManager;
}