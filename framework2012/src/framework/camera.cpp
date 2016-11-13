
#include "framework.hpp"

namespace zfw
{
    namespace world
    {
        Camera::Camera()
        {
        }

        Camera::Camera( const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up )
                : eye( eye ), center( center ), up( up )
        {
        }

        Camera::Camera( const glm::vec3& center, float dist, float angle, float angle2 )
                : center( center )
        {
            convert( dist, angle, angle2, eye, this->center, up );
        }

        void Camera::convert( float dist, float angle, float angle2, glm::vec3& eye, glm::vec3& center, glm::vec3& up )
        {
            float ca = cos( angle/* + ( float )( M_PI * 1.5f )*/ );
            float sa = sin( angle/* + ( float )( M_PI * 1.5f )*/ );

            float ca2 = cos( angle2 );
            float sa2 = sin( angle2 );

            float radius = ca2 * dist;
            float upRadius = radius - sa2;

            glm::vec3 cam( ca * radius, -sa * radius, sa2 * dist );

            eye = center + cam;
            up = glm::vec3( ca * upRadius - cam.x, -sa * upRadius - cam.y, ca2 );
        }

        void Camera::convert( const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up, float& dist, float& angle, float& angle2 )
        {
            glm::vec3 cam = eye - center;

            dist = glm::length( cam );
            angle = atan2( -cam.y, cam.x );
            angle2 = atan2( cam.z, glm::length( glm::vec2( cam ) ) );
        }

        float Camera::getDistance() const
        {
            return glm::length( eye - center );
        }

        /*void Camera::look( GraphicsDriver* driver, glm::vec3 center, float dist, float angle, float angle2 )
        {
            glm::vec3 eye, up;

            convert( dist, angle, angle2, eye, center, up );

            //Engine::getInstance()->getGraphicsDriver()
            driver->setCamera( eye, center, up );
        }*/

        void Camera::move( const glm::vec3& vec )
        {
            eye += vec;
            center += vec;
        }

        void Camera::rotateXY( float alpha, bool absolute )
        {
            float dist, angle, angle2;

            convert( eye, center, up, dist, angle, angle2 );

            if ( absolute )
                angle2 = alpha;
            else
                angle2 += alpha;

            convert( dist, angle, angle2, eye, center, up );
        }

        void Camera::rotateZ( float alpha, bool absolute )
        {
            float dist, angle, angle2;

            convert( eye, center, up, dist, angle, angle2 );

            if ( absolute )
                angle = alpha;
            else
                angle += alpha;

            convert( dist, angle, angle2, eye, center, up );
        }

        void Camera::zoom( float amount, bool absolute )
        {
            float dist, angle, angle2;

            convert( eye, center, up, dist, angle, angle2 );

            if ( absolute )
                dist = amount;
            else if ( dist + amount > 0.0f )
                dist += amount;

            convert( dist, angle, angle2, eye, center, up );
        }
    }
}
