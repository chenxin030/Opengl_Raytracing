// LightSSBO.h
#pragma once
#include "Light.h"
#include <vector>
#include <GL/glew.h>

class LightSSBO {
public:
    GLuint id;
    std::vector<Light> lights;

    LightSSBO() {
        glGenBuffers(1, &id);
    }

    void update() {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, id);
        glBufferData(
            GL_SHADER_STORAGE_BUFFER,
            lights.size() * sizeof(Light),
            lights.data(),
            GL_DYNAMIC_DRAW
        );
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, id); // °ó¶¨µ½Ë÷Òý1
    }
};