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
    Vec3f color;

    Sphere(const Vec3f &c, const float &r, const Vec3f &color) : center(c), radius(r), color(color) {}

    // intersecting algorithm: http://www.lighthouse3d.com/tutorials/maths/ray-sphere-intersection/
    bool ray_intersect(const Vec3f &p, const Vec3f &dir, float &closestDist) const
    {
        Vec3f vpc = center - p;
        float projection = vpc * dir;
        float disToRay = vpc * vpc - projection * projection;

        // if (|vpc| > r) there is no intersection
        if (disToRay > radius * radius)
            return false;
        float t0todisToRay = sqrtf((radius * radius) - disToRay);
        float t0 = projection - t0todisToRay;
        float t1 = projection + t0todisToRay;
        if (t0 < 0)
            t0 = t1;
        if (t0 < 0)
            return false;

        if (t0 < closestDist)
        {
            closestDist = t0;
            return true;
        }
        return true;
    }
};

Vec3f cast_ray(const Vec3f &orig, const Vec3f &dir, const std::vector<Sphere> &scene)
{
    float sphere_dist = std::numeric_limits<float>::max();
    Vec3f fillColor{};
    bool filled = false;

    for (const Sphere &s : scene)
    {
        if (s.ray_intersect(orig, dir, sphere_dist))
        {
            fillColor = s.color;
            filled = true;
        }
    }

    if (!filled)
        return Vec3f(0.3, 0.3, 0.3);
    else
        return fillColor;
}

void render()
{
    const int width = 1024;
    const int height = 768;
    std::vector<Vec3f> framebuffer(width * height);
    const int fov = M_PI / 2.;

    Vec3f babyBlue = {0.537, 0.812, 0.941};
    Vec3f babyPink = {0.973, 0.537, 0.537};

    std::vector<Sphere> scene;
    scene.push_back(Sphere(Vec3f(-3, 0, -16), 2, babyBlue));
    scene.push_back(Sphere(Vec3f(-1.0, -1.5, -12), 2, babyPink));
    scene.push_back(Sphere(Vec3f(1.5, -0.5, -18), 3, babyBlue));
    scene.push_back(Sphere(Vec3f(7, 5, -18), 4, babyPink));

    for (size_t j = 0; j < height; j++)
    {
        for (size_t i = 0; i < width; i++)
        {
            float x = (2 * (i + 0.5) / (float)width - 1) * tan(fov / 2.) * width / (float)height;
            float y = -(2 * (j + 0.5) / (float)height - 1) * tan(fov / 2.);
            Vec3f dir = Vec3f(x, y, -1).normalize();
            framebuffer[i + j * width] = cast_ray(Vec3f(0, 0, 0), dir, scene); // 카메라는 0,0,0에 위치
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
    render();

    return 0;
}
