#ifndef SDKCOMMON_HPP_
#define SDKCOMMON_HPP_

#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <ctime>
#include <cmath>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <malloc.h>

#include <CL/opencl.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#include <linux/limits.h>
#endif

#if defined(__MINGW32__) && !defined(__MINGW64_VERSION_MAJOR)
#define _aligned_malloc __mingw_aligned_malloc 
#define _aligned_free  __mingw_aligned_free 
#endif // __MINGW32__  and __MINGW64_VERSION_MAJOR


#ifndef _WIN32
#if defined(__INTEL_COMPILER)
#pragma warning(disable : 1125)
#endif
#endif

#define SDK_SUCCESS 0
#define SDK_FAILURE 1
#define SDK_EXPECTED_FAILURE 2

namespace streamsdk
{

const char* getOpenCLErrorCodeStr(std::string input);

template<typename T>
const char* getOpenCLErrorCodeStr(T input);

struct Timer
{
    std::string name;
    long long _freq;
    long long _clocks;
    long long _start;
};

struct Table
{
    int _numColumns;
    int _numRows;
    int _columnWidth;
    std::string _delim;
    std::string _dataItems;
};

class SDKCommon
{
private:
    //Timing 
    //Timer *_timers;
    //int _numTimers;
    std::vector<Timer*> _timers;
    
public: 
    SDKCommon();
    ~SDKCommon();
    std::string getPath();
    void error(const char* errorMsg) const;	
    void error(std::string errorMsg) const;
    void expectedError(const char* errorMsg) const;	
    void expectedError(std::string errorMsg) const;
    bool fileToString(std::string &file, std::string &str);
    bool compare(const float *refData, const float *data, 
                    const int length, const float epsilon = 1e-6f); 
    bool compare(const double *refData, const double *data, 
                    const int length, const double epsilon = 1e-6); 
    int displayDevices(cl_platform_id platform, cl_device_type deviceType);
    int validateDeviceId(int deviceId, int deviceCount);

    template<typename T> 
    void printArray(
             const std::string header,
             const T * data, 
             const int width,
             const int height) const;

    template<typename T> 
    int fillRandom(
             T * arrayPtr, 
             const int width,
             const int height,
             const T rangeMin,
             const T rangeMax,
             unsigned int seed=0);	
    
    template<typename T> 
    int fillPos(
             T * arrayPtr, 
             const int width,
             const int height);
    
    template<typename T> 
    int fillConstant(
             T * arrayPtr, 
             const int width,
             const int height,
             const T val);

    
    template<typename T>
    T roundToPowerOf2(T val);

    template<typename T>
    int isPowerOf2(T val);
    
    /* Set default(isAPIerror) parameter to false 
     * if checkVaul is used to check otherthan OpenCL API error code 
     */
    template<typename T> 
    int checkVal(
        T input, 
        T reference, 
        std::string message, bool isAPIerror = true) const;

    template<typename T>
    std::string toString(T t, std::ios_base & (*r)(std::ios_base&)); 

    size_t
    getLocalThreads(size_t globalThreads, size_t maxWorkitemSize);

    // Timing 
    int createTimer();
    int resetTimer(int handle);
    int startTimer(int handle);
    int stopTimer(int handle);
    double readTimer(int handle);

    void printTable(Table* t);
}; 

} // namespace amd

#endif
