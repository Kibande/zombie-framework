
#include "worldcraftpro.hpp"

#include <framework/engine.hpp>
#include <framework/filesystem.hpp>
#include <framework/varsystem.hpp>

#include <StudioKit/worldgeom.hpp>

#include <worldcraft2/tools/blenderimporter.hpp>

#include <littl/File.hpp>

namespace worldcraftpro
{
    using namespace li;

    using StudioKit::WorldVertex_t;
    using StudioKit::IWorldGeomNode;
    using StudioKit::IWorldGeomTree;

    using worldcraft::IBlenderImporter;

    static bool waitforkey = false;

    static bool SysInit(int argc, char** argv)
    {
        g_sys = CreateEngine();

        if (!g_sys->Init(g.eb, kSysNonInteractive))
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

    static bool Face(const IBlenderImporter::Vertex_t vertices[3],
            const IBlenderImporter::Material_t& material,
            IWorldGeomTree* tree)
    {
        String texture = material.texture.c_str();

        if (texture.findSubString("!skip") >= 0)
            return true;

        char params[2048];

        if (!texture.isEmpty())
        {
            if (!Params::BuildIntoBuffer(params, sizeof(params), 1,
                    "texture",      (const char*)("path=media/texture/" + texture + ",wrapx=repeat,wrapy=repeat")
                    ))
                return ErrorBuffer::SetBufferOverflowError(g.eb, li_functionName),
                        false;
        }
        else
        {
            if (!Params::BuildIntoBuffer(params, sizeof(params), 0
                    ))
                return ErrorBuffer::SetBufferOverflowError(g.eb, li_functionName),
                        false;
        }

        size_t materialIndex = tree->GetMaterialByParams(params);

        //Float3 normal = glm::normalize(glm::cross(vertices[2].pos - vertices[0].pos, vertices[1].pos - vertices[0].pos));

        //for (size_t i = 0; i < face.getNumChildren(); i++)
        //    vertices[i].normal = normal;

        tree->AddSolidBrush(vertices, 3, materialIndex);

        return true;
    }

    static bool ProcessMap(const char* path, const char* output, IBlenderImporter::Scene_t* scene)
    {
        // create & init IWorldGeomTree
        unique_ptr<IWorldGeomTree> wgt(StudioKit::TryCreateWorldGeomTree(g_sys->GetModuleHandler(true)));
        ErrorCheck(wgt != nullptr);

        unique_ptr<File> file(File::open(output, "wb+"));

        if (!file)
            return ErrorBuffer::SetError2(g.eb, EX_ACCESS_DENIED, 1,
                    "desc", sprintf_255("Failed to open output file %s.", output)
                    ), false;

        wgt->SetOutputFile(move(file));

        ErrorCheck(wgt->Init(g_sys, "name=" APP_TITLE ",version=" APP_VERSION ",vendor=" APP_VENDOR));

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

        for (auto& light : scene->pointLights)
        {
            wgt->AddEntity(sprintf_4095("Entity: 'light_dynamic' (range: '%.6f', "
                    "colour: '%.6f, %.6f, %.6f', "
                    "pos: '%.6f, %.6f, %.6f')",
                    light.dist,
                    light.color.r, light.color.g, light.color.b,
                    light.pos.x, light.pos.y, light.pos.z));
        }

        //wgt->AddEntity("Entity: 'light_dynamic' (range: '10', "
        //            "colour: '1.0, 1.0, 1.0', "
        //            "pos: '0, 0, 1')");

        // process world geometry

        g_sys->Printf(kLogInfo, "Processing world geometry.");

        ErrorCheck(wgt->Process());

        g_sys->Printf(kLogInfo, "Finished.");
        return true;
    }

    static bool compile(int argc, char** argv)
    {
        String path, output;
        float scale = 1.0f;

        for (int i = 1; i < argc; i++)
        {
            const char* p_params = argv[i];
            const char* key, *value;

            while (Params::Next(p_params, key, value))
            {
                if (strcmp(key, "path") == 0)
                    path = value;
                else if (strcmp(key, "output") == 0)
                    output = value;
                else if (strcmp(key, "scale") == 0)
                    reflection::reflectFromString(scale, value);
                else if (strcmp(key, "waitforkey") == 0)
                    waitforkey = Util::ParseBool(value);
            }
        }

        zombie_assert(!path.isEmpty());
        zombie_assert(!output.isEmpty());

        auto imh = g_sys->GetModuleHandler(true);

        unique_ptr<IBlenderImporter> parser(worldcraft::CreateBlenderImporter());

        parser->Init(g_sys, scale);

        IBlenderImporter::Scene_t scene;
        ErrorPassthru(parser->ImportScene(path, &scene));

        ErrorCheck(ProcessMap(path, output, &scene));

        return true;
    }

    static void compileShutdown()
    {
    }

    void compileMain(int argc, char** argv)
    {
        if (!SysInit(argc, argv) || !compile(argc, argv))
            g_sys->DisplayError(g.eb, true);

        compileShutdown();
        SysShutdown();

        if (waitforkey)
            getchar();
    }
}
