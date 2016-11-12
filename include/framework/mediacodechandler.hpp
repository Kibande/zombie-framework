#pragma once

#include <framework/base.hpp>

namespace zfw
{
    enum { kCodecRequired = 1 };

    class IDecoder;
    class IEncoder;

    class IMediaCodecHandler
    {
        public:
            virtual ~IMediaCodecHandler() {}

            // One day...
            //virtual void RegisterCodec(IInterfaceCollection* codec, bool encoder, bool decoder) = 0;

            virtual void RegisterDecoder(const TypeID& iface, unique_ptr<IDecoder>&& decoder) = 0;
            virtual void RegisterEncoder(const TypeID& iface, unique_ptr<IEncoder>&& encoder) = 0;

            //virtual IDecoder* GetDecoder(const TypeID& iface, const char* params, int flags) = 0;
            virtual IDecoder* GetDecoderByFileSignature(const TypeID& iface, const uint8_t* signature,
                    size_t signatureLength, const char* fileNameOrNull, int flags) = 0;

            //virtual IEncoder* GetEncoder(const std::type_index& iface, const char* params, int flags) = 0;
            virtual IEncoder* GetEncoderByFileType(const TypeID& iface, const char* fileType,
                    const char* resourceNameOrNull, int flags) = 0;

            template <class Decoder>
            Decoder* GetDecoderByFileSignature(const uint8_t* signature, size_t signatureLength,
                    const char* fileNameOrNull, int flags)
            {
                return static_cast<Decoder*>(GetDecoderByFileSignature(typeID<Decoder>(), signature, signatureLength,
                        fileNameOrNull, flags));
            }

            template <class Encoder>
            Encoder* GetEncoderByFileType(const char* fileType, const char* resourceNameOrNull, int flags)
            {
                return static_cast<Encoder*>(GetEncoderByFileType(typeid(Encoder), fileType, resourceNameOrNull, flags));
            }
    };

    class IDecoder
    {
        public:
            enum DecodingResult_t
            {
                kOK,
                kError,
                kUnhandled
            };

            virtual ~IDecoder() {}

            virtual bool GetFileSignature(const uint8_t** signature_out, size_t* signatureLength_out) = 0;
            virtual const char* GetName() = 0;
    };

    class IEncoder
    {
        public:
            enum EncodingResult_t
            {
                kOK,
                kError
            };

            virtual ~IEncoder() {}

            virtual bool GetFileTypes(const char*** fileTypes_out, size_t* numFileTypes_out) = 0;
            virtual const char* GetName() = 0;
    };

    class IPixmapDecoder : public IDecoder
    {
        public:
            virtual IDecoder::DecodingResult_t DecodePixmap(Pixmap_t* pm_out, InputStream* stream,
                    const char* fileName/*, IWorker* workerThread = nullptr*/) = 0;
    };

    class IPixmapEncoder : public IEncoder
    {
        public:
            virtual IEncoder::EncodingResult_t EncodePixmap(const Pixmap_t* pm, OutputStream* stream,
                    const char* fileNameOrNull/*, IWorker* workerThread = nullptr*/) = 0;

            //virtual OutputStream* BeginPixmapEncoding(const PixmapInfo_t& properties) = 0;
            //virtual void FinishPixmapEncoding(OutputStream* stream) = 0;
    };

    /*
        +IAudioCodec       ->[RO OggVorbisAudioCodec]
            + IModelCodec       ->[RO AssimpModelCodec would be cool(studio only), R / W ZmfModelCodec]
            + IPixmapCodec      ->[RO JpegPixmapCodec, RO PngPixmapCodec, R / W ZmfPixmapCodec]
    
        +IVideoCodec       ->[RW FFmpegVideoCodec]
    */
}
