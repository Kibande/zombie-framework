
#include <StudioKit/blenderimporter.hpp>
#include <StudioKit/mapwriter.hpp>
#include <StudioKit/worldgeom.hpp>

#include <framework/errorbuffer.hpp>
#include <framework/errorcheck.hpp>
#include <framework/filesystem.hpp>
#include <framework/system.hpp>
#include <framework/varsystem.hpp>
#include <framework/utility/params.hpp>

#include <littl/Directory.hpp>
#include <littl/File.hpp>
#include <littl/FileName.hpp>

#include <fstream>
#include <unordered_set>

#define APP_TITLE       "mapcompiler"
#define APP_VENDOR      "Minexew Games"
#define APP_VERSION     "3.0"

namespace mapcompiler
{
    using namespace zfw;

    using StudioKit::WorldVertex_t;
    using StudioKit::IWorldGeomNode;

    struct Options
    {
        std::string input, outputContainer, outputName, outputPath = ".", listResources, runGame;
        bool includeResources = false;
        bool waitforkey = false;
        float scale = 1.0f;
    };

    static ErrorBuffer_t* g_eb;
    static ISystem* g_sys;

    static bool SysInit(int argc, char** argv)
    {
        ErrorBuffer::Create(g_eb);

        g_sys = CreateSystem();

        if (!g_sys->Init(g_eb, kSysNonInteractive))
            return false;

        auto fs = g_sys->CreateStdFileSystem(".", kFSAccessStat | kFSAccessRead);
        g_sys->GetFSUnion()->AddFileSystem(move(fs), 100);

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
            const StudioKit::IBlenderImporter::Material_t& material, StudioKit::IWorldGeomTree* tree,
            std::unordered_set<std::string>& reslist)
    {
        std::string texture = material.texture;
        std::string texturePath;

        if (texture.find("!skip") != std::string::npos || texture.find("SKIP") != std::string::npos)
            return true;

        if (!texture.empty())
        {
            // TODO: search for resource location
            texturePath = "textures/" + texture;
        }

        char params[2048];

        if (!texturePath.empty())
        {
            if (!Params::BuildIntoBuffer(params, sizeof(params), 1,
                    "texture",      sprintf_4095("path=%s,wrapx=repeat,wrapy=repeat", texturePath.c_str())
                    ))
                return ErrorBuffer::SetBufferOverflowError(g_eb, li_functionName),
                        false;

            reslist.insert(texturePath);
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

    static bool ProcessMap(StudioKit::IBlenderImporter::Scene_t* scene, const Options& options)
    {
        auto mh = g_sys->GetModuleHandler(true);

        // open output file
        unique_ptr<li::File> file(li::File::open((options.outputPath + "/" + options.outputContainer).c_str(), "wb+"));

        if (!file)
            return ErrorBuffer::SetError2(g_eb, EX_ACCESS_DENIED, 1,
                "desc", sprintf_255("Failed to open output file %s.", options.outputContainer.c_str())
            ), false;

        // init MapWriter
        unique_ptr<StudioKit::IMapWriter> mapWriter(StudioKit::TryCreateMapWriter(mh));
        ErrorCheck(mapWriter != nullptr);
        ErrorCheck(mapWriter->Init(g_sys, file.get(), options.outputName.c_str()));
        mapWriter->SetMetadata("authored_using", "name=" APP_TITLE ",version=" APP_VERSION ",vendor=" APP_VENDOR);

        // create & init IWorldGeomTree
        unique_ptr<StudioKit::IWorldGeomTree> wgt(StudioKit::TryCreateWorldGeomTree(mh));
        ErrorCheck(wgt != nullptr);
        ErrorCheck(wgt->Init(g_sys));

        // add geometry

        std::unordered_set<std::string> reslist;

        for (auto& mesh : scene->meshes)
        {
            for (auto& materialGroup : mesh.materialGroups)
            {
                for (size_t i = 0; i + 2 < materialGroup.vertices.size(); i += 3)
                {
                    if (!Face(&materialGroup.vertices[i],
                        scene->materials[materialGroup.sceneMaterialIndex],
                        wgt.get(), reslist))
                        return false;
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

        // write reslist

        if (!options.listResources.empty())
        {
            std::ofstream ofs(options.listResources);
            
            for (const auto& res : reslist)
                ofs << res << std::endl;
        }

        // include resources if requested

        if (options.includeResources)
        {
            for (const auto& res : reslist)
            {
                std::unique_ptr<zfw::InputStream> input(g_sys->OpenInput(res.c_str()));

                if (!input)
                    g_sys->Printf(kLogError, "Error: couldn't read resource `%s`", res.c_str());
                else
                    mapWriter->AddResource(res.c_str(), input.get());
            }
        }

        // close everything

        mapWriter->Finish();
        mapWriter.reset();

        return true;
    }

    static bool compile(const Options& options)
    {
        auto imh = g_sys->GetModuleHandler(true);

        unique_ptr<StudioKit::IBlenderImporter> parser(StudioKit::CreateBlenderImporter());

        parser->Init(g_sys, options.scale);

        StudioKit::IBlenderImporter::Scene_t scene;
        ErrorCheck(parser->ImportScene(options.input.c_str(), &scene));

        ErrorCheck(ProcessMap(&scene, options));

        return true;
    }

    static bool Set(Options& options, const char* key, const char* value)
    {
        if (strcmp(key, "runGame") == 0)
            options.runGame = value;
        else if (strcmp(key, "includeResources") == 0)
            options.includeResources = Util::ParseBool(value);
        else if (strcmp(key, "input") == 0)
            options.input = value;
        else if (strcmp(key, "listResources") == 0)
            options.listResources = value;
        else if (strcmp(key, "outputContainer") == 0)
            options.outputContainer = value;
        else if (strcmp(key, "outputName") == 0)
            options.outputName = value;
        else if (strcmp(key, "outputPath") == 0)
            options.outputPath = value;
        else if (strcmp(key, "scale") == 0)
            reflection::reflectFromString(options.scale, value);
        else if (strcmp(key, "waitforkey") == 0)
            options.waitforkey = Util::ParseBool(value);
        else
            return false;

        return true;
    }

    static bool ParseOptions(Options& options, const char* p_params)
    {
        const char* key, *value;

        if (!p_params || p_params[0] == '#')
            return false;

        while (Params::Next(p_params, key, value))
        {
            if (!Set(options, key, value))
                fprintf(stderr, "Warning: ignored unknown option `%s`\n", key);
        }

        return true;
    }

    template <typename Options>
    static bool ParseOptions(Options& options, int argc, char** argv)
    {
        for (int i = 1; i < argc; i++)
        {
            if (argv[i][0] == '+')
            {
                unique_ptr<li::File> file(li::File::open(argv[i] + 1));

                while (!file->eof())
                    ParseOptions(options, file->readLine().c_str());
            }
            else
                ParseOptions(options, argv[i]);
        }

        return true;
    }

    static void RunGame(const Options& options)
    {
        li::FileName fileName(options.runGame.c_str());

        li::Directory::setCurrent(fileName.getDirectory());

        printf("\n\n==== EXECUTING %s ====\n\n", fileName.getFileName().c_str());
        system(fileName.getFileName() + " +map " + options.outputName.c_str());
    }

    extern "C" int main(int argc, char** argv)
    {
        Options options;
        int rc = 0;

        ParseOptions(options, argc, argv);

        if (options.input.empty() || options.outputContainer.empty() || options.outputName.empty())
        {
            fprintf(stderr, "usage: mapcompiler [+config ...] input=... outputContainer=... outputName=...\n"
                            "       [includeResources=1] [listResources=...] [outputPath=...] [runGame=...]\n"
                            "       [scale=...] [waitforkey=1]\n\n");
            return -1;
        }

        if (!SysInit(argc, argv) || !compile(options))
        {
            g_sys->DisplayError(g_eb, true);
            rc = -1;
        }

        SysShutdown();

        if (options.waitforkey)
            getchar();

        if (rc == 0 && !options.runGame.empty())
            RunGame(options);

        return rc;
    }
}
