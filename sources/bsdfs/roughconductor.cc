#define SPICA_API_EXPORT
#include "roughconductor.h"

#include "core/memory.h"
#include "core/interaction.h"
#include "core/bsdf.h"
#include "core/fresnel.h"
#include "core/microfacet.h"
#include "core/texture.h"

namespace spica {

RoughConductor::RoughConductor(const std::shared_ptr<Texture<Spectrum>>& eta,
                               const std::shared_ptr<Texture<Spectrum>>& k,
                               const std::shared_ptr<Texture<double>>& uRoughness,
                               const std::shared_ptr<Texture<double>>& vRoughness,
                               const std::string &distribution, 
                               const std::shared_ptr<Texture<double>>& bump,
                               bool remapRoughness) 
    : eta_{ eta }
    , k_{ k }
    , uRoughness_{ uRoughness }
    , vRoughness_{ vRoughness }
    , distribution_{ distribution }
    , bumpMap_{ bump }
    , remapRoughness_{ remapRoughness } {
}

RoughConductor::RoughConductor(RenderParams &params)
    : RoughConductor{std::static_pointer_cast<Texture<Spectrum>>(params.getTexture("eta", true)),
                     std::static_pointer_cast<Texture<Spectrum>>(params.getTexture("k", true)),
                     std::static_pointer_cast<Texture<double>>(params.getTexture("alpha")),
                     std::static_pointer_cast<Texture<double>>(params.getTexture("alpha")),
                     params.getString("distribution", "beckmann", true),
                     std::static_pointer_cast<Texture<double>>(params.getTexture("bumpMap", true))} {
}
    
void RoughConductor::setScatterFuncs(SurfaceInteraction* isect,
                                    MemoryArena& arena) const {
    if (bumpMap_) bump(isect, bumpMap_);

    isect->setBSDF(arena.allocate<BSDF>(*isect));

    double uRough = uRoughness_->evaluate(*isect);
    double vRough = vRoughness_->evaluate(*isect);
    if (remapRoughness_) {
        if (distribution_ == "beckmann") {
            uRough = BeckmannDistribution::roughnessToAlpha(uRough);    
            vRough = BeckmannDistribution::roughnessToAlpha(vRough);        
        } else if (distribution_ == "ggx") {
            uRough = TrowbridgeReitzDistribution::roughnessToAlpha(uRough);    
            vRough = TrowbridgeReitzDistribution::roughnessToAlpha(vRough);
        }
    }

    const Spectrum eta = eta_->evaluate(*isect);
    const Spectrum k = k_->evaluate(*isect);
    Fresnel* fresnel = arena.allocate<FresnelConductor>(Spectrum(1.0), eta, k);

    MicrofacetDistribution* distrib = nullptr;
    if (distribution_ == "beckmann") {
        distrib = arena.allocate<BeckmannDistribution>(uRough, vRough);
    } else if (distribution_ == "ggx") {
        distrib = arena.allocate<TrowbridgeReitzDistribution>(uRough, vRough);
    } else {
        FatalError("Unknown micforacet distribution type: %s", distribution_.c_str());
    }

    isect->bsdf()->add(arena.allocate<MicrofacetReflection>(Spectrum(1.0), distrib, fresnel));
}

}  // namespace spica
