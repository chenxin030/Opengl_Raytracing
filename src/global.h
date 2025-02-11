#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>

void RenderQuad();

// TAA： Halton序列生成函数（低差异序列）
float haltonSequence(int index, int base);