#pragma once
#include <legit/ImGuiProfilerRenderer.h>
#include <chrono>
#include <vector>
#include <thread>

namespace legit
{
    class Profiler
    {
    public:
        static Profiler& Instance()
        {
            static Profiler instance;
            return instance;
        }

        void BeginFrame()
        {
            tasks.clear();
            frameStartTime = GetCurrentTime();
        }

        void EndFrame()
        {
            // Frame is complete, tasks are ready to be consumed
        }

        void AddTask(const std::string& name, double startTime, double endTime, uint32_t color)
        {
            ProfilerTask task;
            task.name = name;
            task.startTime = startTime - frameStartTime;
            task.endTime = endTime - frameStartTime;
            task.color = color;
            tasks.push_back(task);
        }

        const std::vector<ProfilerTask>& GetTasks() const { return tasks; }

    private:
        std::vector<ProfilerTask> tasks;
        double frameStartTime = 0.0;

        double GetCurrentTime()
        {
            auto now = std::chrono::high_resolution_clock::now();
            auto duration = now.time_since_epoch();
            return std::chrono::duration<double>(duration).count();
        }
    };

    class ScopeTimer
    {
    public:
        ScopeTimer(const std::string& name, uint32_t color = Colors::peterRiver)
            : taskName(name), taskColor(color)
        {
            //printf("starting %s\n", taskName.c_str());
            startTime = GetCurrentTime();
        }

        ~ScopeTimer()
        {
            double endTime = GetCurrentTime();
            Profiler::Instance().AddTask(taskName, startTime, endTime, taskColor);
            //printf("done %s\n", taskName.c_str());
        }

    private:
        std::string taskName;
        uint32_t taskColor;
        double startTime;

        double GetCurrentTime()
        {
            auto now = std::chrono::high_resolution_clock::now();
            auto duration = now.time_since_epoch();
            return std::chrono::duration<double>(duration).count();
        }
    };
}

// Convenience macros for easy usage
#define PROFILE_SCOPE(name) legit::ScopeTimer _timer(name)
#define PROFILE_SCOPE_COLOR(name, color) legit::ScopeTimer _timer(name, color)