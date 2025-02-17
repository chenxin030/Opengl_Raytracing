// Light.h
#pragma once
#include <glm/glm.hpp>

enum class LightType { POINT, DIRECTIONAL, AREA };

struct Light {
    alignas(4)   LightType type = LightType::POINT;                 // 光源类型
    alignas(16)  glm::vec3 position = glm::vec3(0.0f);              // 点光源位置
    alignas(16)  glm::vec3 direction = glm::vec3(0.0f, -1.0f, 0.0f);// 定向光方向
    alignas(16)  glm::vec3 color = glm::vec3(1.0f);                 // 光源颜色
    alignas(4)   float intensity = 1.0f;                            // 光照强度
    alignas(4)   float radius = 0.5f;                               // 点光源影响半径（可选）
    alignas(4)   int samples = 4;                                   // 面光源采样次数（可选）
    alignas(4)   float shadowSoftness = 1;                          // 阴影柔化强度
    alignas(4)   int shadowType = 1;                                // 0=无 1=PCF 2=PCSS
    alignas(4)   int pcfSamples = 4;                                // PCF采样数
    alignas(4)   float lightSize = 1;                               // PCSS光源尺寸
    alignas(4)   float angularRadius;                               // 定向光的角度半径（弧度制）
};


// UI用对象（包含名称）
struct UILight {
    char name[128] = "New Light";
    Light light;
};
