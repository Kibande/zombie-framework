
#include "private.hpp"

#include <framework/entity.hpp>
#include <framework/entityhandler.hpp>
#include <framework/errorbuffer.hpp>
#include <framework/errorcheck.hpp>
#include <framework/event.hpp>
#include <framework/filesystem.hpp>
#include <framework/mediacodechandler.hpp>
#include <framework/modulehandler.hpp>
#include <framework/nativedialogs.hpp>
#include <framework/profiler.hpp>
#include <framework/scene.hpp>
#include <framework/system.hpp>
#include <framework/timer.hpp>
#include <framework/varsystem.hpp>
#include <framework/videohandler.hpp>

#include <framework/utility/params.hpp>

#ifdef ZOMBIE_CTR
#include <framework/ctr/ctr.hpp>
#endif

#include <littl/CompilerVersion.hpp>
#include <littl/Directory.hpp>
#include <littl/File.hpp>
#include <littl/FileName.hpp>
#include <littl/PerfTiming.hpp>
#include <littl/Thread.hpp>

#include <reflection/basic_types.hpp>

#include <cstdarg>

#if defined(_MSC_VER)
#include <crtdbg.h>
#else
#include <csignal>
#endif

#ifdef ZOMBIE_EMSCRIPTEN
#include <emscripten.h>
#endif

#ifdef _MSC_VER
#pragma comment(linker, \
    "\"/manifestdependency:type='Win32' "\
    "name='Microsoft.Windows.Common-Controls' "\
    "version='6.0.0.0' "\
    "processorArchitecture='*' "\
    "publicKeyToken='6595b64144ccf1df' "\
    "language='*'\"")
#endif

namespace zfw
{
    using namespace li;

    // keep in sync with 'ExceptionType' in framework.hpp
    static const char* exceptionNames[] = {
        "EX_NO_ERROR",
        "EX_ACCESS_DENIED",
        "EX_ALLOC_ERR",
        "EX_ASSERTION_FAILED",
        "EX_ASSET_CORRUPTED",
        "EX_ASSET_FORMAT_UNSUPPORTED",
        "EX_ASSET_OPEN_ERR",
        "EX_BACKEND_ERR",
        "EX_CONTROLLER_UNAVAILABLE",
        "EX_DATAFILE_ERR",
        "EX_DATAFILE_FORMAT_ERR",
        "EX_DOCUMENT_MALFORMED",
        "EX_FEATURE_DISABLED",
        "EX_INTERNAL_STATE",
        "EX_INVALID_ARGUMENT",
        "EX_INVALID_OPERATION",
        "EX_IO_ERROR",
        "EX_LIBRARY_INTERFACE_ERR",
        "EX_LIBRARY_OPEN_ERR",
        "EX_LIST_OPEN_ERR",
        "EX_NOT_FOUND",
        "EX_NOT_IMPLEMENTED",
        "EX_OBJECT_UNDEFINED",
        "EX_OPERATION_FAILED",
        "EX_SCRIPT_ERROR",
        "EX_SERIALIZATION_ERR",
        "EX_SHADER_COMPILE_ERR",
        "EX_SHADER_LOAD_ERR",
        "EX_WRITE_ERR",

        // New (2013/08)
        "ERR_DATA_FORMAT",
        "ERR_DATA_SIZE",
        "ERR_DATA_STRUCTURE",

        // New (2014+)
        "errApplication",
        "errDataCorruption",
        "errDataNotFound",
        "errNoSuitableCodec",
        "errRequestedEncodingNotSupported",
        "errVariableAlreadyBound",
        "errVariableNotBound",
        "errVariableNotSet",

        ""
    };

    static const char* logTypeNames[] =
    {
        "",
        "",
        "ERROR",
        "WARNING",
        "INFO",
        "DEBUG",
    };

    static const int MAX_FRAME_TICKS = 10;  // (10 fps at tickrate 100)

    struct LogEntry
    {
        uint64_t time;
        LogType_t type;
        char* line;
    };

    static ErrorBuffer_t* s_eb;

    static int frameCounter;
    static unique_ptr<Timer> frameTimer;
    static int tickRate, tickAccum;
    static double tickTime, timeAccum;

    static bool breakLoop, changeScene;
    static shared_ptr<IScene> scene, newScene;

    static Array<char> printfbuf;

    //static Var_t* dev_errorbreak;
    
    static PerfTimer timer;
    static PerfTimer::Counter clock0;
    static time_t time0;

    static List<LogEntry> log;

    static ProfilingSection_t profDrawScene = {"IScene::DrawFrame"};
    static ProfilingSection_t profOnFrame = {"IScene::OnFrame"};
    static ProfilingSection_t profOnTicks = {"IScene::OnTicks"};
    static ProfilingSection_t profVideoHandler = {"VideoHandler"};
    static ProfilingSection_t profVideoHandler2 = {"VideoHandler"};

    // ====================================================================== //
    //  class declaration(s)
    // ====================================================================== //

    class System : public ISystem, public IEssentials
    {
        public:
            virtual bool Init(ErrorBuffer_t* eb, int flags) override;
            virtual void Shutdown() override;

            virtual IEssentials*    GetEssentials() override { return this; }

            virtual void            AssertionFail(const char* expr, const char* functionName, const char* file,
                int line, bool isDebugAssertion) override;
            virtual void            AssertionFailResourceState(const char* resourceName, int actualState, int expectedState,
                                                               const char* functionName, const char* file, int line) override;
            virtual void            ErrorAbort() override;
            virtual ErrorBuffer_t*  GetErrorBuffer() override { return s_eb; }

            // Available pre initialization
            virtual IVarSystem*         GetVarSystem() override { return varSystem.get(); }

            virtual bool            Startup() override;

            // Core Handlers
            virtual IEntityHandler* GetEntityHandler(bool createIfNull) override;
            virtual IFileSystem*    GetFileSystem() override { return fsUnion->GetFileSystem(); }
            virtual IMediaCodecHandler* GetMediaCodecHandler(bool createIfNull) override;
            virtual IModuleHandler* GetModuleHandler(bool createIfNull) override;
            virtual IFSUnion*       GetFSUnion() override { return fsUnion.get(); }
            virtual IVideoHandler*  GetVideoHandler() override { return videoHandler.get(); }
            virtual void            SetVideoHandler(unique_ptr<IVideoHandler>&& handler) { videoHandler = move(handler); }
            //virtual IEntityHandler* InitEntityHandler() override;
            //virtual IModuleHandler* InitModuleHandler() override;

            virtual shared_ptr<IFileSystem> CreateStdFileSystem(const char* basePath, int access) override;

#ifdef ZOMBIE_WITH_BLEB
            virtual shared_ptr<IFileSystem> CreateBlebFileSystem(const char* path, int access) override;
#endif

            // Main loop
            virtual void ChangeScene(shared_ptr<IScene> scene) override;
            virtual void RunMainLoop() override;
            virtual void StopMainLoop() override { breakLoop = true; }
            virtual void ReleaseScene() override;

            // Logging functions
            virtual int Printf(LogType_t logType, const char* format, ...) override;
            virtual int Printfv(LogType_t logType, const char* format, va_list args) override;
            virtual void Log(LogType_t logType, const char* format, ...) override;
            virtual void Logv(LogType_t logType, const char* format, va_list args, size_t length) override;

            // Error handling functions
            virtual void DisplayError(const ErrorBuffer_t* eb, bool fatal) override;
            virtual void PrintError(const ErrorBuffer_t* eb, LogType_t logType) override;

            // Localization functions
            virtual const char* Localize(const char* text) override { return text; }

            // Utility
            virtual void DebugBreak(bool force) override;
            virtual void ParseArgs1(int argc, char* argv[]) override;

            // I/O Utility
            virtual bool CreateDirectoryRecursive(const char* path) override;
            virtual InputStream* OpenInput(const char* path) override;
            virtual OutputStream* OpenOutput(const char* path) override;

            // Timer management
            virtual uint64_t GetGlobalMicros() override;

            virtual int GetFrameCounter() override { return frameCounter; }
            virtual Profiler* GetProfiler() override { return profiler.get(); }
            virtual bool IsProfiling() override { return frameCounter == profileFrame; }
            virtual void ProfileFrame(int frameNumber) override { profileFrame = frameNumber; }

            // Utility Classes
            virtual IResourceManager*   CreateResourceManager(const char* name) override;
            virtual IResourceManager2*  CreateResourceManager2() override;
#ifndef ZOMBIE_NO_SHADER_PREPROCESSOR
            virtual ShaderPreprocessor* CreateShaderPreprocessor() override { return p_CreateShaderPreprocessor(this); }
#else
            virtual ShaderPreprocessor* CreateShaderPreprocessor() override { zombie_assert(false); }
#endif
            virtual Timer*              CreateTimer() override { return p_CreateTimer(s_eb); }

            void AddFileSystem(shared_ptr<IFileSystem> fs, int priority) { fsUnion->AddFileSystem(move(fs), priority); }
            bool OpenFileStream(const char* path, int access,
                    InputStream** is_out,
                    OutputStream** os_out,
                    IOStream** io_out);

#ifndef ZOMBIE_CTR
            // TODO: where to shove this?
            void DisplayApplicationError(const char* timestamp, int errorCode, const char* description,
                const char* solution, const char* additionalParameters, bool fatal, bool nonInteractive);
#endif

        private:
            void p_ClearLog();
			bool p_Frame();
            void p_SetTickRate(int tickrate);
            double p_Update();
            void ExecLine1(const String& line);

			static void p_EmscriptenFrame();

            bool interactive;

            // core handlers
            shared_ptr<IEntityHandler> entityHandler;
            unique_ptr<IMediaCodecHandler> mediaCodecHandler;
            shared_ptr<IModuleHandler> moduleHandler;
            unique_ptr<IFSUnion> fsUnion;
            unique_ptr<IVarSystem> varSystem;
            unique_ptr<IVideoHandler> videoHandler;

            // synchronization
            li::Mutex ioMutex, logMutex, stdoutMutex;

            // profiling
            unique_ptr<Profiler> profiler;
            int profileFrame;
    };

    static unique_ptr<System> s_sys;

    static void DumpLog(OutputStream* stream)
    {
        iterate2 (i, log)
        {
            auto& entry = *i;

            auto t = entry.time - clock0;
            stream->write((const char*) sprintf_t<63>("%-10s[%5u.%04u] ", logTypeNames[entry.type], (int)(t / 1000000), (int)(t % 1000000) / 100));
            stream->writeLine(entry.line);
        }
    }

    static const char* GetErrorName(int errorCode)
    {
        if (errorCode >= 0 && errorCode < MAX_EXCEPTION)
            return exceptionNames[errorCode];
        else
            return "unknown_exception";
    }

    const char* NormalizePath(const char* path)
    {
        // FIXME - SECURITY: Check & normalize path!

        return path;
    }

    // ====================================================================== //
    //  class System
    // ====================================================================== //

    ISystem* CreateSystem()
    {
#ifdef ZOMBIE_CTR
        CtrPrintInit();
#endif

        s_sys.reset(new System);
        return s_sys.get();
    }

    bool System::Init(ErrorBuffer_t* eb, int flags)
    {
        SetEssentials(this->GetEssentials());

        s_eb = eb;
        interactive = !(flags & kSysNonInteractive);

        frameCounter = 0;
        changeScene = false;

        scene = nullptr;
        newScene = nullptr;
        
        entityHandler = nullptr;
        fsUnion = nullptr;
        moduleHandler = nullptr;
        videoHandler = nullptr;

        // Enter Pre-Init
        clock0 = timer.getCurrentMicros();
        time(&time0);

        Printf(kLogAlways, "Zombie Framework " ZOMBIE_BUILDNAME " " ZOMBIE_PLATFORM "-" ZOMBIE_BUILDTYPENAME);
        Printf(kLogAlways, "Copyright (c) 2012, 2013, 2014, 2016 Minexew Games; some rights reserved");
        Printf(kLogAlways, "Compiled using " li_compiled_using);

#if ZOMBIE_API_VERSION >= 201601
		Printf(kLogAlways, "API version %04d.%02d", ZOMBIE_API_VERSION / 100, ZOMBIE_API_VERSION % 100);
#endif

        // Pre-Init
        varSystem.reset(p_CreateVarSystem(this));
        varSystem->SetVariable("sys_tickrate", "60", 0);

        if (!(flags & kSysNoInitFileSystem))
            fsUnion.reset(p_CreateFSUnion(s_eb));

        return true;
    }

    bool System::Startup()
    {
        //dev_errorbreak =        Var::LockInt("dev_errorbreak", false, 0);

        //printk("Using output path: %s", outputFs.path.c_str());

        //Var::SetStr("dev_breakkey", "0:0:0013:0045");

        int sys_tickrate;

        ErrorPassthru(varSystem->GetVariable("sys_tickrate", &sys_tickrate, IVarSystem::kVariableMustExist));
        p_SetTickRate(sys_tickrate);

        profileFrame = -1;
        profiler.reset(Profiler::Create());

        frameTimer.reset(CreateTimer());

        return true;
    }

    void System::Shutdown()
    {
        videoHandler.reset();

        profiler.reset();

        entityHandler.reset();
        mediaCodecHandler.reset();
        moduleHandler.reset();
        fsUnion.reset();

        varSystem.reset();

        p_ClearLog();
    }

    void System::AssertionFail(const char* expr, const char* functionName, const char* file, int line,
            bool isDebugAssertion)
    {
        // Feel free to go wild, You Only AssertFail Once!

        ErrorBuffer::SetError2(g_essentials->GetErrorBuffer(),
                zfw::EX_ASSERTION_FAILED, 4,
                "desc",     sprintf_4095("Failed%s assertion `%s`", isDebugAssertion ? " DEBUG" : "", expr),
                "function", functionName,
                "file",     file,
                "line",     sprintf_15("%i", line)
        );

        g_essentials->ErrorAbort();
    }

    void System::AssertionFailResourceState(const char* resourceName, int actualState, int expectedState,
                                            const char* functionName, const char* file, int line) {
        const char* resourceStateNames[] = { "CREATED", "BOUND", "PRELOADED", "REALIZED", "RELEASED" };

        ErrorBuffer::SetError2(g_essentials->GetErrorBuffer(),
                               zfw::EX_ASSERTION_FAILED, 4,
                               "desc",     sprintf_4095("Failed assertion: Invalid resource state for `%s` - expected `%s`, is `%s`",
                                                        resourceName, resourceStateNames[expectedState], resourceStateNames[actualState]),
                               "function", functionName,
                               "file",     file,
                               "line",     sprintf_15("%i", line)
        );

        g_essentials->ErrorAbort();
    }

    void System::ChangeScene(shared_ptr<IScene> scene)
    {
        newScene = move(scene);
        changeScene = true;
    }

    bool System::CreateDirectoryRecursive(const char* path)
    {
        const char* normalizedPath = NormalizePath(path);

        std::vector<char> buffer_(strlen(normalizedPath) + 1);
        char* buffer = &buffer_[0];
        const char* path_end = path, * slash;

        do
        {
            slash = strchr(path_end, '/');

            if (slash != nullptr)
            {
                memcpy(buffer, path, slash - path);
                buffer[slash - path] = 0;

                path_end = slash + 1;
            }
            else
                strcpy(buffer, path);

            unique_ptr<IDirectory> dir(fsUnion->GetFileSystem()->OpenDirectory(buffer, kDirectoryMayCreate));

            if (dir == nullptr)
                return false;
        }
        while (slash != nullptr);

        return true;
    }

    IResourceManager* System::CreateResourceManager(const char* name)
    {
        return p_CreateResourceManager(s_eb, this, name);
    }

    IResourceManager2* System::CreateResourceManager2()
    {
        return p_CreateResourceManager2(this);
    }

#ifdef ZOMBIE_WITH_BLEB
    shared_ptr<IFileSystem> System::CreateBlebFileSystem(const char* path, int access)
    {
        return p_CreateBlebFileSystem(this, path, access);
    }
#endif

    shared_ptr<IFileSystem> System::CreateStdFileSystem(const char* basePath_in, int access)
    {
        String basePath = FileName::getAbsolutePath(basePath_in);

        if (!basePath.endsWith('/') && !basePath.endsWith('\\'))
            basePath += UnicodeChar('/');

        return p_CreateStdFileSystem(s_eb, basePath, access);
    }

    void System::DebugBreak(bool force)
    {
#if defined(_MSC_VER)
        if (force || IsDebuggerPresent())
            //_CrtDbgBreak();
            __asm { int 3 };
#else
        if (force /*|| VAR_INT(dev_errorbreak) > 0*/)
            __builtin_trap();
#endif
    }

#ifndef ZOMBIE_CTR
    void System::DisplayApplicationError(const char* timestamp, int errorCode, const char* description,
        const char* solution, const char* additionalParameters, bool fatal, bool nonInteractive)
    {
        const char* appName = varSystem->GetVariableOrEmptyString("appName");

        String occuredWhere = (appName != nullptr) ? (String) "in " + appName : (String)Localize("in the application");

#ifdef ZOMBIE_WINNT
        char tempPath[MAX_PATH + 1];
        GetTempPathA(sizeof(tempPath), tempPath);

        String errorReportFilename = (String)tempPath + "ErrorReport.txt";
#else
        String errorReportFilename = "/tmp/ErrorReport.txt";
#endif

        File errorReport(errorReportFilename, true);

        if (errorReport)
        {
            errorReport.writeLine("=== Application Error Report ===");
            errorReport.writeLine();

            if (fatal)
                errorReport.write("  A fatal error occurred ");
            else
                errorReport.write("  An error occurred ");

            errorReport.write(occuredWhere);
            errorReport.writeLine(".");
            errorReport.writeLine();

            errorReport.write("  Error description:\t");
            errorReport.writeLine(description);

            if (solution != nullptr)
            {
                errorReport.write("  Suggested solution:\t");
                errorReport.writeLine(solution);
            }

            errorReport.writeLine();

            errorReport.write("  Timestamp:\t");
            errorReport.writeLine(timestamp);

            errorReport.write("  Error code:\t");

            if (errorCode > 0 && errorCode < MAX_EXCEPTION)
                errorReport.writeLine(exceptionNames[errorCode]);
            else
                errorReport.writeLine((const char*)sprintf_t<15>("%i", errorCode));

            if (additionalParameters != nullptr)
            {
                errorReport.write("  Additional parameters:\t");
                errorReport.writeLine(additionalParameters);
            }

            errorReport.writeLine();
            errorReport.writeLine();
            errorReport.writeLine("=== Application Log ===");
            errorReport.writeLine();
            errorReport.write("  Application started on ");
            errorReport.write(Util::FormatTimestamp(&time0));
            errorReport.writeLine(".");
            errorReport.writeLine();

            DumpLog(&errorReport);
            errorReport.close();
        }

        String message = (fatal ? "A fatal error occurred " : "An error occured ") + occuredWhere + ":\n\n";
        message += description;
        message += "\n";

        if (solution != nullptr)
        {
            message += solution;
            message += "\n";
        }

        message += "\n";

        if (errorCode == EX_ASSERTION_FAILED || errorCode == EX_INTERNAL_STATE)
            message += "Please report this error to the application developer.\n";
        else
            message += "If you believe this is a bug, please report it to the application developer.\n";

#if NATDLG_GOOD_SHELL_OPEN_FILE
        if (!nonInteractive)
#else
        if (false)
#endif
            message += "Would you like to display the full error report now?";
        else
            message += "An error report was saved in " + errorReportFilename + ".";

#if defined(ZOMBIE_WINNT) && defined(ZOMBIE_DEBUG)
        if (!nonInteractive)
            message += "\n(press Cancel to debug)";
#endif

        if (!nonInteractive)
        {
            int rc = NativeDialogs::ErrorWindow(message, fatal ? 1 : 0);

            if (rc == ERRWND_OPEN_REPORT)
                NativeDialogs::ShellOpenFile(errorReportFilename);
            else if (rc == ERRWND_DEBUG_BREAK)
                DebugBreak(true);
        }
        else
            fprintf(stderr, "\n\n%s\n", message.c_str());
    }
#endif

    void System::DisplayError( const ErrorBuffer_t* eb, bool fatal )
    {
        String description, solution, additionalParameters;

        const char* params = eb->params;

        const char *key, *value;

        while (Params::Next(params, key, value))
        {
            if (strcmp(key, "desc") == 0)
                description = value;
            else if (strcmp(key, "solution") == 0)
                solution = value;
            else
            {
                if (!additionalParameters.isEmpty())
                    additionalParameters += ",";

                // TODO: This should be escaped really
                additionalParameters += key;
                additionalParameters += "=";
                additionalParameters += value;
            }
        }

        // TODO: Externally? (availability would be a problem though)

        if (description.isEmpty())
        {
            switch (eb->errorCode)
            {
                case EX_NOT_IMPLEMENTED:
                    description = Localize("This functionality is currently not implemented.");
                    break;
            }
        }

        if (solution.isEmpty())
        {
            switch (eb->errorCode)
            {
                case EX_ASSET_CORRUPTED:
                case EX_ASSET_OPEN_ERR:
                case EX_LIBRARY_INTERFACE_ERR:
                case EX_LIBRARY_OPEN_ERR:
                    solution = Localize("Reinstalling the application might fix this problem.");
                    break;

                case EX_LIST_OPEN_ERR:
                    solution = Localize("Please make sure the application is being started"
                            " from the correct directory.");
                    break;

                case EX_NOT_IMPLEMENTED:
                    solution = Localize("Please make sure you are using an up-to-date version"
                            " of the application.");
                    break;

                case errVariableNotSet:
                    solution = Localize("Resetting application configuration"
                            " or reinstalling the application might fix this problem.");
                    break;

                case EX_WRITE_ERR:
                    solution = Localize("Please make sure your disk is not full and you have"
                            " write permissions to the relevant directory.");
                    break;
            }
        }

#ifndef ZOMBIE_CTR
        DisplayApplicationError(eb->timestamp, eb->errorCode, description, solution, additionalParameters, fatal, !interactive);
#else
        Printf(kLogError, "Error %i: %s", eb->errorCode, GetErrorName(eb->errorCode));
        Printf(kLogError, "%s", description.c_str());
        if (!solution.isEmpty())
            Printf(kLogError, "%s", solution.c_str());
        Printf(kLogError, "%s", additionalParameters.c_str());

       /* Printf(kLogError, "Error %i", eb->errorCode);
        Printf(kLogError, "Params: %s", eb->params);
        */
#endif
    }

    void System::ErrorAbort()
    {
        DisplayError(s_eb, true);

#ifdef ZOMBIE_CTR
        while (!(hidKeysDown() & KEY_START))
        {
            aptMainLoop();
            hidScanInput();
        }

        svcExitProcess();
#endif

#ifdef ZOMBIE_EMSCRIPTEN
		emscripten_cancel_main_loop();
#endif

        exit(-1);
    }

    void System::ExecLine1(const String& line)
    {
        List<String> tokens;
        String buffer;
        size_t index = 0;
        unsigned numTokens = 0;
        bool enclosed = false, escaped = false;

        // TODO: proper parsing and a call to Sys::Exec

        while (index < line.getNumBytes())
        {
            const UnicodeChar next = line.getChar(index);

            if (next == Unicode::invalidChar)
                break;
            else if (next == '\\' && !escaped)
                escaped = true;
            else if (next == '"' && !escaped)
                enclosed = !enclosed;
            else if (next == ' ' && !enclosed && !escaped)
            {
                if (!buffer.isEmpty())
                {
                    tokens.add(buffer);
                    buffer.clear();
                    numTokens++;
                }
            }
            else
            {
                buffer.append(next);
                escaped = false;
            }
        }

        if (!buffer.isEmpty())
            tokens.add(buffer);

        if (tokens[0] == "fs_fullaccess")
        {
            Directory::create(tokens[1]);
            auto fs = CreateStdFileSystem(tokens[1], kFSAccessAll);
            AddFileSystem(fs, 100);
            return;
        }
        else if (tokens[0] == "fs_readonly")
        {
            auto fs = CreateStdFileSystem(tokens[1], kFSAccessStat | kFSAccessRead);
            AddFileSystem(fs, 100);
            return;
        }

        auto var = GetVarSystem();

        if (tokens.getLength() == 1)
            var->SetVariable(tokens[0], "1", 0);
        else if (tokens.getLength() > 1)
            var->SetVariable(tokens[0], tokens[1], 0);
    }

    IEntityHandler* System::GetEntityHandler(bool createIfNull)
    {
        if (createIfNull && entityHandler == nullptr)
            entityHandler.reset(p_CreateEntityHandler(s_eb, this));

        return entityHandler.get();
    }

    uint64_t System::GetGlobalMicros()
    {
        return frameTimer->GetGlobalMicros();
    }

    IMediaCodecHandler* System::GetMediaCodecHandler(bool createIfNull)
    {
        if (createIfNull && mediaCodecHandler == nullptr)
        {
            mediaCodecHandler.reset(p_CreateMediaCodecHandler());

            // built-in codecs

            mediaCodecHandler->RegisterEncoder(typeID<IPixmapEncoder>(), unique_ptr<IEncoder>(p_CreateBmpEncoder(this)));

#ifdef ZOMBIE_WITH_JPEG
            mediaCodecHandler->RegisterDecoder(typeID<IPixmapDecoder>(), unique_ptr<IDecoder>(p_CreateJfifDecoder()));
#endif
#ifdef ZOMBIE_WITH_LODEPNG
            mediaCodecHandler->RegisterDecoder(typeID<IPixmapDecoder>(), unique_ptr<IDecoder>(p_CreateLodePngDecoder(this)));
            mediaCodecHandler->RegisterEncoder(typeID<IPixmapEncoder>(), unique_ptr<IEncoder>(p_CreateLodePngEncoder(this)));
#endif
        }

        return mediaCodecHandler.get();
    }

    IModuleHandler* System::GetModuleHandler(bool createIfNull)
    {
#ifndef ZOMBIE_NO_MODULEHANDLER
        if (createIfNull && moduleHandler == nullptr)
            moduleHandler.reset(p_CreateModuleHandler(s_eb));
#endif

        return moduleHandler.get();
    }

    /*
    IEntityHandler* System::InitEntityHandler()
    {
        zfw::Release(entityHandler);

        entityHandler = p_CreateEntityHandler(s_eb);

        return entityHandler;
    }

    IModuleHandler* System::InitModuleHandler()
    {
        zfw::Release(moduleHandler);

        moduleHandler = p_CreateModuleHandler(s_eb);

        return moduleHandler;
    }
    */

    void System::Log(LogType_t logType, const char* format, ...)
    {
        va_list args;

        va_start( args, format );
        size_t length = vsnprintf( nullptr, 0, format, args );
        va_end( args );
        
        va_start( args, format );
        Logv(logType, format, args, length);
        va_end( args );
    }

    void System::Logv(LogType_t logType, const char* format, va_list args, size_t length)
    {
        li::CriticalSection lg(logMutex);

        auto& entry = log.addEmpty();
        entry.time = timer.getCurrentMicros();
        entry.type = logType;

        entry.line = (char *) malloc(length + 1);
        vsnprintf( entry.line, length + 1, format, args );
    }

    bool System::OpenFileStream(const char* path, int flags,
            InputStream** is_out,
            OutputStream** os_out,
            IOStream** io_out)
    {
        ZFW_ASSERT(fsUnion != nullptr)

        li::CriticalSection lg(ioMutex);

        const char* normalizedPath = NormalizePath(path);

        return fsUnion->GetFileSystem()->OpenFileStream( normalizedPath, flags, is_out, os_out, io_out );
    }

    InputStream* System::OpenInput( const char* path )
    {
        InputStream* is = nullptr;

        OpenFileStream(path, 0, &is, nullptr, nullptr);

        return is;
    }

    OutputStream* System::OpenOutput( const char* path )
    {
        OutputStream* os = nullptr;

        OpenFileStream(path, kFileMayCreate | kFileTruncate, nullptr, &os, nullptr);

        return os;
    }

    void System::p_ClearLog()
    {
        iterate2(i, log)
            free((*i).line);

        log.clear();
    }

	void System::p_EmscriptenFrame()
	{
		s_sys->p_Frame();
	}

	bool System::p_Frame()
	{
		IVideoHandler* videoHandler = GetVideoHandler();

		if (frameCounter == profileFrame)
			profiler->BeginProfiling();

		if (changeScene)
		{
			if (scene != nullptr)
			{
				scene->Shutdown();
				scene.reset();
			}

			scene = move(newScene);
			changeScene = false;

			if (scene != nullptr && !scene->Init())
			{
				DisplayError(s_eb, true);
				return false;
			}
		}

		if (scene == nullptr)
		{
			ErrorBuffer::SetError(s_eb, EX_INVALID_OPERATION,
				"desc", "No scene was specified by the application.",
				nullptr);
			DisplayError(s_eb, true);
			return false;
		}

		if (frameCounter == profileFrame)
			profiler->EnterSection(profVideoHandler);

		videoHandler->BeginFrame();
		videoHandler->ReceiveEvents();
		videoHandler->BeginDrawFrame();

		if (frameCounter == profileFrame)
		{
			profiler->LeaveSection();
			profiler->EnterSection(profOnFrame);
		}

		scene->OnFrame(p_Update());

		if (frameCounter == profileFrame)
		{
			profiler->LeaveSection();
			profiler->EnterSection(profOnTicks);
		}

		if (tickAccum > 0)
			scene->OnTicks(tickAccum);

		if (frameCounter == profileFrame)
		{
			profiler->LeaveSection();
			profiler->EnterSection(profDrawScene);
		}

		scene->DrawScene();

		if (frameCounter == profileFrame)
		{
			profiler->LeaveSection();
			profiler->EnterSection(profVideoHandler2);
		}

		videoHandler->EndFrame(tickAccum);
		tickAccum = 0;

		if (frameCounter == profileFrame)
		{
			profiler->LeaveSection();
			profiler->EndProfiling();

			Printf(kLogInfo, "Profiling frame %d:", frameCounter);
			profiler->PrintProfile();
		}

		// Prevent overflows into negative
		if (++frameCounter < 0)
			frameCounter = 0;

		return true;
	}

    void System::p_SetTickRate(int tickrate)
    {
        tickRate = tickrate;
        tickAccum = 0;

        tickTime = 1.0 / tickrate;
        timeAccum = 0.0;
    }

    double System::p_Update()
    {
        double t;

        if (frameTimer->IsStarted())
        {
            t = frameTimer->GetMicros() / 1000000.0;
            frameTimer->Reset();
        }
        else
        {
            t = 0.0;
            frameTimer->Start();
        }

        timeAccum += t;                                     // Add to time accumulator
        int fullTicks = (int)(timeAccum / tickTime);        // calculate elapsed ticks
        timeAccum -= fullTicks * tickTime;                  // subtract back from accum
        tickAccum += glm::min(fullTicks, MAX_FRAME_TICKS);  // resetting this isn't our job though

        return t;
    }

    void System::ParseArgs1(int argc, char **argv)
    {
#ifndef ZOMBIE_CTR
        String line;

        for (int i = 1; i < argc; i++)
        {
            if (argv[i][0] == '+' || argv[i][0] == '-')
            {
                if (!line.isEmpty())
                    ExecLine1(line);

                line = argv[i] + 1;
            }
            else
            {
                line += " ";
                line += argv[i];
            }
        }

        if (!line.isEmpty())
            ExecLine1(line);
#endif
    }

    void System::PrintError( const ErrorBuffer_t* eb, LogType_t logType )
    {
        const char* desc;

        if (!Params::GetValueForKey(eb->params, "desc", desc))
            desc = "Unknown error.";

        Printf(logType, "%s (%s)", GetErrorName(eb->errorCode), desc);

        const char* params = eb->params;

        const char *key, *value;

        while (Params::Next(params, key, value))
        {
            if (strcmp(key, "desc") != 0)
                Printf(logType, "%s:\t%s", key, value);
        }
    }

    int System::Printf(LogType_t logType, const char* format, ...)
    {
        va_list args;
        
        va_start(args, format);
        int r = Printfv(logType, format, args);
        va_end(args);
        
        va_start( args, format );
        size_t length = vsnprintf( nullptr, 0, format, args );
        va_end( args );
        
        if (logType != kLogNever)
        {
            va_start(args, format);
            Logv(logType, format, args, length);
            va_end(args);
        }

        return r;
    }

    int System::Printfv(LogType_t logType, const char* format, va_list args)
    {
        li::CriticalSection lg(stdoutMutex);

        auto t = timer.getCurrentMicros() - clock0;

#ifndef ZOMBIE_CTR
        printf("[%4u.%04u] ", (int)(t / 1000000), (int)(t % 1000000) / 100/*, logTypeNames[logType]*/);
        int res = vfprintf(stdout, format, args);
        printf("\n");
#else
        char timestamp[20];
        sprintf(timestamp, "[%5u.%03u] ", (int)(t / 1000000), (int)(t % 1000000) / 1000/*, logTypeNames[logType]*/);
        CtrPrints(timestamp);
        int res = CtrPrintfv(format, args);
        CtrPrints("\n");
#endif

        return res;
    }

    void System::ReleaseScene()
    {
        if (scene != nullptr)
        {
            scene->Shutdown();
            scene.reset();
        }
    }

    void System::RunMainLoop()
    {
#ifndef ZOMBIE_EMSCRIPTEN
        breakLoop = false;

        while (!breakLoop)
        {
			if (!p_Frame())
				break;
        }
#else
		emscripten_set_main_loop(p_EmscriptenFrame, 0, 1);
#endif
    }

    /*void Sys::ApplicationError( int errorCode, ... )
    {
        va_list args;

        if (Var::GetInt("dev_errorbreak", false, -1) > 0)
            Sys::DebugBreak(true);
        
        AppError err;
        time(&err.timestamp);
        err.errorCode = errorCode;
        
        unsigned int count;

        va_start( args, errorCode );
        for (count = 0; ; count++)
        {
            if (va_arg(args, const char*) == nullptr
                    || va_arg(args, const char*) == nullptr)
                break;
        }
        va_end( args );

        va_start( args, errorCode );
        size_t length = Params::CalculateLength(count, args);
        va_end( args );

        va_start( args, errorCode );
        char* params = Params::BuildAllocv(count, length, args);
        err.params = params;
        free(params);
        va_end( args );

        throw err;
    }*/

    /*void Sys::ErrorAbort( int errorCode, ... )
    {
        va_list args;

        //if (Var::GetInt("dev_errorbreak", false, -1) > 0)
        //    Sys::DebugBreak(true);
        
        time_t ts;
        time(&ts);

        unsigned int count;

        va_start( args, errorCode );
        for (count = 0; ; count++)
        {
            if (va_arg(args, const char*) == nullptr
                    || va_arg(args, const char*) == nullptr)
                break;
        }
        va_end( args );

        va_start( args, errorCode );
        size_t length = Params::CalculateLength(count, args);
        va_end( args );

        va_start( args, errorCode );
        char* params = Params::BuildAllocv(count, length, args);
        va_end( args );

        s_eb->errorCode = errorCode;
        ErrorBuffer::SetParams(s_eb, params);
        ErrorBuffer::SetTimestamp(s_eb, &ts);

        free(params);
        sys.DisplayError(s_eb, true);
        exit(-1);
    }*/

    /*void Sys::ExecList( const char* path, bool required )
    {
        Reference<InputStream> input = sys.OpenInput( path );

        if ( input == nullptr )
        {
            if ( required )
                Sys::ApplicationError(EX_LIST_OPEN_ERR,
                        "desc", (const char*) sprintf_t<511>("Failed to open configuration script '%s'.", path),
                        "function", li_functionName,
                        nullptr);

            return;
        }

        while ( input->isReadable() )
        {
            String line = input->readLine();

            if ( line.isEmpty() || line.beginsWith(';') )
                continue;

            Sys::ExecLine( line );
        }
    }*/

    /*int Sys::GetTickRate(double* tickTime_out)
    {
        if (tickTime_out)
            *tickTime_out = tickTime;

        return tickRate;
    }*/

    /*void Sys::RaiseException( ExceptionType t, const char* function, const char* format, ... )
    {
#if defined(ZOMBIE_DEBUG)
        // In debug mode, break directly into debugger instead of throwing (and destroying the stack)

#if defined(ZOMBIE_WINNT)
        if (IsDebuggerPresent())
            _CrtDbgBreak();
        else
#else
        // As there is no portable way to test for the presence of a debugger (or is there?), we'll just use the variable here
        if (VAR_INT(dev_errorbreak) > 0)
            raise(SIGINT);
        else
#endif
#endif
        {
            // TODO: Buffer
            char buffer[0x1000];

            va_list args;
            va_start( args, format );
            vsnprintf( buffer, sizeof( buffer ), format, args );
            va_end( args );

            throw SysException( t, function, buffer );
        }
    }*/
}
