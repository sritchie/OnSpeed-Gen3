
#pragma once

#ifdef NATIVE_BUILD
#include <string>
typedef std::string String;
#endif

// Parameters for file replay task
struct SuParamsReplay
    {
    String      sReplayLogFile;
    };

void LogReplayTask(void *pvParams);
void TestPotTask(void *pvParams);
void RangeSweepTask(void *pvParams);
