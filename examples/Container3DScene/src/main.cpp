#include <Container/ContainerApp.hpp>
#include <Container/ContainerScene.hpp>

#include <framework/colorconstants.hpp>
#include <framework/event.hpp>
#include <framework/messagequeue.hpp>
#include <framework/pointentity.hpp>
#include <framework/system.hpp>

#include <RenderingKit/utility/CameraMouseControl.hpp>
#include <RenderingKit/utility/TexturedPainter.hpp>

namespace example {
    using Container::ContainerApp;
    using Container::ContainerScene;
    using namespace zfw;

    // TODO: An instance should be provided by RenderingManager
    static RenderingKit::TexturedPainter3D<> s_tp;

    class Floor : public PointEntityBase {
    public:
        virtual bool Init() override {
            tex.ByPath("media/texture/floor.jpg");

            return true;
        }

        virtual void Draw(const UUID_t* modeOrNull) override {
            if (modeOrNull == nullptr) {
                const Float3 size(10.0f, 10.0f, 0.0f);
                s_tp.DrawFilledPlane(*tex, pos - size * 0.5f, size, RGBA_WHITE);
            }
        }

    private:
        Resource<RenderingKit::ITexture> tex;
    };

    class ExampleScene : public ContainerScene {
    public:
        ExampleScene(ContainerApp* app) : ContainerScene(app, kUseWorld) {}

        virtual bool PreBindDependencies() override {
            this->SetClearColor(Float4(0.9f, 0.9f, 0.9f, 1.0f));

            auto rm = app->GetRenderingHandler()->GetRenderingKit()->GetRenderingManager();
            ErrorCheck(s_tp.Init(rm));

            auto floor = std::make_shared<Floor>();
            this->world->AddEntity(floor);

            auto cam = rm->CreateCamera("Example Camera");
            cam->SetClippingDist(0.2f, 100.0f);
            cam->SetPerspective();
            cam->SetVFov(45.0f * f_pi / 180.0f);
            cam->SetViewWithCenterDistanceYawPitch(Float3(0.0f, 0.0f, 1.6f), -1.0f, 0.0f, 0.0f);

            camControl.SetCamera(cam.get());

            SetWorldCamera(std::move(cam));

            return true;
        }

        virtual bool HandleEvent(MessageHeader* msg) override {
            camControl.HandleMessage(msg);
            return false;
        }

    private:
        RenderingKit::RKCameraMouseControl camControl;
    };

    class MyContainerApp : public ContainerApp {
    public:
        MyContainerApp() : ContainerApp("Container3DScene") {
        }

        std::shared_ptr<zfw::IScene> CreateInitialScene() {
            return std::make_shared<ExampleScene>(this);
        }
    };
}

int main(int argc, char** argv) {
    return Container::runContainerApp<example::MyContainerApp>(argc, argv);
}
