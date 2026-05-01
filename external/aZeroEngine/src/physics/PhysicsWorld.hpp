#pragma once
#include <memory>
#include "Body.hpp"
#include "Jolt/Physics/Collision/CollideShape.h"

namespace aZero
{
	namespace Physics
	{
		class PhysicsEngine;

		class PhysicsWorld
		{
			friend class PhysicsEngine;
		public:
			class MyBodyActivationListener : public JPH::BodyActivationListener
			{
				friend class PhysicsWorld;
			public:
				struct Event_BodyActivation
				{
					uint32_t EntityID;
				};

				MyBodyActivationListener() = default;

				virtual void OnBodyActivated(const JPH::BodyID& inBodyID, uint64_t inBodyUserData) override
				{
					std::unique_lock<std::mutex> lock(m_BodyActivationMutex);
					m_ActivatedEvents.emplace_back(static_cast<uint32_t>(inBodyUserData >> 32));
				}

				virtual void OnBodyDeactivated(const JPH::BodyID& inBodyID, uint64_t inBodyUserData) override
				{
					std::unique_lock<std::mutex> lock(m_BodyActivationMutex);
					m_DeactivatedEvents.emplace_back(static_cast<uint32_t>(inBodyUserData >> 32));
				}

				void ResetEvents()
				{
					m_ActivatedEvents.clear();
					m_DeactivatedEvents.clear();
				}

			private:
				std::mutex m_BodyActivationMutex; // TODO: Split to multiple locks

				std::vector<Event_BodyActivation> m_ActivatedEvents;
				std::vector<Event_BodyActivation> m_DeactivatedEvents;
			};

			class MyContactListener : public JPH::ContactListener
			{
				friend PhysicsWorld;
			public:
				struct Event_ContactValidate
				{
					uint32_t FirstEntityID, SecondEntityID;
					JPH::RVec3Arg InBaseOffset;
					std::unique_ptr<JPH::CollideShapeResult> InCollisionResult; // TODO: Avoid dynamic mem alloc
				};

				struct Event_ContactAdded
				{
					uint32_t FirstEntityID, SecondEntityID;
					JPH::ContactManifold ContactManifold;
					JPH::ContactSettings ContactSettings;
				};

				struct Event_ContactPersisted
				{
					uint32_t FirstEntityID, SecondEntityID;
					JPH::ContactManifold ContactManifold;
					JPH::ContactSettings ContactSettings;
				};

				struct Event_ContactRemoved
				{
					JPH::SubShapeIDPair InSubShapePair;
				};

				MyContactListener() = default;

				// See: ContactListener
				virtual JPH::ValidateResult OnContactValidate(const JPH::Body& inBody1, const JPH::Body& inBody2, JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult& inCollisionResult) override
				{
					std::unique_lock<std::mutex> lock(m_ContactMutex);
					m_ContactValidateEvents.emplace_back(
						static_cast<uint32_t>((inBody1.GetUserData()) >> 32),
						static_cast<uint32_t>((inBody2.GetUserData()) >> 32),
						inBaseOffset,
						std::make_unique<JPH::CollideShapeResult>(inCollisionResult));

					// Allows you to ignore a contact before it is created (using layers to not make objects collide is cheaper!)
					return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
				}

				virtual void OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) override
				{
					std::unique_lock<std::mutex> lock(m_ContactMutex);
					m_ContactAddedEvents.emplace_back(
						static_cast<uint32_t>((inBody1.GetUserData()) >> 32),
						static_cast<uint32_t>((inBody2.GetUserData()) >> 32),
						inManifold,
						ioSettings);
				}

				virtual void OnContactPersisted(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) override
				{
					std::unique_lock<std::mutex> lock(m_ContactMutex);
					m_ContactPersistedEvents.emplace_back(
						static_cast<uint32_t>((inBody1.GetUserData()) >> 32),
						static_cast<uint32_t>((inBody2.GetUserData()) >> 32),
						inManifold,
						ioSettings);
				}

				virtual void OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair) override
				{
					std::unique_lock<std::mutex> lock(m_ContactMutex);
					m_ContactRemovedEvents.emplace_back(inSubShapePair);
				}

				void ResetEvents()
				{
					m_ContactValidateEvents.clear();
					m_ContactAddedEvents.clear();
					m_ContactPersistedEvents.clear();
					m_ContactRemovedEvents.clear();
				}

			private:
				std::mutex m_ContactMutex; // TODO: Split to multiple locks

				std::vector<Event_ContactValidate> m_ContactValidateEvents;
				std::vector<Event_ContactAdded> m_ContactAddedEvents;
				std::vector<Event_ContactPersisted> m_ContactPersistedEvents;
				std::vector<Event_ContactRemoved> m_ContactRemovedEvents;
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
			}

			void ResetBodyEvents()
			{
				m_Body_activation_listener->ResetEvents();
				m_Contact_listener->ResetEvents();
			}

			const std::vector<MyBodyActivationListener::Event_BodyActivation>& GetBodyActivatedEvents() const { return m_Body_activation_listener->m_ActivatedEvents; }
			const std::vector<MyBodyActivationListener::Event_BodyActivation>& GetBodyDeactivatedEvents() const { return m_Body_activation_listener->m_DeactivatedEvents; }
			const std::vector<MyContactListener::Event_ContactValidate>& GetContactValidateEvents() const { return m_Contact_listener->m_ContactValidateEvents; }
			const std::vector<MyContactListener::Event_ContactAdded>& GetContactAddedEvents() const { return m_Contact_listener->m_ContactAddedEvents; }
			const std::vector<MyContactListener::Event_ContactPersisted>& GetContactPersistedEvents() const { return m_Contact_listener->m_ContactPersistedEvents; }
			const std::vector<MyContactListener::Event_ContactRemoved>& GetContactRemovedEvents() const { return m_Contact_listener->m_ContactRemovedEvents; }

		private:
			void Init(JPH::JobSystemThreadPool& jobSystem, float updateFrequency);

			JPH::PhysicsSystem m_System;
			std::unique_ptr<MyBodyActivationListener> m_Body_activation_listener;
			std::unique_ptr<MyContactListener> m_Contact_listener;
			JPH::BodyInterface* m_BodyInterface = nullptr;
			std::unique_ptr<JPH::TempAllocatorImpl> m_Allocator;
			JPH::JobSystemThreadPool* di_JobSystem = nullptr;
			float m_UpdateFrequency;
		};
	}
}