#pragma once

#include "scene/Scene.hpp"

namespace aZero
{
	namespace Scene
	{
		class SceneManager : public NonCopyable, public NonMovable
		{
		private:
			SceneID m_NextSceneID;
			std::unordered_map<std::string, std::shared_ptr<Scene>> m_SceneMap;

		public:
			SceneManager():m_NextSceneID(0){}

			std::weak_ptr<Scene> CreateScene(const std::string& name)
			{
				if (m_NextSceneID == std::numeric_limits<SceneID>::max())
				{
					// TODO: Figure out exactly how to handle this case
						// Maybe just throw exception since it's probably not gonna happen? Otherwise return an empty weak_ptr?
					throw std::runtime_error("Max scenes created during runtime.");
				}

				std::string sceneName = Helper::HandleNameCollision(name, m_SceneMap);
				SceneID newSceneID = m_NextSceneID;
				m_NextSceneID++;
				m_SceneMap[sceneName] = std::shared_ptr<Scene>(new Scene(newSceneID, sceneName));
				
				return m_SceneMap[sceneName];
			}

			bool Delete(const std::string& name)
			{
				if (m_SceneMap.find(name) != m_SceneMap.end())
				{
					m_SceneMap.erase(name);
					return true;
				}
				return false;
			}

			std::weak_ptr<Scene> Get(const std::string& name)
			{
				if (m_SceneMap.find(name) != m_SceneMap.end())
				{
					return m_SceneMap.at(name);
				}
				return std::weak_ptr<Scene>();
			}
		};
	}
}
