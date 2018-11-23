
#include <StudioKit/mapwriter.hpp>

#include <framework/engine.hpp>
#include <framework/errorbuffer.hpp>

#include <bleb/repository.hpp>

#include <littl+bleb/ByteIOStream.hpp>
#include <littl+bleb/StreamByteIO.hpp>

namespace StudioKit
{
    using namespace zfw;

    // ====================================================================== //
    //  class declaration(s)
    // ====================================================================== //

    class MapWriter : public IMapWriter
    {
        public:
            MapWriter();
            ~MapWriter();

            virtual bool Init(zfw::IEngine* sys, IOStream* outputContainerFile, const char* outputName) override;
            virtual void SetMetadata(const char* key, const char* value) override;

            virtual void AddEntity(const char* entityCfx2) override;
            virtual void AddResource(const char* path, zfw::InputStream* file) override;

            virtual bool GetOutputStreams1(OutputStream** materials, OutputStream** vertices) override;

            virtual bool Finish() override;

        private:
            ErrorBuffer_t* eb;
            IEngine* sys;

            std::string outputName;
            int numEntitiesWritten = 0;
            int numResourcesWritten = 0;

            IOStream* outputContainerFile;
            li::StreamByteIO bio;
            bleb::Repository repo;

            unique_ptr<bleb::ByteIO> entities_;
            unique_ptr<OutputStream> entities;

            unique_ptr<bleb::ByteIO> materials_;
            unique_ptr<OutputStream> materials;

            unique_ptr<bleb::ByteIO> geometry_;
            unique_ptr<OutputStream> geometry;
    };

    // ====================================================================== //
    //  class MapWriter
    // ====================================================================== //

    IMapWriter* CreateMapWriter()
    {
        return new MapWriter();
    }

    MapWriter::MapWriter() : repo(&bio)
    {
    }

    MapWriter::~MapWriter()
    {
        entities.reset();
    }

    bool MapWriter::Init(zfw::IEngine* sys, IOStream* outputContainerFile, const char* outputName)
    {
        SetEssentials(sys->GetEssentials());

        this->eb = GetErrorBuffer();
        this->sys = sys;

        bio.set(outputContainerFile);

        repo.setAllocationGranularity(4096);
        
        if (!repo.open(true))
            return ErrorBuffer::SetError3(EX_ASSET_OPEN_ERR, 2,
                    "desc", sprintf_4095("bleb error: %s", repo.getErrorDesc()),
                    "function", li_functionName
                    ), false;

        this->outputName = outputName;

        return true;
    }

    void MapWriter::AddEntity(const char* entityCfx2)
    {
        if (!entities)
        {
            entities_ = repo.openStream(sprintf_255("%s/entities", outputName.c_str()), bleb::kStreamCreate | bleb::kStreamTruncate);
            zombie_assert(entities_);
            entities = std::make_unique<li::ByteIOStream>(entities_.get());

            numEntitiesWritten++;
        }

        entities->writeString(entityCfx2);
    }

    void MapWriter::AddResource(const char* path, zfw::InputStream* file)
    {
        auto output_ = repo.openStream(path, bleb::kStreamCreate | bleb::kStreamTruncate);
        zombie_assert(output_);
        li::ByteIOStream output(output_.get());

        output.copyFrom(file);

        numResourcesWritten++;
    }

    bool MapWriter::Finish()
    {
        /*sys->Printf(kLogInfo, "%8i material groups",    (int) matGrps.getLength());
        sys->Printf(kLogInfo, "%8i faces",              (int) totalVerts / 3);
        sys->Printf(kLogInfo, "%8i total vertices",     (int) totalVerts);*/
        sys->Printf(kLogInfo, "%8u entities", numEntitiesWritten);
        sys->Printf(kLogInfo, "%8u embedded resources", numResourcesWritten);

        return true;
    }

    bool MapWriter::GetOutputStreams1(OutputStream** materials, OutputStream** vertices)
    {
        materials_ = repo.openStream(sprintf_255("%s/materials", outputName.c_str()), bleb::kStreamCreate | bleb::kStreamTruncate);
        zombie_assert(materials_);
        this->materials = std::make_unique<li::ByteIOStream>(materials_.get());
        *materials = this->materials.get();

        geometry_ = repo.openStream(sprintf_255("%s/geometry", outputName.c_str()), bleb::kStreamCreate | bleb::kStreamTruncate);
        zombie_assert(geometry_);
        this->geometry = std::make_unique<li::ByteIOStream>(geometry_.get());
        *vertices = this->geometry.get();

        return true;
    }

    void MapWriter::SetMetadata(const char* key, const char* value)
    {
        repo.setObjectContents(sprintf_255("metadata/%s", key), value, bleb::kPreferInlinePayload);
    }
}
