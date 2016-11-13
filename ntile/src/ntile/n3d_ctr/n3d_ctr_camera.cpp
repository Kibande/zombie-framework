
#include "n3d_ctr_internal.hpp"

#include "math.h"

namespace n3d
{
    static void convert( float dist, float angle, float angle2, Float3& eye, Float3& center, Float3& up )
    {
        float ca = cos( angle );
        float sa = sin( angle );

        float ca2 = cos( angle2 );
        float sa2 = sin( angle2 );

        float radius = ca2 * dist;
        float upRadius = radius - sa2;

        const Float3 cam( ca * radius, -sa * radius, sa2 * dist );

        eye = center + cam;
        up = Float3( ca * upRadius - cam.x, -sa * upRadius - cam.y, ca2 );
    }

    static void convert( const Float3& eye, const Float3& center, const Float3& up, float& dist, float& angle, float& angle2 )
    {
        const Float3 cam = eye - center;

        dist = glm::length( cam );
        angle = atan2( -cam.y, cam.x );
        angle2 = atan2( cam.z, glm::length( glm::vec2( cam ) ) );
    }

    CTRProjectionBuffer::CTRProjectionBuffer()
    {
    }

    void CTRProjectionBuffer::BuildModelView()
    {
        modelView = glm::lookAt( Float3( eye.x, -eye.y, eye.z ), Float3( center.x, -center.y, center.z ), Float3( up.x, -up.y, up.z ) );
        modelView = glm::scale( modelView, Float3( 1.0f, -1.0f, 1.0f ) );
        modelViewTransp = glm::transpose(modelView);
    }

    float CTRProjectionBuffer::CameraGetDistance()
    {
        return glm::length( eye - center );
    }

    void CTRProjectionBuffer::CameraMove( const glm::vec3& vec )
    {
        eye += vec;
        center += vec;
        BuildModelView();
    }

    void CTRProjectionBuffer::CameraRotateXY( float alpha, bool absolute )
    {
        float dist, angle, angle2;

        convert( eye, center, up, dist, angle, angle2 );

        if ( absolute )
            angle2 = alpha;
        else
            angle2 += alpha;

        convert( dist, angle, angle2, eye, center, up );
        BuildModelView();
    }

    void CTRProjectionBuffer::CameraRotateZ(float alpha, bool absolute)
    {
        float dist, angle, angle2;

        convert( eye, center, up, dist, angle, angle2 );

        if ( absolute )
            angle = alpha;
        else
            angle += alpha;

        convert( dist, angle, angle2, eye, center, up );
        BuildModelView();
    }

    void CTRProjectionBuffer::CameraZoom(float amount, bool absolute)
    {
        float dist, angle, angle2;

        convert( eye, center, up, dist, angle, angle2 );

        if ( absolute )
            dist = amount;
        else if ( dist + amount > 0.0f )
            dist += amount;

        convert( dist, angle, angle2, eye, center, up );
        BuildModelView();
    }

    void CTRProjectionBuffer::R_SetUpMatrices()
    {
        switch (proj)
        {
            /*case IProjectionBuffer::PROJ_ORTHO:
            {
                auto projectionTransp = glm::transpose(glm::ortho(left, right, bottom, top, nearZ, farZ));
                modelView = glm::mat4x4();
                break;
            }*/

            case IProjectionBuffer::PROJ_ORTHO_SS:
            {
                glm::mat4x4 projectionTransp;

                // It just wouldn't work any other way.
                // Don't ask me.
                projectionTransp[0][0] = 0.0f;
                projectionTransp[0][1] = -2.0f / 240.0f;
                projectionTransp[0][3] = 1.0f;

                projectionTransp[1][0] = -2.0f / 400.0f;
                projectionTransp[1][1] = 0.0f;
                projectionTransp[1][3] = 1.0f;

                projectionTransp[2][2] = 1.0f / (farZ - nearZ);
                projectionTransp[2][3] = -0.5f;
                glOrthoProjectionMatrixfCTR(&projectionTransp[0][0], 1.0f, -1.0f);

                modelViewTransp = glm::mat4x4();
                glModelviewMatrixfCTR(&modelViewTransp[0][0]);
                break;
            }

            case IProjectionBuffer::PROJ_PERSPECTIVE:
            {
                mtx44 projection;
                initProjectionMatrix((float*) projection, 80.0f*M_PI/180.0f, 240.0f/400.0f, 10.0f, 1000.0f);
                rotateMatrixZ((float*) projection, M_PI/2, false);   //because framebuffer is sideways...
                glPerspectiveProjectionMatrixfCTR((float*) projection, 10.0f, 200.0f, 5.0f);

                glModelviewMatrixfCTR( &modelViewTransp[0][0]);
                break;
            }
        }
    }

    void CTRProjectionBuffer::SetOrtho(float left, float right, float top, float bottom, float nearZ, float farZ)
    {
        proj = PROJ_ORTHO;
        this->left = left;
        this->right = right;
        this->top = top;
        this->bottom = bottom;
        this->nearZ = nearZ;
        this->farZ = farZ;
    }

    void CTRProjectionBuffer::SetOrthoScreenSpace(float nearZ, float farZ)
    {
        proj = PROJ_ORTHO_SS;
        this->nearZ = nearZ;
        this->farZ = farZ;
    }

    void CTRProjectionBuffer::SetVFov(float vfov)
    {
        const float aspect = 400.0f / 240.0f;

        this->vfov = ( float )( nearZ * tan(vfov / 2.0f) );
        this->hfov = this->vfov * aspect;
    }

    void CTRProjectionBuffer::SetView(const Float3& eye, const Float3& center, const Float3& up)
    {
        this->eye = eye;
        this->center = center;
        this->up = up;
        BuildModelView();
    }
}
