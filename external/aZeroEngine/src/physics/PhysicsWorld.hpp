#pragma once
#include "misc/CallbackExecutor.hpp"
#include "Body.hpp"

namespace aZero
{
	namespace Rendering
	{
		class WireframeRenderer;

		namespace WireframeShape
		{
			class Sphere;
			class AABB;
		}
	}

	namespace Physics
	{
		class PhysicsEngine;

		class PhysicsWorld
		{
			friend class PhysicsEngine;
		public:
			class MyBodyActivationListener : public JPH::BodyActivationListener
			{
			public:
				MyBodyActivationListener() = default;

				MyBodyActivationListener(CallbackExecutor& callbackExecutor)
					:m_CallbackExecutor(&callbackExecutor) {}

				virtual void OnBodyActivated(const JPH::BodyID& inBodyID, uint64_t inBodyUserData) override
				{
					m_CallbackExecutor->Append([inBodyID] {
						std::cout << "Body activated: " << inBodyID.GetIndex() << "\n";
						});
				}

				virtual void OnBodyDeactivated(const JPH::BodyID& inBodyID, uint64_t inBodyUserData) override
				{
					m_CallbackExecutor->Append([inBodyID] {
						std::cout << "Body deactivated: " << inBodyID.GetIndex() << "\n";
						});
				}
			private:
				CallbackExecutor* m_CallbackExecutor = nullptr;
			};

			class MyContactListener : public JPH::ContactListener
			{
			public:
				MyContactListener() = default;

				MyContactListener(CallbackExecutor& callbackExecutor)
					:m_CallbackExecutor(&callbackExecutor) {}

				// See: ContactListener
				virtual JPH::ValidateResult OnContactValidate(const JPH::Body& inBody1, const JPH::Body& inBody2, JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult& inCollisionResult) override
				{
					JPH::BodyID one = inBody1.GetID();
					JPH::BodyID two = inBody2.GetID();
					m_CallbackExecutor->Append([one, two] {
						std::cout << "Body " << one.GetIndex() << " validated with " << two.GetIndex() << "\n";
						});

					// Allows you to ignore a contact before it is created (using layers to not make objects collide is cheaper!)
					return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
				}

				virtual void OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) override
				{
					JPH::BodyID one = inBody1.GetID();
					JPH::BodyID two = inBody2.GetID();
					m_CallbackExecutor->Append([one, two] {
						std::cout << "Body " << one.GetIndex() << " collided with " << two.GetIndex() << "\n";
						});
				}

				virtual void OnContactPersisted(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) override
				{
					JPH::BodyID one = inBody1.GetID();
					JPH::BodyID two = inBody2.GetID();
					m_CallbackExecutor->Append([one, two] {
						std::cout << "Body " << one.GetIndex() << " persisted with " << two.GetIndex() << "\n";
						});
				}

				virtual void OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair) override
				{
					JPH::BodyID one = inSubShapePair.GetBody1ID();
					JPH::BodyID two = inSubShapePair.GetBody2ID();
					m_CallbackExecutor->Append([one, two] {
						std::cout << "Body " << one.GetIndex() << " stopped colliding with " << two.GetIndex() << "\n";
						});
				}

			private:
				CallbackExecutor* m_CallbackExecutor = nullptr;
			};

			PhysicsWorld() = default;

			Body CreateBody(const JPH::BodyCreationSettings& settings, bool addToPhysics = true)
			{
				JPH::Body* body = m_BodyInterface->CreateBody(settings);
				if (body && addToPhysics)
				{
					m_BodyInterface->AddBody(body->GetID(), JPH::EActivation::Activate);
				}

				if (!body)
				{
					throw; // TODO: Handle
				}

				return Body(body->GetID(), *m_BodyInterface, m_System.GetBodyLockInterface());
			}

			void AddBodiesToPhysics(const std::vector<Body>& bodies)
			{
				std::vector<JPH::BodyID> tempBodies;
				tempBodies.reserve(bodies.size());
				for (const auto& body : bodies)
				{
					tempBodies.emplace_back(body.m_ID);
				}
				m_BodyInterface->AddBodiesPrepare(tempBodies.data(), tempBodies.size());
				m_BodyInterface->AddBodiesFinalize(tempBodies.data(), tempBodies.size(), JPH::BodyInterface::AddState(), JPH::EActivation::Activate);
			}

			void FreezeBodies(const std::vector<Body>& bodies)
			{
				std::vector<JPH::BodyID> tempBodies;
				tempBodies.reserve(bodies.size());
				for (const auto& body : bodies)
				{
					tempBodies.emplace_back(body.m_ID);
				}
				m_BodyInterface->RemoveBodies(tempBodies.data(), tempBodies.size());
			}

			void DestroyBody(const Body& body)
			{
				m_BodyInterface->DestroyBody(body.m_ID);
			}

			void DestroyBodies(const std::vector<Body>& bodies)
			{
				std::vector<JPH::BodyID> tempBodies;
				tempBodies.reserve(bodies.size());
				for (const auto& body : bodies)
				{
					tempBodies.emplace_back(body.m_ID);
				}
				m_BodyInterface->DestroyBodies(tempBodies.data(), tempBodies.size());
			}

			void OptimizeBroadPhase()
			{
				m_System.OptimizeBroadPhase();
			}

			void Update()
			{
				m_System.Update(m_UpdateFrequency, 1, m_Allocator.get(), di_JobSystem);
				this->ResolveCallbacks();
			}

			void ResolveCallbacks()
			{
				m_ActivationCallbackExecutor.Execute();
				m_ContactCallbackExecutor.Execute();
			}

			void AddDebugSphere(const Rendering::WireframeShape::Sphere& sphere);
			void AddDebugAABB(const Rendering::WireframeShape::AABB& aabb);

		private:
			void Init(JPH::JobSystemThreadPool& jobSystem, Rendering::WireframeRenderer& colliderRenderer, float updateFrequency);

			// TODO: Make the listeners fill in arrays of data with each collision that is later processed on the main thread

			CallbackExecutor m_ActivationCallbackExecutor;
			CallbackExecutor m_ContactCallbackExecutor;
			JPH::PhysicsSystem m_System;
			MyBodyActivationListener m_Body_activation_listener;
			MyContactListener m_Contact_listener;
			JPH::BodyInterface* m_BodyInterface = nullptr;
			std::unique_ptr<JPH::TempAllocatorImpl> m_Allocator;
			JPH::JobSystemThreadPool* di_JobSystem = nullptr;
			Rendering::WireframeRenderer* di_ColliderRenderer = nullptr;
			float m_UpdateFrequency;
		};
	}
}