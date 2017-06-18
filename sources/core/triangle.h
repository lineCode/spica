#ifdef _MSC_VER
#pragma once
#endif

#ifndef _SPICA_TRIANGLE_H_
#define _SPICA_TRIANGLE_H_

#include <array>

#include "core/core.hpp"

#include "core/common.h"
#include "core/point2d.h"
#include "core/point3d.h"
#include "core/normal3d.h"
#include "core/transform.h"

namespace spica {

class SPICA_EXPORTS Triangle {
public:
    // Public methods
    Triangle();
    Triangle(const Point3d& p0, const Point3d& p1, const Point3d& p2,
             const Transform& objectToWorld = Transform());
    Triangle(const Point3d& p0, const Point3d& p1, const Point3d& p2,
             const Normal3d& n0, const Normal3d& n1, const Normal3d& n2,
             const Transform& objectToWorld = Transform());
    Triangle(const Point3d& p0, const Point3d& p1, const Point3d& p2,
             const Normal3d& n0, const Normal3d& n1, const Normal3d& n2,
             const Point2d& uv0, const Point2d& uv1, const Point2d& uv2,
             const Transform& objectToWorld = Transform());
    Triangle(const Triangle& t);

    ~Triangle();

    Triangle& operator=(const Triangle& t);
    const Point3d& operator[](int i) const;

    bool intersect(const Ray& ray, double *tHit, Point2d *uv) const;

    Point2d sample(const Point2d& rands, Point3d *pos, Normal3d *nrm) const;

    Bounds3d worldBound()  const;
    Bounds3d objectBound() const;

    double area() const;

    Point3d gravity() const;
    const Normal3d& normal(int i) const;
    const Point2d&  uv(int i) const;

private:
    // Private fields
    std::array<Point3d,  3> points_;
    std::array<Normal3d, 3> normals_;
    std::array<Point2d,  3> uvs_;

};  // class Triangle

}  // namespace spica

#endif  // _SPICA_TRIANGLE_H_
