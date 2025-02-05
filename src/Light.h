// Light.h
#pragma once
#include <glm/glm.hpp>

enum class LightType { POINT, DIRECTIONAL, AREA };

struct Light {
    alignas(4)   LightType type;          // ��Դ����
    alignas(16)  glm::vec3 position;      // ���Դλ��
    alignas(16)  glm::vec3 direction;     // ����ⷽ��
    alignas(16)  glm::vec3 color;         // ��Դ��ɫ
    alignas(4)   float intensity;         // ����ǿ��
    alignas(4)   float radius;            // ���ԴӰ��뾶����ѡ��
	alignas(4)   int samples;             // ���Դ������������ѡ��
};