//
// Copyright (c) 2008 Advanced Micro Devices, Inc. All rights reserved.
//

#ifndef SDKAPPLICATION_H_
#define SDKAPPLICATION_H_
#include <SDKUtil/SDKCommon.hpp>
#include <SDKUtil/SDKCommandArgs.hpp>

#define CLUTIL_DEFAULTOPTIONS 5 

class SDKApplication
{
protected:
	virtual int initialize()=0;
	virtual int run()=0;
	virtual int cleanup()=0;
};

class SDKSample : public SDKApplication
{
protected:
	streamsdk::SDKCommandArgs *sampleArgs;
	streamsdk::SDKCommon *sampleCommon;
	std::string name; 
	double totalTime;
	int quiet;
	int verify;
	int verbose;
	int timing;
    std::string deviceType;
	
protected:
	//virtual void PrintHeader()=0;
	//virtual void PrintInput()=0;
	//virtual void PrintOutput()=0;
	virtual int setup()=0;
	virtual int verifyResults()=0;	
	virtual int initialize();
	virtual void printStats(std::string *stdStr, std::string * stats, int n);
	virtual ~SDKSample();

public:
	SDKSample(std::string sampleName);
	SDKSample(const char* sampleName);
	int parseCommandLine(int argc, char **argv);
	void usage();
};

#endif


