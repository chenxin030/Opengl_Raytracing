#pragma once
#include <glm/glm.hpp>
#include <string>
#include "Material.h"

enum class ObjectType { SPHERE, PLANE };

struct Object {
    alignas(4) ObjectType type;
    alignas(16) glm::vec3 position;
    alignas(16) Material material;
    alignas(4) float radius;
    alignas(16) glm::vec3 normal;
    alignas(4) float distance;
};

// UI用对象（包含名称）
struct UIObject {
    char name[128] = "New Object";
    Object obj;
};
