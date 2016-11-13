
#include <framework/rendering.hpp>

#include <superscanf.h>

#include <cctype>

namespace zfw
{
namespace render
{
    enum
    {
        GRAPHICS_IMAGE, GRAPHICS_ATLAS
    };

    enum
    {
        ANIM_NONE, ANIM_HSTRIP, ANIM_VSTRIP
    };

    class GraphicsImpl : public Graphics
    {
        int type, anim;

        String texture;
        Int2 atlas_pos, atlas_size;
        int numFrames, fps;

        Reference<ITexture> tex;
        Int2 size;
        Float2 uv[2], uv_window, uv_window_at;
        int frame;
        double delta_in;
        float delta_per_frame;

        public:
            GraphicsImpl(const char* texture)
                    : type(GRAPHICS_IMAGE), anim(ANIM_NONE), texture(texture)
            {
            }

            GraphicsImpl(const char* texture, const Int2& atlas_pos, const Int2& atlas_size)
                    : type(GRAPHICS_ATLAS), anim(ANIM_NONE), texture(texture), atlas_pos(atlas_pos), atlas_size(atlas_size)
            {
            }

            virtual void AcquireResources() override
            {
                tex = Loader::LoadTexture(texture, true);

                if (type == GRAPHICS_IMAGE) {
                    uv[0] = Float2(0.0f, 0.0f);
                    uv[1] = Float2(1.0f, 1.0f);
                    size = Float2(tex->GetSize());
                } else {
                    uv[0] = Float2(atlas_pos) / Float2(tex->GetSize());
                    uv[1] = Float2(atlas_pos + atlas_size) / Float2(tex->GetSize());
                    size = atlas_size;
                }

                if (anim == ANIM_HSTRIP) {
                    uv_window = (uv[1] - uv[0]) / Float2(numFrames, 1.0f);
                    uv_window_at = uv[0];
                    size.x /= numFrames;
                } else if (anim == ANIM_VSTRIP) {
                    uv_window = (uv[1] - uv[0]) / Float2(1.0f, numFrames);
                    uv_window_at = uv[0];
                    size.y /= numFrames;
                }
            }

            virtual void DropResources() override
            {
                tex.release();
            }

            virtual void OnFrame(double delta) override
            {
                if (anim != ANIM_NONE)
                {
                    delta_in += delta;

                    while (delta_in > delta_per_frame)
                    {
                        if (++frame < numFrames)
                        {
                            if (anim == ANIM_HSTRIP)
                                uv_window_at.x += uv_window.x;
                            else if (anim == ANIM_VSTRIP)
                                uv_window_at.y += uv_window.y;
                        }
                        else
                        {
                            frame = 0;
                            uv_window_at = uv[0];
                        }

                        delta_in -= delta_per_frame;
                    }
                }
            }

            virtual Int2 GetSize() override
            {
                return size;
            }

            virtual void Draw(const Short2& pos, const Short2& size) override
            {
                zr::R::SetTexture(tex);

                if (anim == ANIM_NONE)
                    zr::R::DrawFilledRect(pos, pos + size, uv[0], uv[1], RGBA_WHITE);
                else
                    zr::R::DrawFilledRect(pos, pos + size, uv_window_at, uv_window_at + uv_window, RGBA_WHITE);
            }

            virtual void DrawBillboarded(const Float3& pos, const Float2& size) override
            {
                zr::R::SetTexture(tex);

                if (anim == ANIM_NONE)
                    zr::R::DrawRectBillboard(pos, size, uv[0], uv[1], RGBA_WHITE);
                else
                    zr::R::DrawRectBillboard(pos, size, uv_window_at, uv_window_at + uv_window, RGBA_WHITE);
            }

            void SetAnim(int type, int frames, int fps)
            {
                anim = type;
                numFrames = frames;
                this->fps = fps;
                frame = 0;
                delta_in = 0.0;
                delta_per_frame = 1.0f / fps;
            }
    };

    Graphics* Graphics::Create( const char* desc, bool required )
    {
        char* new_desc;

        char path_buffer[1024];
        Int2 atlas_pos, atlas_size;

        Reference<GraphicsImpl> graphics;

        // FIXME: Texture reusage

        if (ssscanf2(desc, (char **)&new_desc, 0, "image(%S)", path_buffer, sizeof(path_buffer)) == 1)
        {
            graphics = new GraphicsImpl(path_buffer);
        }
        else if (ssscanf2(desc, (char **)&new_desc, 0, "atlas(%S,%i,%i,%i,%i)", path_buffer, sizeof(path_buffer),
                &atlas_pos.x, &atlas_pos.y, &atlas_size.x, &atlas_size.y) == 5)
        {
            graphics = new GraphicsImpl(path_buffer, atlas_pos, atlas_size);
        }
        else
        {
            // FIXME
            ZFW_ASSERT(false)
        }

        while (new_desc && isspace(*new_desc))
            new_desc++;

        int num_frames, fps;

        if (ssscanf(new_desc, 0, "anim_hstrip(%i,%i)", &num_frames, &fps) == 2)
        {
            graphics->SetAnim(ANIM_HSTRIP, num_frames, fps);
        }
        else if (ssscanf(new_desc, 0, "anim_vstrip(%i,%i)", &num_frames, &fps) == 2)
        {
            graphics->SetAnim(ANIM_VSTRIP, num_frames, fps);
        }

        return graphics.detach();
    }
}
}
