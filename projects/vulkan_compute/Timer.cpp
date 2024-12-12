#include "Timer.h"

/*
#include <cassert>                                  // For debug asserts
#include <vector>                                   // For collecting Elapsed Times in Timer_Average
#include <string>                                   // For creating/passing filenames
*/
#include <Windows.h>                                // For LARGE_INTEGER type (Required by QueryPerformanceCounter)
#include <fstream>                                  // For saving times to a file
#include <sstream>                                  // For converting std::tm time into a string.
#include <iomanip>                                  // For put_time() (std::tm -> sstream)



Timer::Timer()
{
	QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>( & m_clockFrequency));
    QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&m_startTime));
	m_endTime = m_startTime;
    m_elapsedTime.QuadPart = 0;
}

void Timer::m_Restart()
{
    QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&m_clockFrequency));
    QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&m_startTime));
    m_endTime = m_startTime;
}

void Timer::m_Stop()
{
    QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&m_endTime));
    m_elapsedTime.QuadPart = m_endTime.QuadPart - m_startTime.QuadPart;
}

void Timer::UnitsToString(TimeUnits i_units, std::string& o_string)
{
    switch (i_units)
    {
    case TimeUnits::Seconds:
        o_string = "Seconds";
        break;

    case TimeUnits::Milliseconds:
        o_string = "Milliseconds";
        break;

    case TimeUnits::Microseconds:
        o_string = "Microseconds";
        break;

    case TimeUnits::Nanoseconds:
        o_string = "Nanoseconds";
        break;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///													~~New Class~~												///
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Timer_Average::Timer_Average(int i_numIterations) : Timer::Timer()
{
    m_elapsedTimes.reserve(i_numIterations);
    assert(m_elapsedTimes.capacity() == i_numIterations);
}

void Timer_Average::m_Reset()
{
    m_elapsedTimes.clear();
    m_Restart();
}

void Timer_Average::m_Resize(int i_numIterations)
{
    m_elapsedTimes.reserve(i_numIterations);
    assert(m_elapsedTimes.capacity() == i_numIterations);
}

bool Timer_Average::m_AllTimesCollected() const
{
    return m_elapsedTimes.capacity() == m_elapsedTimes.size();
}

void Timer_Average::m_Stop()
{
    QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&m_endTime));
    m_elapsedTime.QuadPart = m_endTime.QuadPart - m_startTime.QuadPart;
    m_elapsedTimes.push_back(m_elapsedTime);
}

void Timer_Average::m_TimeAFunction(void(*function)())
{
    while (!m_AllTimesCollected())
    {
        m_Restart();
        function();
        m_Stop();
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///													~~New Class~~												///
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Timer_File::m_GetTimeString(std::string& o_string) const
{
    std::time_t time = std::time(nullptr);
    std::tm localTime;
    localtime_s(&localTime, &time);
    std::ostringstream oss;
    oss << std::put_time(&localTime, "%d-%m-%Y-%H-%M-%S.csv");
    o_string += oss.str();
}

void Timer_File::m_PrintToFile(TimeUnits i_Units, const std::string& i_filePrefix)
{
    int capacity = m_elapsedTimes.size();
    std::string filename = i_filePrefix;
    m_GetTimeString(filename);
    std::ofstream file(filename, std::ios::trunc | std::ios::out);
    if (!file.good())
    {
        assert(file.good());
        return;
    }
    std::string unitName;
    UnitsToString(i_Units, unitName);
    file << "Frame,Frame Duration in " << unitName << "\n";
    for (int i = 0; i < capacity; i++)
    {
        file << i << "," << m_GetSpecificTime<int>(i_Units, i) << "\n";
    }
    file.close();
}
