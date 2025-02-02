#pragma once
#include <glm/glm.hpp>

enum class ObjectType { SPHERE, PLANE };


struct Object {
    int type;
    glm::vec3 position;     
    glm::vec3 color;
    float radius;           // 球体专用
    glm::vec3 normal;       // 平面专用
	float distance;	        // 平面专用
};