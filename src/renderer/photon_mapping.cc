#define SPICA_PHOTON_MAPPING_EXPORT
#include "photon_mapping.h"

#include <cmath>
#include <cstdio>
#include <algorithm>

#include "renderer_helper.h"
#include "../utils/sampler.h"

namespace spica {

    // --------------------------------------------------
    // Photon map
    // --------------------------------------------------
    Photon::Photon()
        : Vector3()
        , _flux()
        , _direction()
    {
    }

    Photon::Photon(const Vector3& position, const Color& flux, const Vector3& direction)
        : Vector3(position)
        , _flux(flux)
        , _direction(direction)
    {
    }

    Photon::Photon(const Photon& photon)
        : Vector3()
        , _flux()
        , _direction()
    {
        operator=(photon);
    }

    Photon::~Photon()
    {
    }

    Photon& Photon::operator=(const Photon& photon) {
        Vector3::operator=(photon);
        this->_flux      = photon._flux;
        this->_direction = photon._direction;
        return *this;
    }

    // --------------------------------------------------
    // Photon map
    // --------------------------------------------------

    PhotonMap::PhotonMap()
        : _kdtree()
    {
    }

    PhotonMap::~PhotonMap()
    {
    }

    void PhotonMap::clear() {
        _kdtree.release();
    }

    void PhotonMap::construct(const std::vector<Photon>& photons) {
        _kdtree.construct(photons);
    }

    void PhotonMap::findKNN(const Photon& query, std::vector<Photon>* photons, const int numTargetPhotons, const double targetRadius) const {
        _kdtree.knnSearch(query, KnnQuery(K_NEAREST | EPSILON_BALL, numTargetPhotons, targetRadius), photons);
    }

    // --------------------------------------------------
    // Photon mapping renderer
    // --------------------------------------------------

    PMRenderer::PMRenderer()
    {
    }

    PMRenderer::PMRenderer(const PMRenderer& renderer)
    {
    }

    PMRenderer::~PMRenderer()
    {
    }

    PMRenderer& PMRenderer::operator=(const PMRenderer& renderer) {
        return *this;
    }

    int PMRenderer::render(const Scene& scene, const Camera& camera, const Random& rng, const int samplePerPixel, const int numTargetPhotons, const double targetRadius) {
        const int width = camera.imageW();
        const int height = camera.imageH();
        Image image(width, height);

        int proc = 0;
        for (int i = 0; i < samplePerPixel; i++) {
            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                    image.pixel(width - x - 1, y) += executePT(scene, camera, x, y, rng, numTargetPhotons, targetRadius) / samplePerPixel;
                }

                proc += 1;
                printf("%6.2f %% processed...\n", 100.0 * proc / (samplePerPixel * height));
            }
        }
        image.savePPM("photonmap.ppm");
        return 0;
    }

    Color PMRenderer::executePT(const Scene& scene, const Camera& camera, const double pixelX, const double pixelY, const Random& rng, const int numTargetPhotons, const double targetRadius) const {
        Vector3 posOnSensor;
        Vector3 posOnObjplane;
        Vector3 posOnLens;
        double pImage, pLens;
        camera.samplePoints(pixelX, pixelY, rng, posOnSensor, posOnObjplane, posOnLens, pImage, pLens);
        
        Vector3 lens2sensor = posOnSensor - posOnLens;
        const double cosine = Vector3::dot(camera.direction(), lens2sensor.normalized());
        const double weight = cosine * cosine / lens2sensor.squaredNorm();

        const Ray ray(posOnLens, Vector3::normalize(posOnObjplane - posOnLens));
        return radiance(scene, ray, rng, numTargetPhotons, targetRadius, 0) * (weight * camera.sensitivity() / (pImage * pLens));
    }

    Color PMRenderer::radiance(const Scene& scene, const Ray& ray, const Random& rng, const int numTargetPhotons, const double targetRadius, const int depth, const int depthLimit, const int maxDepth) const {
        Intersection isect;
        if (!scene.intersect(ray, isect)) {
            return scene.bgColor();
        }

        const int objID = isect.objectId();
        const Material& mtrl = scene.getMaterial(objID);
        const Hitpoint hitpoint = isect.hitpoint();
        const Vector3 orientNormal = Vector3::dot(ray.direction(), hitpoint.normal()) < 0.0 ? hitpoint.normal() : -hitpoint.normal();

        // Russian roulette
        double roulette = std::max(mtrl.color.red(), std::max(mtrl.color.green(), mtrl.color.blue()));
        if (depth > maxDepth) {
            if (rng.nextReal() > roulette) {
                return mtrl.emission;
            }
        }
        roulette = 1.0;

        if (mtrl.reftype == REFLECTION_DIFFUSE) {
            // Estimate irradiance with photon map
            Photon query = Photon(hitpoint.position(), Color(), Vector3());
            std::vector<Photon> photons;
            photonMap.findKNN(query, &photons, numTargetPhotons, targetRadius);

            const int numPhotons = static_cast<int>(photons.size());
            std::vector<double> dists(numPhotons);
            double maxdist = 0.0;
            for (int i = 0; i < numPhotons; i++) {
                double dist = (photons[i] - query).norm();
                dists[i] = dist;
                maxdist = std::max(maxdist, dist);
            }

            // Cone filter
            const double k = 1.1;
            Color totalFlux = Color(0.0, 0.0, 0.0);
            for (int i = 0; i < numPhotons; i++) {
                const double w = 1.0 - (dists[i] / (k * maxdist));
                const Color v = mtrl.color.cwiseMultiply(photons[i].flux()) / PI;
                totalFlux += w * v;
            }
            totalFlux /= (1.0 - 2.0 / (3.0 * k));
            
            if (numPhotons != 0) {
                return mtrl.emission + totalFlux / ((PI * maxdist * maxdist) * roulette); 
            }
        } else if (mtrl.reftype == REFLECTION_SPECULAR) {
            Vector3 nextDir = Vector3::reflect(ray.direction(), hitpoint.normal());
            Ray nextRay = Ray(hitpoint.position(), nextDir);
            Color nextRad = radiance(scene, nextRay, rng, numTargetPhotons, targetRadius, depth + 1, depthLimit, maxDepth);
            return mtrl.emission + mtrl.color.cwiseMultiply(nextRad) / roulette;
        } else if (mtrl.reftype == REFLECTION_REFRACTION) {
            bool isIncoming = Vector3::dot(hitpoint.normal(), orientNormal) > 0.0;
            Vector3 reflectDir, refractDir;
            double fresnelRe, fresnelTr;
            if (helper::isTotalRef(isIncoming, hitpoint.position(), ray.direction(), hitpoint.normal(), orientNormal, &reflectDir, &refractDir, &fresnelRe, &fresnelTr)) {
                // Total reflection
                Ray nextRay = Ray(hitpoint.position(), reflectDir);
                Color nextRad = radiance(scene, nextRay, rng, numTargetPhotons, targetRadius, depth + 1, depthLimit, maxDepth);
                return mtrl.emission + mtrl.color.cwiseMultiply(nextRad) / roulette;
            } else {
                // Reflect or reflact
                const double probRef = 0.25 + REFLECT_PROBABLITY * fresnelRe;
                if (rng.nextReal() < probRef) {
                    // Reflect
                    Ray nextRay = Ray(hitpoint.position(), reflectDir);
                    Color nextRad = radiance(scene, nextRay, rng, numTargetPhotons, targetRadius, depth + 1, depthLimit, maxDepth);
                    return mtrl.emission + mtrl.color.cwiseMultiply(nextRad) * (fresnelRe / (probRef * roulette));
                } else {
                    // Refract
                    Ray nextRay = Ray(hitpoint.position(), refractDir);
                    Color nextRad = radiance(scene, nextRay, rng, numTargetPhotons, targetRadius, depth + 1, depthLimit, maxDepth);
                    return mtrl.emission + mtrl.color.cwiseMultiply(nextRad) * (fresnelTr / ((1.0 - probRef) * roulette));
                }
            }
        }

        return Color();
    }

    void PMRenderer::buildPM(const Scene& scene, const Camera& camera, const Random& rng, const int numPhotons) {
        std::cout << "Shooting photons..." << std::endl;

        // Shooting photons
        std::vector<Photon> photons;
        for (int pid = 0; pid < numPhotons; pid++) {
            // Generate sample on the light
            const int lightID = scene.lightID();
            const Primitive* light = scene.get(lightID);

            Vector3 positionOnLignt, normalOnLight;
            sampler::on(light, &positionOnLignt, &normalOnLight);
            double pdfAreaOnLight = 1.0 / light->area();

            double totalPdfA = pdfAreaOnLight;

            Color currentFlux = light->area() * scene.getMaterial(lightID).emission / numPhotons;
            Vector3 nextDir;
            sampler::onHemisphere(normalOnLight, &nextDir);

            Ray currentRay(positionOnLignt, nextDir);
            Vector3 prevNormal = normalOnLight;

            for (;;) {
                // Remove photon with zero flux
                if (std::max(currentFlux.red(), std::max(currentFlux.green(), currentFlux.blue())) < 0.0) {
                    break;
                }

                Intersection isect;
                bool isHit = scene.intersect(currentRay, isect);

                // If not hit the scene, then break
                if (!isHit) {
                    break;
                }

                // Hitting object
                const Material& mtrl = scene.getMaterial(isect.objectId());
                const Hitpoint& hitpoint = isect.hitpoint();

                Vector3 orientingNormal = hitpoint.normal();
                if (Vector3::dot(hitpoint.normal(), currentRay.direction()) > 0.0) {
                    orientingNormal *= -1.0;
                }

                if (mtrl.reftype == REFLECTION_DIFFUSE) {
                    photons.push_back(Photon(hitpoint.position(), currentFlux, currentRay.direction()));

                    const double probContinueTrace = (mtrl.color.red() + mtrl.color.green() + mtrl.color.blue()) / 3.0;
                    if (probContinueTrace > rng.nextReal()) {
                        // Continue trace
                        sampler::onHemisphere(orientingNormal, &nextDir);
                        currentRay = Ray(hitpoint.position(), nextDir);
                        currentFlux = currentFlux.cwiseMultiply(mtrl.color) / probContinueTrace;
                    }
                    else {
                        // Absorb (finish trace)
                        break;
                    }
                }
                else if (mtrl.reftype == REFLECTION_SPECULAR) {
                    nextDir = Vector3::reflect(currentRay.direction(), hitpoint.normal());
                    currentRay = Ray(hitpoint.position(), nextDir);
                }
                else if (mtrl.reftype == REFLECTION_REFRACTION) {
                    // Ray of reflection
                    Vector3 reflectDir = Vector3::reflect(currentRay.direction(), hitpoint.normal());
                    Ray reflectRay = Ray(hitpoint.position(), reflectDir);

                    // Determine reflact or not
                    bool isIncoming = Vector3::dot(hitpoint.normal(), orientingNormal) > 0.0;
                    const double nnt = isIncoming ? IOR_VACCUM / IOR_OBJECT : IOR_OBJECT / IOR_VACCUM;
                    const double ddn = Vector3::dot(currentRay.direction(), orientingNormal);
                    const double cos2t = 1.0 - nnt * nnt * (1.0 - ddn * ddn);

                    if (cos2t < 0.0) {
                        // Total reflection
                        currentRay = reflectRay;
                        currentFlux = currentFlux.cwiseMultiply(mtrl.color);
                        continue;
                    }

                    // Ray of reflaction
                    Vector3 reflactDir = Vector3::normalize(currentRay.direction() * nnt - ((isIncoming ? 1.0 : -1.0) * (ddn * nnt + sqrt(cos2t))) * hitpoint.normal());

                    // Compute reflaction ratio
                    const double a = IOR_VACCUM - IOR_OBJECT;
                    const double b = IOR_VACCUM + IOR_OBJECT;
                    const double R0 = (a * a) / (b * b);
                    const double c = 1.0 - (isIncoming ? -ddn : Vector3::dot(reflactDir, hitpoint.normal()));
                    const double Re = R0 + (1.0 - R0) * pow(c, 5.0);
                    const double Tr = 1.0 - Re;

                    if (rng.nextReal() < Re) {
                        // Reflection
                        currentRay = reflectRay;
                        currentFlux = currentFlux.cwiseMultiply(mtrl.color);
                    } else {
                        // Reflaction
                        currentRay = Ray(hitpoint.position(), reflactDir);
                        currentFlux = currentFlux.cwiseMultiply(mtrl.color);
                    }
                }
            }

            printf("%6.2f %% processed...\r", 100.0 * pid / numPhotons);
        }
        printf("\n\n");

        // Construct photon map
        photonMap.clear();
        photonMap.construct(photons);
    }

}
