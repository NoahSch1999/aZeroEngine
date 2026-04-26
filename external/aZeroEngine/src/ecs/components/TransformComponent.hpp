#pragma once
#include "graphics_api/D3D12Include.hpp"

namespace aZero
{
	namespace Scene
	{
		class SceneNew;
	}

	namespace ECS
	{
		class TransformComponent
		{
		private:
			DXM::Matrix m_Transform = DXM::Matrix::Identity;

			friend class Scene::SceneNew;

			Entity m_AttachedEntityID;
			Entity m_ParentID = Entity();
			std::vector<Entity> m_ChildrenIDs;

			// NOTE: Assumes that the previous parent has has ::RemoveChild() called with this component
			void SetParent(TransformComponent& parent)
			{
				if (parent.m_AttachedEntityID.GetID() == InvalidEntityID)
				{
					m_ParentID = Entity();
					return;
				}

				if (m_AttachedEntityID.GetID() != parent.m_AttachedEntityID.GetID())
				{
					parent.AddChild(*this);
					m_ParentID = parent.m_AttachedEntityID;
				}
			}

			void AddChild(TransformComponent& child)
			{
				for (auto& childID : m_ChildrenIDs)
				{
					if (child.m_AttachedEntityID.GetID() == childID.GetID())
					{
						return;
					}
				}
				child.m_ParentID = m_AttachedEntityID;
				m_ChildrenIDs.emplace_back(child.m_AttachedEntityID);
			}

			void RemoveChild(TransformComponent& child)
			{
				for (auto i = m_ChildrenIDs.begin(); i != m_ChildrenIDs.end(); ++i)
				{
					if (i->GetID() == child.m_AttachedEntityID.GetID())
					{
						TransformComponent nullComponent;
						child.SetParent(nullComponent);
						m_ChildrenIDs.erase(i);
						return;
					}
				}
			}

		public:
			TransformComponent() = default;
			TransformComponent(Entity attachedEntityID)
				:m_AttachedEntityID(attachedEntityID)
			{

			}
			TransformComponent(const DXM::Matrix& Transform, Entity attachedEntityID)
				:m_AttachedEntityID(attachedEntityID)
			{
				this->SetTransform(Transform);
			}

			void SetTransform(const DXM::Matrix& Transform) 
			{ 
				m_Transform = Transform; 
			}

			const DXM::Matrix& GetTransform() const 
			{ 
				return m_Transform; 
			}

			Entity GetAttachedEntity() const { return m_AttachedEntityID; }
			Entity GetParent() const { return m_ParentID; }
			const std::vector<Entity>& GetChildren() const { return m_ChildrenIDs; }
		};
	}
}