
#include <framework/mediacodechandler.hpp>
#include <framework/utility/errorbuffer.hpp>
#include <framework/utility/essentials.hpp>

#include <littl/HashMap.hpp>

#include <algorithm>
#include <vector>

namespace zfw
{
    using namespace li;

    namespace
    {
        // musn't be static (is used as a template argument), but we don't want to pollute the zfw namespace
        size_t getHash(const TypeID& t)
        {
            return t.hash_code();
        }
    }

    // ====================================================================== //
    //  class declaration(s)
    // ====================================================================== //

    class MediaCodecHandler : public IMediaCodecHandler
    {
        struct Signature
        {
            size_t length;
            std::type_index iface;
            IDecoder* decoder;
            uint8_t bytes[8];
        };

        std::vector<std::pair<TypeID, unique_ptr<IDecoder>>> decoders;
        std::vector<std::pair<TypeID, unique_ptr<IEncoder>>> encoders;
        std::vector<Signature> signatures;

        public:
            virtual void RegisterDecoder(const TypeID& iface, unique_ptr<IDecoder>&& decoder) override;
            virtual void RegisterEncoder(const TypeID& iface, unique_ptr<IEncoder>&& encoder) override;

            //virtual IDecoder* GetDecoder(const TypeID& iface, const char* params, int flags) override;
            virtual IDecoder* GetDecoderByFileSignature(const TypeID& iface, const uint8_t* signature,
                size_t signatureLength, const char* fileNameOrNull, int flags) override;

            //virtual void* GetEncoder(const std::type_index& iface, const char* params, int flags) = 0;
            virtual IEncoder* GetEncoderByFileType(const TypeID& iface, const char* fileType,
                    const char* resourceNameOrNull, int flags) override;
    };

    // ====================================================================== //
    //  class MediaCodecHandler
    // ====================================================================== //

    IMediaCodecHandler* p_CreateMediaCodecHandler()
    {
        return new MediaCodecHandler();
    }

    IDecoder* MediaCodecHandler::GetDecoderByFileSignature(const TypeID& iface, const uint8_t* signature,
            size_t signatureLength, const char* fileNameOrNull, int flags)
    {
        for (const auto& s : signatures)
        {
            if (s.iface == iface && s.length <= signatureLength && memcmp(signature, s.bytes, s.length) == 0)
                return s.decoder;
        }

        if (flags & kCodecRequired)
        {
            return ErrorBuffer::SetError3(errNoSuitableCodec, 1,
                    "desc", "No suitable codec found for the provided file signature",
                    "fileName", fileNameOrNull
                    ), nullptr;
        }

        return nullptr;
    }

    IEncoder* MediaCodecHandler::GetEncoderByFileType(const TypeID& iface, const char* fileType,
            const char* resourceNameOrNull, int flags)
    {
        for (const auto& encoder : encoders)
        {
            if (encoder.first != iface)
                continue;

            const char** fileTypes;
            size_t numFileTypes;

            if (encoder.second->GetFileTypes(&fileTypes, &numFileTypes))
            {
                for (size_t i = 0; i < numFileTypes; i++)
                    if (strcmp(fileTypes[i], fileType) == 0)
                        return encoder.second.get();
            }
        }

        if (flags & kCodecRequired)
        {
            return ErrorBuffer::SetError3(errNoSuitableCodec, 1,
                    "desc", "No suitable codec found for the required file type",
                    "resourceName", resourceNameOrNull
                    ), nullptr;
        }

        return nullptr;
    }

    void MediaCodecHandler::RegisterDecoder(const TypeID& iface, unique_ptr<IDecoder>&& decoder)
    {
        const uint8_t* signature;
        size_t signatureLength;

        if (decoder->GetFileSignature(&signature, &signatureLength))
        {
            Signature s {signatureLength, iface, decoder.get()};

            zombie_assert(signatureLength <= lengthof(s.bytes));
            memcpy(s.bytes, signature, signatureLength);

            signatures.push_back(move(s));
        }

        //decoders.set(std::type_index(iface), move(decoder));
        decoders.push_back(std::make_pair(iface, move(decoder)));
    }

    void MediaCodecHandler::RegisterEncoder(const TypeID& iface, unique_ptr<IEncoder>&& encoder)
    {
        encoders.push_back(std::make_pair(iface, move(encoder)));
    }
}
