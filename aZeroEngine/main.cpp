#include "Core/Engine.h"

// https://devblogs.microsoft.com/directx/gettingstarted-dx12agility/
extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 614; }

extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }

using namespace aZero;

int WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR commandLine, int showCommand)
{
#ifdef _DEBUG

	AllocConsole();
	freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);

	Microsoft::WRL::ComPtr<ID3D12Debug> spDebugController0;
	Microsoft::WRL::ComPtr<ID3D12Debug1> spDebugController1;
	D3D12GetDebugInterface(IID_PPV_ARGS(&spDebugController0));

	Microsoft::WRL::ComPtr<ID3D12Debug> d3d12Debug;
	Microsoft::WRL::ComPtr<IDXGIDebug> idxgiDebug;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&d3d12Debug))))
		d3d12Debug->EnableDebugLayer();
	DXGIGetDebugInterface1(0, IID_PPV_ARGS(&idxgiDebug));
#endif // DEBUG

	{
		/*
		Add lights
		* FOR NOW, since the lights arent neccessarily in a packed array and we dont have a tree to get the lights,
		*		The lights will be referenced by a buffer which has the following:
		*			1. All indices to the lights in a packed array
		*				The indices will be gotten via the entity -> PackedGPULookupBuffer index
		*				This means that the index can be used to get the data from the shader-side PackedGPULookupBuffer
		*			2. Num indices so the pixel shaders etc. can iterate over the data
		*
			Point light basic shader math
			Spot light basic shader math
			Dir light basic shader math

		Add materials
			Basic material supporting:
				1. diffuse texture
				2. normal map texture
			Materials will be stored in a material cpu cache
				Materials will have a unique id
			Static mesh component hold material id
				If the component is updated and the material id isnt registered to the renderer, the material is registered
					Basically by doing a PackedGPULookupBuffer::Register + Update
					The renderer also holds a refcount to how many static mesh comps reference it.
						So increase refcount by 1
				If the component is updated and the material id refcount is 0, unregister the material from the renderer
			Renderer::MarkMateriaDirty() will update the material if its registered

		Add camera component and multi-view rendering
			Render pipeline are called per active camera component.
				This includes, culling, geometry, light etc. (atleast for now)
			When updating a camera comp, the flag "IsActive" is checked.
				If changed, the camera comp (or ent) is added/removed from the renderer list of cameras.

		Restructure project folders
		Setup engine so it can be easily used with git, preferably as a static or dynamic (or both) lib
		Create level editor project with uses the engine lib
		Scene: save, load, management
		BIG QUESTION: Maybe mesh gpu allocations can be handled the same way as the materials using a refcount system?
			By doing this the loading, unloading etc. can be handled automatically.
			The only problem occurs if there is a single static mesh comp referencing it and swapping in and out.
				However, if the cpu data is cached it could be ok, but not ideal.
		BIG QUESTION: How to handle parenting????
			Maybe when we update a component of an entity, we mark the entity as dirty with that specific component
			Before rendering, we go through this list and do the Renderer::UpdateRenderComponents() for all the components (and affected ones) of that entity
			Because all the transform components have been moved for the frame at this point,
				we can just go through the hierarchies that form below all the dirty entities.
				At this point, if an entity is static, we call this->ReInsertIntoOctree(Entity).
		*/

		// Setup engine
		aZero::Engine Engine(instance, { 1920, 1080 });
		std::shared_ptr<aZero::Window> ActiveWindow = Engine.GetWindow();


		aZero::Scene* TestScene = Engine.CreateScene("Test");
		Engine.SetScene(TestScene);

		aZero::ECS::Entity& Ent1 = TestScene->CreateEntity("P1");
		aZero::gComponentManager.AddComponent(Ent1, ECS::TransformComponent(DXM::Matrix::CreateTranslation(DXM::Vector3(0, 0, 10.f))));
		aZero::gComponentManager.AddComponent(Ent1, ECS::StaticMeshComponent());

		aZero::ECS::Entity& Ent2 = TestScene->CreateEntity("P2");
		aZero::gComponentManager.AddComponent(Ent2, ECS::TransformComponent(DXM::Matrix::CreateScale(0.5) * DXM::Matrix::CreateTranslation(DXM::Vector3(0, 2, 10.f))));
		aZero::gComponentManager.AddComponent(Ent2, ECS::StaticMeshComponent());

		// Run engine
		while (ActiveWindow->IsOpen())
		{
			if (GetAsyncKeyState(VK_ESCAPE))
			{
				break;
			}

			Engine.BeginFrame();

			static float Rot = 0.f;
			Rot += 0.005f;
			aZero::gComponentManager.GetComponent<ECS::TransformComponent>(Ent2).SetTransform(DXM::Matrix::CreateScale(0.5f) * DXM::Matrix::CreateRotationY(Rot) * DXM::Matrix::CreateTranslation(DXM::Vector3(0,0,10.f)));
			aZero::gComponentManager.UpdateRenderComponent<ECS::TransformComponent>(Ent2);
			Engine.Update();

			Engine.Render();
		}

	}

#ifdef _DEBUG
	idxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, (DXGI_DEBUG_RLO_FLAGS)(DXGI_DEBUG_RLO_IGNORE_INTERNAL | DXGI_DEBUG_RLO_DETAIL));
#endif // DEBUG

	return 0;
}