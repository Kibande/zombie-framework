#pragma once

#include <framework/event.hpp>
#include <framework/framework.hpp>

namespace zombie
{
    using namespace zfw;

    class IOverlay
    {
        public:
            virtual ~IOverlay() {}

            virtual void Exit() {}

            virtual void Draw( const glm::vec2& pos, const glm::vec2& maxSize ) {}
            virtual void OnFrame( double delta ) {}

            virtual bool IsFinished() = 0;
    };

    /*class ISubMenuHost
    {
        public:
            virtual void InvokeSubMenu(MenuOverlay* menu, MenuItem& item) = 0;
    };*/

    struct FileChooserItem
    {
        String name;
        FileMeta_t metadata;

        int iconIndex;
    };

    struct MenuItem
    {
        enum Type {command, textInput, toggle, slider, subCommand, colour3};
        enum { DISABLED = 1 };

        Type t;
        String label;
        int flags, id;

        bool *bValue;
        float *value, min, max;
        glm::vec4* cValue;
        String* tvalue;
    };

    class UIOverlay : public IOverlay
    {
        protected:
            enum { ACTIVE, FADE_IN, FADE_OUT, FINISHED }
                    state;
            float alpha;

        protected:
            void FadeOut();

            void UIOverlay_Draw( const glm::vec2& pos, const glm::vec2& maxSize );
            void UIOverlay_OnFrame( double delta );

        public:
            UIOverlay();

            bool IsFinished();
    };

    class MenuOverlay : public UIOverlay
    {
        intptr_t id;

        String query;
        List<MenuItem> items;

        uint16_t inputs;
        int selection;

        int width;

        MenuOverlay* submenu;

        static void MoveSlider( MenuItem& item, float delta );

        public:
            MenuOverlay( const char* query, int id = -1 );
            static MenuOverlay* CreateColourEditor( const char* label, glm::vec4* value );

            virtual void Init();
            virtual void Exit();

            virtual void Draw( const glm::vec2& pos, const glm::vec2& maxSize );
            virtual void OnFrame( double delta );

            int AddColour3( const char* label, glm::vec4* value, int flags );
            int AddCommand( const char* label, int cid, int flags );
            int AddSlider( const char* label, float& value, float min, float max, int flags );
            int AddTextField( const char* label, String* value, int flags );
            int AddToggle( const char* label, bool& value, int flags );
            void Compile();
            int GetID() const { return id; }
            int GetSelection() const { return selection; }
            int GetSelectionID();
    };

    class FileChooserOverlay : public UIOverlay, public IListDirCallback
    {
        public:
            enum Purpose { P_OPEN, P_SAVE_AS };
            enum View { VIEW_LIST32 };
            enum { CONFIRM_OVERWRITE = 1 };

        private:
            intptr_t id;
            int flags;

            Purpose purpose;
            View view;

            String query;
            List<FileChooserItem> items;

            int selection;
            int numVisible, viewOffset;

            String extFilter, saveAsName;

            MenuOverlay* submenu;

        protected:
            virtual bool OnDirEntry(const char* name, const FileMeta_t* meta);

        public:
            FileChooserOverlay( Purpose purpose, const char* query, int id, int flags );
            static MenuOverlay* CreateFilenameEditor( const char* label, String* value );
            static MenuOverlay* CreateOverwriteQuery( const char* label );

            virtual void Init();
            virtual void Exit();

            virtual void Draw( const glm::vec2& pos, const glm::vec2& maxSize );
            virtual void OnFrame( double delta );

            void AddFile(const char* path);
            void AddFileWithMeta(const char* path, FileMeta_t* meta);
            bool AddDirContents(const char* path, const char* extension);
            void Compile();
            int GetID() const { return id; }
            int GetSelection() const { return selection; }
            const char* GetSelectionName();
    };
}