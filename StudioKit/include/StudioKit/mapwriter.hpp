#pragma once

#include <framework/base.hpp>
#include <framework/modulehandler.hpp>

#include <reflection/magic.hpp>

namespace StudioKit
{
    class IMapWriter
    {
        public:
            virtual ~IMapWriter() {}

            virtual bool Init(zfw::ISystem* sys, zfw::IOStream* outputContainerFile, const char* outputName) = 0;
            virtual void SetMetadata(const char* key, const char* value) = 0;

            virtual void AddEntity(const char* entityCfx2) = 0;

            virtual bool GetOutputStreams1(zfw::OutputStream** materials, zfw::OutputStream** vertices) = 0;

            virtual bool Finish() = 0;

            REFL_CLASS_NAME("IMapWriter", 1)
    };

#ifdef STUDIO_KIT_STATIC
    IMapWriter* CreateMapWriter();

    ZOMBIE_IFACTORYLOCAL(MapWriter)
#else
    ZOMBIE_IFACTORY(MapWriter, "StudioKit")
#endif
}
