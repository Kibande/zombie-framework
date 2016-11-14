#pragma once

#include <framework/base.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <exception>

#define ZFW_ASSERT(expr_)\
if (!(expr_)) {\
    zfw::Sys::RaiseException(zfw::EX_ASSERTION_FAILED, li_functionName, "Failed assertion `" #expr_ "`");\
}

#ifdef _DEBUG
#define ZFW_DBGASSERT(expr_)\
if (!(expr_)) {\
    zfw::Sys::RaiseException(zfw::EX_ASSERTION_FAILED, li_functionName, "Failed DEBUG assertion `" #expr_ "`");\
}
#else
#define ZFW_DBGASSERT(expr_)
#endif

#define METHOD_NOT_IMPLEMENTED(prototype_)\
prototype_ {\
    zfw::Sys::RaiseException(zfw::EX_NOT_IMPLEMENTED, li_functionName, "The method `" #prototype_ "` is not implemented.");\
}

#define VAR_FLOAT(var_) ((var_)->basictypes.f)
#define VAR_INT(var_) ((var_)->basictypes.i)
#define VAR_PTRINT(var_) (&(var_)->basictypes.i)

#define NAME(name_) extern Name name_;
#define DECLNAME(name_) uint8_t NameDecl_##name_; Name name_ = &NameDecl_##name_;

namespace zfw
{
    typedef glm::ivec2 Int2;
    typedef glm::ivec3 Int3;
    typedef glm::ivec4 Int4;

    typedef glm::vec2 Float2;
    typedef glm::vec3 Float3;
    typedef glm::vec4 Float4;

    typedef glm::tvec4<uint8_t> Byte4;

    typedef glm::tvec2<short> Short2;

    typedef const glm::tvec4<uint8_t> CByte4;

    typedef const glm::tvec2<short> CShort2;

    typedef const glm::ivec2 CInt2;
    typedef const glm::ivec3 CInt3;
    typedef const glm::ivec4 CInt4;

    typedef const glm::vec2 CFloat2;
    typedef const glm::vec3 CFloat3;
    typedef const glm::vec4 CFloat4;

    // keep in sync with 'exceptionNames' in sys.cpp
    enum ExceptionType
    {
        EX_ALLOC_ERR,
        EX_ASSERTION_FAILED,
        EX_ASSET_OPEN_ERR,
        EX_BACKEND_ERR,
        EX_CONTROLLER_UNAVAILABLE,
        EX_DATAFILE_ERR,
        EX_DATAFILE_FORMAT_ERR,
        EX_FEATURE_DISABLED,
        EX_INTERNAL_STATE,
        EX_INVALID_ARGUMENT,
        EX_INVALID_OPERATION,
        EX_IO_ERROR,
        EX_LIST_OPEN_ERR,
        EX_NOT_IMPLEMENTED,
        EX_SERIALIZATION_ERR,
        EX_SHADER_COMPILE_ERR,
        EX_SHADER_LOAD_ERR,
        EX_VARIABLE_UNDEFINED,
        EX_VARIABLE_LOCKED,
        EX_VARIABLE_TYPE_LOCKED
    };

    enum ErrorDisplayMode
    {
        ERR_DISPLAY_ERROR,
        ERR_DISPLAY_UNCAUGHT,
        ERR_DISPLAY_FATAL,
        ERR_DISPLAY_WARNING,
        ERR_DISPLAY_USER
    };

    enum
    {
        STORAGE_LOCAL,
        STORAGE_CONFIG,
        STORAGE_USER
    };

    enum
    {
        VAR_UTF8 = 1,
        VAR_INT = 2,
        VAR_FLOAT = 4,
        VAR_TYPEMASK = 0xff,
        VAR_LOCKED = 1<<30,
        VAR_LOCKTYPE = 1<<31
    };

    enum
    {
        ALIGN_LEFT = 0,
        ALIGN_CENTER = 1,
        ALIGN_RIGHT = 2,
        ALIGN_TOP = 0,
        ALIGN_MIDDLE = 4,
        ALIGN_BOTTOM = 8
    };
    
    static const Name NO_NAME = nullptr;

    static const glm::vec4 COLOUR_WHITE = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    static const glm::vec4 COLOUR_BLACK = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

    static inline glm::vec4 COLOUR_BLACKA(float a) {
        return glm::vec4(0.0f, 0.0f, 0.0f, a);
    }

    static inline glm::vec4 COLOUR_WHITEA(float a) {
        return glm::vec4(1.0f, 1.0f, 1.0f, a);
    }

    static inline glm::vec4 COLOUR_GREY(float l, float a = 1.0f) {
        return glm::vec4(l, l, l, a);
    }

    inline Byte4 RGBA_COLOUR(int r, int g, int b, int a = 0xFF) {
        return Byte4(r, g, b, a);
    }

    inline Byte4 RGBA_COLOUR(float r, float g, float b, float a = 1.0f) {
        return Byte4(r * 255.0f, g * 255.0f, b * 255.0f, a * 255.0f);
    }

    static const Byte4 RGBA_WHITE(0xFF, 0xFF, 0xFF, 0xFF);

    inline Byte4 RGBA_BLACK_A(int a) {
        return Byte4(0, 0, 0, a);
    }

    inline Byte4 RGBA_BLACK_A(float a) {
        return Byte4(0, 0, 0, a * 255.0f);
    }

    inline Byte4 RGBA_GREY(float l, float a = 1.0f) {
        return Byte4(l * 255.0f, l * 255.0f, l * 255.0f, a * 255.0f);
    }

    inline Byte4 RGBA_WHITE_A(float a) {
        return Byte4(255, 255, 255, a * 255.0f);
    }

    struct VkeyState_t;

    struct FileMeta_t
    {
        enum Type { F_FILE, F_DIR, F_UNKNOWN };

        Type type;
        uint64_t fileSize;
    };

    struct Var_t
    {
        int flags;
        String name, text;
        union{
            int i;
            float f;
        } basictypes;

        Var_t* prev, *next;

        Var_t( int flags, const char* name, const char* text );
        Var_t( int flags, const char* name, int value );

        bool ConvertToFloat();
        bool ConvertToInt();
        bool ConvertToStr();
        int GetInt();
        const char* GetStr();
        void LockType();
        void Print();
        void SetFloat( float value );
        void SetInt( int value );
        void SetStr( const char* value );
        //void Unlock() { flags &= ~VAR_LOCKED; }
    };

    class Var
    {
        public:
            static void Clear();
            static void Dump();
            static Var_t* Find( const char* name );
            static bool GetInt( const char* name, int* i, bool required );
            static int GetInt( const char* name, bool required, int def = 0 );
            static const char* GetStr( const char* name, bool required );
            static Var_t* Lock( const char* name, bool lockvalue = true, int defaultval = -1 );
            static Var_t* Lock( const char* name, bool lockvalue, float defaultval );
            static Var_t* LockFloat( const char* name, bool lockvalue = true, float defaultval = -1 );
            static Var_t* LockInt( const char* name, bool lockvalue = true, int defaultval = -1 );
            //static Var_t* LockStr( const char* name );
            //static Var_t* TypeLockString( const char* name );
            static Var_t* SetFloat( const char* name, float value );
            static Var_t* SetInt( const char* name, int value );
            static Var_t* SetStr( const char* name, const char* value );
            static void Unlock( Var_t*& var );
    };

    class IListDirCallback
    {
        public:
            virtual bool OnDirEntry(const char* name, const FileMeta_t* meta) = 0;
    };

    class IScene
    {
        public:
            virtual ~IScene() {}

            virtual void Init() {}
            virtual void Exit() {}
            virtual void ReleaseMedia() {}
            virtual void ReloadMedia() {}

            virtual void DrawScene() {}
            virtual void OnFrame( double delta ) {}
    };

    class SysException : public std::exception
    {
        ExceptionType t;
        String function, desc;

        public:
            //SysException( ExceptionType t, String&& function, String&& desc ) : t(t), function((String&&)function), desc((String&&)desc) {}
            SysException( ExceptionType t, const char* function, const char* desc ) : t(t), function(function), desc(desc) {}
            ~SysException() throw() {}

            const char* GetFunctionName() const throw() { return function; }
            ExceptionType GetType() const throw() { return t; }
            virtual const char* what() const throw() { return desc; }
    };

    struct Flag
    {
        const char* name;
        int value;
    };

    class Sys
    {
        public:
            static void Init();
            static void Exit();

            // Primary control functions
            static void BreakGameLoop();
            static void ChangeScene( IScene* scene );
            static void MainLoop();

            static void EmitMessage(int16_t scope,
                    ClientId sender, ClientId recipient,
                    int32_t messageClass, int32_t messageId);

            // I/O functions
            static bool AddFileSystem(const char* path);
            static bool ListDir( const char* path, IListDirCallback* callback );
            static SeekableInputStream* OpenInput( const char* path );
            static OutputStream* OpenOutput( const char* path, int storage );
            static SeekableIOStream* OpenRandomIO( const char* path );

            // Localization functions
            static const char* Localize( const char* text );

            // Logging functions
            static int printf(const char* format, ...);
            static int RegisterLogClass(void (*callback)(const char* format, ...));
            static void Log(int log_class, const char* format, ...);

            // Utility functions
            static char* tmpsprintf( size_t length, const char* format, ... );

            // Error handling functions
            static void DisplayError( const SysException& exception, int display_as );
            static li_noreturn(void RaiseException( ExceptionType t, const char* function, const char* format, ... )); //__attribute__ ((format (printf, 3, 4)))

            // random crap
            static void ExecLine( const String& line );
            static void ExecList( const char* path, bool required );

            static double Update();
    };

    class Util
    {
        public:
            static Int2         Align(Int2 pos, Int2 size, int alignment);
            static const char*  Md5String(const char *string);

            static int          ParseEnum(const char* string, int& value, const Flag* flags);
            static int          ParseFlagVec(const char* string, int& value, const Flag* flags);
            static int          ParseIntVec(const char* string, int* vec, size_t minCount, size_t maxCount);
            static int          ParseVec(const char* string, int (*callback)(const char* value, const char** end_value, void *user), void *user);
            static int          ParseVec(const char* string, float* vec, size_t minCount, size_t maxCount);

            static const char*  Format(const Int2& vec);
            static const char*  Format(const Float2& vec);
            static const char*  Format(const Float3& vec);
            static const char*  Format(const Float4& vec);

            static bool         StrEmpty(const char* str) { return str == nullptr || *str == 0; }

            static Float3 TransformVec(const Float4& v, const glm::mat4x4& m)
            {
                const float w_scale = 1.0f / (m[0][3] * v.x + m[1][3] * v.y + m[2][3] * v.z + m[3][3] * v.w);

                return Float3(
                        m[0][0] * v.x + m[1][0] * v.y + m[2][0] * v.z + m[3][0] * v.w,
                        m[0][1] * v.x + m[1][1] * v.y + m[2][1] * v.z + m[3][1] * v.w,
                        m[0][2] * v.x + m[1][2] * v.y + m[2][2] * v.z + m[3][2] * v.w
                ) * w_scale;
            }
    };

    class Timer
    {
        uint64_t startTicks, pauseTicks;
        bool paused, started;

        public:
            Timer();

            uint64_t getMicros() const;
            static uint64_t getRelativeMicroseconds();
            bool isStarted() const { return started; }
            bool isPaused() const { return paused; }
            void pause();
            void reset();
            void start();
            void stop();
            void unpause();
    };

    namespace media
    {
        enum { REVERSE_WINDING = 1, WITH_NORMALS = 2, FRONT = 4, BACK = 8, LEFT = 16, RIGHT = 32, TOP = 64, BOTTOM = 128, ALL_SIDES = 4+8+16+32+64+128,
            WITH_UVS = 256, WIREFRAME = 512 };
        enum MeshLayout { MESH_LINEAR, MESH_INDEXED };
        enum MPrimType { MPRIM_LINES, MPRIM_TRIANGLES };

		typedef uint16_t VertexIndex_t;

        enum TexFormat
        {
            TEX_RGB8,
            TEX_RGBA8,
        };

        struct DIBitmap
        {
            TexFormat format;
            Int2 size;
            size_t stride;
            Array<uint8_t> data;
        };

        struct DIMesh
        {
            MPrimType format;
            MeshLayout layout;
            int flags;

            size_t numVertices, numIndices;

            Array<float> coords, normals, uvs;

            Array<VertexIndex_t> indices;
        };

        class Bitmap
        {
            public:
                static void Alloc( DIBitmap* bmp, uint16_t w, uint16_t h, TexFormat format );
                static void Blit( DIBitmap* dst, int x, int y, DIBitmap* src );
                static void Blit( DIBitmap* dst, int x, int y, DIBitmap* src, int x1, int y1, int w, int h );
                static size_t GetBytesPerPixel( TexFormat format );
                static void ResizeToFit( DIBitmap* bmp, bool lazy );
        };

        class MediaLoader
        {
            public:
                static bool LoadBitmap( const char* path, DIBitmap* bmp, bool required );
        };

        class PresetFactory
        {
            public:
                static void CreateBitmap2x2( const glm::vec4& colour, TexFormat format, DIBitmap* bmp );
                static void CreateCube( const glm::vec3& size, const glm::vec3& origin, int flags, DIMesh* mesh );
                static void CreatePlane( const glm::vec2& size, const glm::vec3& origin, int flags, DIMesh* mesh );
        };
    }

    namespace world
    {
        class Camera
        {
            public:
                glm::vec3 eye, center, up;

            public:
                Camera();
                Camera( const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up );
                Camera( const glm::vec3& center, float dist, float angle, float angle2 );

                static void convert( float dist, float angle, float angle2, glm::vec3& eye, glm::vec3& center, glm::vec3& up );
                static void convert( const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up, float& dist, float& angle, float& angle2 );

                //const glm::vec3& getCenterPos() const { return center; }
                float getDistance() const;
                //const glm::vec3& getEyePos() const { return eye; }
                //const glm::vec3& getUpVector() const { return up; }

                void move( const glm::vec3& vec );

                void setCenterPos( const glm::vec3& pos ) { center = pos; }
                void setEyePos( const glm::vec3& pos ) { eye = pos; }
                void setUpVector( const glm::vec3& pos ) { up = pos; }

                void rotateXY( float alpha, bool absolute = false );
                void rotateZ( float alpha, bool absolute = false );

                void zoom( float amount, bool absolute = false );
        };
    }

    namespace render
    {
        enum BlendMode_t
        {
            BLEND_NORMAL,
            BLEND_ADD
        };

        enum
        {
            FONT_BOLD = 1,
            FONT_ITALIC = 2,
            FONT_STREAMED = 4
        };

        enum
        {
            TEXT_LEFT = 0,
            TEXT_CENTER = 1,
            TEXT_RIGHT = 2,
            TEXT_TOP = 0,
            TEXT_MIDDLE = 4,
            TEXT_BOTTOM = 8,
            TEXT_SHADOW = 16
        };

        enum
        {
            R_3D = 1,
            R_NORMALS = 2,
            R_UVS = 4,
            R_COLOURS = 8
        };

        extern bool altDown, ctrlDown, shiftDown;

        class Video
        {
            public:
                static void Init();
                static void Exit();

                static void Reset();
                static void SetViewport(unsigned int w, unsigned int h);

                static bool MoveWindow(int x, int y);
                static bool SetWindowTransparency(bool enabled);

                static void BeginFrame();
                static void ReceiveEvents();
                static void EndFrame();

                static void EnableKeyRepeat( bool enable );

                static int GetJoystickDev( const char* name );
                static const char* GetJoystickName( int dev );
                static bool IsUp(const VkeyState_t& vkey);
                static bool IsDown(const VkeyState_t& vkey);
                static bool IsLeft(const VkeyState_t& vkey);
                static bool IsRight(const VkeyState_t& vkey);
                static bool IsEnter(const VkeyState_t& vkey);
                static bool IsEsc(const VkeyState_t& vkey);

                static bool IsAltDown() { return altDown; }
                static bool IsCtrlDown() { return ctrlDown; }
                static bool IsShiftDown() { return shiftDown; }
        };

        class IFont
        {
            public:
                virtual ~IFont() {}

                virtual void Debug() {}

                virtual void DrawString( const char* text, const glm::vec3& pos, const glm::vec4& blend, int flags ) = 0;

                virtual int GetLineSkip() = 0;
                virtual glm::ivec2 GetTextSize( const char* text ) = 0;
                virtual void SetTransform( const glm::mat4& transform ) = 0;
        };

        class IProgram
        {
            public:
                virtual ~IProgram() {}

                virtual int GetUniformLocation(const char *name) = 0;
                virtual void SetUniform(int location, CFloat3& vec) = 0;
        };

        class ITexture : public ReferencedClass
        {
            protected:
                virtual ~ITexture() {}

            public:
                virtual bool IsFinalized() = 0;

                virtual void Download(media::DIBitmap* bmp) = 0;
                virtual const char* GetFileName() = 0;
                virtual glm::ivec2 GetSize() = 0;

                li_ReferencedClass_override(ITexture)
        };

        class Graphics : public ReferencedClass
        {
            public:
                static Graphics* Create( const char* desc, bool required );
                virtual ~Graphics() {}

                virtual void AcquireResources() {}
                virtual void DropResources() {}

                virtual void OnFrame(double delta) {}

                virtual Int2 GetSize() = 0;
                virtual void Draw(const Short2& pos, const Short2& size) = 0;
                virtual void DrawBillboarded(const Float3& pos, const Float2& size) = 0;

                li_ReferencedClass_override(Graphics)
        };

        class Loader
        {
            public:
                static ITexture* CreateTexture( const char* name, media::DIBitmap* bmp );
                static ITexture* LoadTexture( const char* path, bool required );
                static IProgram* LoadShadingProgram( const char* path );
                static IFont* OpenFont( const char* path, int size, int flags, int preloadMin, int preloadMax );
        };

        class R
        {
            public:
				static void Clear();
                static void SelectShadingProgram( IProgram* program );
                static void SetBlendColour( const glm::vec4& colour );
                static void SetBlendMode( BlendMode_t blendMode );
                static void SetClearColour( const glm::vec4& colour );
                static void SetRenderFlags( int flags );
                static void SetTexture( ITexture* tex );

                static void EnableDepthTest( bool enable );
                static void PopTransform();
                static void PushTransform( const glm::mat4& transform );
                static void Set2DView();
                static void SetCamera( const world::Camera& cam );
                static void SetCamera( const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up );
                static void SetPerspectiveProjection( float near, float far, float fov );

                static void AddLightPt( const glm::vec3& pos, const glm::vec3& ambient, const glm::vec3& diffuse, const glm::vec3& specular, float range );
                static void ClearLights();
                static void SetAmbient( const glm::vec3& colour );

                static void DrawDIMesh( media::DIMesh* mesh, const glm::mat4& modelView );
                static void DrawLine( const glm::vec3& a, const glm::vec3& b );
                static void DrawRectangle( glm::vec2 a, glm::vec2 b, float z );
                static void DrawTexture( ITexture* tex, glm::ivec2 pos );
                static void DrawTexture( ITexture* tex, const glm::vec3& pos, glm::vec2 size );
                static void DrawTexture( ITexture* tex, glm::vec2 pos, glm::vec2 size, const glm::vec2 uv[2] );
                static void DrawTexture( ITexture* tex, glm::vec2 pos, glm::vec2 size, const glm::vec2 uv[2], const glm::mat4& transform );
        };
    }
}
