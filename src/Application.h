#pragma once
#include "Shader.h"
#include "ImGUIManager.h"
#include "SSBO.h"
#include "PerformanceProfiler.h"
#include <GLFW/glfw3.h>

class ForwardShadingPipline {
private:
	GLFWwindow* window = nullptr;
	Shader raytracingShader;
	Shader outputShader;
	GLuint outputTex;
	ImGuiManager imguiManager;
	SSBO ssbo;
	LightSSBO lightSSBO;
	// bloom
	GLuint bloomFBOs[2], bloomTextures[2];
	Shader extractShader; 
	Shader blurShader; 
	Shader bloomCombineShader; 
	// TAA： 添加历史缓冲区
	GLuint historyTex[2]; // 双缓冲
	GLuint taaFBO;
	Shader taaShader;
	// AO
	AOManager* aoManager = nullptr;
	GLuint gPositionTex, gNormalTex; // 几何缓冲区
    // GPU Time Query
	PerformanceProfiler gProfiler;


public:
	ForwardShadingPipline() { Init(); }
	~ForwardShadingPipline() {
		glDeleteTextures(1, &outputTex);
		glDeleteFramebuffers(2, bloomFBOs);
		glDeleteTextures(2, bloomTextures);
		glDeleteFramebuffers(1, &taaFBO);
		glDeleteTextures(2, historyTex);
		glDeleteTextures(1, &gPositionTex);
		glDeleteTextures(1, &gNormalTex);

		glfwTerminate();
	}
	void Init();
	void InitFWEW();
	void InitShdaer();
	void InitOutputTex();
	void InitBloom();
	void InitTAA();
	void InitAO();

	void Run();
};