#include "gtest/gtest.h"

#include "../include/spica.h"
using namespace spica;

#include <iostream>
#include <fstream>

#include "test_macros.h"

const int nTrial = 100;

TEST(SamplerTest, HemisphereSampleTest) {
    Normal n(0.0, 1.0, 0.0);

    for (int i = 0; i < nTrial; i++) {
        Vector3D d;
        sampler::onHemisphere(n, &d);

        EXPECT_LE(0.0, vect::dot(n, d));
        EXPECT_LE(vect::dot(n, d), 1.0);
    }
}

TEST(SamplerTest, DiskSampleTest) {
    Point C(0.0, 0.0, 0.0);
    Normal N(0.0, 0.0, 1.0);
    double R = 5.0;
    Disk disk(C, N, R);

    for (int i = 0; i < nTrial; i++) {
        Point position;
        Normal normal;
        sampler::onDisk(disk, &position, &normal);

        EXPECT_EQ_VEC(N, normal);
        EXPECT_LE((C - position).norm(), R);
        EXPECT_EQ(0.0, vect::dot(C - position, N));
    }
}

TEST(SamplerTest, SphereSampleTest) {
    Point c(0.0, 1.0, 2.0);
    double r = 5.0;
    Sphere sph(c, r);

    for (int i = 0; i < nTrial; i++) {
        Point v;
        Normal n;
        sampler::onSphere(sph, &v, &n);
        EXPECT_FLOAT_EQ(r, (v - c).norm());
        EXPECT_EQ_VEC(n, (v - c).normalized());
    }
}

TEST(SamplerTest, TriangleSampleTest) {
    Point a(0.0, 1.0, 0.0);
    Point b(0.0, 1.0, 1.0);
    Point c(0.0, 0.0, 1.0);
    Triangle tri(a, b, c);

    for (int i = 0; i < nTrial; i++) {
        Point v;
        Normal n;
        sampler::onTriangle(tri, &v, &n);
        EXPECT_EQ_VEC(tri.normal(), n);
        EXPECT_FLOAT_EQ(0.0, vect::dot(v - tri[0], n));

        Vector3D e1 = tri[1] - tri[0];
        Vector3D e2 = tri[2] - tri[0];
        Vector3D e  = v - tri[0];
        EXPECT_LE(0.0, e1.dot(e) / e1.squaredNorm());
        EXPECT_GE(1.0, e1.dot(e) / e1.squaredNorm());
        EXPECT_LE(0.0, e2.dot(e) / e2.squaredNorm());
        EXPECT_GE(1.0, e2.dot(e) / e2.squaredNorm());
    }
}

TEST(SamplerTest, PoissonDiskTest) {
    Trimesh mesh(kDataDirectory + "bunny.ply");
    std::vector<Triangle> tris = mesh.getTriangleList();
    double mindist = 0.1;

    std::vector<Point> points;
    std::vector<Normal> normals;
    sampler::poissonDisk(tris, mindist, &points, &normals);

    EXPECT_NE(0, points.size());
    EXPECT_EQ(points.size(), normals.size());
    for (int i = 0; i < points.size(); i++) {
        for (int j = i + 1; j < points.size(); j++) {
            EXPECT_LE((points[i] - points[j]).norm(), 2.0 * mindist);
        }
    }
}