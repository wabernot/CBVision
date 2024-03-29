#ifndef FRAME_PROVIDER_H_
#define FRAME_PROVIDER_H_
#include <stdio.h>
#include <string>
#include "../utils/CmdLine.h"
#include "DecEnc.h"
#include <vector>
#include <sys/types.h>


using namespace std;
class FrameProvider
{
private:
	CmdLine* cmdLine;
	DecEnc* dec;
	int multipleInputFiles;
	char folderPath[1024];
	int folderPathLen;
	vector<char*> inputFileNames;
	vector<char*>::reverse_iterator inputFileNamesRIter;
	Image<unsigned char>* internalImg;
	int width;
	int height;
	//int PrefetchNextFrame();
	int CheckDecoderInitialized();

public:
	FrameProvider(CmdLine* cmdLine, DecEnc* d);
	~FrameProvider();
	int GetNextFrame(Image<unsigned char>* &retImg);
	int GetWidth();
	int GetHeight();
	int GetTimestamp();
};
#endif