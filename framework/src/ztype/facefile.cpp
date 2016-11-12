
#include <ztype/ztype.hpp>

#include <littl/File.hpp>

#include <memory>

namespace ztype
{
    using namespace li;

    class FaceFileImpl : public FaceFile
    {
        File file;

        uint32_t num_sections;
        uint32_t *sections;

        uint32_t num_ranges;
        uint32_t *ranges;

        public:
            FaceFileImpl(File&& file, uint32_t num_sections, uint32_t *sections);
            virtual ~FaceFileImpl();

            virtual bool GetRanges(size_t *num_ranges, const uint32_t **ranges) override;
            virtual bool ReadFTexData(uint32_t cp_min, uint32_t count, CharEntry *entries) override;
            virtual bool ReadHashSection(uint8_t *md5) override;
            virtual bool ReadMetrSection(FaceMetrics *metrics) override;

            bool GoToSection(const char* FourCC);
            bool LoadRanges();
    };

    FaceFileImpl::FaceFileImpl(File&& file, uint32_t num_sections, uint32_t *sections)
            : file(std::move(file)), num_sections(num_sections), sections(sections)
    {
        num_ranges = 0;
        ranges = nullptr;
    }

    FaceFileImpl::~FaceFileImpl()
    {
        free(ranges);
        free(sections);
    }

    bool FaceFileImpl::GetRanges(size_t *num_ranges, const uint32_t **ranges)
    {
        if (this->ranges == nullptr && !LoadRanges())
            return false;

        *num_ranges = this->num_ranges;
        *ranges = this->ranges;

        return true;
    }

    bool FaceFileImpl::GoToSection(const char* FourCC)
    {
        size_t i;

        for (i = 0; i < num_sections; i++)
        {
            if (sections[i * 2] == *(uint32_t *)FourCC)
                break;
        }

        if (i == num_sections)
            return false;

        return file.setPos(sections[i * 2 + 1]);
    }

    bool FaceFileImpl::LoadRanges()
    {
        if (!GoToSection("FTex"))
            return false;

        if (!file.readLE(&num_ranges) || num_ranges > 10000)
            return false;

        ranges = (uint32_t *) malloc(num_ranges * 2 * sizeof(uint32_t));

        if (ranges == nullptr)
            return false;

        if (file.read(ranges, num_ranges * 2 * sizeof(uint32_t)) != num_ranges * 2 * sizeof(uint32_t))
        {
            free(ranges);
            ranges = nullptr;
            return false;
        }

        return true;
    }

    bool FaceFileImpl::ReadFTexData(uint32_t cp_min, uint32_t count, CharEntry *entries)
    {
        // FIXME: Use multiple ranges if necessary

        size_t i;
        for (i = 0; i < num_ranges; i++)
            if (cp_min >= ranges[i * 2] && count <= ranges[i * 2 + 1])
                break;

        if (i == num_ranges)
            return false;

        if (!GoToSection("FTex"))
            return false;

        // skip range list, other ranges and leading CharEntries
        if (!file.seek((1 + num_ranges * 2) * sizeof(uint32_t)))
            return false;

        for (size_t j = 0; j < i; j++)
            if (!file.seek(ranges[j * 2 + 1] * sizeof(CharEntry)))
                return false;
        
        if (!file.seek((cp_min - ranges[i * 2]) * sizeof(CharEntry)))
            return false;

        return file.read(entries, count * sizeof(CharEntry)) == count * sizeof(CharEntry);
    }

    bool FaceFileImpl::ReadHashSection(uint8_t *md5)
    {
        if (!GoToSection("Hash"))
            return false;

        return (file.read(md5, 16) == 16);
    }

    bool FaceFileImpl::ReadMetrSection(FaceMetrics *metrics)
    {
        if (!GoToSection("Metr"))
            return false;

        return (file.read(metrics, sizeof(FaceMetrics)) == 1);
    }

    FaceFile* FaceFile::Open(const char *file_name)
    {
        File file(file_name);

        if (!file)
            return nullptr;

        uint32_t magic, num_sections;

        if (!file.readLE(&magic) || magic != 0xCECA87F0)
            return nullptr;

        if (!file.readLE(&num_sections) || num_sections > 1000)
            return nullptr;

        uint32_t *sections = (uint32_t *) malloc(num_sections * 2 * sizeof(uint32_t));

        if (sections == nullptr)
            return nullptr;

        if (file.read(sections, num_sections * 2 * sizeof(uint32_t)) < num_sections * 2 * sizeof(uint32_t))
        {
            free(sections);
            return nullptr;
        }

        return new FaceFileImpl(std::move(file), num_sections, sections);
    }
}
