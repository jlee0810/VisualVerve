#include <limits>
#include <cmath>
#include <iostream>
#include <fstream>
#include <vector>
#include "geometry.h"

struct Sphere
{
    // Define the center and radius of the sphere
    Vec3f center;
    float radius;

    Sphere(const Vec3f &c, const float &r) : center(c), radius(r) {}

    // intersecting algorithm: http://www.lighthouse3d.com/tutorials/maths/ray-sphere-intersection/
    bool ray_intersect(const Vec3f &p, const Vec3f &dir, float &t0) const
    {
        Vec3f vpc = center - p;
        float projection = vpc * dir;
        float disToRay = vpc * vpc - projection * projection;

        // if (|vpc| > r) there is no intersection
        if (disToRay > radius * radius)
            return false;
        float t0todisToRay = sqrtf((radius * radius) - disToRay);
        t0 = projection - t0todisToRay;
        float t1 = projection + t0todisToRay;
        if (t0 < 0)
            t0 = t1;
        if (t0 < 0)
            return false;
        return true;
    }
};

Vec3f cast_ray(const Vec3f &orig, const Vec3f &dir, const Sphere &sphere)
{
    float sphere_dist = std::numeric_limits<float>::max();
    if (!sphere.ray_intersect(orig, dir, sphere_dist))
    {
        return Vec3f(0.2, 0.7, 0.8); // background color
    }
    return Vec3f(0.4, 0.4, 0.3);
}

void render(const Sphere &sphere)
{
    const int width = 1024;
    const int height = 768;
    std::vector<Vec3f> framebuffer(width * height);
    const int fov = M_PI / 2.;

    for (size_t j = 0; j < height; j++)
    {
        for (size_t i = 0; i < width; i++)
        {
            float x = (2 * (i + 0.5) / (float)width - 1) * tan(fov / 2.) * width / (float)height;
            float y = -(2 * (j + 0.5) / (float)height - 1) * tan(fov / 2.);
            Vec3f dir = Vec3f(x, y, -1).normalize();
            framebuffer[i + j * width] = cast_ray(Vec3f(0, 0, 0), dir, sphere);
        }
    }

    std::ofstream ofs; // save the framebuffer to file
    ofs.open("./out.ppm");
    ofs << "P6\n"
        << width << " " << height << "\n255\n";
    for (size_t i = 0; i < height * width; ++i)
    {
        for (size_t j = 0; j < 3; j++)
        {
            ofs << (char)(255 * std::max(0.f, std::min(1.f, framebuffer[i][j])));
        }
    }
    ofs.close();
}

int main()
{
    Sphere sphere(Vec3f(-3, 0, -16), 2);
    render(sphere);

    return 0;
}
