#pragma once

#include <framework/datafile.hpp>
#include <framework/event.hpp>
#include <framework/framework.hpp>

#include <gameui/gameui.hpp>

namespace skelet
{
    using namespace zfw;
    using namespace zfw::render;
    /*
    static const int MAX_GAMEPADS = 4;

    enum { MENU_LOCKED = 1 };

    enum { SDS_ZOMBMAP_HDR = 1, SDS_ZOMBMAP_EDITOR = 2, SDS_ZOMBMAP_ENTITIES = 11 };

    struct Humanoid_t
    {
        glm::vec3 pos;

        media::DIMesh head, body, arm, leg;

        float arms_angle;
    };

    struct HumanoidDef_t
    {
        float head_width, head_height, head_depth;
        float body_width, body_height, body_depth;
        float arm_r, arm_length;
        float leg_r, leg_length;

        float arms_angle;
    };

    struct Menu_t
    {
        const char** options;
        const int num_options;
        const int flags;
        const float angle, scale, zoom, dist_from_center;

        int selection;

        glm::vec4 colour, bg_colour;
        float alpha;
    };

    extern const HumanoidDef_t humanlike;

    extern IFont *font_ui_medium, *font_ui_big;
    extern ITexture* tex_buttons;

    extern GamepadCfg_t gamepads[MAX_GAMEPADS];

    void CreateHumanoid(const HumanoidDef_t& def, Humanoid_t& model);
    void DrawButtonIcon(int x, int y, int xx, int yy);
    void DrawFileIcon32(int x, int y, int xx, int yy);
    void DrawHumanoid(Humanoid_t& model, float anim, float angle);
    void DrawUIIcon32(int x, int y, int xx, int yy);
    int DumpGamepadCfg(int index, GamepadCfg_t& cfg);
    int GetGamepadCfg(int index, GamepadCfg_t& cfg);

    bool IsUp(const VkeyState_t& vkey);
    bool IsDown(const VkeyState_t& vkey);
    bool IsLeft(const VkeyState_t& vkey);
    bool IsRight(const VkeyState_t& vkey);
    bool IsA(const VkeyState_t& vkey);
    bool IsB(const VkeyState_t& vkey);
    bool IsX(const VkeyState_t& vkey);
    bool IsY(const VkeyState_t& vkey);
    bool IsL(const VkeyState_t& vkey);
    bool IsR(const VkeyState_t& vkey);
    bool IsMenu(const VkeyState_t& vkey);
    bool IsMUp(const VkeyState_t& vkey);
    bool IsMDown(const VkeyState_t& vkey);
    bool IsMLeft(const VkeyState_t& vkey);
    bool IsMRight(const VkeyState_t& vkey);
    bool IsAUp(const VkeyState_t& vkey);
    bool IsADown(const VkeyState_t& vkey);
    bool IsALeft(const VkeyState_t& vkey);
    bool IsARight(const VkeyState_t& vkey);

    inline bool IsAnyUp(const VkeyState_t& vkey) { return Video::IsUp(vkey) || IsUp(vkey); }
    inline bool IsAnyDown(const VkeyState_t& vkey) { return Video::IsDown(vkey) || IsDown(vkey); }
    inline bool IsAnyLeft(const VkeyState_t& vkey) { return Video::IsLeft(vkey) || IsLeft(vkey); }
    inline bool IsAnyRight(const VkeyState_t& vkey) { return Video::IsRight(vkey) || IsRight(vkey); }
    inline bool IsConfirm(const VkeyState_t& vkey) { return Video::IsEnter(vkey) || IsA(vkey); }
    inline bool IsCancel(const VkeyState_t& vkey) { return Video::IsEsc(vkey) || IsB(vkey); }
    inline bool IsAnyMenu(const VkeyState_t& vkey) { return Video::IsEsc(vkey) || IsMenu(vkey); }
    */
    void SharedReleaseMedia();
    void SharedReloadMedia();
    /*
    enum
    {
        MENU_SELECTION,
        MENU_INPUT
    };

    enum
    {
        MODE_SEL,
        MODE_INSERT_ENT,
        MODE_MOVE_ENT,
        MODE_RESIZE_ENT
    };

    enum { ENT_CUBE, ENT_PLANE };

    struct WorldEnt
    {
        int type;
        glm::vec3 pos, size;
        glm::vec4 colour;
        String textureName;

        physics::ICollObj* coll;

        void Init()
        {
            coll = nullptr;
        }

        void Release()
        {
            li::destroy(coll);
            textureName.clear();
        }
    };

    class IMapLoadListener
    {
        public:
            virtual void OnMapBegin( const char* mapName ) = 0;
            virtual void OnMapEditorData( const glm::vec3& viewPos, float zoom ) = 0;
            virtual void OnMapEntity( const WorldEnt& ent ) = 0;
    };

    class MapLoader
    {
        datafile::SDStream& sds;

        public:
            MapLoader( datafile::SDStream& sds );

            void Init();
            void Exit();

            void LoadMap( IMapLoadListener* listener );
    };

    class MapWriter
    {
        datafile::SDSWriter& sw;
        OutputStream* section;

        public:
            MapWriter( datafile::SDSWriter& sw );

            void Init(const char* mapName);
            void Exit();

            void EntitiesBegin();
            void EntitiesAdd( const WorldEnt& ent );
            void EntitiesEnd();

            void EditorData( const glm::vec3& viewPos, float zoom );
    };
    */
    class SkeletScene: public IScene
    {
        // UI
        gameui::UIResManager* uiRes;
        gameui::WidgetContainer* uiContainer;

        gameui::Panel* propertiesPanel;

        bool useRibbon;

        // le scene
        /*world::Camera c;

        // menu cube
        media::DIMesh cube;
        glm::vec4 cubeColour;
        glm::mat4 cubeTransform;
        IProgram* cubeShader;
        ITexture* white_tex;

        // menus and transitions
        Menu_t *current, *trans_to;
        float transition;

        // ui
        IFont* menuFont, *uiFont;
        IOverlay* overlay;

        // variables
        Var_t *r_viewportw, *r_viewporth;
        String str_select, str_confirm;
        int confGamepadIndex, gamepadIndex;*/

        public:
            SkeletScene();
            virtual ~SkeletScene();

            virtual void Init();
            virtual void Exit();

            virtual void DrawScene() override;
            virtual void OnFrame( double delta ) override;
            virtual void ReleaseMedia() override;
            virtual void ReloadMedia() override;

            void CreateUI();
            void ReleasePrivateMedia();
    };
    /*
    class GameScene : public IScene, public IMapLoadListener
    {
        // le scene
        world::Camera c;
        media::DIMesh ground;

        // player model
        Humanoid_t playerModel;

        //
        ITexture* white_tex;
        IProgram* worldShader;

        media::DIMesh buildCube, buildPlane;
        List<WorldEnt> entities;
        physics::IPhysWorld* physWorld;
        physics::ICollObj* playerCollObj;

        // variables
        uint16_t inputs;
        Var_t *r_viewportw, *r_viewporth, *in_mousex, *in_mousey;

        void ClearEntities();

        public:
            GameScene();
            virtual ~GameScene();

            virtual void Init();
            virtual void Exit();

            virtual void DrawScene() override;
            virtual void OnFrame( double delta ) override;
            virtual void ReleaseMedia() override;
            virtual void ReloadMedia() override;

            virtual void OnMapBegin( const char* mapName ) override;
            virtual void OnMapEditorData( const glm::vec3& viewPos, float zoom ) override;
            virtual void OnMapEntity( const WorldEnt& ent ) override;

            void LoadMap();
            bool TestAABB_World(const glm::vec3 c[2]);
    };

    class EditorScene : public IScene, public IMapLoadListener
    {
        int mode;

        // le scene
        world::Camera c;
        glm::vec3 viewPos;

        List<IOverlay*> overlays;

        //
        ITexture* white_tex;
        IProgram* worldShader;

        ITexture* selection_tex;

        IFont* uiFont;

        //
        glm::vec3 buildOrigin, buildPos, buildSize;
        media::DIMesh buildCube, buildPlane;

        List<WorldEnt> entities;
        int selectedEnt, entType;

        // variables
        String saveName;
        uint16_t inputs;
        float inputInterval, selectionGlow;
        float zoom;
        Var_t *r_viewportw, *r_viewporth, *in_mousex, *in_mousey;

        String str_insert, str_move, str_select;

        public:
            EditorScene();
            virtual ~EditorScene();

            virtual void Init();
            virtual void Exit();

            virtual void DrawScene() override;
            virtual void OnFrame( double delta ) override;
            virtual void ReleaseMedia() override;
            virtual void ReloadMedia() override;

            virtual void OnMapBegin( const char* mapName ) override;
            virtual void OnMapEditorData( const glm::vec3& viewPos, float zoom ) override;
            virtual void OnMapEntity( const WorldEnt& ent ) override;

            void LoadMap();
            void NewMap();
            int FindEntAtPos( const glm::vec3& pos, int index = 0 );
            void SaveMap();
            void UpdateBuildingMinMax();
            void UpdateSelection();
    };*/
}
