#include <bleb/byteio_stdio.hpp>
#include <bleb/repository.hpp>

#include <extras/argument_parsing.hpp>
#include <reflection/basic_types.hpp>

#include <littl/File.hpp>

#include <memory>

#ifdef li_MSW
#define popen _popen
#define pclose _pclose
#endif

using String = std::string;
using std::unique_ptr;

unique_ptr<bleb::Repository> open(const String& path, bool canCreateNew) {
    FILE* f = bleb::StdioFileByteIO::getFile(path.c_str(), canCreateNew);

    if (!f) {
        fprintf(stderr, "ntile-restool: failed to open file '%s'\n", path.c_str());
        return nullptr;
    }

    auto io = new bleb::StdioFileByteIO(f, true);
    unique_ptr<bleb::Repository> repo(new bleb::Repository(io, true));

    if (!repo->open(canCreateNew)) {
        fprintf(stderr, "ntile-restool: failed to open repository in file '%s'\n", path.c_str());
        return nullptr;
    }

    return std::move(repo);
}

bool setString(bleb::Repository* repo, const char* objectName, const String& value)
{
    repo->setObjectContents(objectName, value.c_str(), value.length(), bleb::kPreferInlinePayload);

    return true;
}

struct MakeCommand {
    MakeCommand() {}

    String resource;
    String tool;
    String recipe;
    String sourceFile;
    String path;

    REFL_BEGIN("MakeCommand", 1)
        ARG_REQUIRED(resource,      "-resource",    "")
        ARG_REQUIRED(tool,          "-tool",        "")
        ARG_REQUIRED(recipe,        "-recipe",      "")
        ARG_REQUIRED(sourceFile,    "-source",      "")
        ARG_REQUIRED(path,          "",             "")
    REFL_END

    int execute() {
        // Process source file

        li::FileStat stat;
        if (!li::File::statFileOrDirectory(sourceFile.c_str(), &stat)) {
            fprintf(stderr, "failed to stat '%s'!\n", sourceFile.c_str());
            return 1;
        }

        // Invoke $(tool) $(source)

        String cmdLine = tool + " " + sourceFile;

        FILE* io = popen(cmdLine.c_str(), "rb");

        if (!io) {
            fprintf(stderr, "failed to run '%s'!\n", tool.c_str());
            return 1;
        }

        li::File objectList(io);

        auto repo = open(path, true);

        if (repo == nullptr)
            return -1;

        setString(repo.get(), "resource/mountPoint", resource);
        setString(repo.get(), "resource/sourceFile", sourceFile);
        setString(repo.get(), "resource/tool", tool);
        setString(repo.get(), "resource/recipe", recipe);
        setString(repo.get(), "resource/timestamp", std::to_string(stat.modificationTime));

        for (;;) {
            auto line = objectList.readLine();

            if (line.isEmpty())
                break;

            auto sep = line.findChar('\t');
            if (sep <= 0)
                continue;

            auto objectName = line.leftPart(sep);
            auto inputFile = line.dropLeftPart(sep + 1);

            std::unique_ptr<bleb::ByteIO> stream(repo->openStream(objectName.c_str(),
                    bleb::kStreamCreate | bleb::kStreamTruncate));
            assert(stream);
            size_t pos = 0;

            FILE* input = fopen(inputFile.c_str(), "rb");

            if (!input) {
                fprintf(stderr, "ntile-restool: failed to open '%s' for input\n", inputFile.c_str());
                return -1;
            }

            while (!feof(input)) {
                uint8_t in[4096];
                size_t got = fread(in, 1, sizeof(in), input);

                if (got) {
                    stream->setBytesAt(pos, in, got);
                    pos += got;
                }
            }
        }

        objectList.release();
        pclose(io);

        repo.reset();

        return 0;
    }
};

using argument_parsing::Command_t;
using argument_parsing::execute;
using argument_parsing::help;

static const Command_t commands[] = {
    {"make",         "compile a resource blebfile", execute<MakeCommand>, help<MakeCommand>},
    {}
};

int main(int argc, char* argv[]) {
    return argument_parsing::multiCommandDispatch(argc - 1, argv + 1, "ntile-restool", commands);
}

#include <reflection/default_error_handler.cpp>
