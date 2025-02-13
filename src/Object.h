#pragma once
#include <glm/glm.hpp>
#include <string>
#include "Material.h"

enum class ObjectType { SPHERE, PLANE };

struct Object {
    alignas(4) ObjectType type;         // 4�ֽڶ��룬ռ4�ֽ�
    alignas(16) glm::vec3 position;     // 16�ֽڶ��룬ռ12�ֽ�
    alignas(4) float radius = 1.0f;            // 4�ֽڶ��룬ռ4�ֽ�
    alignas(16) glm::vec3 normal = glm::vec3(0.0f, 1.0f, 0.0f);       // 16�ֽڶ��룬ռ12�ֽ�
    alignas(16) glm::vec2 size = glm::vec2(1.0f, 1.0f);       // ������ƽ��ߴ磨width, height��
    alignas(16) Material material;      // 16�ֽڶ��룬ռ48�ֽ�
};


// UI�ö��󣨰������ƣ�
struct UIObject {
    char name[128] = "New Object";
    Object obj;
};
