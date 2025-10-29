#include "scene/Scene.hpp"

namespace aZero
{
	namespace SceneTemp
	{
		class SceneManager
		{
		private:
			SceneTemp::SceneID m_NextSceneID;
			std::unordered_map<std::string, std::shared_ptr<Scene::SceneNew>> m_SceneMap;

			// TODO: Make this public for other classes (ex. AssetManager)
			bool ValidateName(std::string& name)
			{
				// TODO: Recursive logic to change name if needed to make it unique
				return true;
			}

		public:
			SceneManager():m_NextSceneID(0){}

			std::weak_ptr<Scene::SceneNew> CreateScene(const std::string& name)
			{
				if (m_NextSceneID == std::numeric_limits<SceneTemp::SceneID>::max())
				{
					// TODO: Figure out exactly how to handle this case
					throw;
				}

				std::string sceneName = name;
				this->ValidateName(sceneName);

				SceneTemp::SceneID newSceneID = m_NextSceneID;
				m_NextSceneID++;
				m_SceneMap[sceneName] = std::shared_ptr<Scene::SceneNew>(new Scene::SceneNew(newSceneID, sceneName));
				
				return m_SceneMap[sceneName];
			}

			bool Delete(const std::string& name)
			{
				if (m_SceneMap.find(name) != m_SceneMap.end())
				{
					// TODO: Deallocate everything needed and then the rest via the scene destructor
					m_SceneMap.erase(name);
					return true;
				}
				return false;
			}

			std::weak_ptr<Scene::SceneNew> Get(const std::string& name)
			{
				if (m_SceneMap.find(name) != m_SceneMap.end())
				{
					return m_SceneMap.at(name);
				}
				return std::weak_ptr<Scene::SceneNew>();
			}
		};
	}
}
