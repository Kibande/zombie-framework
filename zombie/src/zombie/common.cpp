
#include "zombie.hpp"

namespace zombie
{
    const HumanoidDef_t humanlike = { 0.2f, 0.25f, 0.25f, 0.3f, 0.6f, 0.1f, 0.08f, 0.45f, 0.1f, 0.7f, 20.0f };

    IFont *font_ui_medium = nullptr, *font_ui_big = nullptr;
    ITexture* tex_buttons = nullptr, *tex_fileicons = nullptr, *tex_uiwidgets = nullptr;

    GamepadCfg_t gamepads[MAX_GAMEPADS];

    void CreateHumanoid(const HumanoidDef_t& def, Humanoid_t& model)
    {
        media::PresetFactory::CreateCube( glm::vec3(def.head_depth, def.head_width, def.head_height), glm::vec3(def.head_depth / 2, def.head_width / 2, 0.0f),
                media::ALL_SIDES | media::WITH_NORMALS, &model.head );

        media::PresetFactory::CreateCube( glm::vec3(def.body_depth, def.body_width, def.body_height), glm::vec3(def.body_depth / 2, def.body_width / 2, def.body_height),
                media::ALL_SIDES | media::WITH_NORMALS, &model.body );

        media::PresetFactory::CreateCube( glm::vec3(def.arm_r, def.arm_r, def.arm_length), glm::vec3(def.arm_r / 2, def.arm_r / 2, def.arm_length - def.arm_r / 2),
                media::ALL_SIDES | media::WITH_NORMALS, &model.arm );

        media::PresetFactory::CreateCube( glm::vec3(def.leg_r, def.leg_r, def.leg_length), glm::vec3(def.leg_r / 2, def.leg_r / 2,def.leg_length),
                media::ALL_SIDES | media::WITH_NORMALS, &model.leg );

        model.arms_angle = def.arms_angle;
    }

    void DrawButtonIcon(int x, int y, int xx, int yy)
    {
        static const float w = 256.0f / 16.0f;
        static const float h = 32.0f / 16.0f;

        glm::vec2 uvs[2] = { glm::vec2(xx / w, yy / h), glm::vec2((xx+1) / w, (yy+1) / h) };
        glm::vec2 uvs1[2] = { glm::vec2(xx / w, (yy+1) / h), glm::vec2((xx+1) / w, (yy+2) / h) };

        R::DrawTexture( tex_buttons, glm::vec2(x + 1, y + 1), glm::vec2(16, 16), uvs1 );
        R::DrawTexture( tex_buttons, glm::vec2(x, y), glm::vec2(16, 16), uvs );
    }

    void DrawHumanoid(Humanoid_t& model, float anim, float angle)
    {
        R::PushTransform( glm::rotate( glm::translate(glm::mat4(), model.pos), angle, glm::vec3(0.0f, 0.0f, -1.0f) ) );

        glm::mat4 trans;
        float legAnim;

        if ( anim < 0.25f )
            legAnim = anim * 25.0f / 0.25f;
        else if ( anim < 0.5f )
            legAnim = 25.0f - (anim - 0.25f) * 25.0f / 0.25f;
        else if ( anim < 0.75f )
            legAnim = (anim - 0.5f) * -25.0f / 0.25f;
        else
            legAnim = -25.0f - (anim - 0.75f) * -25.0f / 0.25f;

        // Head
        R::SetBlendColour( glm::vec4(0.1f, 0.8f, 1.0f, 1.0f) );
        R::DrawDIMesh( &model.head, trans );

        // Body
        R::SetBlendColour( glm::vec4(0.8f, 0.1f, 0.1f, 1.0f) );
        R::DrawDIMesh( &model.body, trans );

        // Left Arm
        R::SetBlendColour( glm::vec4(0.8f, 1.0f, 0.1f, 1.0f) );
        //R::DrawDIMesh( &arm, glm::rotate( glm::translate( trans, glm::vec3( 0.0f, -0.15f - 0.04f, -0.05f )), leftArmRot, glm::vec3(0.0f, -1.0f, 0.0f) ) );
        R::DrawDIMesh( &model.arm, glm::rotate( glm::rotate( glm::translate( trans, glm::vec3( 0.0f, -0.15f - 0.04f, -0.05f )), model.arms_angle, glm::vec3(0.0f, 0.0f, 1.0f) ),
                90.0f, glm::vec3(0.0f, -1.0f, 0.0f) ) );

        // Right Arm
        R::SetBlendColour( glm::vec4(0.8f, 1.0f, 0.1f, 1.0f) );
        //R::DrawDIMesh( &arm, glm::rotate( glm::translate( trans, glm::vec3( 0.0f, -0.15f - 0.04f, -0.05f )), leftArmRot, glm::vec3(0.0f, -1.0f, 0.0f) ) );
        R::DrawDIMesh( &model.arm, glm::rotate( glm::rotate( glm::translate( trans, glm::vec3( 0.0f, 0.15f + 0.04f, -0.05f )), -model.arms_angle, glm::vec3(0.0f, 0.0f, 1.0f) ),
                90.0f, glm::vec3(0.0f, -1.0f, 0.0f) ) );

        // Left Leg
        R::SetBlendColour( glm::vec4(0.1f, 1.0f, 0.1f, 1.0f) );
        R::DrawDIMesh( &model.leg, glm::rotate( glm::translate( trans, glm::vec3( 0.0f, -0.075f, -0.6f )), legAnim, glm::vec3(0.0f, -1.0f, 0.0f) ) );

        // Right Leg
        R::SetBlendColour( glm::vec4(0.1f, 1.0f, 0.1f, 1.0f) );
        R::DrawDIMesh( &model.leg, glm::rotate( glm::translate( trans, glm::vec3( 0.0f, 0.075f, -0.6f )), -legAnim, glm::vec3(0.0f, -1.0f, 0.0f) ) );

        R::PopTransform();
    }

    void DrawUIIcon32(int x, int y, int xx, int yy)
    {
        static const float w = 256.0f / 32.0f;
        static const float h = 256.0f / 32.0f;

        glm::vec2 uvs[2] = { glm::vec2(xx / w, yy / h), glm::vec2((xx+1) / w, (yy+1) / h) };

        R::DrawTexture( tex_uiwidgets, glm::vec2(x, y), glm::vec2(32, 32), uvs );
    }

    void DrawFileIcon32(int x, int y, int xx, int yy)
    {
        static const float w = 256.0f / 32.0f;
        static const float h = 32.0f / 32.0f;

        glm::vec2 uvs[2] = { glm::vec2(xx / w, yy / h), glm::vec2((xx+1) / w, (yy+1) / h) };

        R::DrawTexture( tex_fileicons, glm::vec2(x, y), glm::vec2(32, 32), uvs );
    }

    int DumpGamepadCfg(int index, GamepadCfg_t& cfg)
    {
        String fileName = Sys::tmpsprintf(64, "gamepad_%i.txt", index);
        //rename(fileName, fileName + ".bak");

        Reference<OutputStream> config = Sys::OpenOutput(fileName, STORAGE_CONFIG);

        if ( config == nullptr )
            return EX_LIST_OPEN_ERR;

        for (int i = 0; i < NUM_BUTTONS; i++)
        {
            String vkey = Event::FormatVkey(cfg.buttons[i]);
            config->writeLine( Sys::tmpsprintf(256, "in_gp%i_%s \"%s\"", index, Event::GetButtonShortName(i), vkey.c_str()) );
        }

        return 0;
    }

    int GetGamepadCfg(int index, GamepadCfg_t& cfg)
    {
        for (int i = 0; i < NUM_BUTTONS; i++)
        {
            const char* value = Var::GetStr(Sys::tmpsprintf(20, "in_gp%i_%s", index, Event::GetButtonShortName(i)), false);

            if (value != nullptr)
                Event::ParseVkey(value, cfg.buttons[i]);
        }

        return 0;
    }

#define BTNTEST(FUNC_, NAME_) bool FUNC_(const VkeyState_t& vkey)\
    {\
        for (int i = 0; i < MAX_GAMEPADS; i++)\
            if (memcmp( &vkey, &gamepads[i].NAME_, sizeof(Vkey_t) ) == 0)\
                return true;\
\
        return false;\
    }

    BTNTEST(IsUp, up)
    BTNTEST(IsDown, down)
    BTNTEST(IsLeft, left)
    BTNTEST(IsRight, right)
    BTNTEST(IsA, a)
    BTNTEST(IsB, b)
    BTNTEST(IsX, x)
    BTNTEST(IsY, y)
    BTNTEST(IsL, l)
    BTNTEST(IsR, r)
    BTNTEST(IsMenu, menu)
    BTNTEST(IsMUp, mup)
    BTNTEST(IsMDown, mdown)
    BTNTEST(IsMLeft, mleft)
    BTNTEST(IsMRight, mright)
    BTNTEST(IsAUp, aup)
    BTNTEST(IsADown, adown)
    BTNTEST(IsALeft, aleft)
    BTNTEST(IsARight, aright)

    void SharedReleaseMedia()
    {
        li::destroy( font_ui_medium );
        li::destroy( font_ui_big );
		li::release (tex_buttons);
        li::release( tex_fileicons );
        li::release( tex_uiwidgets );
    }

    void SharedReloadMedia()
    {
        if ( font_ui_medium == nullptr )
            font_ui_medium = Loader::OpenFont( "media/font/DejaVuSans.ttf", 24, FONT_BOLD, 0, 255 );

        if ( font_ui_big == nullptr )
            font_ui_big = Loader::OpenFont( "media/font/DejaVuSans.ttf", 36, FONT_BOLD, 0, 255 );

        if ( tex_buttons == nullptr )
            tex_buttons = Loader::LoadTexture( "media/buttons.png", true );

        if ( tex_fileicons == nullptr )
            tex_fileicons = Loader::LoadTexture( "media/fileicons.png", true );

        if ( tex_uiwidgets == nullptr )
            tex_uiwidgets = Loader::LoadTexture( "media/uiwidgets.png", true );
    }
}
