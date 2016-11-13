
#include "modelloader.hpp"
#include "ntile.hpp"

#include <vector>

namespace ntile
{
	enum { MAGIC_NUMBER = 32 };
	class ModelLoaderImpl
	{
		public:
			ModelLoaderImpl(ISystem* sys) : sys(sys) {}

			bool LoadModel(const char* path, std::vector<n3d::Mesh*>& meshes);

		private:
			ISystem* sys;
	};

	bool ModelLoaderImpl::LoadModel(const char* path, std::vector<n3d::Mesh*>& meshes)
	{
		/*const size_t vertexSize = MAGIC_NUMBER;
		const size_t numVertices = mdl1->getSize() / vertexSize;

		auto mesh = new n3d::Mesh;
		meshes.push_back(mesh);

		mesh->vb.reset(ir->CreateVertexBuffer());
		mesh->vb->Upload(&bytes[0], numVertices * vertexSize);

		mesh->primitiveType = PRIMITIVE_TRIANGLES;
		mesh->format = g_modelVertexFormat.get();
		mesh->offset = 0;
		mesh->count = numVertices;

		mesh->transform = glm::translate(glm::mat4x4(), glm::vec3(512.0f, 640.0f, 0.0f));*/

		return true;
	}

	/*IModel* ModelLoader::LoadModel(ISystem* sys, n3d::IRenderer* ir, const char* path)
	{
		ModelLoaderImpl ldr(sys);

		std::vector<n3d::Mesh*> meshes;
		zombie_assert(ldr.LoadModel(path, meshes));

		return ir->CreateModelFromMeshes(&meshes[0], 1);
	}*/
}
