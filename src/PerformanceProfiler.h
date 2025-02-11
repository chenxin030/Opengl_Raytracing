#pragma once
#include <vector>
#include <GL/glew.h>

class PerformanceProfiler {
public:
    enum class Stage {
        RayTracing,
        BloomExtract,
        BloomBlur,
        TAA,
        Count // 保持最后
    };

    struct FrameStats {
        double cpuTime = 0.0;
        double gpuTimes[static_cast<int>(Stage::Count)] = { 0 };
        bool gpuDataValid = false;
    };

    PerformanceProfiler(size_t historySize = 60);
    ~PerformanceProfiler();

    void BeginFrame();
    void BeginGPUSection(Stage stage);
    void EndGPUSection(Stage stage);
    void EndFrame(float cpuTimeMs);

    const FrameStats& GetLatestStats() const;
    const std::vector<float>& GetGPUTimeHistory() const;

    void DrawImGuiPanel() const;

private:
	static constexpr int QUERY_FRAME_COUNT = 2; // 双缓冲
    static constexpr int STAGES_PER_FRAME = static_cast<int>(Stage::Count) * 2;

    GLuint m_queryPool[QUERY_FRAME_COUNT][STAGES_PER_FRAME];
    int m_currentFrameIndex = 0;
    int m_activeQueryIndex = 0;

    std::vector<FrameStats> m_frameHistory;
    std::vector<float> m_gpuTimeHistory;

    void ProcessQueries();
};