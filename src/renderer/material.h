#ifndef SPICA_MATERIAL_H_
#define SPICA_MATERIAL_H_

#include "../utils/Vector3.h"

namespace spica {

    typedef Vector3 Color;

    enum ReflectionType {
        REFLECTION_DIFFUSE = 0x01,
        REFLECTION_SPECULAR = 0x02,
        REFLECTION_REFRACTION = 0x04
    };

    const double indexOfRef = 1.5;
}

#endif