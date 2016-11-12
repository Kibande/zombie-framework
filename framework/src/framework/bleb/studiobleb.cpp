
#include "studiobleb.hpp"

#include <bleb/repository.hpp>
#include <bleb/byteio_stdio.hpp>

#include <littl/Directory.hpp>
#include <littl/File.hpp>

#include <framework/system.hpp>
#include <framework/utility/essentials.hpp>

namespace zfw
{
    static bool getString(bleb::Repository* repo, const char* key, li::String& str_out)
    {
        uint8_t* data = nullptr;
        size_t length = 0;

        repo->getObjectContents(key, data, length);

        if (!data)
            return false;

        str_out.set((const char*) data, length);
        free(data);

        return true;
    }

    static li::String s_Escape(const li::String& arg)
    {
        return "\"" + arg + "\"";
    }

    bool StudioBlebManager::Init(ISystem* sys)
    {
        this->sys = sys;

        return true;
    }

    void StudioBlebManager::MountAllInDirectory(const char* nativePath)
    {
        unique_ptr<li::Directory> dir(li::Directory::open(nativePath));

        if (!dir)
            return;

        li::List<li::String> entries;
        dir->list(entries);

        const auto base = (li::String) nativePath + "/";

        for (const auto& entry : entries)
        {
            if (entry.beginsWith('.'))
                continue;

            MountBleb(base + entry);
        }
    }

    bool StudioBlebManager::MountBleb(const char* nativePath)
    {
        // Open the blebfile first, to get sourceFile and recipe

        FILE* f = fopen(nativePath, "rb");
        
        if (!f)
        {
            sys->Printf(kLogError, "StudioBleb Error: cannot open %s", nativePath);
            return false;
        }

        bleb::StdioFileByteIO bio(f, true);
        bleb::Repository repo(&bio, false);

        if (!repo.open(false))
        {
            sys->Printf(kLogError, "Bleb error: %s", repo.getErrorDesc());
            return false;
        }

        li::String mountPoint, sourceFile, recipe, timestamp, tool;
        if (!getString(&repo, "resource/mountPoint", mountPoint)
                || !getString(&repo, "resource/sourceFile", sourceFile)
                || !getString(&repo, "resource/recipe", recipe)
                || !getString(&repo, "resource/timestamp", timestamp)
                || !getString(&repo, "resource/tool", tool))
        {
            sys->Printf(kLogError, "StudioBleb Error: incomplete resource blebfile (mountPoint=%s sourceFile=%s recipe=%s tool=%s)",
                    mountPoint.c_str(), sourceFile.c_str(), recipe.c_str(), tool.c_str());
            return false;
        }

        repo.close();
        bio.close();

        // Remount bleb as a filesystem
        shared_ptr<IFileSystem> fs = sys->CreateBlebFileSystem(nativePath, kFSAccessStat | kFSAccessRead);

        if (!fs)
            return false;

        mountedBlebs.emplace_back();
        auto& res = mountedBlebs.back();
        res.nativePath = nativePath;
        res.mountPoint = mountPoint;
        res.sourceFile = move(sourceFile);
        res.recipe = move(recipe);
        res.tool = move(tool);
        res.fs = fs;
        res.timestamp = strtol(timestamp.c_str(), NULL, 10);

        sys->GetFSUnion()->AddFileSystem(std::move(fs), 0, move(mountPoint) + "/");

        return true;
    }

    bool StudioBlebManager::Refresh()
    {
        bool modified = false;

        for (auto& res : mountedBlebs)
        {
            li::FileStat stat;
            if (!li::File::statFileOrDirectory(res.sourceFile, &stat))
            {
                sys->Printf(kLogWarning, "StudioBleb Warning: failed to re-stat '%s'", res.sourceFile.c_str());
                stat.modificationTime = 0;
            }

            if (stat.modificationTime > res.timestamp && RefreshResource(res))
            {
                modified = true;
                res.timestamp = stat.modificationTime;
            }
        }

        return modified;
    }

    bool StudioBlebManager::RefreshResource(MountedBleb_t& res)
    {
        auto fsUnion = sys->GetFSUnion();
        fsUnion->RemoveFileSystem(res.fs.get());
        res.fs.reset();

        // Rebuild resource
        li::String cmdline = "ntile-restool make -resource " + s_Escape(res.mountPoint)
                + " -tool " + s_Escape(res.tool)
                + " -recipe " + s_Escape(res.recipe)
                + " -source " + s_Escape(res.sourceFile)
                + " " + s_Escape(res.nativePath);

        sys->Printf(kLogDebugInfo, "%s", cmdline.c_str());

        if (system(cmdline) != 0)
            sys->Printf(kLogWarning, "StudioBleb Warning: failed to remake '%s'", res.sourceFile.c_str());

        // Mount anew
        shared_ptr<IFileSystem> fs = sys->CreateBlebFileSystem(res.nativePath, kFSAccessStat | kFSAccessRead);

        if (!fs)
            return false;

        res.fs = fs;
        sys->GetFSUnion()->AddFileSystem(std::move(fs), 0, res.mountPoint + "/");

        return true;
    }
}
