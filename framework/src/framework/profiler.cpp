
#include <framework/profiler.hpp>

#include <littl/PerfTiming.hpp>

namespace zfw
{
    using namespace li;

    static ProfilingSection_t root = {"_root"};

    // ====================================================================== //
    //  class declaration(s)
    // ====================================================================== //

    class ProfilerImpl : public Profiler
    {
        public:
            virtual void BeginProfiling() final override;
            virtual void EnterSection(ProfilingSection_t& section) final override;
            virtual void LeaveSection() final override;
            virtual void EndProfiling() final override;

            virtual void PrintProfile() final override;

        private:
            void p_PrintSectionProfile(ProfilingSection_t* section, int indent);

            PerfTimer timer;

            ProfilingSection_t* current;
    };

    // ====================================================================== //
    //  class Profiler
    // ====================================================================== //

    Profiler* Profiler::Create()
    {
        return new ProfilerImpl();
    }

    void ProfilerImpl::BeginProfiling()
    {
        current = nullptr;
        //timer.start();

        EnterSection(root);
    }

    void ProfilerImpl::EndProfiling()
    {
        LeaveSection();
    }

    void ProfilerImpl::EnterSection(ProfilingSection_t& section)
    {
        if (current)
        {
            if (!current->firstChild)
                current->firstChild = &section;
            else
                current->lastChild->nextSibling = &section;

            current->lastChild = &section;
        }

        section.parent = current;
        current = &section;
        current->begin = timer.getCurrentMicros();
        current->nextSibling = nullptr;
        current->firstChild = nullptr;
        current->lastChild = nullptr;
    }

    void ProfilerImpl::LeaveSection()
    {
        current->end = timer.getCurrentMicros();
        current = current->parent;
    }

    void ProfilerImpl::p_PrintSectionProfile(ProfilingSection_t* section, int indent)
    {
        for (int i = 0; i < indent; i++)
            printf("| ");

        auto sectionTime = section->end - section->begin;
        printf("%s ", section->name);

        for (int i = strlen(section->name); i < 30; i++)
            printf(" ");

        printf(" %.6f ms\n", sectionTime / 1000.0f);

        if (section->firstChild)
            p_PrintSectionProfile(section->firstChild, indent + 1);

        if (section->nextSibling)
            p_PrintSectionProfile(section->nextSibling, indent);
    }

    void ProfilerImpl::PrintProfile()
    {
        p_PrintSectionProfile(&root, 0);


        /*printf("\nProfile [%5u.%06u -\n", (int)(profileMin / 1000000), (int)(profileMin % 1000000));

        iterate2 (i, sections)
        {
            if ((*i).flags & SECT_DISABLED)
                continue;

            auto t0 = (*i).begin - profileMin;
            auto t1 = (*i).end - profileMin;
            auto d = t1 - t0;

            printf("  +[%5u.%06u .. %5u.%06u] %5u.%06u\t'%s'\n",
                (int)(t0 / 1000000), (int)(t0 % 1000000),
                (int)(t1 / 1000000), (int)(t1 % 1000000),
                (int)(d / 1000000), (int)(d % 1000000),
                (*i).name);
        }

        printf("+ %5u.%06u]\n\n", (int)((profileMax - profileMin) / 1000000), (int)((profileMax - profileMin) % 1000000));*/
    }
}
