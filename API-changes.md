2019.01

	- RenderingKit: less ambigous initialization

2017.01:

	- RenderingKit: remove SetTexture(const char* name, shared_ptr<ITexture>&& texture) overload
	- RenderingKit: remove some methods from IDeferredShadingManager (replacement = ?)
	- RenderingKit: remove support for IResourceManager
	- RenderingKit: remove BasicPainter::InitWithShader(IRenderingManager* rm, shared_ptr<IShader> program);

2016.01
	- Do not include <framework/framework.hpp>

	- remove manual resource management for entities; Resources shall be managed by IResourceManager2 and created in Init()
	- remove IEntity::{Acquire,Drop}Resources

	- introduce EntityWorld::IterateEntities
	- remove EntityWorld::GetEntityList

	- RenderingKit: remove IRenderingKitHost, IFPMaterial
