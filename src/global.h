#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>

void RenderQuad();

// TAA�� Halton�������ɺ������Ͳ������У�
float haltonSequence(int index, int base);