#define SPICA_SPHERE_EXPORT
#include "sphere.h"

#include <cmath>

#include "../utils/common.h"
#include "triangle.h"

namespace spica {

    Sphere::Sphere()
        : _radius(0.0)
        , _center()
    {
    }

    Sphere::Sphere(double radius, const Vector3D& center)
        :  _radius(radius)
        , _center(center)
    {
    }

    Sphere::Sphere(const Sphere& sphere)
        : _radius()
        , _center()
    {
        operator=(sphere);
    }

    Sphere::~Sphere()
    {
    }

    Sphere& Sphere::operator=(const Sphere& sphere) {
        this->_radius = sphere._radius;
        this->_center = sphere._center;
        return *this;
    }

    bool Sphere::intersect(const Ray& ray, Hitpoint* hitpoint) const {
        const Vector3D VtoC = _center - ray.origin();
        const double b = VtoC.dot(ray.direction());
        const double D4 = b * b - VtoC.dot(VtoC) + _radius * _radius;

        if (D4 < 0.0) return false;

        const double sqrtD4 = sqrt(D4);
        const double t1 = b - sqrtD4;
        const double t2 = b + sqrtD4;

        if (t1 < EPS && t2 < EPS) return false;

        if (t1 > EPS) {
            hitpoint->setDistance(t1);
        } else {
            hitpoint->setDistance(t2);
        }

        hitpoint->setPosition(ray.origin() + hitpoint->distance() * ray.direction());
        hitpoint->setNormal((hitpoint->position() - _center).normalized());

        return true;
    }

    double Sphere::area() const {
        return 4.0 * PI * _radius * _radius;
    }

    std::vector<Triangle> Sphere::triangulate() const {
        std::vector<Triangle> retval;
        return std::move(retval);
    }

}  // namespace spica
