
// Project:             Zombie Framework (Minexew Games 2013)

// Safety:              64-bit          UNTESTED SAFE
// Safety:              endianness      UNTESTED SAFE

// Copyright:           Uses libyuv (3-BSD)
// Copyright:           Uses vpx (3-BSD)
// Copyright:           Contains example code from vpx
// Copyright:           Original code by Xeatheran Minexew, 2013

#include <framework/vp8videocapture.hpp>

#include <libyuv/convert.h>

#define VPX_CODEC_DISABLE_COMPAT 1
#include <vpx/vpx_encoder.h>
#include <vpx/vp8cx.h>

// warning C4706: assignment within conditional expression
#pragma warning (disable : 4706)

#define interface   (vpx_codec_vp8_cx())
#define fourcc      0x30385056

namespace zfw
{
    static void mem_put_le16(char *mem, unsigned int val)
    {
        mem[0] = val;
        mem[1] = val>>8;
    }
 
    static void mem_put_le32(char *mem, unsigned int val)
    {
        mem[0] = val;
        mem[1] = val>>8;
        mem[2] = val>>16;
        mem[3] = val>>24;
    }

    static int die_codec(vpx_codec_ctx_t *ctx, const char *s)
    {
        const char *detail = vpx_codec_error_detail(ctx);

        printf("%s: %s\n", s, vpx_codec_error(ctx));
        if (detail)
            printf("    %s\n",detail);

        return -1;
    }

    static void write_ivf_file_header(SeekableOutputStream *outfile,
            const vpx_codec_enc_cfg_t *cfg, int frame_cnt)
    {
        char header[32];
 
        if (cfg->g_pass != VPX_RC_ONE_PASS && cfg->g_pass != VPX_RC_LAST_PASS)
            return;

        header[0] = 'D';
        header[1] = 'K';
        header[2] = 'I';
        header[3] = 'F';
        mem_put_le16(header+4,  0);                   /* version */
        mem_put_le16(header+6,  32);                  /* headersize */
        mem_put_le32(header+8,  fourcc);              /* fourcc */
        mem_put_le16(header+12, cfg->g_w);            /* width */
        mem_put_le16(header+14, cfg->g_h);            /* height */
        mem_put_le32(header+16, cfg->g_timebase.den); /* rate */
        mem_put_le32(header+20, cfg->g_timebase.num); /* scale */
        mem_put_le32(header+24, frame_cnt);           /* length */
        mem_put_le32(header+28, 0);                   /* unused */
 
        outfile->rawWrite(header, 32);
    }

    static void write_ivf_frame_header(SeekableOutputStream *outfile, const vpx_codec_cx_pkt_t *pkt)
    {
        char             header[12];
        vpx_codec_pts_t  pts;
 
        if (pkt->kind != VPX_CODEC_CX_FRAME_PKT)
            return;
 
        pts = pkt->data.frame.pts;
        mem_put_le32(header, pkt->data.frame.sz);
        mem_put_le32(header+4, pts&0xFFFFFFFF);
        mem_put_le32(header+8, pts >> 32);
 
        outfile->rawWrite(header, 12);
    }

    class VP8VideoCaptureImpl : public IVideoCapture
    {
        SeekableOutputStream*   output;
        Int2                    resolution;
        Int2                    frameRateNumDen;

        uint8_t*                frameBuffer;

        double tickTime, timeSkew, videoFrameTime;

        // libvpx stuff
        vpx_codec_ctx_t         codec;
        vpx_codec_enc_cfg_t     cfg;
        vpx_image_t             raw;

        int                     frame_cnt;
        int                     flags;

        public:
            VP8VideoCaptureImpl(SeekableOutputStream* output, Int2 resolution, Int2 frameRateNumDen)
                    : output(output), resolution(resolution), frameRateNumDen(frameRateNumDen)
            {
                videoFrameTime = (double) frameRateNumDen.y / frameRateNumDen.x;
            }

            virtual void Release() override
            {
                delete this;
            }

            virtual int StartCapture(Int2 resolution, int tickRate) override;
            virtual int EndCapture() override;

            virtual int OfferFrameBGRFlipped(int frameid, int frameticks, uint8_t** rgb_buf_out) override;
            virtual void OnFrameReady(int frameid) override;
    };

    int VP8VideoCaptureImpl::EndCapture()
    {
        bool got_data;

        do
        {
            vpx_codec_iter_t iter = NULL;
            const vpx_codec_cx_pkt_t *pkt;
 
            if (vpx_codec_encode(&codec, NULL, frame_cnt, 1, flags, VPX_DL_REALTIME))
                return die_codec(&codec, "Failed to encode frame");

            got_data = 0;

            while ((pkt = vpx_codec_get_cx_data(&codec, &iter)))
            {
                got_data = 1;

                switch(pkt->kind)
                {
                    case VPX_CODEC_CX_FRAME_PKT:
                        write_ivf_frame_header(output, pkt);
                        output->rawWrite(pkt->data.frame.buf, pkt->data.frame.sz);
                        break;

                    default:
                        break;
                }

                printf(pkt->kind == VPX_CODEC_CX_FRAME_PKT && (pkt->data.frame.flags & VPX_FRAME_IS_KEY) ? "K":".");
            }

            frame_cnt++;
        }
        while (got_data);
 
        printf("\nvp8enc:\tProcessed %d frames.\n",frame_cnt-1);
        vpx_img_free(&raw);
        if (vpx_codec_destroy(&codec))
            return die_codec(&codec, "Failed to destroy codec");
 
        if (output->setPos(0))
            write_ivf_file_header(output, &cfg, frame_cnt-1);

        return 0;
    }

    int VP8VideoCaptureImpl::OfferFrameBGRFlipped(int frameid, int frameticks, uint8_t** rgb_buf_out)
    {
        timeSkew += frameticks * tickTime;

        if (timeSkew > videoFrameTime)
        {
            *rgb_buf_out = frameBuffer;
            return 1;
        }
        else
            return 0;
    }

    void VP8VideoCaptureImpl::OnFrameReady(int frameid)
    {
        // Convert RGB data to YUV 4:2:0 and flip vertically (negative height)
        libyuv::RGB24ToI420(frameBuffer, resolution.x * 3,
                raw.planes[0], raw.stride[0],
                raw.planes[1], raw.stride[1],
                raw.planes[2], raw.stride[2],
                raw.w, -raw.h);

        // Insert one frame multiple times if needed
        while (timeSkew > videoFrameTime)
        {
            vpx_codec_iter_t iter = NULL;
            const vpx_codec_cx_pkt_t *pkt;
 
            if (vpx_codec_encode(&codec, &raw, frame_cnt, 1, flags, VPX_DL_REALTIME))
            {
                die_codec(&codec, "Failed to encode frame");
                return;
            }

            while ((pkt = vpx_codec_get_cx_data(&codec, &iter)))
            {
                switch (pkt->kind)
                {
                    case VPX_CODEC_CX_FRAME_PKT:
                        write_ivf_frame_header(output, pkt);
                        output->rawWrite(pkt->data.frame.buf, pkt->data.frame.sz);
                        break;

                    default:
                        break;
                }

                //printf(pkt->kind == VPX_CODEC_CX_FRAME_PKT && (pkt->data.frame.flags & VPX_FRAME_IS_KEY) ? "K":".");
            }

            timeSkew -= videoFrameTime;
            frame_cnt++;
        }
    }

    int VP8VideoCaptureImpl::StartCapture(Int2 resolution, int tickRate)
    {
        tickTime = 1.0 / tickRate;
        timeSkew = 0.0;

        vpx_codec_err_t      res;

        const int width = resolution.x, height = resolution.y;

        ZFW_ASSERT(width == this->resolution.x)
        ZFW_ASSERT(height == this->resolution.y)

        ZFW_ASSERT(width >= 16 && width % 2 == 0)
        ZFW_ASSERT(height >= 16 && height % 2 == 0)

        ZFW_ASSERT(vpx_img_alloc(&raw, VPX_IMG_FMT_I420, width, height, 1))

        printf("vp8cap:\tUsing %s\n",vpx_codec_iface_name(interface));
 
        frame_cnt = 0;
        flags = 0;

        // Populate encoder configuration
        res = vpx_codec_enc_config_default(interface, &cfg, 0);
        if (res)
        {
            printf("vp8cap:\tFailed to get config: %s\n", vpx_codec_err_to_string(res));
            return -1;
        }
 
        // Update the default configuration with our settings
        cfg.rc_target_bitrate = width * height * cfg.rc_target_bitrate / cfg.g_w / cfg.g_h;
        cfg.g_w = width;
        cfg.g_h = height;
        cfg.g_timebase.num = frameRateNumDen.y;
        cfg.g_timebase.den = frameRateNumDen.x;
 
        write_ivf_file_header(output, &cfg, 0);
 
        // Initialize codec
        if (vpx_codec_enc_init(&codec, interface, &cfg, 0))
            return die_codec(&codec, "Failed to initialize encoder");
 
        frameBuffer = Allocator<>::allocate(width * height * 3);

        return 0;
    }

    IVideoCapture* VP8VideoCapture::Create(SeekableOutputStream* output, Int2 resolution, Int2 frameRateNumDen)
    {
        return new VP8VideoCaptureImpl(output, resolution, frameRateNumDen);
    }
}
