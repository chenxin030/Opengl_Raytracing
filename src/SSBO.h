#pragma once
#include <vector>
#include "Object.h"
#include <GL/glew.h>
#include <iostream>

class SSBO {
public:
    GLuint id;
    std::vector<Object> objects;

    SSBO() = default;
    void Init() {
        glGenBuffers(1, &id);
    }
    void update() {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, id);
        glBufferData(GL_SHADER_STORAGE_BUFFER,
            objects.size() * sizeof(Object),
            objects.data(),
            GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, id);
    }
};