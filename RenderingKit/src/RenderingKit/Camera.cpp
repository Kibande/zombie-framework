
#include "RenderingKitImpl.hpp"

namespace RenderingKit
{
    enum RKProjectionType_t
    {
        kProjectionInvalid,
        kProjectionOrtho,
        kProjectionOrthoFakeFov,
        kProjectionOrthoScreenSpace,
        kProjectionPerspective
    };

    class GLCamera : public IGLCamera
    {
        RenderingKit* rk;
        IRenderingManagerBackend* rm;
        String name;
        int32_t numRefs;

        RKProjectionType_t proj;
        Float3 eye, center, up;
        float vfov;
        float left, right, top, bottom;
        float nearZ, farZ;

        glm::mat4 projection, modelView;

        void BuildModelView();

        public:
            GLCamera(zfw::ErrorBuffer_t* eb, RenderingKit* rk, IRenderingManagerBackend* rm, const char* name);
            
            virtual const char* GetName() override { return name.c_str(); }

            virtual float CameraGetDistance() override;
            virtual void CameraMove(const Float3& vec) override;
            virtual void CameraRotateXY(float alpha, bool absolute) override;
            virtual void CameraRotateZ(float alpha, bool absolute) override;
            virtual void CameraZoom(float amount, bool absolute) override;

            virtual Float3 GetCenter() override { return center; }
            virtual Float3 GetEye() override { return eye; }
            virtual Float3 GetUp() override { return up; }

            virtual void GetModelViewMatrix(glm::mat4* output) override
            {
                *output = modelView;
            }

            virtual void GetProjectionModelViewMatrix(glm::mat4* output) override
            {
                *output = (projection * modelView);
            }

            virtual void SetClippingDist(float nearClip, float farClip) override;
            virtual void SetOrtho(float left, float right, float top, float bottom) override;
            virtual void SetOrthoFakeFOV() override { proj = kProjectionOrthoFakeFov; }
            virtual void SetOrthoScreenSpace() override { proj = kProjectionOrthoScreenSpace; }
            virtual void SetPerspective() override { proj = kProjectionPerspective; }
            virtual void SetVFov(float vfov_radians) override { this->vfov = vfov_radians; }
            virtual void SetView(const Float3& eye, const Float3& center, const Float3& up) override;
            virtual void SetView2(const Float3& center, const float eyeDistance, float yaw, float pitch) override;

            virtual void GLSetUpMatrices(const Int2& viewport, glm::mat4x4*& projection_out, glm::mat4x4*& modelView_out) override;
    };

    static void s_EyeAndUpFromAngles( const Float3& center, float eyeDistance, float yaw, float pitch, Float3& eye_out, Float3& up )
    {
        using zfw::f_pi;

        if (pitch > f_pi * 0.5f - 10e-4f)
            pitch = f_pi * 0.5f;
        else if (pitch < -f_pi * 0.5f + 10e-4f)
            pitch = -f_pi * 0.5f;

        const float ca = cos( yaw );
        const float sa = sin( yaw );

        const float ca2 = cos( pitch );
        const float sa2 = sin( pitch );

        const float radius = ca2 * eyeDistance;
        const float upRadius = radius - sa2;

        const Float3 cam( ca * radius, -sa * radius, sa2 * eyeDistance );

        eye_out = center + cam;

        if (fabs(radius) > 10e-5f)
            up = Float3( ca * upRadius - cam.x, -sa * upRadius - cam.y, ca2 );
    }

    static void convert( const Float3& eye, const Float3& center, const Float3& up, float& dist, float& angle, float& angle2 )
    {
        const Float3 cam = eye - center;

        dist = glm::length( cam );
        angle = atan2( -cam.y, cam.x );
        angle2 = atan2( cam.z, glm::length( glm::vec2( cam ) ) );
    }

    shared_ptr<ICamera> p_CreateCamera(zfw::ErrorBuffer_t* eb, RenderingKit* rk, IRenderingManagerBackend* rm, const char* name)
    {
        return std::make_shared<GLCamera>(eb, rk, rm, name);
    }

    GLCamera::GLCamera(zfw::ErrorBuffer_t* eb, RenderingKit* rk, IRenderingManagerBackend* rm, const char* name)
            : name(name)
    {
        this->rk = rk;
        this->rm = rm;
        numRefs = 1;

        proj = kProjectionInvalid;
        nearZ = 1.0f;
        farZ = 1000.0f;
        vfov = 45.0f * zfw::f_pi / 180.0f;
    }

    void GLCamera::BuildModelView()
    {
        modelView = glm::lookAt( Float3( eye.x, -eye.y, eye.z ), Float3( center.x, -center.y, center.z ), Float3( up.x, -up.y, up.z ) );
        modelView = glm::scale( modelView, Float3( 1.0f, -1.0f, 1.0f ) );
    }

    float GLCamera::CameraGetDistance()
    {
        return glm::length( eye - center );
    }

    void GLCamera::CameraMove( const glm::vec3& vec )
    {
        eye += vec;
        center += vec;
        BuildModelView();
    }

    void GLCamera::CameraRotateXY( float alpha, bool absolute )
    {
        float dist, angle, angle2;

        convert( eye, center, up, dist, angle, angle2 );

        if ( absolute )
            angle2 = alpha;
        else
            angle2 += alpha;

        s_EyeAndUpFromAngles( center, dist, angle, angle2, eye, up );
        BuildModelView();
    }

    void GLCamera::CameraRotateZ(float alpha, bool absolute)
    {
        float dist, angle, angle2;

        convert( eye, center, up, dist, angle, angle2 );

        if ( absolute )
            angle = alpha;
        else
            angle += alpha;

        s_EyeAndUpFromAngles( center, dist, angle, angle2, eye, up );
        BuildModelView();
    }

    void GLCamera::CameraZoom(float amount, bool absolute)
    {
        float dist, angle, angle2;

        convert( eye, center, up, dist, angle, angle2 );

        if ( absolute )
            dist = amount;
        else if ( dist + amount > 0.0f )
            dist += amount;

        s_EyeAndUpFromAngles( center, dist, angle, angle2, eye, up );
        BuildModelView();
    }

    void GLCamera::GLSetUpMatrices(const Int2& viewportSize, glm::mat4x4*& projection_out, glm::mat4x4*& modelView_out)
    {
        ZFW_DBGASSERT(proj != kProjectionInvalid)

        switch (proj)
        {
            case kProjectionOrtho:
                projection = glm::ortho(left, right, bottom, top, nearZ, farZ);
                modelView = glm::mat4x4();
                break;

            case kProjectionOrthoFakeFov:
            {
                const float aspectRatio = (float) viewportSize.x / viewportSize.y;
                const float vfovHeight = CameraGetDistance() * tan(vfov);

                const Float2 vfov( -vfovHeight / 2, vfovHeight / 2 );
                const Float2 hfov( vfov * aspectRatio );

                projection = glm::ortho(hfov.x, hfov.y, vfov.x, vfov.y, nearZ, farZ);
                break;
            }

            case kProjectionOrthoScreenSpace:
                projection = glm::ortho(0.0f, (float) viewportSize.x, (float) viewportSize.y, 0.0f, nearZ, farZ);
                modelView = glm::mat4x4();
                break;

            case kProjectionPerspective:
            {
                const float aspectRatio = (float) viewportSize.x / viewportSize.y;

                const float vfov2 = ( float )( nearZ * tan(this->vfov / 2.0f) );
                const float hfov2 = vfov2 * aspectRatio;

                projection = glm::frustum(-hfov2, hfov2, -vfov2, vfov2, nearZ, farZ);
                break;
            }
        }

        projection_out = &projection;
        modelView_out = &modelView;
    }

    void GLCamera::SetClippingDist(float nearClip, float farClip)
    {
        this->nearZ = nearClip;
        this->farZ = farClip;
    }

    void GLCamera::SetOrtho(float left, float right, float top, float bottom)
    {
        proj = kProjectionOrtho;
        this->left = left;
        this->right = right;
        this->top = top;
        this->bottom = bottom;
    }

    void GLCamera::SetView(const Float3& eye, const Float3& center, const Float3& up)
    {
        this->eye = eye;
        this->center = center;
        this->up = up;
        BuildModelView();
    }

    void GLCamera::SetView2(const Float3& center, const float eyeDistance, float yaw, float pitch)
    {
        this->center = center;
        this->up = Float3(0.0f, -1.0f, 0.0f);
        s_EyeAndUpFromAngles(center, eyeDistance, yaw, pitch, this->eye, this->up);
        BuildModelView();
    }
}
