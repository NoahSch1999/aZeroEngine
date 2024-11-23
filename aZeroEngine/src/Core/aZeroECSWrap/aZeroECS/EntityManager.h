#pragma once
#include <queue>
#include "Entity.h"

namespace aZero
{
	namespace ECS
	{
		/** @brief A class used to generate unique Entity objects
		*/
		class EntityManager
		{
		private:
			unsigned int m_CurrentMax = 0;
			std::queue<ECS::EntityID> m_FreeEntityIDs;

		public:
			EntityManager() = default;
			~EntityManager() = default;

			EntityManager(const EntityManager&) = delete;
			EntityManager operator=(const EntityManager&) = delete;
			EntityManager(EntityManager&& Other) = delete;
			EntityManager& operator=(EntityManager&& Other) = delete;

			/** Creates a new Entity object with a unique ID.
			@return Entity
			*/
			Entity CreateEntity()
			{
				Entity NewEntity;

				if (!m_FreeEntityIDs.empty())
				{
					NewEntity.m_ID = m_FreeEntityIDs.front();
					m_FreeEntityIDs.pop();
				}
				else
				{
					NewEntity.m_ID = m_CurrentMax++;
				}

				return NewEntity;
			}

			/** Recycles the input Entity ID so that it can be reused.
			* Invalidates the input Entity for future use.
			@param Ent The Entity which should be recycled
			@return void
			*/
			void RemoveEntity(Entity& Ent)
			{
				if (Ent.m_ID == UINT_MAX)
					return;

				m_FreeEntityIDs.push(Ent.m_ID);
				Ent.m_ID = UINT_MAX;
			}
		};
	}
}