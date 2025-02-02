#pragma once
#include <glm/glm.hpp>

enum class ObjectType { SPHERE, PLANE };


struct Object {
    alignas(4) int type;
    alignas(16) glm::vec3 position;     
    alignas(16) glm::vec3 color;
    alignas(4) float radius;           // ����ר��
    alignas(16) glm::vec3 normal;       // ƽ��ר��
	alignas(4) float distance;	        // ƽ��ר��
};