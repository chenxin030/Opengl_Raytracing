
#pragma once
#include <GLFW/glfw3.h>
#include "Camera.h"
#include "LightSSBO.h"

class SSBO;

class ImGuiManager {
public:
    ImGuiManager(GLFWwindow* window);
    ~ImGuiManager();

    void BeginFrame();
    void EndFrame();

    void DrawObjectsList(class SSBO& ssbo);
    void DrawCameraControls(Camera& camera);
    void DrawLightController(LightSSBO& lightSSBO);
    void DrawFPS();

	void HandleCameraMovement(Camera& camera, float deltaTime);

private:

    GLFWwindow* m_Window;

    void SetupStyle();
};