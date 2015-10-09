#define SPICA_PT_RENDERER_EXPORT
#include "pathtrace.h"

#include <iostream>
#include <fstream>
#include <algorithm>

#include "../utils/common.h"
#include "../utils/vector3d.h"
#include "../utils/sampler.h"
#include "../utils/image.h"

#include "../random/random_sampler.h"

#include "../scenes/scene.h"
#include "renderer_helper.h"
#include "subsurface_integrator.h"

namespace spica {

    PathRenderer::PathRenderer()
        : IRenderer()
    {
    }

    PathRenderer::~PathRenderer()
    {
    }

    void PathRenderer::render(const Scene& scene, const Camera& camera,
                                     const RenderParameters& params) {
        const int width  = camera.imageW();
        const int height = camera.imageH();

        // Preparation for accouting for BSSRDF
        _integrator->initialize(scene);

        // Prepare random number generators
        RandomSampler* samplers = new RandomSampler[kNumThreads];
        for (int i = 0; i < kNumThreads; i++) {
            switch (params.randomType()) {
            case PSEUDO_RANDOM_TWISTER:
                samplers[i] = Random::factory(i);
                break;

            case QUASI_MONTE_CARLO:
                samplers[i] = Halton::factory(200, true, i);
                break;

            default:
                std::cerr << "[ERROR] Unknown random number generator type!!"
                          << std::endl;
                std::abort();
            }
        }

        // Vectors spanning screen
        Image buffer = Image(width, height);

        // Distribute rendering tasks
        const int taskPerThread = (height + kNumThreads - 1) / kNumThreads;
        std::vector<std::vector<int> > tasks(kNumThreads);
        for (int y = 0; y < height; y++) {
            tasks[y % kNumThreads].push_back(y);
        }

        // Trace rays
        int processed = 0;
        _result.resize(width, height);
        buffer.fill(Color::BLACK);
        for (int i = 0; i < params.samplePerPixel(); i++) {
            if (i % kNumThreads == 0) {
                _integrator->construct(scene, params);
            }

            for (int t = 0; t < taskPerThread; t++) {
                ompfor (int threadID = 0; threadID < kNumThreads; threadID++) {
                    if (t < tasks[threadID].size()) {
                        Stack<double> rstk;
                        const int y = tasks[threadID][t];
                        for (int x = 0; x < width; x++) {
                            samplers[threadID].request(&rstk, 300);
                            buffer.pixel(width - x - 1, y) +=
                                tracePath(scene, camera, params, x, y, rstk);
                        }
                    }
                }
            }

            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                    _result.pixel(x, y) = buffer(x, y) / (i + 1);
                }
            }

            char filename[256];
            sprintf(filename, params.saveFilenameFormat().c_str(), i + 1);
            _result.gammaCorrect(1.0 / 2.2);
            _result.save(filename);

            printf("%6.2f %%  processed -> %s\r",
                    100.0 * (i + 1) / params.samplePerPixel(), filename);
        }
        printf("\nFinish!!\n");

        delete[] samplers;
    }

    Color PathRenderer::tracePath(const Scene& scene, const Camera& camera, 
                                  const RenderParameters& params,
                                  const double pixelX, const double pixelY,
                                  Stack<double>& rands) {
        CameraSample camSample = camera.sample(pixelX, pixelY, rands);
        const Ray ray = camSample.ray();

        return radiance(scene, params, ray, rands, 0) * (camera.sensitivity() / camSample.pdf());
    }

    Color PathRenderer::radiance(const Scene& scene,
                                const RenderParameters& params,
                                const Ray& ray, Stack<double>& rstack,
                                int bounces) const {
        if (bounces >= params.bounceLimit()) {
            return Color::BLACK;
        }

        Intersection isect;
        if (!scene.intersect(ray, isect)) {
            return Color::BLACK;
        }

        // Require random numbers
        const double randnums[3] = { rstack.pop(), rstack.pop(), rstack.pop() };

        // Get intersecting material
        const int objectID     = isect.objectId();
        const BSDF& bsdf       = scene.getBsdf(objectID);
        const Color& refl      = scene.getReflectance(isect);
        const Hitpoint& hpoint = isect.hitpoint();

        // Russian roulette
        double roulette = max3(refl.red(), refl.green(), refl.blue());
        if (bounces < params.bounceStartRoulette()) {
            roulette = 1.0;
        } else {
            if (roulette <= randnums[0]) {
                return Color::BLACK;
            }
        }

        // Variables for next bounce
        Color bssrdfRad(0.0, 0.0, 0.0);
        Vector3D nextdir;
        double pdf = 1.0;

        // Account for BSSRDF
        if (bsdf.type() & BsdfType::Bssrdf) {
            Assertion(_integrator != NULL,
                      "Subsurface intergrator is NULL !!");
            bssrdfRad = bsdf.sampleBssrdf(ray.direction(),
                                          hpoint.position(),
                                          hpoint.normal(),
                                          randnums[1], randnums[2],
                                          *_integrator,
                                          &nextdir, &pdf);
        } else {
            // Sample next direction
            bsdf.sample(ray.direction(), hpoint.normal(), 
                        randnums[1], randnums[2], &nextdir, &pdf);
        }

        // Compute next bounce
        const Ray nextray(hpoint.position(), nextdir);
        const Color nextrad = radiance(scene, params, nextray,
                                        rstack, bounces + 1);            

        Color directrad = directSample(scene, objectID, ray.direction(),
                                       hpoint.position(), hpoint.normal(),
                                       refl, bounces, rstack);

        return (bssrdfRad + directrad + refl * nextrad / pdf) / roulette;
    }

    Color PathRenderer::directSample(const Scene& scene, const int triID,
                                     const Vector3D& in, const Vector3D& v,
                                     const Vector3D& n, const Color& refl, 
                                     int bounces, Stack<double>& rstk) const {
        // Acquire random numbers
        const double rands[3] = { rstk.pop(), rstk.pop(), rstk.pop() };

        const BSDF&  bsdf = scene.getBsdf(triID);

        if (bsdf.type() & BsdfType::Scatter) {
            if (!scene.isLightCheck(triID)) {
                const LightSample ls = scene.sampleLight(rstk);
                double pdf;
                Vector3D nextdir;
                bsdf.sample(in, n, rands[0], rands[1], &nextdir, &pdf);
        
                const Vector3D lightDir = (ls.position() - v).normalized();
                const double dist2 = (ls.position() - v).squaredNorm();
                const double dot0  = Vector3D::dot(n, lightDir);
                const double dot1  = Vector3D::dot(ls.normal(), -lightDir);

                if (dot0 > EPS && dot1 > EPS) {
                    const double G = dot0 * dot1 / dist2;
                    Intersection isect;
                    if (scene.intersect(Ray(v, lightDir), isect)) {
                        if (scene.isLightCheck(isect.objectId())) {
                            return (refl * ls.Le()) * (INV_PI * G * scene.lightArea()); 
                        }
                    }
                }
            } else if (bounces == 0) {
                return scene.directLight(in);
            }
        } else if (bsdf.type() & BsdfType::Dielectric) {
            // Dielectric surface
            double pdf;
            Vector3D nextdir;
            bsdf.sample(in, n, rands[0], rands[1], &nextdir, &pdf);
            
            Intersection isect;
            if (scene.intersect(Ray(v, nextdir), isect)) {
                const int objID = isect.objectId();
                if (scene.isLightCheck(objID)) {
                    return (refl * scene.directLight(in)) / pdf;
                }
            }
        } else {
            std::cerr << "Invalid BSDF detected: this is "
                         "neigher scattering nor dielectric!!" << std::endl;
            std::abort();
        }
        return Color(0.0, 0.0, 0.0);
    }

}  // namespace spica
