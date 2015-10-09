#ifndef SPICA_RAY_H_
#define SPICA_RAY_H_

#if defined(_WIN32) || defined(__WIN32__)
    #ifdef SPICA_RAY_EXPORT
        #define SPICA_RAY_DLL __declspec(dllexport)
    #else
        #define SPICA_RAY_DLL __declspec(dllimport)
    #endif
#elif defined(linux) || defined(__linux)
    #define SPICA_RAY_DLL
#endif

#include "../utils/vector2d.h"
#include "../utils/vector3d.h"

namespace spica {

    class SPICA_RAY_DLL Ray {
    private:
        Vector3D _origin;
        Vector3D _direction;
        Vector3D _invdir;

    public:
        Ray();
        Ray(const Vector3D& origin, const Vector3D& direction);
        Ray(const Ray& ray);
        ~Ray();

        Ray& operator=(const Ray& ray);

        inline Vector3D origin()    const { return _origin; }
        inline Vector3D direction() const { return _direction; }
        inline Vector3D invdir()    const { return _invdir; }

    private:
        void calcInvdir();
    };

    class SPICA_RAY_DLL Hitpoint {
    private:
        double _distance;
        Vector3D _normal;
        Vector3D _position;
        Vector2D _texcoord;

    public:
        Hitpoint();
        Hitpoint(const Hitpoint& hp);
        ~Hitpoint();

        Hitpoint& operator=(const Hitpoint& hp);

        inline double distance() const { return _distance; }
        inline const Vector3D& normal() const { return _normal; }
        inline const Vector3D& position() const { return _position; }
        inline const Vector2D& texcoord() const { return _texcoord; }

        inline void setDistance(double distance) { _distance = distance; }
        inline void setNormal(const Vector3D& normal) { _normal = normal; }
        inline void setPosition(const Vector3D& position) { _position = position; }
        inline void setTexcoord(const Vector2D& texcoord) { _texcoord = texcoord; }
    };

    class SPICA_RAY_DLL Intersection {
    private:
        Hitpoint _hitPoint;
        int _objectId;

    public:
        Intersection();
        Intersection(const Intersection& intersection);
        ~Intersection();

        Intersection& operator=(const Intersection& intersection);

        double hittingDistance() const;

        inline int objectId() const { return _objectId; }
        inline const Hitpoint& hitpoint() const { return _hitPoint; }

        inline void setHitpoint(const Hitpoint& hitpoint) { _hitPoint = hitpoint; }
        inline void setObjectId(int id) { _objectId = id; }
    };

}  // namespace spica

#endif  // SPICA_RAY_H_
