#include <SDKUtil/SDKCommon.hpp>
#include <CL/cl.h>
namespace streamsdk
{
SDKCommon::SDKCommon()
{

}

SDKCommon::~SDKCommon()
{
    while(!_timers.empty())
    {
        Timer *temp = _timers.back();
        _timers.pop_back();
        delete temp;
    }
}

/* Returns the path of executable being generated */
std::string
SDKCommon::getPath()
{
#ifdef _WIN32
    char buffer[MAX_PATH];
    if(!GetModuleFileName(NULL, buffer, sizeof(buffer)))
        throw std::string("GetModuleFileName() failed!");
    std::string str(buffer);
    /* '\' == 92 */
    int last = (int)str.find_last_of((char)92);
#else
    char buffer[PATH_MAX + 1];
    if(readlink("/proc/self/exe",buffer, sizeof(buffer) - 1) == -1)
        throw std::string("readlink() failed!");
    std::string str(buffer);
    /* '/' == 47 */
    int last = (int)str.find_last_of((char)47);
#endif
    return str.substr(0, last + 1);
}

/*
 * Prints no more than 256 elements of the given array.
 * Prints full array if length is less than 256.
 * Prints Array name followed by elements.
 */
template<typename T> 
void SDKCommon::printArray(
	std::string header, 
	const T * data, 
	const int width,
	const int height) const
{
	std::cout<<"\n"<<header<<"\n";
	for(int i = 0; i < height; i++)
	{
		for(int j = 0; j < width; j++)
		{
			std::cout<<data[i*width+j]<<" ";
		}
		std::cout<<"\n";
	}
	std::cout<<"\n";
}

template<typename T> 
int SDKCommon::fillRandom(
		 T * arrayPtr, 
	     const int width,
		 const int height,
		 const T rangeMin,
		 const T rangeMax,
         unsigned int seed)
{
	if(!arrayPtr)
	{
		error("Cannot fill array. NULL pointer.");
		return 0;
	}

    if(!seed)
        seed = (unsigned int)time(NULL);

    srand(seed);
    double range = double(rangeMax - rangeMin) + 1.0; 

    /* random initialisation of input */
    for(int i = 0; i < height; i++)
	    for(int j = 0; j < width; j++)
		{
			int index = i*width + j;
			arrayPtr[index] = rangeMin + T(range*rand()/(RAND_MAX + 1.0)); 
		}

	return 1;
}

template<typename T> 
int SDKCommon::fillPos(
		 T * arrayPtr, 
	     const int width,
		 const int height)
{
	if(!arrayPtr)
	{
		error("Cannot fill array. NULL pointer.");
		return 0;
	}

    /* initialisation of input with positions*/
    for(T i = 0; i < height; i++)
	    for(T j = 0; j < width; j++)
		{
			T index = i*width + j;
			arrayPtr[index] = index;
		}

	return 1;
}

template<typename T> 
int SDKCommon::fillConstant(
		 T * arrayPtr, 
	     const int width,
		 const int height,
		 const T val)
{
	if(!arrayPtr)
	{
		error("Cannot fill array. NULL pointer.");
		return 0;
	}

    /* initialisation of input with constant value*/
    for(int i = 0; i < height; i++)
	    for(int j = 0; j < width; j++)
		{
			int index = i*width + j;
			arrayPtr[index] = val;
		}

	return 1;
}

template<typename T>
T SDKCommon::roundToPowerOf2(T val)
{
	int bytes = sizeof(T);

	val--;
	for(int i = 0; i < bytes; i++)
		val |= val >> (1<<i);  
	val++;

	return val;
}

template<typename T>
int SDKCommon::isPowerOf2(T val)
{
	long long _val = val;
	if((_val & (-_val))-_val == 0 && _val != 0)
		return 1;
	else
		return 0;
}
const char* 
SDKCommon::getOpenCLErrorCodeStr(std::string input) const
{
    return "unknown error code"; 
}

template<typename T>
const char* 
SDKCommon::getOpenCLErrorCodeStr(T input) const
{
    int errorCode = (int)input;
    switch(errorCode)
    {
        case CL_DEVICE_NOT_FOUND:
            return "CL_DEVICE_NOT_FOUND";
        case CL_DEVICE_NOT_AVAILABLE:
            return "CL_DEVICE_NOT_AVAILABLE";               
        case CL_COMPILER_NOT_AVAILABLE:
            return "CL_COMPILER_NOT_AVAILABLE";           
        case CL_MEM_OBJECT_ALLOCATION_FAILURE:
            return "CL_MEM_OBJECT_ALLOCATION_FAILURE";      
        case CL_OUT_OF_RESOURCES:
            return "CL_OUT_OF_RESOURCES";                    
        case CL_OUT_OF_HOST_MEMORY:
            return "CL_OUT_OF_HOST_MEMORY";                 
        case CL_PROFILING_INFO_NOT_AVAILABLE:
            return "CL_PROFILING_INFO_NOT_AVAILABLE";        
        case CL_MEM_COPY_OVERLAP:
            return "CL_MEM_COPY_OVERLAP";                    
        case CL_IMAGE_FORMAT_MISMATCH:
            return "CL_IMAGE_FORMAT_MISMATCH";               
        case CL_IMAGE_FORMAT_NOT_SUPPORTED:
            return "CL_IMAGE_FORMAT_NOT_SUPPORTED";         
        case CL_BUILD_PROGRAM_FAILURE:
            return "CL_BUILD_PROGRAM_FAILURE";              
        case CL_MAP_FAILURE:
            return "CL_MAP_FAILURE";                         
        case CL_INVALID_VALUE:
            return "CL_INVALID_VALUE";                      
        case CL_INVALID_DEVICE_TYPE:
            return "CL_INVALID_DEVICE_TYPE";               
        case CL_INVALID_PLATFORM:
            return "CL_INVALID_PLATFORM";                   
        case CL_INVALID_DEVICE:
            return "CL_INVALID_DEVICE";                    
        case CL_INVALID_CONTEXT:
            return "CL_INVALID_CONTEXT";                    
        case CL_INVALID_QUEUE_PROPERTIES:
            return "CL_INVALID_QUEUE_PROPERTIES";           
        case CL_INVALID_COMMAND_QUEUE:
            return "CL_INVALID_COMMAND_QUEUE";              
        case CL_INVALID_HOST_PTR:
            return "CL_INVALID_HOST_PTR";                   
        case CL_INVALID_MEM_OBJECT:
            return "CL_INVALID_MEM_OBJECT";                  
        case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:
            return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";    
        case CL_INVALID_IMAGE_SIZE:
             return "CL_INVALID_IMAGE_SIZE";                 
        case CL_INVALID_SAMPLER:
            return "CL_INVALID_SAMPLER";                    
        case CL_INVALID_BINARY:
            return "CL_INVALID_BINARY";                     
        case CL_INVALID_BUILD_OPTIONS:
            return "CL_INVALID_BUILD_OPTIONS";              
        case CL_INVALID_PROGRAM:
            return "CL_INVALID_PROGRAM";                    
        case CL_INVALID_PROGRAM_EXECUTABLE:
            return "CL_INVALID_PROGRAM_EXECUTABLE";          
        case CL_INVALID_KERNEL_NAME:
            return "CL_INVALID_KERNEL_NAME";                
        case CL_INVALID_KERNEL_DEFINITION:
            return "CL_INVALID_KERNEL_DEFINITION";          
        case CL_INVALID_KERNEL:
            return "CL_INVALID_KERNEL";                     
        case CL_INVALID_ARG_INDEX:
            return "CL_INVALID_ARG_INDEX";                   
        case CL_INVALID_ARG_VALUE:
            return "CL_INVALID_ARG_VALUE";                   
        case CL_INVALID_ARG_SIZE:
            return "CL_INVALID_ARG_SIZE";                    
        case CL_INVALID_KERNEL_ARGS:
            return "CL_INVALID_KERNEL_ARGS";                
        case CL_INVALID_WORK_DIMENSION:
            return "CL_INVALID_WORK_DIMENSION";              
        case CL_INVALID_WORK_GROUP_SIZE:
            return "CL_INVALID_WORK_GROUP_SIZE";             
        case CL_INVALID_WORK_ITEM_SIZE:
            return "CL_INVALID_WORK_ITEM_SIZE";             
        case CL_INVALID_GLOBAL_OFFSET:
            return "CL_INVALID_GLOBAL_OFFSET";              
        case CL_INVALID_EVENT_WAIT_LIST:
            return "CL_INVALID_EVENT_WAIT_LIST";             
        case CL_INVALID_EVENT:
            return "CL_INVALID_EVENT";                      
        case CL_INVALID_OPERATION:
            return "CL_INVALID_OPERATION";                 
        case CL_INVALID_GL_OBJECT:
            return "CL_INVALID_GL_OBJECT";                  
        case CL_INVALID_BUFFER_SIZE:
            return "CL_INVALID_BUFFER_SIZE";                 
        case CL_INVALID_MIP_LEVEL:
            return "CL_INVALID_MIP_LEVEL";                   
        case CL_INVALID_GLOBAL_WORK_SIZE:
            return "CL_INVALID_GLOBAL_WORK_SIZE";            
        default:
            return "unknown error code";
    }

    return "unknown error code";
}
template<typename T>
int SDKCommon::checkVal(
	T input, 
	T reference, 
	std::string message,
    bool isAPIerror) const
{
	if(input==reference)
	{
		return 1;
	}
	else
	{
		if(isAPIerror)
        {
            std::cout<<"Error: "<< message << " Error code : ";
            std::cout << getOpenCLErrorCodeStr(input) << std::endl;
        }
        else
            error(message);   
		return 0;
	}
}

template<typename T>
std::string SDKCommon::toString(T t, std::ios_base &(*r)(std::ios_base&))
{
  std::ostringstream output;
  output << r << t;
  return output.str();
}

bool
SDKCommon::compare(const float *refData, const float *data, 
                        const int length, const float epsilon)
{
    float error = 0.0f;
    float ref = 0.0f;

    for(int i = 1; i < length; ++i) 
    {
        float diff = refData[i] - data[i];
        error += diff * diff;
        ref += refData[i] * refData[i];
    }

    float normRef =::sqrtf((float) ref);
    if (::fabs((float) ref) < 1e-7f) {
        return false;
    }
    float normError = ::sqrtf((float) error);
    error = normError / normRef;

    return error < epsilon;
}

size_t
SDKCommon::getLocalThreads(const size_t globalThreads,
                           const size_t maxWorkItemSize)
{
    if(maxWorkItemSize < globalThreads)
    {
        if(globalThreads%maxWorkItemSize == 0)
            return maxWorkItemSize;
        else
        {
            for(size_t i=maxWorkItemSize-1; i > 0; --i)
            {
                if(globalThreads%i == 0)
                    return i;
            }
        }
    }
    else
    {
        return globalThreads;
    }

    return 1;
}

int SDKCommon::createTimer()
{
    Timer* newTimer = new Timer;
    newTimer->_start = 0;
    newTimer->_clocks = 0;

#ifdef _WIN32
	QueryPerformanceFrequency((LARGE_INTEGER*)&newTimer->_freq);
#else
    newTimer->_freq = 1.0E3;
#endif
    
    /* Push back the address of new Timer instance created */
    _timers.push_back(newTimer);

	/*if(_numTimers == 1)
	{
		_timers = newTimer; 
	}
	else
	{
		Timer *save = _timers;

		_timers = new Timer[_numTimers];
		memcpy(_timers,save,sizeof(Timer)*(_numTimers-1));
		_timers[_numTimers-1] = *newTimer;
		delete newTimer;
        newTimer = 0;

        if(_numTimers <= 2 )
        {
		    delete save; 
        }
        else
        {
            delete[] save;
        }
        save = 0;
	}*/

    return (int)(_timers.size() - 1);
}

int SDKCommon::resetTimer(int handle)
{
    if(handle >= (int)_timers.size())
	{
		error("Cannot reset timer. Invalid handle.");
		return 0;
	}

	(_timers[handle]->_start) = 0;
	(_timers[handle]->_clocks) = 0;
	return 1;
}

int SDKCommon::startTimer(int handle)
{
    if(handle >= (int)_timers.size())
	{
		error("Cannot reset timer. Invalid handle.");
		return 0;
	}

#ifdef _WIN32
    QueryPerformanceCounter((LARGE_INTEGER*)&(_timers[handle]->_start));	
#else
    struct timeval s;
    gettimeofday(&s, 0);
    _timers[handle]->_start = (long long)s.tv_sec * 1.0E3 + (long long)s.tv_usec / 1.0E3;
#endif

	return 1;
}

int SDKCommon::stopTimer(int handle)
{
    long long n=0;

    if(handle >= (int)_timers.size())
	{
		error("Cannot reset timer. Invalid handle.");
		return 0;
	}

#ifdef _WIN32
    QueryPerformanceCounter((LARGE_INTEGER*)&(n));	
#else
    struct timeval s;
    gettimeofday(&s, 0);
    n = (long long)s.tv_sec * 1.0E3+ (long long)s.tv_usec / 1.0E3;
#endif

    n -= _timers[handle]->_start;
    _timers[handle]->_start = 0;
    _timers[handle]->_clocks += n;

	return 1;
}

double SDKCommon::readTimer(int handle)
{
    if(handle >= (int)_timers.size())
	{
		error("Cannot read timer. Invalid handle.");
		return 0;
	}

	double reading = double(_timers[handle]->_clocks);
	reading = double(reading / _timers[handle]->_freq);

	return reading;
}

void SDKCommon::printTable(Table *t)
{
	if(t == NULL)
	{
		error("Cannot print table, NULL pointer.");
		return;
	}

	int count = 0;
	// Skip delimiters at beginning.
	std::string::size_type curIndex = t->_dataItems.find_first_not_of(t->_delim, 0);
    // Find first "non-delimiter".
    std::string::size_type nextIndex = 
		t->_dataItems.find_first_of(t->_delim, curIndex);

	while (std::string::npos != nextIndex || std::string::npos != curIndex)
	{
		// Found a token, add it to the vector.
		// tokens.push_back(str.substr(curIndex, nextIndex - curIndex));
		std::cout<<std::setw(t->_columnWidth)<<std::left
				 <<t->_dataItems.substr(curIndex, nextIndex - curIndex);				 
		// Skip delimiters.  Note the "not_of"
		curIndex = t->_dataItems.find_first_not_of(t->_delim, nextIndex);
		// Find next "non-delimiter"
		nextIndex = t->_dataItems.find_first_of(t->_delim, curIndex);
		
		count++;

		if(count%t->_numColumns==0)
			std::cout<<"\n";
	}	
}

bool 
SDKCommon::fileToString(std::string &fileName, std::string &str)
{
    size_t      size;
    char*       buf;

    // Open file stream
	std::fstream f(fileName.c_str(), (std::fstream::in | std::fstream::binary));

    // Check if we have opened file stream
    if (f.is_open()) 
	{
        size_t  sizeFile;

        // Find the stream size
        f.seekg(0, std::fstream::end);
        size = sizeFile = f.tellg();
        f.seekg(0, std::fstream::beg);

        buf = new char[size + 1];
        if (!buf) {
            f.close();
            return  false;
        }

        // Read file
        f.read(buf, sizeFile);
        f.close();
        str[size] = '\0';

        str = buf;

        return true;
    }
	else
	{
		error("Converting file to string. Cannot open file.");
		str = "";	
		return false;
	}
}

void 
SDKCommon::error(const char* errorMsg) const
{
    std::cout<<"Error: "<<errorMsg<<"\n";
}

void 
SDKCommon::error(std::string errorMsg) const
{
	std::cout<<"Error: "<<errorMsg<<"\n";
}

/////////////////////////////////////////////////////////////////
// Template Instantiations 
/////////////////////////////////////////////////////////////////
template 
void SDKCommon::printArray<short>(const std::string, 
		const short*, int, int)const;
template 
void SDKCommon::printArray<unsigned char>(const std::string, 
		const unsigned char *, int, int)const;
template 
void SDKCommon::printArray<unsigned int>(const std::string, 
		const unsigned int *, int, int)const;
template 
void SDKCommon::printArray<int>(const std::string, 
		const int *, int, int)const;
template 
void SDKCommon::printArray<long>(const std::string, 
		const long*, int, int)const;
template 
void SDKCommon::printArray<float>(const std::string, 
		const float*, int, int)const;
template 
void SDKCommon::printArray<double>(const std::string, 
		const double*, int, int)const;

template 
int SDKCommon::fillRandom<unsigned char>(unsigned char* arrayPtr, 
		const int width, const int height, 
		unsigned char rangeMin, unsigned char rangeMax, unsigned int seed);	
template 
int SDKCommon::fillRandom<unsigned int>(unsigned int* arrayPtr, 
		const int width, const int height, 
		unsigned int rangeMin, unsigned int rangeMax, unsigned int seed);	
template 
int SDKCommon::fillRandom<int>(int* arrayPtr, 
		const int width, const int height, 
		int rangeMin, int rangeMax, unsigned int seed);	
template 
int SDKCommon::fillRandom<long>(long* arrayPtr, 
		const int width, const int height, 
		long rangeMin, long rangeMax, unsigned int seed);	
template 
int SDKCommon::fillRandom<float>(float* arrayPtr, 
		const int width, const int height, 
		float rangeMin, float rangeMax, unsigned int seed);	
template 
int SDKCommon::fillRandom<double>(double* arrayPtr, 
		const int width, const int height, 
		double rangeMin, double rangeMax, unsigned int seed);	

template 
short SDKCommon::roundToPowerOf2<short>(short val);
template 
unsigned int SDKCommon::roundToPowerOf2<unsigned int>(unsigned int val);
template 
int SDKCommon::roundToPowerOf2<int>(int val);
template 
long SDKCommon::roundToPowerOf2<long>(long val);

template
int SDKCommon::isPowerOf2<short>(short val);
template
int SDKCommon::isPowerOf2<unsigned int>(unsigned int val);
template
int SDKCommon::isPowerOf2<int>(int val);
template
int SDKCommon::isPowerOf2<long>(long val);

template<> 
int SDKCommon::fillPos<short>(short * arrayPtr, const int width, const int height);
template<> 
int SDKCommon::fillPos<unsigned int>(unsigned int * arrayPtr, const int width, const int height);
template<> 
int SDKCommon::fillPos<int>(int * arrayPtr, const int width, const int height);
template<> 
int SDKCommon::fillPos<long>(long * arrayPtr, const int width, const int height);

template<> 
int SDKCommon::fillConstant<short>(short * arrayPtr, 
		const int width, const int height, 
		const short val);
template<> 
int SDKCommon::fillConstant(unsigned int * arrayPtr, 
		const int width, const int height, 
		const unsigned int val);
template<> 
int SDKCommon::fillConstant(int * arrayPtr, 
		const int width, const int height, 
		const int val);
template<> 
int SDKCommon::fillConstant(long * arrayPtr, 
		const int width, const int height, 
		const long val);
template<> 
int SDKCommon::fillConstant(long * arrayPtr, 
		const int width, const int height, 
		const long val);
template<> 
int SDKCommon::fillConstant(long * arrayPtr, 
		const int width, const int height, 
		const long val);

template
const char* SDKCommon::getOpenCLErrorCodeStr(int input) const;

template
int SDKCommon::checkVal<char>(char input, char reference, std::string message, bool isAPIerror) const;
template
int SDKCommon::checkVal<std::string>(std::string input, std::string reference, std::string message, bool isAPIerror) const;
template
int SDKCommon::checkVal<short>(short input, short reference, std::string message, bool isAPIerror) const;
template
int SDKCommon::checkVal<unsigned int>(unsigned int  input, unsigned int  reference, std::string message, bool isAPIerror) const;
template
int SDKCommon::checkVal<int>(int input, int reference, std::string message, bool isAPIerror) const;
template
int SDKCommon::checkVal<long>(long input, long reference, std::string message, bool isAPIerror) const;


template
std::string SDKCommon::toString<char>(char t, std::ios_base &(*r)(std::ios_base&));
template
std::string SDKCommon::toString<short>(short t, std::ios_base &(*r)(std::ios_base&));
template
std::string SDKCommon::toString<unsigned int>(unsigned int t, std::ios_base &(*r)(std::ios_base&));
template
std::string SDKCommon::toString<int>(int t, std::ios_base &(*r)(std::ios_base&));
template
std::string SDKCommon::toString<long>(long t, std::ios_base &(*r)(std::ios_base&));
template
std::string SDKCommon::toString<float>(float t, std::ios_base &(*r)(std::ios_base&));
template
std::string SDKCommon::toString<double>(double t, std::ios_base &(*r)(std::ios_base&));
}

