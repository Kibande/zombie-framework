
#include "event.hpp"
#include "framework.hpp"
#include "rendering.hpp"

#include <littl/Directory.hpp>
#include <littl/File.hpp>
#include <littl/Thread.hpp>

#include <cstdarg>

#if defined(ZOMBIE_DEBUG)
#if defined(ZOMBIE_WINNT)
#include <crtdbg.h>
#else
#include <signal.h>
#endif
#endif

#ifdef ZOMBIE_EMSCRIPTEN
#include <emscripten.h>
#endif

namespace zfw
{
    using namespace zfw::render;

    // keep in sync with 'ExceptionType' in framework.hpp
    static const char* exceptionNames[] = {
        "EX_ALLOC_ERR",
        "EX_ASSERTION_FAILED",
        "EX_ASSET_OPEN_ERR",
        "EX_BACKEND_ERR",
        "EX_CONTROLLER_UNAVAILABLE",
        "EX_DATAFILE_ERR",
        "EX_DATAFILE_FORMAT_ERR",
        "EX_FEATURE_DISABLED",
        "EX_INTERNAL_STATE",
        "EX_INVALID_ARGUMENT",
        "EX_INVALID_OPERATION",
        "EX_IO_ERROR",
        "EX_LIST_OPEN_ERR",
        "EX_NOT_IMPLEMENTED",
        "EX_SERIALIZATION_ERR",
        "EX_SHADER_COMPILE_ERR",
        "EX_SHADER_LOAD_ERR",
        "EX_VARIABLE_UNDEFINED",
        "EX_VARIABLE_LOCKED",
        "EX_VARIABLE_TYPE_LOCKED" };

    static Timer frameDelta;

    static bool breakLoop, changeScene;
    static IScene* scene, * newScene;

    static Array<char> printfbuf;

    static Mutex* sync_ioMutex = nullptr;
    static Mutex* sync_logMutex = nullptr;

    struct FileSystem {
        String path;
        int protocol;
    };
    
    static List<FileSystem> fileSystems;
    static FileSystem outputFs;
    
    bool Sys::AddFileSystem(const char* path)
    {
        li_synchronized(sync_ioMutex);

        FileSystem fs = { path, 0 };
        
        if (!fs.path.endsWith('/'))
            fs.path.append('/');
        
        fileSystems.add((FileSystem&&) fs);
        return true;
    }
    
    void Sys::BreakGameLoop()
    {
        breakLoop = true;
    }

    void Sys::ChangeScene( IScene* scene )
    {
        newScene = scene;
        changeScene = true;
    }

    void Sys::DisplayError( const SysException& exception, int display_as )
    {
        String text;

        if  ( display_as == ERR_DISPLAY_UNCAUGHT )
        {
            text = "-- Uncaught Exception --";
        }
        else if  ( display_as == ERR_DISPLAY_FATAL )
        {
            text = "-- Fatal Error --";
        }
        else
        {
            text = "-- Application Error --";
        }

        text += "\n\n";
        text += Localize( "Exception" );
        text += ": ";
        text += exceptionNames[exception.GetType()];

        const char* exceptionNameLoc = Localize( exceptionNames[exception.GetType()] );
        if ( exceptionNameLoc != exceptionNames[exception.GetType()] )
        {
            text += " (";
            text += exceptionNameLoc;
            text += ")";
        }

        text += "\n";
        text += Localize( "In function" );
        text += ": ";
        text += exception.GetFunctionName();
        text += "\n";
        text += Localize( "Description" );
        text += ": ";
        text += exception.what();

#ifdef ZOMBIE_WINNT
        MessageBoxA( nullptr, text, "Application Error", MB_ICONERROR );
#else
        fprintf(stderr, "%s\n\nPress any key.\n", text.c_str());
        getchar();
#endif
    }

    void Sys::ExecLine( const String& line )
    {
        List<String> tokens;
        String buffer;
        size_t index = 0;
        unsigned numTokens = 0;
        bool enclosed = false, escaped = false;

        // TODO: proper parsing and a call to Sys::Exec

        while ( index < line.getNumBytes() )
        {
            Unicode::Char next = line.getChar( index );

            if ( next == Unicode::invalidChar )
                break;
            else if ( next == '\\' && !escaped )
                escaped = true;
            else if ( next == '"' && !escaped )
                enclosed = !enclosed;
            else if ( next == ' ' && !enclosed && !escaped )
            {
                if ( !buffer.isEmpty() )
                {
                    tokens.add( buffer );
                    buffer.clear();
                    numTokens++;
                }
            }
            else
            {
                buffer.append( UnicodeChar( next ) );
                escaped = false;
            }
        }

        if ( !buffer.isEmpty() )
            tokens.add( buffer );

        if (tokens[0] == "fs_add")
        {
            AddFileSystem(tokens[1]);
            return;
        }
        
        if ( tokens.getLength() == 1 )
            Var::SetInt( tokens[0], 1 );
        else if ( tokens.getLength() > 1 )
            Var::SetStr( tokens[0], tokens[1] );
    }

    void Sys::ExecList( const char* path, bool required )
    {
        Reference<InputStream> input = Sys::OpenInput( path );

        if ( input == nullptr )
        {
            if ( required )
                Sys::RaiseException( EX_LIST_OPEN_ERR, "Sys::ExecList", "failed to open list '%s'", path );

            return;
        }

        while ( input->isReadable() )
        {
            String line = input->readLine();

            if ( line.isEmpty() || line.beginsWith(';') )
                continue;

            Sys::ExecLine( line );
        }
    }

    void Sys::Exit()
    {
        li::destroy(sync_ioMutex);
        li::destroy(sync_logMutex);

        Var::Clear();
    }

    void Sys::Init()
    {
        changeScene = false;

        scene = nullptr;
        newScene = nullptr;
        
        sync_ioMutex = new Mutex();
        sync_logMutex = new Mutex();

        AddFileSystem(".");
        
        outputFs.path = "./";
        outputFs.protocol = 0;
    }

    bool Sys::ListDir( const char* path, IListDirCallback* callback )
    {
        // FIXME: This needs to use VFS

        List<Directory::Entry> dirlist;

        {
            li_synchronized(sync_ioMutex);

            Object<Directory> dir = Directory::open(path);

            if (dir == nullptr)
                return false;

            dir->list(dirlist);
        }

        iterate2(i, dirlist)
        {
            FileMeta_t meta;
            const Directory::Entry& ent = i;

            meta.type = (ent.isDirectory) ? FileMeta_t::F_DIR : FileMeta_t::F_FILE;
            meta.fileSize = ent.size;

            if (!callback->OnDirEntry(ent.name, &meta))
                return false;
        }

        return true;
    }

    const char* Sys::Localize( const char* text )
    {
        return text;
    }

    void Sys::Log(int log_class, const char* format, ...)
    {
        // FIXME: support for log_class and output redirection

        char tmbuffer[0x100];
        time_t rawtime;

        time(&rawtime);
        // TODO: Move this into Util
        strftime(tmbuffer, sizeof(tmbuffer), "%Y-%m-%dT%H:%M:%S", localtime(&rawtime));

        li_synchronized(sync_logMutex)

        printf("[%s]\t", tmbuffer);

        switch (log_class)
        {
            case ERR_DISPLAY_ERROR: printf("ERROR\t"); break;
            case ERR_DISPLAY_FATAL: printf("FATAL\t"); break;
            case ERR_DISPLAY_UNCAUGHT: printf("UNCGHT\t"); break;
            case ERR_DISPLAY_USER: printf("USER\t"); break;
            case ERR_DISPLAY_WARNING: printf("WARNING\t"); break;
        }

        va_list args;
        va_start( args, format );
        vprintf( format, args );
        va_end( args );
    }

    static void DoFrame()
    {
        if ( changeScene )
        {
            if ( scene != nullptr )
            {
                scene->Exit();
                delete scene;
            }

            scene = newScene;
            changeScene = false;

            if ( scene != nullptr )
                scene->Init();
        }

        Video::BeginFrame();
        Video::ReceiveEvents();

        if ( scene != nullptr )
        {
            scene->OnFrame( Sys::Update() );
            scene->DrawScene();
        }
        else
        {
            zr::R::SetClearColour(0.0f, 0.1f, 0.2f, 0.0f);
            zr::R::Clear();

            Event_t* event;

            while ( ( event = Event::Pop() ) != nullptr )
                if ( event->type == EV_VKEY && event->vkey.vk.inclass == IN_OTHER && event->vkey.vk.key == OTHER_CLOSE )
                {
                    Sys::BreakGameLoop();
                    break;
                }
        }

        Video::EndFrame();
    }

    void Sys::MainLoop()
    {
#ifndef ZOMBIE_EMSCRIPTEN
        breakLoop = false;

        while ( !breakLoop )
        {
            DoFrame();
        }

        if ( scene != nullptr )
        {
            scene->Exit();
            delete scene;
            scene = nullptr;
        }
#else
        emscripten_set_main_loop(DoFrame, 0, 1);
#endif
    }

    SeekableInputStream* Sys::OpenInput( const char* path )
    {
        li_synchronized(sync_ioMutex);

        SeekableInputStream* sis = nullptr;
        
        iterate2 (i, fileSystems)
        {
            FileSystem& fs = i;
            
            switch (fs.protocol)
            {
                case 0:
                    sis = File::open(fs.path + path);
                    break;
            }
            
            if ( sis != nullptr )
                return sis;
        }
        
        return nullptr;
    }

    OutputStream* Sys::OpenOutput( const char* path, int storage )
    {
        //li_synchronized(sync_ioMutex);

        return File::open( path, "wb" );
    }

    SeekableIOStream* Sys::OpenRandomIO( const char* path )
    {
        File* f = File::open( path, "rb+" );

        if (f == nullptr)
            f = File::open( path, "wb+" );

        return f;
    }
    
    int Sys::printf(const char* format, ...)
    {
        va_list args;
        
        va_start(args, format);
        int r = ::vprintf(format, args);
        va_end(args);
        
        return r;
    }

    void Sys::RaiseException( ExceptionType t, const char* function, const char* format, ... )
    {
#if defined(ZOMBIE_DEBUG)
#if defined(ZOMBIE_WINNT)
        if (IsDebuggerPresent())
            _CrtDbgBreak();
        else
#else
        if (1)
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
    }

    char* Sys::tmpsprintf( size_t length, const char* format, ... )
    {
        va_list args;

        printfbuf.resize( length + 1, true );

        va_start( args, format );
        vsnprintf( printfbuf.getPtr(), length + 1, format, args );
        va_end( args );

        return printfbuf.getPtr();
    }

    double Sys::Update()
    {
        double t;

        if ( frameDelta.isStarted() )
            t = frameDelta.getMicros() / 1000000.0;
        else
            t = 0.0;

        frameDelta.start();

        return t;
    }
}
