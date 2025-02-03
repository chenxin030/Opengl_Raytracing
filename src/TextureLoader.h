// TextureLoader.h
#pragma 
GLuint LoadSkybox(const std::vector<std::string>& faces);
GLuint ConvertHDRToCubemap(const char* hdrPath, int cubemapSize = 512);