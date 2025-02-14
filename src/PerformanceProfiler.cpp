#include "PerformanceProfiler.h"
#include <imgui.h>
#include <algorithm>
#include <iostream>

PerformanceProfiler::PerformanceProfiler(size_t historySize)
    : m_frameHistory(historySize),
    m_gpuTimeHistory(historySize, 0.0f) {
}

PerformanceProfiler::~PerformanceProfiler() {
    glDeleteQueries(QUERY_FRAME_COUNT * STAGES_PER_FRAME, &m_queryPool[0][0]);
}

void PerformanceProfiler::Init(){
    glGenQueries(QUERY_FRAME_COUNT * STAGES_PER_FRAME, &m_queryPool[0][0]);
}

void PerformanceProfiler::BeginFrame() {
    m_activeQueryIndex = m_currentFrameIndex % QUERY_FRAME_COUNT;
}

void PerformanceProfiler::BeginGPUSection(Stage stage) {
    const int baseIdx = static_cast<int>(stage) * 2;
    glQueryCounter(m_queryPool[m_activeQueryIndex][baseIdx], GL_TIMESTAMP);
}

void PerformanceProfiler::EndGPUSection(Stage stage) {
    const int baseIdx = static_cast<int>(stage) * 2 + 1;
    glQueryCounter(m_queryPool[m_activeQueryIndex][baseIdx], GL_TIMESTAMP);
}

void PerformanceProfiler::EndFrame(float cpuTimeMs) {
    FrameStats stats;
    stats.cpuTime = cpuTimeMs;

    m_currentFrameIndex++;
    // 处理前一帧的查询
    ProcessQueries();

    // 更新历史记录
    if (m_frameHistory.size() >= m_frameHistory.capacity()) {
        m_frameHistory.erase(m_frameHistory.begin());
    }
    m_frameHistory.push_back(stats);

}

void PerformanceProfiler::ProcessQueries() {
    // 查询结果需要等待GPU完成
    glFinish();
    // 从上一帧的查询中获取数据
    const int prevQueryIndex = (m_currentFrameIndex - 1) % QUERY_FRAME_COUNT;
    for (int i = 0; i < STAGES_PER_FRAME; i += 2) {
        GLuint64 startTime, endTime;
        glGetQueryObjectui64v(m_queryPool[prevQueryIndex][i], GL_QUERY_RESULT, &startTime);
        glGetQueryObjectui64v(m_queryPool[prevQueryIndex][i + 1], GL_QUERY_RESULT, &endTime);
        // 计算时间间隔
        const double timeMs = static_cast<double>(endTime - startTime) / 1000000.0;
        const int stageIdx = i / 2;
        m_frameHistory.back().gpuTimes[stageIdx] = timeMs;
        m_gpuTimeHistory.push_back(timeMs);
    }
    m_frameHistory.back().gpuDataValid = true;
}

const PerformanceProfiler::FrameStats&
PerformanceProfiler::GetLatestStats() const {
    return m_frameHistory.back();
}

const std::vector<float>&
PerformanceProfiler::GetGPUTimeHistory() const {
    return m_gpuTimeHistory;
}

void PerformanceProfiler::DrawImGuiPanel() const {
    ImGui::Begin("Performance Profiler");

    if (m_frameHistory.empty()) {
        ImGui::Text("No data available");
        ImGui::End();
        return;
    }

    // 查找最近的有效数据
    const FrameStats* validStats = nullptr;
    for (auto it = m_frameHistory.rbegin(); it != m_frameHistory.rend(); ++it) {
        if (it->gpuDataValid) {
            validStats = &(*it);
            break;
        }
    }

    ImGui::TextColored(ImVec4(1, 0.5, 0, 1), "CPU Frame: %.2f ms", validStats->cpuTime);
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0, 1, 1, 1), "GPU Breakdown:");
    ImGui::Text("RayTracing: %6.2f ms", validStats->gpuTimes[0]);
    ImGui::Text("BloomExtract: %6.2f ms", validStats->gpuTimes[1]);
    ImGui::Text("BloomBlur: %6.2f ms", validStats->gpuTimes[2]);
    ImGui::Text("TAA: %6.2f ms", validStats->gpuTimes[3]);

    // 绘制历史图表
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0, 1, 0, 1), "GPU Time History:");
    ImGui::PlotLines("##GPU Time", &m_gpuTimeHistory[0], m_gpuTimeHistory.size(), 0, nullptr, 0.0f, 50.0f, ImVec2(0, 80));

    ImGui::End();
}