#ifndef Guier_hpp
#define Guier_hpp

#include <gameui/gameui.hpp>

#include <framework/utility/essentials.hpp>
#include <framework/utility/params.hpp>
#include <framework/utility/util.hpp>

#include <optional>
#include <string>
#include <vector>

namespace guier {
    struct Guier_FontBuilder {
        auto Path(std::string&& path_) { this->path_ = move(path_); return *this; }
        auto Size(int size_) { this->size_ = size_; return *this; }

        std::optional<std::string> path_;
        std::optional<int> size_;
    };

    struct _GuiContainerState {
        gameui::WidgetContainer* widget;
        bool saved = false;

        std::vector<_GuiContainerState> childStates;

        template <typename Class>
        Class* Self() { return static_cast<Class*>(widget); }
    };

    struct Guier;
    struct Guier_TableLayout;

    struct Guier_Button {
        Guier_Button(gameui::Button* widget) : widget(widget) {}

        bool WasClicked() { return widget->WasClicked(); }

        gameui::Button* widget;
    };

    struct Guier_Container {
        Guier_Container(Guier& g, _GuiContainerState& state, bool newlyCreated) : g(g), state(state), newlyCreated(newlyCreated) {}

        Guier_TableLayout TableLayout(int numColumns);

        Guier_Button Button(const char* text);
        void StaticText(const char* text, const Guier_FontBuilder& font);

        bool IsSaved() const { return state.saved; }
        void Save() { state.saved = true; }
        void Unsave() { state.saved = false; }

        Guier& g;
        _GuiContainerState& state;
        bool newlyCreated;

        size_t nextContainerIndex = 0;
        size_t nextWidgetIndex = 0;
    };

    struct Guier_TableLayout : Guier_Container {
        using Guier_Container::Guier_Container;

        void SetColumnGrowable(size_t column, bool growable) { if (newlyCreated) this->state.Self<gameui::TableLayout>()->SetColumnGrowable(column, growable); }
        void SetRowGrowable(size_t row, bool growable) { if (newlyCreated) this->state.Self<gameui::TableLayout>()->SetRowGrowable(row, growable); }
    };

    struct Guier_Ui : Guier_Container {
        using Guier_Container::Guier_Container;
    };

    struct Guier {
        Guier(zfw::IEngine* sys, gameui::UIThemer* themer, gameui::UIContainer* container) : sys(sys), themer(themer), container(container) {}

        Guier_Ui Ui() {
            if (state) {
                if (!state->saved) {
                    state->widget->RemoveAll();
                    state->childStates.clear();
                }

                return Guier_Ui(*this, *state, false);
            }

            state = {container};
            return Guier_Ui(*this, *state, true);
        }

        intptr_t GetFont(const Guier_FontBuilder& fontBuilder) {
            zombie_assert(fontBuilder.path_ && fontBuilder.size_);

            // FIXME: will use slow lookup
            auto x = zfw::Params::BuildAlloc(2, "path", fontBuilder.path_->c_str(), "size", sprintf_15("%d", *fontBuilder.size_));
            auto id = themer->GetFontId(x);
            free(x);
            return id;
        }

        zfw::IEngine* sys;
        gameui::UIThemer* themer;
        gameui::UIContainer* container;

        std::optional<_GuiContainerState> state;
    };

    Guier_Button Guier_Container::Button(const char* text) {
        size_t idx = nextContainerIndex + nextWidgetIndex;      // TODO: explain this!!
        nextWidgetIndex++;

        auto& l = state.widget->GetWidgetList();

        if (idx < l.getLength()) {
            return static_cast<gameui::Button*>(l[idx]);
        }
        else {
            auto widget = new gameui::Button(g.themer, text);
            widget->AcquireResources();
            state.widget->Add(widget);
            return widget;
        }
    }

    void Guier_Container::StaticText(const char* text, const Guier_FontBuilder& font) {
        size_t idx = nextContainerIndex + nextWidgetIndex;      // TODO: explain this!!
        nextWidgetIndex++;

        auto& l = state.widget->GetWidgetList();

        if (idx < l.getLength()) {
            return;
        }
        else {
            auto widget = new gameui::StaticText(g.themer, text, g.GetFont(font));
            widget->AcquireResources();
            state.widget->Add(widget);
        }
    }

    Guier_TableLayout Guier_Container::TableLayout(int numColumns) {
        size_t pos = nextContainerIndex;
        nextContainerIndex++;

        if (pos < state.childStates.size()) {
            // exists, but contents might not be saved
            if (!state.childStates[pos].saved) {
                state.childStates[pos].widget->RemoveAll();
                state.childStates[pos].childStates.clear();
            }

            return Guier_TableLayout {g, state.childStates[pos], false};
        }

        auto tableLayout = new gameui::TableLayout(numColumns);
        tableLayout->SetOnlineUpdate(false);
        state.widget->Add(tableLayout);
        state.childStates.push_back(_GuiContainerState {tableLayout});
        return Guier_TableLayout {g, state.childStates.back(), true};
    }
}

#endif
