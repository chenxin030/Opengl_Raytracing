#pragma once
#include "Shader.h"
#include "ImGUIManager.h"
#include "SSBO.h"
#include "PerformanceProfiler.h"
#include <GLFW/glfw3.h>

class ForwardShadingPipline {
private:
	GLFWwindow* window = nullptr;
	ImGuiManager imguiManager;
	SSBO ssbo;
	LightSSBO lightSSBO;
	// GPU Time Query
	PerformanceProfiler gProfiler;

	GLuint blueNoiseTex;
	Shader raytracingShader;
	// display
	Shader outputShader;
	GLuint outputTex;
	// bloom
	GLuint bloomFBOs[2], bloomTextures[2];
	Shader extractShader; 
	Shader blurShader; 
	Shader bloomCombineShader; 
	// TAA�� �����ʷ������
	GLuint historyTex[2]; // ˫����
	GLuint taaFBO;
	Shader taaShader;
	// AO
	AOManager* aoManager = nullptr;
	GLuint gPositionTex, gNormalTex; // ���λ�����


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
	void InitBlueNoiseTex();
	void InitShdaer();
	void InitOutputTex();
	void InitBloom();
	void InitTAA();
	void InitAO();

	void Render();
};