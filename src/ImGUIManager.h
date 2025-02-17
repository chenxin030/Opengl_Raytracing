
#pragma once
#include <GLFW/glfw3.h>
#include <filesystem>
#include <vector>

#include "Camera.h"
#include "LightSSBO.h"
#include "AO.h"
#include "Object.h"

class SSBO;

class ImGuiManager {
public:
    GLFWwindow* m_Window;

    ImGuiManager() = default;
    ImGuiManager(GLFWwindow* window);
    void Init();
    ~ImGuiManager();

    void BeginFrame();
    void EndFrame();

    void DrawObjectsList(class SSBO& ssbo);
    void DrawLightController(LightSSBO& lightSSBO);
    void DrawCameraControls(Camera& camera);
	void DrawTAASettings();

    void DrawFPS();

	// Skybox
    GLuint GetCurrentSkyboxTexture() const { return m_CurrentSkyboxTexture; }
    bool IsSkyboxEnabled() const { return m_UseSkybox; }
    void ChooseSkybox();

    // Load / Save Scene
    void LoadSave(SSBO& ssbo, LightSSBO& lightSSBO);
    struct FileDialog {
        bool show = false;
        bool isOpenMode = true;
        std::string currentPath;
        std::string selectedFile;
        std::vector<std::filesystem::directory_entry> entries;
    } m_FileDialog;
    void DrawFileDialog(SSBO& ssbo, LightSSBO& lightSSBO);
    void RefreshFileList();

    // TAA
	bool IsTAAEnabled() const { return m_EnableTAA; }
	float GetTAABlendFactor() const { return m_TAABlendFactor; }

    // AO
    AOManager* aoManager;

	void HandleCameraMovement(Camera& camera, float deltaTime);
    
    std::vector<UIObject> m_UIObjects;
    std::vector<UILight> m_UILights;
private:

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

    // TAA
    bool m_EnableTAA = true;
    float m_TAABlendFactor = 0.1f; // 历史帧混合系数

    void SetupStyle();
};