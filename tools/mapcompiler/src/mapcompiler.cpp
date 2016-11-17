
#include <StudioKit/blenderimporter.hpp>
#include <StudioKit/mapwriter.hpp>
#include <StudioKit/worldgeom.hpp>

#include <framework/errorbuffer.hpp>
#include <framework/errorcheck.hpp>
#include <framework/filesystem.hpp>
#include <framework/system.hpp>
#include <framework/varsystem.hpp>
#include <framework/utility/params.hpp>

#include <littl/File.hpp>

#define APP_TITLE       "mapcompiler"
#define APP_VENDOR      "Minexew Games"
#define APP_VERSION     "3.0"

namespace mapcompiler
{
    using namespace zfw;

    using StudioKit::WorldVertex_t;
    using StudioKit::IWorldGeomNode;

    static ErrorBuffer_t* g_eb;
    static ISystem* g_sys;
    static bool waitforkey = false;

    static bool SysInit(int argc, char** argv)
    {
        ErrorBuffer::Create(g_eb);

        g_sys = CreateSystem();

        if (!g_sys->Init(g_eb, kSysNonInteractive))
            return false;

        auto var = g_sys->GetVarSystem();
        var->SetVariable("appName", "CompileMap", 0);

        if (!g_sys->Startup())
            return false;

        return true;
    }

    static void SysShutdown()
    {
        g_sys->Shutdown();
    }

    static bool Face(const StudioKit::IBlenderImporter::Vertex_t vertices[3],
            const StudioKit::IBlenderImporter::Material_t& material,
        StudioKit::IWorldGeomTree* tree)
    {
        std::string texture = material.texture;

        if (texture.find("!skip") != std::string::npos || texture.find("SKIP") != std::string::npos)
            return true;

        char params[2048];

        if (!texture.empty())
        {
            if (!Params::BuildIntoBuffer(params, sizeof(params), 1,
                    "texture",      sprintf_4095("path=media/texture/%s,wrapx=repeat,wrapy=repeat", texture.c_str())
                    ))
                return ErrorBuffer::SetBufferOverflowError(g_eb, li_functionName),
                        false;
        }
        else
        {
            if (!Params::BuildIntoBuffer(params, sizeof(params), 0
                    ))
                return ErrorBuffer::SetBufferOverflowError(g_eb, li_functionName),
                        false;
        }

        size_t materialIndex = tree->GetMaterialByParams(params);

        //Float3 normal = glm::normalize(glm::cross(vertices[2].pos - vertices[0].pos, vertices[1].pos - vertices[0].pos));

        //for (size_t i = 0; i < face.getNumChildren(); i++)
        //    vertices[i].normal = normal;

        tree->AddSolidBrush(vertices, 3, materialIndex);

        return true;
    }

    static bool ProcessMap(const char* input, const char* outputContainer, const char* outputName, StudioKit::IBlenderImporter::Scene_t* scene)
    {
        auto mh = g_sys->GetModuleHandler(true);

        // open output file
        unique_ptr<li::File> file(li::File::open(outputContainer, "wb+"));

        if (!file)
            return ErrorBuffer::SetError2(g_eb, EX_ACCESS_DENIED, 1,
                "desc", sprintf_255("Failed to open output file %s.", outputContainer)
            ), false;

        // init MapWriter
        unique_ptr<StudioKit::IMapWriter> mapWriter(StudioKit::TryCreateMapWriter(mh));
        ErrorCheck(mapWriter != nullptr);
        ErrorCheck(mapWriter->Init(g_sys, file.get(), outputName));
        mapWriter->SetMetadata("authored_using", "name=" APP_TITLE ",version=" APP_VERSION ",vendor=" APP_VENDOR);

        // create & init IWorldGeomTree
        unique_ptr<StudioKit::IWorldGeomTree> wgt(StudioKit::TryCreateWorldGeomTree(mh));
        ErrorCheck(wgt != nullptr);
        ErrorCheck(wgt->Init(g_sys));

        // add geometry

        for (auto& mesh : scene->meshes)
        {
            for (auto& materialGroup : mesh.materialGroups)
            {
                for (size_t i = 0; i + 2 < materialGroup.vertices.size(); i += 3)
                {
                    ErrorPassthru(Face(&materialGroup.vertices[i],
                            scene->materials[materialGroup.sceneMaterialIndex],
                            wgt.get()));
                }
            }
        }

        // process world geometry

        g_sys->Printf(kLogInfo, "Processing world geometry.");

        OutputStream* materials, *geometry;
        ErrorCheck(mapWriter->GetOutputStreams1(&materials, &geometry));

        ErrorCheck(wgt->Process(materials, geometry));

        // add entities

        g_sys->Printf(kLogInfo, "Processing entities.");

        for (auto& light : scene->pointLights)
        {
            mapWriter->AddEntity(sprintf_4095("Entity: 'light_dynamic' (range: '%.6f', "
                "colour: '%.6f, %.6f, %.6f', "
                "pos: '%.6f, %.6f, %.6f')",
                light.dist,
                light.color.r, light.color.g, light.color.b,
                light.pos.x, light.pos.y, light.pos.z));
        }

        wgt.reset();

        mapWriter->Finish();
        mapWriter.reset();

        return true;
    }

    static bool compile(int argc, char** argv)
    {
        std::string input, outputContainer, outputName;
        float scale = 1.0f;

        for (int i = 1; i < argc; i++)
        {
            const char* p_params = argv[i];
            const char* key, *value;

            while (Params::Next(p_params, key, value))
            {
                if (strcmp(key, "input") == 0)
                    input = value;
                else if (strcmp(key, "outputContainer") == 0)
                    outputContainer = value;
                else if (strcmp(key, "outputName") == 0)
                    outputName = value;
                else if (strcmp(key, "scale") == 0)
                    reflection::reflectFromString(scale, value);
                else if (strcmp(key, "waitforkey") == 0)
                    waitforkey = Util::ParseBool(value);
            }
        }

        if (input.empty() || outputContainer.empty() || outputName.empty())
        {
            fprintf(stderr, "\nusage: mapcompiler input=... outputContainer=... outputName=... [scale=...] [waitforkey=1]");
            return true;
        }

        auto imh = g_sys->GetModuleHandler(true);

        unique_ptr<StudioKit::IBlenderImporter> parser(StudioKit::CreateBlenderImporter());

        parser->Init(g_sys, scale);

        StudioKit::IBlenderImporter::Scene_t scene;
        ErrorCheck(parser->ImportScene(input.c_str(), &scene));

        ErrorCheck(ProcessMap(input.c_str(), outputContainer.c_str(), outputName.c_str(), &scene));

        return true;
    }

    extern "C" void main(int argc, char** argv)
    {
        if (!SysInit(argc, argv) || !compile(argc, argv))
            g_sys->DisplayError(g_eb, true);

        SysShutdown();

        if (waitforkey)
            getchar();
    }
}
