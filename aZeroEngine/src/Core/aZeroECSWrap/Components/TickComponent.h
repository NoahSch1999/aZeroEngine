#pragma once
#include "TransformComponent.h"
#include <random>

namespace aZero
{
	namespace ECS
	{
		class TickComponent_Interface_Class
		{
		private:
			virtual void OnStart_Override() = 0;
			virtual void OnEnd_Override() = 0;

			virtual void PrePhysicsUpdate_Override() = 0;
			virtual void PostPhysicsUpdate_Override() = 0;

		public:
			TickComponent_Interface_Class() = default;

			void OnStart()
			{
				OnStart_Override();
			}

			void OnEnd()
			{
				OnEnd_Override();
			}

			void PrePhysicsUpdate()
			{
				PrePhysicsUpdate_Override();
			}

			void PostPhysicsUpdate()
			{
				PostPhysicsUpdate_Override();
			}
		};
		// TODO : Change from shared_ptr (It needs to be copyable for the ComponentArray::RemoveComponent function)
		#define TickComponent_Interface std::shared_ptr<aZero::ECS::TickComponent_Interface_Class>

		class ExampleTickComponent_Class : public TickComponent_Interface_Class
		{
		public:
			ECS::Entity m_Entity;
			int m_Random;
			ECS::ComponentArray<TransformComponent>* m_TransformComponents = nullptr;
		private:
			// Inherited via TickComponent
			virtual void OnStart_Override() override
			{

			}
			virtual void OnEnd_Override() override
			{

			}
			virtual void PrePhysicsUpdate_Override() override
			{
				if (m_TransformComponents->HasComponent(m_Entity))
				{
					ECS::TransformComponent& Comp = *m_TransformComponents->GetComponent(m_Entity);

					DXM::Vector3 Position = DXM::Vector3::Zero;
					static double LerpVal = 0.f;
					LerpVal += 0.001;
					Position.x = sin(LerpVal) * (float)m_Random;
					Position.z = Comp.GetTransform().Translation().z;
					DXM::Matrix TempWorld = DXM::Matrix::CreateScale(0.2) * 
						DXM::Matrix::CreateRotationX(LerpVal) * DXM::Matrix::CreateRotationY(LerpVal) * DXM::Matrix::CreateTranslation(Position);
					
					Comp.SetTransform(TempWorld);
				}
			}
			virtual void PostPhysicsUpdate_Override() override
			{

			}

		public:
			ExampleTickComponent_Class(Entity Ent, ECS::ComponentArray<TransformComponent>* TransformComponents)
			{
				m_TransformComponents = TransformComponents;
				std::random_device dev;
				std::mt19937 rng(dev());
				std::uniform_real_distribution<> dist6(0, 2);
				m_Random = dist6(rng);
				m_Entity = Ent;
			}
		};

		#define ExampleTickComponent(Entity, TransformComponents) TickComponent_Interface(std::make_shared<aZero::ECS::ExampleTickComponent_Class>(Entity, TransformComponents))
	}
}