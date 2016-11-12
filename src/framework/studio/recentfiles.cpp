
#include "recentfiles.hpp"

#include <framework/framework.hpp>

#include <utility>

namespace zfw
{
namespace studio
{
    using namespace li;

    class RecentFilesImpl : public RecentFiles
    {
        public:
            RecentFilesImpl(ISystem* sys);

            virtual bool Load(const char* filename) override;
            virtual bool Save(const char* filename) override;

            bool StreamLoad(InputStream* stream);
            bool StreamSave(OutputStream* stream);

            virtual void BumpItem(const char* item) override;
            virtual size_t GetNumItems() override { return items.getLength(); }
            virtual bool IsDirty() override { return isDirty; }
            virtual const char* GetItemAt(size_t index) { return (index < items.getLength()) ? items[index].c_str() : nullptr; }
            virtual void SetMaxNumItems(size_t maxNumItems) override { this->maxNumItems = maxNumItems; }

        protected:
            ISystem* sys;

            List<String> items;
            size_t maxNumItems;

            bool isDirty;
    };

    RecentFiles* RecentFiles::Create(ISystem* sys)
    {
        return new RecentFilesImpl(sys);
    }

    RecentFilesImpl::RecentFilesImpl(ISystem* sys) : sys(sys)
    {
        maxNumItems = 10;
        isDirty = false;
    }

    void RecentFilesImpl::BumpItem(const char* item)
    {
        if (Util::StrEmpty(item))
            return;

        intptr_t pos = items.find(item);

        if (pos > 0)
            items.remove(pos);

        if (pos != 0)
        {
            items.insert(item, 0);
            isDirty = true;
        }
    }

    bool RecentFilesImpl::Load(const char* filename)
    {
        unique_ptr<InputStream> is(sys->OpenInput(filename));

        if (is != nullptr)
            return StreamLoad(is.get());
        else
            return false;
    }

    bool RecentFilesImpl::Save(const char* filename)
    {
        if (!isDirty)
            return true;

        unique_ptr<OutputStream> os(sys->OpenOutput(filename));

        if (os != nullptr)
            return StreamSave(os.get());
        else
            return false;
    }

    bool RecentFilesImpl::StreamLoad(InputStream* stream)
    {
        items.clear();

        while (true)
        {
            String entry = stream->readLine();

            if (entry.isEmpty())
                break;

            items.add(move(entry));
        }

        isDirty = false;
        return true;
    }

    bool RecentFilesImpl::StreamSave(OutputStream* stream)
    {
        iterate2 (i, items)
            stream->writeLine((*i));

        return true;
    }
}
}
