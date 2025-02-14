#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "Camera.h"

extern const int WIDTH;
extern const int HEIGHT;
extern Camera camera;
extern bool firstMouse;
extern float lastX;
extern float lastY;
extern float deltaTime;
extern float lastFrame;


void RenderQuad();

// TAA： Halton序列生成函数（低差异序列）
float haltonSequence(int index, int base);

void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);