
#pragma once
#include <GLFW/glfw3.h>

class SSBO;

class ImGuiManager {
public:
    ImGuiManager(GLFWwindow* window);
    ~ImGuiManager();

    void BeginFrame();
    void EndFrame();
    void DrawObjectController(class SSBO& ssbo);
    void DrawObjectsList(class SSBO& ssbo);

private:
    void SetupStyle();
};