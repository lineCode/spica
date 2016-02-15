#define SPICA_API_EXPORT
#include "primitive.h"

#include "../core/interaction.h"
#include "../shape/shape.h"
#include "../material/material.h"

namespace spica {

// -----------------------------------------------------------------------------
// Primitive method definitions
// -----------------------------------------------------------------------------

const std::shared_ptr<const AreaLight>& Aggregate::areaLight() const {
    Warning("Deprecated function!!");
    return nullptr;
}

const std::shared_ptr<const Material>& Aggregate::material() const {
    Warning("Deprecated function!!");
    return nullptr;
}

void Aggregate::setScatterFuncs(SurfaceInteraction* intr,
                                MemoryArena& arena) const {
    Warning("Deprecated function!!");    
}

// -----------------------------------------------------------------------------
// GeometricPrimitive method definitions
// -----------------------------------------------------------------------------

GeometricPrimitive::GeometricPrimitive(const std::shared_ptr<Shape>& shape,
                                       const std::shared_ptr<Material>& material,
                                       const std::shared_ptr<AreaLight>& areaLight)
    : shape_{ shape }
    , material_{ material_ }
    , areaLight_{ areaLight } {    
}

Bound3d GeometricPrimitive::worldBound() const {
    return shape_->worldBound();
}

bool GeometricPrimitive::intersect(const Ray& ray, SurfaceInteraction* isect) const {
    double tHit;
    if (!shape_->intersect(ray, &tHit, isect)) return false;

    isect->setPrimitive(this);
    return true;
}

const std::shared_ptr<const Material>& GeometricPrimitive::material() const {
    return material_;
}

const std::shared_ptr<const AreaLight>& GeometricPrimitive::areaLight() const {
    return areaLight_;
}

void GeometricPrimitive::setScatterFuncs(SurfaceInteraction* isect, MemoryArena& arena) const {
    if (material_) {
        material_->setScatterFuncs(isect, arena);
    }
}

}  // namespace spica