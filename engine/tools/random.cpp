#include "random.hpp"

 // Perlin noise implementation - https://stackoverflow.com/questions/29711668/perlin-noise-generation
static int numOctaves = 2, primeIndex = 0;
 static float fade(float t) {
     return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
 }

 static float lerp(float a, float b, float t) {
     return a + (b - a) * t;
 }

 static float lattice_noise3D(int seed, int x, int y, int z) {
     uint32_t n = 0;

     n ^= static_cast<uint32_t>(x) * 374761393u;
     n ^= static_cast<uint32_t>(y) * 668265263u;
     n ^= static_cast<uint32_t>(z) * 2246822519u;
     n ^= static_cast<uint32_t>(seed) * 3266489917u;

     n ^= n >> 13;
     n *= 1274126177u;
     n ^= n >> 16;

     return static_cast<float>(n) / 2147483647.5f - 1.0f;
 }

 static float interpolated_noise3D(int seed, float x, float y, float z) {
     const int xi = static_cast<int>(std::floor(x));
     const int yi = static_cast<int>(std::floor(y));
     const int zi = static_cast<int>(std::floor(z));

     const float xf = x - static_cast<float>(xi);
     const float yf = y - static_cast<float>(yi);
     const float zf = z - static_cast<float>(zi);

     const float u = fade(xf);
     const float v = fade(yf);
     const float w = fade(zf);

     const float n000 = lattice_noise3D(seed, xi, yi, zi);
     const float n100 = lattice_noise3D(seed, xi + 1, yi, zi);
     const float n010 = lattice_noise3D(seed, xi, yi + 1, zi);
     const float n110 = lattice_noise3D(seed, xi + 1, yi + 1, zi);

     const float n001 = lattice_noise3D(seed, xi, yi, zi + 1);
     const float n101 = lattice_noise3D(seed, xi + 1, yi, zi + 1);
     const float n011 = lattice_noise3D(seed, xi, yi + 1, zi + 1);
     const float n111 = lattice_noise3D(seed, xi + 1, yi + 1, zi + 1);

     const float x00 = lerp(n000, n100, u);
     const float x10 = lerp(n010, n110, u);
     const float x01 = lerp(n001, n101, u);
     const float x11 = lerp(n011, n111, u);

     const float y0 = lerp(x00, x10, v);
     const float y1 = lerp(x01, x11, v);

     return lerp(y0, y1, w);
 }

 float Random::noise3D(float x, float y, float z) {
     float total = 0.0f;
     float frequency = 5.0f;
     float amplitude = 1.0f;
     float totalAmplitude = 0.0f;

     for (int i = 0; i < numOctaves; ++i) {
         total += interpolated_noise3D(primeIndex + i, x * frequency, y * frequency, z * frequency) * amplitude;
         totalAmplitude += amplitude;

         amplitude *= 0.5f;
         frequency *= 2.0f;
     }

     return ((total / totalAmplitude) + 1.f) * 0.5f;
 }
