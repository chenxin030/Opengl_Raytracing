#pragma once
#include <glm/glm.hpp>

enum class ObjectType { SPHERE, PLANE };


struct Object {
    alignas(4) int type;
    alignas(16) glm::vec3 position;     
    alignas(16) glm::vec3 color;
    alignas(4) float radius;           // 球体专用
    alignas(16) glm::vec3 normal;       // 平面专用
	alignas(4) float distance;	        // 平面专用
};