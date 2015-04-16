#define SPICA_RENDERER_EXPORT
#include "Renderer.h"

#include <iostream>
#include <fstream>
#include <algorithm>

#include "../utils/common.h"
#include "../utils/Vector3.h"
#include "material.h"
#include "Scene.h"

namespace spica {

    Renderer::Renderer(int width, int height, int samples, int supsamples) 
        : RendererBase(width, height, samples, supsamples) 
    {
    }

    Renderer::Renderer(const Renderer& renderer)
        : RendererBase(renderer)
    {
    }

    Renderer::~Renderer() 
    {
    }

    Renderer& Renderer::operator=(const Renderer& renderer) {
        RendererBase::operator=(renderer);
        return *this;
    }

    void Renderer::setSize(int width, int height) {
        this->_width = width;
        this->_height = height;
    }

    void Renderer::setSamples(int samples, int supsamples) {
        this->_samplePerPixel = samples;
        this->_supsamplePerAxis = supsamples;
    }

    int Renderer::render(const Scene& scene) {
        // Camera position
        const Vector3 cameraPos(50.0, 52.0, 220.0);
        const Vector3 cameraDir = Vector3(0.0, -0.04, -1.0).normalize();
        const Vector3 cameraUp(0.0, 1.0, 0.0);

        // Screen size in world space
        const double screenWidth = 30.0 * _width / _height;
        const double screenHeight = 30.0;

        // Distance to the screen
        const double distToScreen = 40.0;

        // Vectors spanning screen
        const Vector3 screenX = cameraDir.cross(cameraUp).normalize() * screenWidth;
        const Vector3 screenY = screenX.cross(cameraDir).normalize() * screenHeight;
        const Vector3 scrrenCenter = cameraPos + cameraDir * distToScreen;

        Color* image = new Color[_width * _height];

        Random rng = Random::getRNG();
        ompfor (int y = 0; y < _height; y++) {
            std::cout << "Row: " << y << " is processing..." << std::endl;

            for (int x = 0; x < _width; x++) {
                const int pixelIndex = (_height - y - 1) * _width + x;
                for (int sy = 0; sy < _supsamplePerAxis; sy++) {
                    for (int sx = 0; sx < _supsamplePerAxis; sx++) {
                        Color accum;
                        for (int s = 0; s < _samplePerPixel; s++) {
                            const double rate = (1.0 / _supsamplePerAxis);
                            const double rx = sx * rate + rate / 2.0;
                            const double ry = sy * rate + rate / 2.0;
                            const Vector3 screenPos = scrrenCenter + screenX * ((rx + x) / _width - 0.5) + screenY * ((ry + y) / _height - 0.5);
                            const Vector3 rayDirection = (screenPos - cameraPos).normalize();
                        
                            accum += radiance(scene, Ray(cameraPos, rayDirection), rng, 0) / (_samplePerPixel * _supsamplePerAxis * _supsamplePerAxis);
                        }
                        image[pixelIndex] += accum;
                    }
                }
            }
        }

        savePPM("simplept.ppm", image, _width, _height);
        delete[] image;

        return 0;
    }

    Color Renderer::radiance(const Scene& scene, const Ray& ray, Random& rng, const int depth) {
        Intersection intersection;

        // NOT intersect the scene
        if (!scene.intersect(ray, intersection)) {
            return backgroundColor;
        }

        const Primitive* currentObj = scene.getObjectPtr(intersection.objectId());
        const HitPoint& hitpoint = intersection.hitPoint();
        const Vector3 orientNormal = hitpoint.normal().dot(ray.direction()) < 0.0 ? hitpoint.normal() : (-1.0 * hitpoint.normal());

        double rouletteProb =std::max(currentObj->color().x(), std::max(currentObj->color().y(), currentObj->color().z()));

        if (depth > depthLimit) {
            rouletteProb *= pow(0.5, depth - depthLimit);
        }

        if (depth > maxDepth) {
            if (rng.randReal() > rouletteProb) {
                return currentObj->emission();
            }
        } else {
            rouletteProb = 1.0;
        }

        Color incomingRad;
        Color weight = Color(1.0, 0.0, 0.0);

        if (currentObj->reftype() == REFLECTION_DIFFUSE) {
            Vector3 u, v, w;
            w = orientNormal;
            if (abs(w.x()) > EPS) {
                u = Vector3(0.0, 1.0, 0.0).cross(w).normalize();
            } else {
                u = Vector3(1.0, 0.0, 0.0).cross(w).normalize();
            }

            v = w.cross(u);

            const double r1 = 2.0 * PI * rng.randReal();
            const double r2 = rng.randReal();
            const double r2s = sqrt(r2);
            Vector3 nextDir = (u * cos(r1) * r2s + v * sin(r1) * r2s + w * sqrt(1.0 - r2)).normalize();

            incomingRad = radiance(scene, Ray(hitpoint.position(), nextDir), rng, depth + 1);
            weight = currentObj->color() / rouletteProb;

        } else if (currentObj->reftype() == REFLECTION_SPECULAR) {
            Vector3 nextDir = ray.direction() - (2.0 * hitpoint.normal().dot(ray.direction())) * hitpoint.normal();
            incomingRad = radiance(scene, Ray(hitpoint.position(), nextDir), rng, depth + 1);
            weight = currentObj->color() / rouletteProb;

        } else if (currentObj->reftype() == REFLECTION_REFRACTION) {
            Vector3 reflectDir = ray.direction() - (2.0 * hitpoint.normal().dot(ray.direction())) * hitpoint.normal();
            const Ray reflectRay = Ray(hitpoint.position(), reflectDir);

            // Incoming or outgoing
            const bool isIncoming = hitpoint.normal().dot(orientNormal) > 0.0;

            // Snell's rule
            const double nc = 1.0;
            const double nt = indexOfRef;
            const double nnt = isIncoming ? nc / nt : nt / nc;
            const double ddn = ray.direction().dot(orientNormal);
            const double cos2t = 1.0 - nnt * nnt * (1.0  - ddn * ddn);

            if (cos2t < 0.0) { // Total reflection
                incomingRad = radiance(scene, reflectRay, rng, depth + 1);
                weight += currentObj->color() / rouletteProb;
            } else {
                Vector3 refractDir = (ray.direction() * nnt - hitpoint.normal() * (isIncoming ? 1.0 : -1.0) * (ddn * nnt + sqrt(cos2t))).normalize();
                const Ray refractRay = Ray(hitpoint.position(), refractDir);

                // Schlick's approximation of Fresnel coefficient
                const double a = nt - nc;
                const double b = nt + nc;
                const double R0 = (a * a) / (b * b);

                const double c = 1.0 - (isIncoming ? -ddn : - refractDir.dot(orientNormal));
                const double Re = R0 + (1.0 - R0) * pow(c, 5.0);
                const double nnt2 = pow(isIncoming ? nc / nt : nt / nc, 2.0);
                const double Tr = (1.0 - Re) * nnt2;

                const double prob = 0.25 + 0.5 * Re;
                if (depth > 2) {
                    if (rng.randReal() < prob) {
                        incomingRad = radiance(scene, reflectRay, rng, depth + 1) * Re;
                        weight = currentObj->color() / (prob * rouletteProb);
                    } else {
                        incomingRad = radiance(scene, refractRay, rng, depth + 1) * Tr;
                        weight = currentObj->color() / ((1.0 - prob) * rouletteProb);
                    }
                } else {
                    incomingRad = radiance(scene, reflectRay, rng, depth + 1) * Re + radiance(scene, refractRay, rng, depth + 1) * Tr;
                    weight = currentObj->color() / rouletteProb;
                }
            }
        }

        return currentObj->emission() + weight.cwiseMultiply(incomingRad);
    }

    void Renderer::savePPM(std::string filename, Color* image, int width, int height) {
        std::ofstream ofs(filename.c_str(), std::ios::out);
        ofs << "P3" << std::endl;
        ofs << width << " " << height << " 255" << std::endl;

        for (int i = 0; i < width * height; i++) {
            int r = (int)(255.0 * std::max(0.0, std::min(image[i].x(), 1.0)));
            int g = (int)(255.0 * std::max(0.0, std::min(image[i].y(), 1.0)));
            int b = (int)(255.0 * std::max(0.0, std::min(image[i].z(), 1.0)));
            ofs << r << " " << g << " " << b << std::endl;
        }
        ofs.close();
    }
}