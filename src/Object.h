#pragma once
#include <glm/glm.hpp>

enum class ObjectType { SPHERE, PLANE };


struct Object {
    int type;
    glm::vec3 position;     
    glm::vec3 color;
    float radius;           // ����ר��
    glm::vec3 normal;       // ƽ��ר��
	float distance;	        // ƽ��ר��
};