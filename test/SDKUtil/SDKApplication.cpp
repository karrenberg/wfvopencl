#include <SDKUtil/SDKApplication.hpp>

int 
SDKSample::initialize()
{
	sampleCommon = new streamsdk::SDKCommon();
	
	streamsdk::Option *optionList = new streamsdk::Option[CLUTIL_DEFAULTOPTIONS];
	if(!optionList)
	{
		std::cout<<"error. Failed to allocate memory (optionList)\n";
		return 0;
	}
	optionList[0]._sVersion = "";
	optionList[0]._lVersion = "device";
	optionList[0]._description = "Execute the openCL kernel on a device [cpu|gpu]";
	optionList[0]._type = streamsdk::CA_ARG_STRING;
	optionList[0]._value = &deviceType;

	optionList[1]._sVersion = "q";
	optionList[1]._lVersion = "quiet";
	optionList[1]._description = "Quiet mode. Suppress all text output.";
	optionList[1]._type = streamsdk::CA_NO_ARGUMENT;
	optionList[1]._value = &quiet;

	optionList[2]._sVersion = "e";
	optionList[2]._lVersion = "verify";
	optionList[2]._description = "Verify results against reference implementation.";
	optionList[2]._type = streamsdk::CA_NO_ARGUMENT;
	optionList[2]._value = &verify;

	optionList[3]._sVersion = "v";
	optionList[3]._lVersion = "verbose";
	optionList[3]._description = "Verbose output.";
	optionList[3]._type = streamsdk::CA_NO_ARGUMENT;
	optionList[3]._value = &verbose;

	optionList[4]._sVersion = "t";
	optionList[4]._lVersion = "timing";
	optionList[4]._description = "Print timing.";
	optionList[4]._type = streamsdk::CA_NO_ARGUMENT;
	optionList[4]._value = &timing;


	sampleArgs = new streamsdk::SDKCommandArgs(CLUTIL_DEFAULTOPTIONS, optionList);
	if(!sampleArgs)
	{
		std::cout<<"Failed to allocate memory. (sampleArgs)\n";
		return 0;
	}
				
	return 1;
}

void SDKSample::printStats(std::string *statsStr, std::string * stats, int n)
{
	if(timing)
	{
		streamsdk::Table sampleStats;

		sampleStats._numColumns = n;
		sampleStats._numRows = 1;
		sampleStats._columnWidth = 25;
		sampleStats._delim = '$';
        
        sampleStats._dataItems = "";
        for(int i=0; i < n; ++i)
        {
            sampleStats._dataItems.append( statsStr[i] + "$");
        }
        sampleStats._dataItems.append("$");

        for(int i=0; i < n; ++i)
        {
            sampleStats._dataItems.append( stats[i] + "$");
        }

		sampleCommon->printTable(&sampleStats);
	}
}

int SDKSample::parseCommandLine(int argc, char**argv)
{
	if(sampleArgs==NULL)
	{
		std::cout<<"error. Command line parser not initialized.\n";
		return 0;
	}
	else
	{
		if(!sampleArgs->parse(argv,argc))
		{
			usage();
			return 0;
		}

		if(sampleArgs->isArgSet("h",true))
		{
			usage();
			return 1;
		}
	}

    /* check about the validity of the device type */
    if(!((deviceType.compare("cpu") == 0 ) || (deviceType.compare("gpu") ==0)))
    {
        std::cout << "error. Invalid device options. only \"cpu\" or \"gpu\" supported\n";
        usage();
        return 0;
    }
	
	return 1;
}

void SDKSample::usage()
{
	if(sampleArgs==NULL)
		std::cout<<"error. Command line parser not initialized.\n";
	else
	{
		std::cout<<"Usage\n";
		std::cout<<sampleArgs->help();
	}
}

SDKSample::SDKSample(std::string sampleName)
{
	name = sampleName;
	sampleCommon = NULL;
	sampleArgs = NULL;
	quiet = 0;
	verify = 0;
	verbose = 0;
	timing = 0;
    deviceType = "gpu";
}

SDKSample::SDKSample(const char* sampleName)
{
	name = sampleName;
	sampleCommon = NULL;
	sampleArgs = NULL;
	quiet = 0;
	verify = 0;
	verbose = 0;
	timing = 0;
    deviceType = "gpu";
}

SDKSample::~SDKSample()
{
    delete sampleCommon;
    delete sampleArgs;
}
