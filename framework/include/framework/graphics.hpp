#pragma once

#include <framework/base.hpp>
#include <framework/datamodel.hpp>
#include <framework/resource.hpp>

namespace zfw
{
    // ====================================================================== //
    //  interface IGraphics extends IResource
    //      definitions:
    //          type=gradient-hor,c1=<colour1>,c2=<colour2>
    //          type=gradient-vert,c1=<colour1>,c2=<colour2>
    //          type=image,src=<path>
    //          type=image,src=<path>,area=<u0>:<v0>-<u1>:<v1>
    //          type=image,src=<path>,pxarea=<x0>:<y0>-<x1>:<y1>
    //          type=solid,c=<colour>
    //      optional parameters:
    //          size=<w>:<h>    (defaults to 100x100)
    //      multiple states:
    //          <definition>;state=<stateName>;<partial-definition>...
    //      limitations:
    //          implicit state must come first
    //          'type' is mandatory for the implicit state and /must/ always come first within a param set
    //          different states must be same size (will be amended later)
    //
    //      examples:
    //          CreateGraphics("type=image,src=ui/button.png;state=mouseOver;src=ui/button_hover.png")
    //          CreateGraphics("type=solid,c=#FFF;state=dark;type=gradient-vert,c1=#000,c2=#333")
    // ====================================================================== //
    class IGraphics: public IResource
    {
        public:
            virtual void Draw(const UUID_t* modeOrNull, const Float3& pos) = 0;
            virtual void DrawStretched(const UUID_t* modeOrNull, const Float3& pos, const Float3& size) = 0;
            //virtual void OnTick() = 0;

            virtual Float2 GetSize() = 0;
            virtual bool SelectState(const char* stateNameOrNull) = 0;
    };

    class IGraphics2: public IResource2
    {
        public:
            virtual void Draw(const UUID_t* modeOrNull, const Float3& pos) = 0;
            virtual void DrawStretched(const UUID_t* modeOrNull, const Float3& pos, const Float3& size) = 0;
            //virtual void OnTick() = 0;

            virtual Float2 GetSize() = 0;
            virtual bool SelectState(const char* stateNameOrNull) = 0;
    };
}
