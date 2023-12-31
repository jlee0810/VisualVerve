#include <limits>
#include <cmath>
#include <iostream>
#include <fstream>
#include <vector>
#include "geometry.h"

struct Material
{
    Material(const float &r, const Vec4f &a, const Vec3f &color, const float &spec)
        : refractive_index{r}, albedo(a), diffuse_color(color), specular_exponent(spec) {}
    Material() : refractive_index{1}, albedo(1, 0, 0, 0), diffuse_color(), specular_exponent() {}

    float refractive_index; // index of refraction
    Vec4f albedo;           // fraction of light that is diffusely reflected by a body
    Vec3f diffuse_color;
    float specular_exponent;
};

struct Sphere
{
    // Define the center and radius of the sphere
    Vec3f center;
    float radius;
    Material material;

    Sphere(const Vec3f &c, const float &r, const Material &mat) : center{c}, radius{r}, material{mat} {}

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
        return false;
    }
};

struct Light
{
    // Point Light Source
    Light(const Vec3f &p, const float &i) : position(p), intensity(i) {}
    Vec3f position;
    float intensity;
};

Vec3f reflect(const Vec3f &light, const Vec3f &normal)
{
    return light - normal * 2.f * (light * normal);
}

Vec3f refract(const Vec3f &I, const Vec3f &N, const float &refractive_index)
{
    float cosi = -std::max(-1.f, std::min(1.f, I * N));
    float etai = 1, etat = refractive_index;
    Vec3f n = N;
    if (cosi < 0)
    {
        cosi = -cosi;
        std::swap(etai, etat);
        n = -N;
    }
    float eta = etai / etat;
    float k = 1 - eta * eta * (1 - cosi * cosi);
    return k < 0 ? Vec3f(0, 0, 0) : I * eta + n * (eta * cosi - sqrtf(k));
}

bool sceneIntersect(const Vec3f &orig, const Vec3f &dir, const std::vector<Sphere> &scene, Vec3f &hit, Vec3f &N, Material &mat)
{
    float sphereDist = std::numeric_limits<float>::max();
    Vec3f fillColor{};
    for (const Sphere &s : scene)
    {
        if (s.ray_intersect(orig, dir, sphereDist))
        {
            hit = orig + dir * sphereDist;
            N = (hit - s.center).normalize();
            mat = s.material;
        }
    }
    return sphereDist < 1000; // Ray is not infinite we set a limit to 1000 (== far plane is 1000)
}

Vec3f cast_ray(const Vec3f &orig, const Vec3f &dir, const std::vector<Sphere> &scene, const std::vector<Light> &lights = {}, size_t depth = 0)
{
    Vec3f point, N;
    Material mat;

    // depth here is the max number of times the ray can bounce off an object
    if (depth > 4 || !sceneIntersect(orig, dir, scene, point, N, mat))
        return Vec3f(0.3, 0.3, 0.3); // background color

    Vec3f reflect_dir = reflect(dir, N).normalize();
    Vec3f refract_dir = refract(dir, N, mat.refractive_index).normalize();
    Vec3f reflect_orig = reflect_dir * N < 0 ? point - N * 1e-3 : point + N * 1e-3;
    Vec3f refract_orig = refract_dir * N < 0 ? point - N * 1e-3 : point + N * 1e-3;
    Vec3f reflect_color = cast_ray(reflect_orig, reflect_dir, scene, lights, depth + 1);
    Vec3f refract_color = cast_ray(refract_orig, refract_dir, scene, lights, depth + 1);

    float diffuseIntensity = 0, specular_light_intensity = 0;
    for (const Light &l : lights)
    {
        Vec3f lightDir = (l.position - point).normalize(); // Vector from light source to point
        float listDist = (l.position - point).norm();      // Distance from light source to point

        Vec3f shadow_orig = lightDir * N < 0 ? point - N * 1e-3 : point + N * 1e-3; // Point + N * 1e-3 is to avoid shadow acne
        Vec3f shadow_pt, shadow_N;

        Material tmpmaterial;
        if (sceneIntersect(shadow_orig, lightDir, scene, shadow_pt, shadow_N, tmpmaterial) && (shadow_pt - shadow_orig).norm() < listDist)
            continue; // do not get diffuse or specular intensity

        diffuseIntensity += l.intensity * std::max(0.f, lightDir * N); // Diffuse intensity is the dot product of the light direction and the normal
        specular_light_intensity += powf(std::max(0.f, reflect(lightDir, N) * dir), mat.specular_exponent) * l.intensity;
    }

    return mat.diffuse_color * diffuseIntensity * mat.albedo[0] + Vec3f(1., 1., 1.) * specular_light_intensity * mat.albedo[1] + reflect_color * mat.albedo[2] + refract_color * mat.albedo[3];
}

void render()
{
    const int width = 1024;
    const int height = 768;
    std::vector<Vec3f> framebuffer(width * height);
    const int fov = M_PI / 2.;

    Material babyBlue = {1.0, Vec4f(0.6, 0.3, 0.1, 0.0), Vec3f(0.537, 0.812, 0.941), 50};
    Material babyPink = {1.0, Vec4f(0.6, 0.3, 0.0, 0.0), Vec3f(0.941, 0.537, 0.812), 5};
    Material mirror = {1.0, Vec4f(0.0, 10.0, 0.8, 0.0), Vec3f(1.0, 1.0, 1.0), 1425.};
    Material glass = {1.5, Vec4f(0.0, 0.5, 0.1, 0.8), Vec3f(0.6, 0.7, 0.8), 125.};

    std::vector<Sphere> scene;
    scene.push_back(Sphere(Vec3f(-3, 0, -16), 2, babyPink));
    scene.push_back(Sphere(Vec3f(-1.0, -1.5, -12), 2, glass));
    scene.push_back(Sphere(Vec3f(1.5, -0.5, -18), 3, babyBlue));
    scene.push_back(Sphere(Vec3f(7, 5, -18), 4, mirror));

    std::vector<Light> lights;
    lights.emplace_back(Vec3f(-20, 20, 20), 1.5f);

    for (size_t j = 0; j < height; j++)
    {
        for (size_t i = 0; i < width; i++)
        {
            float x = (2 * (i + 0.5) / (float)width - 1) * tan(fov / 2.) * width / (float)height;
            float y = -(2 * (j + 0.5) / (float)height - 1) * tan(fov / 2.);
            Vec3f dir = Vec3f(x, y, -1).normalize();
            framebuffer[i + j * width] = cast_ray(Vec3f(0, 0, 0), dir, scene, lights); // Place camera at 0,0,0
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
