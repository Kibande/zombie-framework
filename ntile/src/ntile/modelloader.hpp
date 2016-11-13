
#include "n3d.hpp"

namespace ntile
{
	class ModelLoader
	{
		public:
			static IModel* LoadModel(ISystem* sys, n3d::IRenderer* ir, const char* path);
	};
}
