#ifdef _MSC_VER
#pragma once
#endif

#ifndef _SPICA_TEXTURE_H_
#define _SPICA_TEXTURE_H_

#include "core/core.hpp"
#include "core/common.h"
#include "core/cobject.h"
#include "core/vector3d.h"

#include "core/render.hpp"

namespace spica {

/**
 * Texture mapping interface.
 */
class SPICA_EXPORTS TextureMapping2D {
public:
    virtual ~TextureMapping2D() {}

    virtual Point2d map(const Point3d& p) const = 0;
    virtual Point2d map(const SurfaceInteraction& intr,
                        Vector2d *dstdx = nullptr,
                        Vector2d *dstdy = nullptr) const = 0;
};

class SPICA_EXPORTS UVMapping2D : public TextureMapping2D {
public:
    explicit UVMapping2D(double su = 1.0, double sv = 1.0,
                         double du = 0.0, double dv = 0.0,
                         bool invertHorizontal = true);

    Point2d map(const Point3d& p) const override;
    Point2d map(const SurfaceInteraction &intr,
                Vector2d *dstdx = nullptr,
                Vector2d *dstdy = nullptr) const override;

private:
    const double su_, sv_, du_, dv_;
    const bool invertHorizontal_;
};

class SPICA_EXPORTS PlanarMapping2D : public TextureMapping2D {
public:
    PlanarMapping2D(const Vector3d& vs, const Vector3d& vt, double ds = 0.0,
                    double dt = 0.0);

    Point2d map(const Point3d& p) const override;
    Point2d map(const SurfaceInteraction& intr,
                Vector2d* dstdx = nullptr,
                Vector2d* dstdy = nullptr) const override;

private:
    const Vector3d vs_, vt_;
    const double ds_, dt_;
};

/**
 * Texture interface.
 */
template <class T>
class SPICA_EXPORTS Texture : public CObject {
public:
    virtual T evaluate(const SurfaceInteraction& intr) const = 0;
    virtual ~Texture() {}
};

}  // namespace spica

#endif  // _SPICA_TEXTURE_H_
