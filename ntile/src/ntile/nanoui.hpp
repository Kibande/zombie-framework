#pragma once

#include <ntile/nbase.hpp>

#include <framework/datamodel.hpp>
#include <framework/event.hpp>

namespace n3d
{
    class IFont;
}

namespace ntile
{
    class NanoUI
    {
        public:
            enum { kMessageInstant = 1, kMessageHasLocation = 2 };

            NanoUI() : numDialogs(0), font(nullptr) {}

            bool Init();
            void Shutdown() {}

            bool IsBlocking();
            int HandleMessage(int h, MessageHeader* msg);
            void OnTick();

            void ShowMessage(const char* text, int flags);

            void Draw();

        private:
            enum { kMaxDialogs = 1 };

            struct Dialog_t
            {
                String message;
                unsigned int charsShown;
                int flags, wait;
                Float3 location;
            };

            Dialog_t dialogStack[kMaxDialogs];
            size_t numDialogs;

            Vkey_t button;
            n3d::IFont* font;
    };
}
