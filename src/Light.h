// Light.h
#pragma once
#include <glm/glm.hpp>

enum class LightType { POINT, DIRECTIONAL, AREA };

struct Light {
    alignas(4)   LightType type = LightType::POINT;                 // ��Դ����
    alignas(16)  glm::vec3 position = glm::vec3(0.0f);              // ���Դλ��
    alignas(16)  glm::vec3 direction = glm::vec3(0.0f, -1.0f, 0.0f);// ����ⷽ��
    alignas(16)  glm::vec3 color = glm::vec3(1.0f);                 // ��Դ��ɫ
    alignas(4)   float intensity = 1.0f;                            // ����ǿ��
    alignas(4)   float radius = 0.0f;                               // ���ԴӰ��뾶����ѡ��
    alignas(4)   int samples = 1;                                   // ���Դ������������ѡ��
};