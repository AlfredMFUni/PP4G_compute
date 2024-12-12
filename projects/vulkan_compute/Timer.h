#pragma once


//#include <Windows.h>                                // For LARGE_INTEGER type (Required by QueryPerformanceCounter)
#include <cassert>                                  // For debug asserts
#include <vector>                                   // For collecting Elapsed Times in Timer_Average
#include <string>                                   // For creating/passing filenames


/// <summary>
/// A precision timer.
/// <para>Starts timer on construction.</para>
/// </summary>
class Timer
{
protected:
    //annoying hack solution. due to incompatability with raylib and Windows.h.
    union A_LARGE_INTEGER {
        struct {
            unsigned long LowPart;
            long HighPart;
        } DUMMYSTRUCTNAME;
        struct {
            unsigned long LowPart;
            long HighPart;
        } u;
        long long QuadPart;
    };

    A_LARGE_INTEGER m_startTime, m_endTime, m_clockFrequency, m_elapsedTime;


public:
    enum class TimeUnits
    {
        Seconds = 1,
        Milliseconds = 1000,
        Microseconds = 1000000,
        Nanoseconds = 1000000000
    };

	Timer();

	/// <summary>
	/// Restarts the timer, gets new clock frequency.
    /// <para>Does not effect m_elapsedTime</para>
	/// </summary>
	void m_Restart();

	/// <summary>
	/// Stops the timer, Calculates elapsed time.
    /// <para>Overwritten by subsequent calls.</para>
	/// </summary>
	void m_Stop();

    /// <summary>
    /// Returns the name of the unit in a string. Through the o_string variable.
    /// </summary>
    /// <param name="i_units"></param>
    /// <param name="o_string"></param>
    void UnitsToString(TimeUnits i_units, std::string& o_string);

	/// <summary>
	/// Provides the time between the most recent calls of m_Restart and m_Stop.
    /// <para>m_Stop must have been called at least once.</para>
    /// </summary>
	/// <typeparam name="Type">Type to return.</typeparam>
	/// <param name="i_Units">Desired time resolution.</param>
	/// <returns>Elapsed time as Templated type, in desired time resolution.</returns>
	template<class Type>
    Type m_GetElapsedTime(TimeUnits i_Units = TimeUnits::Nanoseconds) const;

};


template<class Type>
Type Timer::m_GetElapsedTime(TimeUnits i_Units) const
{
    assert((bool)m_elapsedTime.QuadPart);
    return Type((m_elapsedTime.QuadPart * (double)i_Units) / m_clockFrequency.QuadPart);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///													~~New Class~~												///
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// <summary>
/// A precision timer specialized for optimisation tasks.
/// <para>Starts timer on construction.</para>
/// Publically inherits from Timer.
/// </summary>
class Timer_Average : public Timer
{
protected:
    std::vector<A_LARGE_INTEGER> m_elapsedTimes;
public:

	Timer_Average(int i_numIterations);


    /// <summary>
    /// Empties the vector, restarts the timer.
    /// </summary>
    void m_Reset();

    /// <summary>
    /// Resizes vector to i_numIterations.
    /// </summary>
    /// <param name="i_numIterations">Number of tests to perform before taking the average.</param>
    void m_Resize(int i_numIterations);

    /// <summary>
    /// Tests vector.size() against vector.capacity()
    /// </summary>
    /// <returns>True if equal</returns>
    bool m_AllTimesCollected() const;

    /// <summary>
    /// Specialisation of Timer::m_Stop(),
	/// <para>appends elapsed time to vector</para>
    /// </summary>
    void m_Stop();

    /// <summary>
    /// Times a function, until elapsed times vector is at capacity.
    /// </summary>
    /// <param name="function">Void, no parameters.</param>
    void m_TimeAFunction(void (*function)());

    /// <summary>
    /// Times a member function of templated type, 
	/// <para>until elapsed times vector is at capacity.</para>
    /// </summary>
    /// <typeparam name="Type">Class method is member function of.</typeparam>
    /// <param name="i_Type">Instance of class to call method on.</param>
    /// <param name="method">Void member function, no parameters.</param>
    template<class Type>
    void m_TimeAFunction(Type& i_Type, void(Type::* method)());

    /// <summary>
    /// Calculates the average time from the vector of times.
    /// </summary>
    /// <typeparam name="Type">Type to return.</typeparam>
    /// <param name="i_Units">Desired time resolution.</param>
    /// <returns>Avg elapsed time as Templated type, in desired time resolution.</returns>
    template<class Type>
    Type m_GetAverageTime(TimeUnits i_Units) const;
    
    /// <summary>
    /// Finds the greatest elapsed time in the vector.
    /// </summary>
    /// <typeparam name="Type">Type to return.</typeparam>
    /// <param name="i_Units">Desired time resolution.</param>
    /// <returns>Max elapsed time as Templated type, in desired time resolution.</returns>
    template<class Type>
    Type m_GetMaxTime(TimeUnits i_Units) const;
    
    
    /// <summary>
    /// Finds the smallest elapsed time in the vector.
    /// </summary>
    /// <typeparam name="Type">Type to return.</typeparam>
    /// <param name="i_Units">Desired time resolution.</param>
    /// <returns>Min elapsed time as Templated type, in desired time resolution.</returns>
    template<class Type>
    Type m_GetMinTime(TimeUnits i_Units) const;
    
    /// <summary>
    /// Finds both the extreme values in the time vector.
    /// </summary>
    /// <typeparam name="Type">Type to return.</typeparam>
    /// <param name="i_Units">Desired time resolution.</param>
    /// <param name="o_min">Min elapsed time as Templated type, in desired time resolution.</param>
    /// <param name="o_max">Max elapsed time as Templated type, in desired time resolution.</param>
    template<class Type>
    void m_GetExtremeTimes(TimeUnits i_Units, Type& o_min, Type& o_max) const;
    
    /// <summary>
    /// Finds time at specified index.
    /// </summary>
    /// <typeparam name="Type">Type to return.</typeparam>
    /// <param name="i_Units">Desired time resolution.</param>
    /// <param name="i_index">Index of time to retrieve.</param>
    /// <returns>Requested time as Templated type, in desired time resolution.</returns>
    template<class Type>
    Type m_GetSpecificTime(TimeUnits i_Units, int i_index) const;
};

template<class Type>
inline void Timer_Average::m_TimeAFunction(Type& i_Type, void(Type::* method)())
{
    while (!m_AllTimesCollected())
    {
        m_Restart();
        i_Type.method();
        m_Stop();
    }
}

template<class Type>
inline Type Timer_Average::m_GetAverageTime(TimeUnits i_Units) const
{
    int size = m_elapsedTimes.size();
    assert(size);
    A_LARGE_INTEGER total = { 0 };
    for (int i = 0; i < size; i++)
    {
        total.QuadPart += m_elapsedTimes.at(i).QuadPart;
    }
    return Type((total.QuadPart * (double)i_Units) / (m_clockFrequency.QuadPart * size));
}

template<class Type>
inline Type Timer_Average::m_GetMaxTime(TimeUnits i_Units) const
{
    int size = m_elapsedTimes.size();
    assert(size);
    A_LARGE_INTEGER max = { 0 };
    for (int i = 0; i < size; i++)
    {
        const A_LARGE_INTEGER& current = m_elapsedTimes.at(i);
        if (max.QuadPart < current.QuadPart)
        {
            max = current;
        }
    }
    return Type((max.QuadPart * (double)i_Units) / m_clockFrequency.QuadPart);
}

template<class Type>
inline Type Timer_Average::m_GetMinTime(TimeUnits i_Units) const
{
    int size = m_elapsedTimes.size();
    assert(size);
    A_LARGE_INTEGER min = m_elapsedTimes.at(0);
    for (int i = 0; i < size; i++)
    {
        const A_LARGE_INTEGER& current = m_elapsedTimes.at(i);
        if (min.QuadPart > current.QuadPart)
        {
            min = current;
        }
    }
    return Type((min.QuadPart * (double)i_Units) / m_clockFrequency.QuadPart);
}

template<class Type>
inline void Timer_Average::m_GetExtremeTimes(TimeUnits i_Units, Type& o_min, Type& o_max) const
{
    int size = m_elapsedTimes.size();
    assert(size);
    A_LARGE_INTEGER max = { 0 };
    A_LARGE_INTEGER min = m_elapsedTimes.at(0);
    for (int i = 0; i < size; i++)
    {
        const A_LARGE_INTEGER& current = m_elapsedTimes.at(i);
        if (min.QuadPart > current.QuadPart)
        {
            min = current;
        }
        if (max.QuadPart < current.QuadPart)
        {
            max = current;
        }
    }
    o_min = Type((min.QuadPart * (double)i_Units) / m_clockFrequency.QuadPart);
    o_max = Type((max.QuadPart * (double)i_Units) / m_clockFrequency.QuadPart);
}

template<class Type>
inline Type Timer_Average::m_GetSpecificTime(TimeUnits i_Units, int i_index) const
{
    int size = m_elapsedTimes.size();
    assert(i_index <= size);
    return Type((m_elapsedTimes.at(i_index).QuadPart * (double)i_Units) / m_clockFrequency.QuadPart);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///													~~New Class~~												///
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// <summary>
/// Specialist Timer_Average, with write-to-file capabilities.
/// Publically inherits from Timer_Average.
/// </summary>
class Timer_File : public Timer_Average
{
    /// <summary>
    /// Appends the current local time and '.csv' to any string passed to it.
    /// </summary>
    /// <param name="o_string">Output parameter for string.</param>
    void m_GetTimeString(std::string& o_string) const;
public:
    using Timer_Average::Timer_Average;
	
	/// <summary>
	/// Writes the contents of the vector to a .csv file.
	/// <para>Title of file is date and time this function is called,</para>
	/// with i_filePrefix, attatched at the beginning.
	/// </summary>
	/// <param name="i_filePrefix">Optional name for file.</param>
    void m_PrintToFile(TimeUnits i_units, const std::string& i_filePrefix = "");
};