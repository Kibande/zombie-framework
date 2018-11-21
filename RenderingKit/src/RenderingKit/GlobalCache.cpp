#include "RenderingKitImpl.hpp"

#include <vector>

namespace RenderingKit
{
    namespace {
        using AttributeLocationMapping = std::pair<std::string, int>;

        std::vector<AttributeLocationMapping> attribLocations;
        int nextAttribLocation = 0;

        std::vector<unique_ptr<GLVertexFormat>> vertexFormats;
    }

    int GlobalCache::GetAttribLocationByName(const char* name) {
        for (const auto& mapping : attribLocations) {
            if (mapping.first == name) {
                return mapping.second;
            }
        }

        int location = nextAttribLocation++;
        attribLocations.emplace_back(name, location);
        return location;
    }

    GLVertexFormat* GlobalCache::ResolveVertexFormat(const VertexFormatInfo& vf_in) {
        // Has a GLVertexFormat been created for this VertexFormatInfo
        if (*vf_in.cache) {
            return reinterpret_cast<GLVertexFormat*>(*vf_in.cache);
        }

        // Nope, search the known VFs for a match

        for (auto& vf : vertexFormats) {
            if (vf->Equals(vf_in)) {
                *vf_in.cache = vf.get();
                return vf.get();
            }
        }

        // No match; need to create a new one

        auto vf = make_unique<GLVertexFormat>(vf_in, *this);
        auto vf_ptr = vf.get();
        vertexFormats.push_back(move(vf));

        *vf_in.cache = vf_ptr;
        return vf_ptr;
    }

    /*void GlobalCache::VisitAttribLocationMappings(const std::function<void(const char*, int)>& visitor) {
        for (const auto& mapping : attribLocations) {
            visitor(mapping.first.c_str(), mapping.second);
        }
    }*/
}
