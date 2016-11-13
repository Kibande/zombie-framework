
#include "n3d_gl_internal.hpp"

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

    GLProjectionBuffer::GLProjectionBuffer()
    {
    }

    void GLProjectionBuffer::BuildModelView()
    {
        modelView = glm::lookAt( Float3( eye.x, -eye.y, eye.z ), Float3( center.x, -center.y, center.z ), Float3( up.x, -up.y, up.z ) );
        modelView = glm::scale( modelView, Float3( 1.0f, -1.0f, 1.0f ) );
    }

    float GLProjectionBuffer::CameraGetDistance()
    {
        return glm::length( eye - center );
    }

    void GLProjectionBuffer::CameraMove( const glm::vec3& vec )
    {
        eye += vec;
        center += vec;
        BuildModelView();
    }

    void GLProjectionBuffer::CameraRotateXY( float alpha, bool absolute )
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

    void GLProjectionBuffer::CameraRotateZ(float alpha, bool absolute)
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

    void GLProjectionBuffer::CameraZoom(float amount, bool absolute)
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

    void GLProjectionBuffer::R_SetUpMatrices()
    {
        switch (proj)
        {
            case IProjectionBuffer::PROJ_ORTHO:
                projection = glm::ortho(left, right, bottom, top, nearZ, farZ);
                modelView = glm::mat4x4();
                break;
        
            case IProjectionBuffer::PROJ_ORTHO_SS:
                projection = glm::ortho(0.0f, (float) gl.renderViewport.x, (float) gl.renderViewport.y, 0.0f, nearZ, farZ);
                modelView = glm::mat4x4();
                break;

            case IProjectionBuffer::PROJ_PERSPECTIVE:
            {
                projection = glm::frustum(-hfov, hfov, -vfov, vfov, nearZ, farZ);
                break;
            }
        }

        glMatrixMode( GL_PROJECTION );
        glLoadMatrixf( &projection[0][0] );

        glMatrixMode( GL_MODELVIEW );
        glLoadMatrixf( &modelView[0][0] );
    }

    void GLProjectionBuffer::SetOrtho(float left, float right, float top, float bottom, float nearZ, float farZ)
    {
        proj = PROJ_ORTHO;
        this->left = left;
        this->right = right;
        this->top = top;
        this->bottom = bottom;
        this->nearZ = nearZ;
        this->farZ = farZ;
    }

    void GLProjectionBuffer::SetOrthoScreenSpace(float nearZ, float farZ)
    {
        proj = PROJ_ORTHO_SS;
        this->nearZ = nearZ;
        this->farZ = farZ;
    }

    void GLProjectionBuffer::SetVFov(float vfov)
    {
        const float aspect = (float) gl.renderViewport.x / gl.renderViewport.y;

        this->vfov = ( float )( nearZ * tan(vfov / 2.0f) );
        this->hfov = this->vfov * aspect;
    }

    void GLProjectionBuffer::SetView(const Float3& eye, const Float3& center, const Float3& up)
    {
        this->eye = eye;
        this->center = center;
        this->up = up;
        BuildModelView();
    }
}
