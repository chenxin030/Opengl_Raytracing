// Material.h
#pragma once
#include <glm/glm.hpp>

enum MaterialType {
    MATERIAL_METALLIC,
    MATERIAL_DIELECTRIC,
    MATERIAL_PLASTIC
};

struct Material {
    alignas(4) MaterialType type = MATERIAL_PLASTIC;    // 4字节对齐，占4字节
    alignas(16) glm::vec3 albedo = glm::vec3(1.0f);     // 16字节对齐，占12字节
    alignas(4) float metallic = 0.0f;                   // 4字节对齐，占4字节
    alignas(4) float roughness = 0.5f;                  // 4字节对齐，占4字节
    alignas(4) float ior = 1.0f;                        // 4字节对齐，占4字节
    alignas(4) float transparency = 0.0f;               // 4字节对齐，占4字节
    alignas(4) float specular = 0.5f;                   // 4字节对齐，占4字节
};

