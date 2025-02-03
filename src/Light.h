// Light.h
#pragma once
#include <glm/glm.hpp>

enum class LightType { POINT, DIRECTIONAL };

struct Light {
    alignas(4)   LightType type;          // 光源类型
    alignas(16)   glm::vec3 position;      // 点光源位置
    alignas(16)   glm::vec3 direction;     // 定向光方向
    alignas(16)   glm::vec3 color;         // 光源颜色
    alignas(4)   float intensity;         // 光照强度
    alignas(4)   float radius;            // 点光源影响半径（可选）
};