#include "FrameProvider.h"
#include "HikDecEnc.h"
#include "dirent.h"
#include <iostream>
#include <algorithm>

bool strCompare(char* str1, char* str2)
{
	if (strcmp(str1, str2) >= 0)
		return true;
	else
		return false;
}

FrameProvider::FrameProvider(CmdLine *cl, DecEnc* d):
	multipleInputFiles(0),
	cmdLine(cl),
	dec(d),
	internalImg(NULL)
{
	//variables for handling input file names

	char fileExtension[5];
	fileExtension[4] = 0;
	char fileNameStem[256];
	fileNameStem[255] = 0;
	
	folderPath[1023] = 0; 
	// check if there is a * in the input file name that signals a multi-file input
	int res = sscanf(cmdLine->cmdLineArgs[CmdLine::ArgInputFile]->strVal,"%[^*]*%s",folderPath, fileExtension);
	if (res > 1)
	{
		multipleInputFiles = 1;
	}
	folderPathLen = strlen(folderPath);
	int fileNameStemLen = folderPathLen;
	//find the last /
	while (folderPath[folderPathLen] != '/' && folderPathLen > 0)
	{
		folderPathLen--;
	}
	//end the folder path at the last /
	fileNameStemLen -= folderPathLen;
	//copy file stem including null termination
	memcpy(fileNameStem, folderPath + folderPathLen + 1, fileNameStemLen);
	//substract 1 for the null termination
	fileNameStemLen--;
	folderPathLen++;
	folderPath[folderPathLen] = 0;

	//make a list of all files to process
	if (multipleInputFiles)
	{
		DIR *dp;
		struct dirent *dirp;
		if((dp  = opendir(folderPath)) == NULL) {
			cerr << "Error: [FrameProvider] Could not open dir '"<<folderPath<<"' (" << errno << ")" << endl;
			return;
		}

		while ((dirp = readdir(dp)) != NULL)
		{
			char* crtFileName = dirp->d_name;
			int crtFileNameLen = strlen(crtFileName);
			if (strncmp(crtFileName, fileNameStem, fileNameStemLen) != 0)
				continue;
			if (strncmp(crtFileName + crtFileNameLen - 4, fileExtension, 4) != 0)
				continue;
			char* file = new char[crtFileNameLen + 1];
			memcpy(file, crtFileName, crtFileNameLen + 1);
			inputFileNames.push_back(file);
		}
		closedir(dp);
		sort(inputFileNames.begin(), inputFileNames.end(), strCompare);
	}
	else
	{
		inputFileNames.push_back(fileNameStem);
	}
	width=0;
	height=0;
	CheckDecoderInitialized();
	width = dec->DecodingGetW();
	height = dec->DecodingGetH();
}

FrameProvider::~FrameProvider()
{
	dec->DecodingEnd();
}

int FrameProvider::CheckDecoderInitialized()
{
	if (!dec->DecodingInitialized())
	{
		if (inputFileNames.size() == 0)
		{
			//cout<<"Error: [FrameProvider] No more files left\n";
			return 0;
		}
		//copy the folder path
		char crtFile[1024];
		memcpy(crtFile, folderPath, folderPathLen);
		//loop through all the input files
		char* fileName = inputFileNames.back();
		cout<<"Info: crt frame provider file is "<<fileName<<endl;
		int fileNameLen = strlen(fileName);
		memcpy(crtFile + folderPathLen, fileName, fileNameLen + 1);
		// init the decoder
		int decInitResult = dec->DecodingInit(crtFile);
		inputFileNames.pop_back();
		
		//delete fileName;
		//fileName = NULL;
		if (!decInitResult)
		{
			cerr<<"Error: [FrameProvider] Could not initialize the decoder for file "<<crtFile<<endl;
			return FrameProvider::CheckDecoderInitialized();
		}
		//once a image size has been chosen all images use it
		if (width != 0 || height != 0)
		{
			dec->DecodingSetW(width);
			dec->DecodingSetH(height);
		}
		if (internalImg == NULL)
		{
			//skiping the first 2 frames to get the proper frame size (hikvision reports the wrong size)
			while (dec->DecodingGetW() == 0 || dec->DecodingGetH() == 0)
			{
				cerr<<"Error: [FrameProvider] Decoder has w or h = 0\n. Trying to get a new frame...";
				dec->DecodingGetNextFrame(NULL);
			}
			internalImg = new Image<unsigned char>(dec->DecodingGetW(), dec->DecodingGetH());
		}
	}
	return 1;
}

int FrameProvider::GetNextFrame(Image<unsigned char> *&retImg)
{
	if (!CheckDecoderInitialized())
	{
		return 0;
	}
	while(!dec->DecodingGetNextFrame(internalImg))
	{
		if (!CheckDecoderInitialized())
		{
			return 0;
		}
	}

	if (retImg == NULL)
	{
		retImg = internalImg;
	}
	else
	{
		retImg->Copy(*internalImg);
	}
	return 1;
}

int FrameProvider::GetWidth()
{
	return width;
}

int FrameProvider::GetHeight()
{
	return height;
}

int FrameProvider::GetTimestamp()
{
	if (cmdLine->cmdLineArgs[CmdLine::ArgTimestampSource]->intVal == 0)
	{
		return dec->DecodingGetCrtFrame();
	}
	else
	{
		return dec->DecodingGetTimestamp();
	}
}