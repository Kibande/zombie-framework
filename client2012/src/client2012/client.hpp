#pragma once

#include <framework/datafile.hpp>
#include <framework/entity.hpp>
#include <framework/event.hpp>
#include <framework/framework.hpp>
#include <framework/session.hpp>
#include <framework/stage.hpp>

#include <contentkit/texturecache.hpp>

#include <gameui/gameui.hpp>

namespace client
{
    using namespace zfw;
    using namespace zfw::render;

    using contentkit::TexturePreviewCache;

    class WorldCollisionHandler;

    void ReloadSharedMedia();
    void ReleaseSharedMedia();

    // FIXME: implement, lifecycle, document ...
    extern TexturePreviewCache *texPreviewCache;

    extern Session* g_session;
    extern World* g_world;
    extern WorldCollisionHandler *g_collHandler;
    extern zr::Font* g_headsUpFont;

    struct BuiltInBrush
    {
        const char *name, *texture;
    };
    
    struct CharacterSkin
    {
        Byte4 head_colour, body_colour, leg_colour[2], arm_colour[2], feet_colour;
    };

    class WorldCollisionHandler: public ICollisionHandler, public StageLayer
    {
        protected:
            EntityCollectorByInterface<PointEntity> collidables;

            Var_t *dev_drawbounds;

        public:
            WorldCollisionHandler();
            ~WorldCollisionHandler();

            virtual bool CHCanMove(PointEntity *ent, CFloat3& pos, CFloat3& newPos) override;
            virtual void Draw() override;
    };
    
    class weapon_base : public PointEntity
    {
        protected:
            weapon_base() {}
    };

    class character_base : public ActorEntity
    {
        static const int MAX_WEAPONS = 2;
        
        protected:
            String name;
        
            weapon_base* weapons[MAX_WEAPONS];
        
            CharacterSkin skin;

            float armAnimPhase,     // < 0.0f when holding a gun
                legAnimPhase, eyeDir;

            float gravity;

        protected:
            character_base(CFloat3& pos);
            DECL_PARTIAL_SERIALIZE_UNSERIALIZE()

            virtual bool        CanJump();

        public:
            virtual void        Draw(Name name) override;
            virtual bool        GetBoundingBox(Float3& min, Float3& max) override;
            virtual void        OnFrame(double delta) override;
            virtual void        SetMovement(bool enable, Name movementType, CFloat3& direction) override;
        
            virtual void        GiveWeapon(weapon_base* weapon);
    };
    
    class character_player : public character_base
    {
        protected:
            enum ControlMethod { CTRL_HUMAN, CTRL_AI, CTRL_SCRIPT };

            ControlMethod controlMethod;

        public:
            static IEntity* Create() { return new character_player(Float3()); }
            DECL_SERIALIZE_UNSERIALIZE()
    
            character_player(CFloat3& pos);

            virtual void        OnFrame(double delta) override;
    };
    
    class character_zombie : public character_base
    {
        public:
            static IEntity* Create() { return new character_zombie(Float3()); }
            DECL_SERIALIZE_UNSERIALIZE()
            
            character_zombie(CFloat3& pos);
    };

    class ai_zombie : public AIEntity
    {
        character_zombie* linked_ent;
        
        character_player* target;
        int ticksSinceLastRetarget;

        int16_t linked_ent_id;

        public:
            ai_zombie();
            static IEntity*     Create() { return new ai_zombie(); }
            DECL_SERIALIZE_UNSERIALIZE()
        
            void LinkEntity(character_zombie* ent) { linked_ent = ent; }
            void OnThink(int ticks);
    };

    class prop_woodenbox : public PointEntity
    {
        public:
            prop_woodenbox(CFloat3& pos);
            static IEntity*     Create() { return new prop_woodenbox(Float3()); }
            DECL_SERIALIZE_UNSERIALIZE()

            virtual void        Draw(Name name) override;
            virtual bool        GetBoundingBox(Float3& min, Float3& max) override;
    };

    class weapon_flaregun : public weapon_base
    {
        public:
            static IEntity*     Create() { return new weapon_flaregun(); }
            DECL_SERIALIZE_UNSERIALIZE()
        
        virtual void        Draw(Name name) override;
    };
    
    class world_background : public PointEntity
    {
        public:
            static IEntity*     Create() { return new world_background(); }
            DECL_SERIALIZE_UNSERIALIZE()

            virtual void        Draw(Name name) override;
            virtual bool        GetBoundingBox(Float3& min, Float3& max) override;
    };
    
    class WorldStage:
            public Stage,
            public IScene,
            public IProjectionHandler,
            public ISessionListener
    {
        character_player* player;
        
        // UI
        gameui::UIResManager* uiRes;
        gameui::UIContainer* uiContainer;
        
        gameui::Window *contentMgr, *materialPanel, *menu;
        gameui::Popup *materialMenu, *exportMenu;

        int ui_titlefont;
        int toolbarSelection;

        // variables
        Var_t *r_viewportw, *r_viewporth, *w_fov, *w_topview, *dev_drawbounds;

        MessageQueue* msgQueue;

        protected:
            void ClearWorld();
            void CreateUI();
            void ReleasePrivateMedia();
            void RestoreWorld();
            void SetStatus(const char* status);
            int TryUIEvent(const Event_t* event);

        public:
            WorldStage();
            virtual ~WorldStage();

            virtual void Init();
            virtual void Exit();

            virtual void DrawScene() override;
            virtual void OnFrame( double delta ) override;
            virtual void ReleaseMedia() override;
            virtual void ReloadMedia() override;
        
            virtual int OnIncomingConnection(const char* remoteHostname, const char* playerName, const char* playerPassword) override;

            virtual MessageQueue* GetMessageQueue() override { return msgQueue; }
            virtual void SetUpProjection() override;
    };
}
