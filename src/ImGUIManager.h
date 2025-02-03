
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
    GLuint GetCurrentSkyboxTexture() const { return m_CurrentSkyboxTexture; }
    bool IsSkyboxEnabled() const { return m_UseSkybox; }
    void ChooseSkybox();

	void HandleCameraMovement(Camera& camera, float deltaTime);

private:

    GLFWwindow* m_Window;

    // skybox
    bool m_UseSkybox = true;
    int m_SelectedSkyboxIndex = 0;
    GLuint m_CurrentSkyboxTexture = 0;
    const std::vector<std::string> m_SkyboxPaths = {
        "belfast_sunset_puresky_2k.hdr",
        "christmas_photo_studio_01_2k.hdr",
        "rainforest_trail_2k.hdr",
        "small_cave_2k.hdr",
        "syferfontein_0d_clear_puresky_2k.hdr",
        "the_sky_is_on_fire_2k.hdr",
        "vestibule_2k.hdr"
    };
    const std::vector<const char*> m_SkyboxNames = {
        "Belfast Sunset",
        "Christmas Studio",
        "Rainforest Trail",
        "Small Cave",
        "Syferfontein Clear",
        "Sky on Fire",
        "Vestibule"
    };

    void SetupStyle();
};