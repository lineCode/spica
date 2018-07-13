#define SPICA_API_EXPORT
#include "integrator.h"

#include "core/memory.h"
#include "core/parallel.h"
#include "core/renderparams.h"
#include "core/sampler.h"

#include "core/interaction.h"
#include "core/bxdf.h"
#include "core/bsdf.h"
#include "core/fresnel.h"

#include "core/camera.h"
#include "core/film.h"

namespace spica {

// -----------------------------------------------------------------------------
// Integrator method definitions
// -----------------------------------------------------------------------------

Integrator::Integrator() {
}

Integrator::~Integrator() {
}

void Integrator::render(const std::shared_ptr<const Camera> &camera,
                        const Scene &scene,
                        RenderParams &params) {
    this->camera_ = camera;
}

// -----------------------------------------------------------------------------
// SamplerIntegrator method definitions
// -----------------------------------------------------------------------------

SamplerIntegrator::SamplerIntegrator(const std::shared_ptr<Sampler>& sampler)
    : Integrator{}
    , sampler_{ sampler } {
}

SamplerIntegrator::~SamplerIntegrator() {
}

void SamplerIntegrator::initialize(const Scene& scene,
                                   RenderParams& parmas,
                                   Sampler& sampler) {
}

void SamplerIntegrator::loopStarted(const Scene& scene,
                                    RenderParams& parmas,
                                    Sampler& sampler) {
}

void SamplerIntegrator::loopFinished(const Scene& scene,
                                     RenderParams& parmas,
                                     Sampler& sampler) {
}

void SamplerIntegrator::render(const std::shared_ptr<const Camera> &camera,
                               const Scene& scene,
                               RenderParams& params) {
    // Initialization
    Integrator::render(camera, scene, params);
    auto initSampler = sampler_->clone((unsigned int)time(0));
    initialize(scene, params, *initSampler);

    const int width = camera_->film()->resolution().x();
    const int height = camera_->film()->resolution().y();

    const int numThreads = numSystemThreads();
    auto samplers = std::vector<std::unique_ptr<Sampler>>(numThreads);
    auto arenas   = std::vector<MemoryArena>(numThreads);

    // Trace rays
    const int numPixels  = width * height;
    const int numSamples = params.getInt("sampleCount");
    for (int i = 0; i < numSamples; i++) {
        // Before loop computations
        loopStarted(scene, params, *initSampler);

        // Prepare samplers
        if (i % numThreads == 0) {
            for (int t = 0; t < numThreads; t++) {
                auto seed = static_cast<unsigned int>(time(0) + t);
                samplers[t] = sampler_->clone(seed);
            }
        }

        std::atomic<int> proc(0);
        parallel_for(0, numPixels, [&](int pid) {
            const int threadID = getThreadID();
            const int y = pid / width;
            const int x = pid % width;
            const Point2d randFilm = samplers[threadID]->get2D();
            const Point2d randLens = samplers[threadID]->get2D();
            const Ray ray = camera_->spawnRay(Point2i(x, y), randFilm, randLens);

            const Point2i pixel(width - x - 1, y);
            camera_->film()->addPixel(pixel, randFilm,
                                        Li(scene, params, ray, *samplers[threadID],
                                            arenas[threadID]));

            proc++;
            if (proc % 1000 == 0 || proc == numPixels) {
                printf("\r[ %d / %d ] %6.2f %% processed...", i + 1, numSamples, 100.0 * proc / numPixels);
                fflush(stdout);
            }
        });
        printf("\n");

        camera_->film()->save(i + 1);

        for (int t = 0; t < numThreads; t++) {
            arenas[t].reset();
        }

        // After loop computations
        loopFinished(scene, params, *initSampler);
    }
    printf("Finish!!\n");
}

Spectrum SamplerIntegrator::specularReflect(const Scene& scene,
                                            RenderParams& params,
                                            const Ray& ray,
                                            const SurfaceInteraction& isect,
                                            Sampler& sampler,
                                            MemoryArena& arena,
                                            int depth) const {
    Vector3d wi, wo = isect.wo();
    double pdf;
    BxDFType type = BxDFType::Reflection | BxDFType::Specular;
    Spectrum f = isect.bsdf()->sample(wo, &wi, sampler.get2D(), &pdf, type);
    
    const Normal3d nrm = isect.normal();
    if (pdf > 0.0 && !f.isBlack() && vect::absDot(wi, nrm) != 0.0) {
        Ray r = isect.spawnRay(wi);
        return f * Li(scene, params, r, sampler, arena, depth + 1) *
               vect::absDot(wi, nrm) / pdf;
    } 
    return Spectrum(0.0);
}

Spectrum SamplerIntegrator::specularTransmit(const Scene& scene,
                                             RenderParams& params,
                                             const Ray& ray,
                                             const SurfaceInteraction& isect,
                                             Sampler& sampler,
                                             MemoryArena& arena,
                                             int depth) const {
    Vector3d wi, wo = isect.wo();
    double pdf;
    const Point3d& pos = isect.pos();
    const Normal3d& nrm = isect.normal();
    BxDFType type = BxDFType::Transmission | BxDFType::Specular;
    Spectrum f = isect.bsdf()->sample(wo, &wi, sampler.get2D(), &pdf, type);
    if (pdf > 0.0 && !f.isBlack() && vect::absDot(wi, nrm) != 0.0) {
        Ray r = isect.spawnRay(wi);
        return f * Li(scene, params, r, sampler, arena, depth + 1) *
               vect::absDot(wi, nrm) / pdf;
    }
    return Spectrum(0.0);
}

}  // namespace spica