#ifdef _MSC_VER
#pragma once
#endif

#ifndef _SPICA_IMAGEMAP_H_
#define _SPICA_IMAGEMAP_H_

#include <string>
#include <memory>

#include "../core/common.h"
#include "../core/forward_decl.h"
#include "../core/spectrum.h"

#include "texture.h"

namespace spica {

class ImageTexture : public Texture<Spectrum> {
public:
    ImageTexture(const Image& image,
                 std::unique_ptr<TextureMapping2D> texmap,
                 ImageWrap wrap);

    Spectrum evaluate(const SurfaceInteraction& intr) const;

private:
    // Private field
    std::unique_ptr<MipMap> mipmap_;
    std::unique_ptr<TextureMapping2D> texmap_;
};

}  // namespace spica

#endif  // _SPICA_IMAGEMAP_H_
