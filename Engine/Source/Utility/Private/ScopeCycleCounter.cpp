#include "pch.h"
#include "Utility/Public/ScopeCycleCounter.h"

TMap<FString, FTimeProfile> FScopeCycleCounter::TimeProfileMap;

//Map에 이미 있으면 시간, 콜스택 추가
void FScopeCycleCounter::AddTimeProfile(const TStatId& Key, double InMilliseconds)
{
    auto It = TimeProfileMap.find(Key.Key);
    if (It != TimeProfileMap.end())
    {
        TimeProfileMap[Key.Key] = FTimeProfile{ InMilliseconds, 1 };
    }
    else
    {
        TimeProfileMap[Key.Key].Milliseconds += InMilliseconds;
        TimeProfileMap[Key.Key].CallCount++;
    }
}
const FTimeProfile& FScopeCycleCounter::GetTimeProfile(const FString& Key)
{
    return TimeProfileMap[Key];
}
const TArray<FString> FScopeCycleCounter::GetTimeProfileKeys()
{
    TArray<std::string> Keys;
    Keys.reserve(TimeProfileMap.size());
    for (const auto& Pair : TimeProfileMap)
    {
        Keys.push_back(Pair.first);
    }
    return Keys;
}
const TArray<FTimeProfile> FScopeCycleCounter::GetTimeProfileValues()
{
    TArray<FTimeProfile> Values;
    Values.reserve(TimeProfileMap.size());
    for (const auto& Pair : TimeProfileMap)
    {
        Values.push_back(Pair.second);
    }
    return Values;
}

double FWindowsPlatformTime::GSecondsPerCycle = 0.0;
bool FWindowsPlatformTime::bInitialized = false;
