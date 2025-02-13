#pragma once
#include <glm/glm.hpp>
#include <string>
#include "Material.h"

enum class ObjectType { SPHERE, PLANE };

struct Object {
    alignas(4) ObjectType type;         // 4字节对齐，占4字节
    alignas(16) glm::vec3 position;     // 16字节对齐，占12字节
    alignas(4) float radius = 1.0f;            // 4字节对齐，占4字节
    alignas(16) glm::vec3 normal = glm::vec3(0.0f, 1.0f, 0.0f);       // 16字节对齐，占12字节
    alignas(16) glm::vec2 size = glm::vec2(1.0f, 1.0f);       // 新增：平面尺寸（width, height）
    alignas(16) Material material;      // 16字节对齐，占48字节
};


// UI用对象（包含名称）
struct UIObject {
    char name[128] = "New Object";
    Object obj;
};
