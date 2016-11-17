////////////////////////////////////////////////////////////////////////////////
//
// *** Measuring the effects of the SSO (Small String Optimization) ***
// 
// The STL strings implement the SSO; ATL's CString doesn't.
// 
// This code creates a vector of strings and sort it, both with
// STL and ATL strings.
// 
// by Giovanni Dicanio
// 
////////////////////////////////////////////////////////////////////////////////

#include <algorithm>        // For std::shuffle
#include <iostream>         // For std::cout
#include <random>           // For std::mt19937
#include <string>           // For std::string, std::wstring
#include <vector>           // For std::vector
#include <Windows.h>        // For high-resolution performance counters
#include <atlstr.h>         // For ATL CString
using namespace std;


//------------------------------------------------------------------------------
// Convenient wrappers for high-resolution performance counter Win32 APIs
//------------------------------------------------------------------------------

// Wraps Win32's QueryPerformanceCounter
long long Counter() 
{
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    return li.QuadPart;
}

// Wraps Win32's QueryPerformanceFrequency
long long Frequency() 
{
    LARGE_INTEGER li;
    QueryPerformanceFrequency(&li);
    return li.QuadPart;
}

// Given start and finish counter values, converts them in a duration expressed in milliseconds.
double MillisecondsFromDeltaCounter(const long long start, const long long finish) 
{
    return (finish - start) * 1000.0 / Frequency();
}


// Store the results of a single test run: push_back time and sorting time.
struct PerfData 
{   
    PerfData()
        : PushBackTimeMs{0.0}
        , SortTimeMs{0.0}
    {}

    PerfData(double push_back_time_ms, double sort_time_ms, const std::string& description)
        : PushBackTimeMs(push_back_time_ms)
        , SortTimeMs(sort_time_ms)
        , Description(description)
    {}

    double PushBackTimeMs;
    double SortTimeMs;
    std::string Description;
};


void PrintTime(const PerfData& perf_data) 
{
    cout << perf_data.Description << ":\n";
    cout << "  push_back : " << perf_data.PushBackTimeMs << " ms\n";
    cout << "  sort      : " << perf_data.SortTimeMs << " ms\n\n";
}


//------------------------------------------------------------------------------
//                          Benchmark Core
//------------------------------------------------------------------------------

// Given a set of raw pointers pointing to NUL-terminated Unicode UTF-16 strings,
// builds a string vector and sorts it.
// This is the "core" of this benchmark.
// This function template is called for both ATL::CStringW and STL's std::wstring.
template <typename StringT>
PerfData MeasurePushBackAndSort(const std::vector<const wchar_t*> shuffled_ptrs, 
                                const std::string& description) 
{
    long long start = 0;
    long long finish = 0;

    vector<StringT> v;
    
    //
    // Measure push_back time
    // 
    start = Counter();
    for (const auto& ptr : shuffled_ptrs)
    {
        v.push_back(ptr);
    }
    finish = Counter();
    const double push_back_time_ms = MillisecondsFromDeltaCounter(start, finish);

    //
    // Measure sort time
    // 
    start = Counter();
    sort(v.begin(), v.end());
    finish = Counter();
    const double sort_time_ms = MillisecondsFromDeltaCounter(start, finish);

    return PerfData(push_back_time_ms, sort_time_ms, description);
}


// Console application entry point
int main() 
{
    cout << "\n*** SSO Performance Benchmark ***\n";
    cout << " by Giovanni Dicanio\n\n";

#ifdef _WIN64
    cout << "(64-bit)\n\n";
#endif

    //
    // Prepare the string data for the benchmark
    // 

    // Build a vector of shuffled small strings
    const auto shuffled = []() -> vector<wstring> 
    {
        // Build 200,000 small strings
        vector<wstring> v;
        for (int i = 0; i < 200*1000; ++i) 
        {
            v.push_back(L"#" + to_wstring(i));           
        }
 
        // Shuffle them
        mt19937 prng(64);
        shuffle(v.begin(), v.end(), prng);

        return v;
    }();

    // Build a vector of *pointers* to the strings previously built.
    // This vector of (observing) pointers is passed to the benchmark's core function.
    const auto shuffled_ptrs = [&]() -> vector<const wchar_t *> 
    {
        vector<const wchar_t *> v;
        v.reserve(shuffled.size());
        
        for (const auto& s : shuffled) 
        {
            v.push_back(s.c_str());
        }

        return v;
    }();


    //
    // Run the push_back and sort benchmark a few times, and print each iteration's results
    // 
    PrintTime( MeasurePushBackAndSort< ATL::CStringW >(shuffled_ptrs, "ATL1") );
    PrintTime( MeasurePushBackAndSort< wstring       >(shuffled_ptrs, "STL1") );
    PrintTime( MeasurePushBackAndSort< ATL::CStringW >(shuffled_ptrs, "ATL2") );
    PrintTime( MeasurePushBackAndSort< wstring       >(shuffled_ptrs, "STL2") );
    PrintTime( MeasurePushBackAndSort< ATL::CStringW >(shuffled_ptrs, "ATL3") );
    PrintTime( MeasurePushBackAndSort< wstring       >(shuffled_ptrs, "STL3") );
}

