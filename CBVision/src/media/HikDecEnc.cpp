#include "HikDecEnc.h"
#include <time.h>
#include <iostream>

using namespace std;


HikDecEnc* HikDecEnc::instance = NULL;
const unsigned int HikDecEnc::ImageBufferSize = 10;
////////////////////////////////////////////////////////////////////////////////

HikDecEnc::HikDecEnc()
{
	instance = this;
	encInitialized = 0;
	decInitialized = 0;
	imageBufferReadingIndex = 0;
	imageBufferWritingIndex = 0;
	//decImgBuf = NULL;
	timestamp = 0;
	processing = 0;
	pthread_mutex_init(&decodingAndProcessingMutex, NULL);
	pthread_cond_init(&decodingAndProcessingCond, NULL);
	pthread_mutex_init(&decodingAndProcessingMutex1, NULL);
	pthread_cond_init(&decodingAndProcessingCond1, NULL);
	pthread_mutex_init(&imageBufferWritingIndexMutex, NULL);
	pthread_mutex_init(&imageBufferReadingIndexMutex, NULL);
	/*INITINFO initInfo;
	initInfo.uWidth = 0;
	initInfo.uHeight = 0;
	if (PlayM4_Init(initInfo, 0) == -1)
	{
		cerr<<"Error: PlayM4_Init error!\n";
	}*/
	decW = 0;
	decH = 0;
	decFrameRate = 25;
	encFrameRate = 25;	
}


////////////////////////////////////////////////////////////////////////////////


int HikDecEnc::EncodingInit(const char *fileName, const char* codecName, const int w, const int h, const float frameRate, int bitrate, PixelFormat pixFormat)
{
	cerr<<"Error: HikDecEnc::EncodingInit() - Encoding not available!\n";
	return 0;
}


////////////////////////////////////////////////////////////////////////////////


int HikDecEnc::EncodingAddFrame(const Image<unsigned char>& argImg)
{
	cerr<<"Error: HikDecEnc::EncodingAddFrame() - Encoding not available!\n";
	return 0;
}


////////////////////////////////////////////////////////////////////////////////

void HikDecEnc::EncodingEnd()
{
	cerr<<"Error: HikDecEnc::EncodingEnd() - Encoding not available!\n";
}

////////////////////////////////////////////////////////////////////////////////

void HikDecEnc::DecCallBack(int port, char* pBuf, int nSize, FRAME_INFO* pFrameInfo, int nReserved1, int nReserved2)
{

	pthread_mutex_lock (&instance->imageBufferReadingIndexMutex);
	unsigned int readingIndex = instance->imageBufferReadingIndex;
	pthread_mutex_unlock (&instance->imageBufferReadingIndexMutex);
	unsigned int writingIndex = instance->imageBufferWritingIndex;

	
	pthread_mutex_lock (&instance->decodingAndProcessingMutex1);
	if ((writingIndex + 1) % instance->ImageBufferSize == readingIndex)
	{
		//cout<<"DecCallBack cond"<<" ri="<<readingIndex<<" wi="<<writingIndex<<endl;
		pthread_cond_wait(&instance->decodingAndProcessingCond1, &instance->decodingAndProcessingMutex1);
	}
	pthread_mutex_unlock (&instance->decodingAndProcessingMutex1);
	
	
	
	//cout<<"total="<<PlayM4_GetFileFrames(0)<<" played="<<PlayM4_GetPlayedFrames(0)<<" crtFNum="<<PlayM4_GetCurrentFrameNum(0)<<endl;
	if (PlayM4_GetFileTotalFrames(0) > PlayM4_GetPlayedFrames(0) + 2)
	{	
		/*if (instance->decW == 0 || instance->decH == 0)
		{
			instance->decW = pFrameInfo->nWidth;
			instance->decH = pFrameInfo->nHeight;
		}*/

	

		if (instance->decW != 0 && instance->decH != 0)
		{
			//cout<<"Copying "<< pFrameInfo->nWidth<<"x"<<pFrameInfo->nHeight<<" image to a "<<instance->decW<<"x"<<instance->decH<<" image\n";
			instance->decImgConvertCtx = sws_getCachedContext(instance->decImgConvertCtx, pFrameInfo->nWidth, pFrameInfo->nHeight, PIX_FMT_YUV420P, instance->decW, instance->decH, PIX_FMT_GRAY8, SWS_BICUBIC, NULL, NULL, NULL);
			if(instance->decImgConvertCtx == NULL)
			{
				cerr<<"Error: Cannot initialize the conversion context!\n";
				return;
			}

			instance->decFrameRate=PlayM4_GetCurrentFrameRate(0);
			
			if (instance->imageBuffer.size() == 0)
			{
				for (unsigned int i = 0; i < ImageBufferSize; i++)
				{
					instance->imageBuffer.push_back(new Image<unsigned char>(instance->decW, instance->decH));
				}
				instance->timestampBuffer = new unsigned int[ImageBufferSize];
			}



			unsigned char* imageData = instance->imageBuffer[writingIndex]->GetDataW();
			
			int yv12Strides[4];
			yv12Strides[0] = pFrameInfo->nWidth;
			yv12Strides[1] = pFrameInfo->nWidth >> 1;
			yv12Strides[2] = pFrameInfo->nWidth >> 1;
			yv12Strides[3] = 0;
			unsigned char* yv12Slices[4];
			yv12Slices[0] = (unsigned char *)pBuf;
			yv12Slices[1] = yv12Slices[0] + pFrameInfo->nWidth * pFrameInfo->nHeight;
			yv12Slices[2] = yv12Slices[0] + 5 * pFrameInfo->nWidth * pFrameInfo->nHeight / 4;
			yv12Slices[3] = NULL;
			
			int g8Strides[4];
			g8Strides[0] = instance->decW;
			g8Strides[1] = 0;
			g8Strides[2] = 0;
			g8Strides[3] = 0;
			unsigned char* g8Slices[4];
			g8Slices[0] = imageData;
			g8Slices[1] = g8Slices[2] = g8Slices[3] = NULL;
			


			sws_scale(instance->decImgConvertCtx, (unsigned char* const *)yv12Slices, yv12Strides, 0, pFrameInfo->nHeight, (unsigned char* const *)g8Slices, g8Strides);
			
			//memcpy(imageData, pBuf, instance->decW * instance->decH);
			
			unsigned int ts = PlayM4_GetSpecialData(0);
			
			struct tm dateTime;
			dateTime.tm_year = (ts >> 26) + 100;
			dateTime.tm_mon = (ts >> 22) & 15;
			dateTime.tm_mday = (ts >> 17) & 31;
			dateTime.tm_hour = (ts >> 12) & 31;
			dateTime.tm_min = (ts >> 6) & 63;
			dateTime.tm_sec = (ts >> 0) & 63;
			instance->timestampBuffer[writingIndex] = mktime(&dateTime);
			//cout<<"DecCallBack "<<instance->timestampBuffer[writingIndex]<<" ri="<<readingIndex<<" wi="<<writingIndex<<endl;
			pthread_mutex_lock (&instance->imageBufferWritingIndexMutex);
			instance->imageBufferWritingIndex = (instance->imageBufferWritingIndex + 1) % instance->ImageBufferSize;
			pthread_mutex_unlock (&instance->imageBufferWritingIndexMutex);
		}
	}
	else
	{
		if (PlayM4_Stop(0) < 0)
		{
			cerr<<"Error: [HikDecEnc] Could not stop file!\n";
			return;
		}

		//PlayM4_CloseFile(0);
		instance->decInitialized = 0;
		instance->decFileEnd = 1;
	}


	pthread_mutex_lock (&instance->imageBufferReadingIndexMutex);
	pthread_cond_signal(&instance->decodingAndProcessingCond);
	pthread_mutex_unlock (&instance->imageBufferReadingIndexMutex);
	//PlayM4_OneByOne(0);

}


void HikDecEnc::FileEndCallBack(int port, void* pUser)
{
	//PlayM4_CloseFile(0);
	instance->decInitialized = 0;
	instance->decFileEnd = 1;	
	instance->processing = 0;
	if (PlayM4_Stop(0) < 0)
	{
		cerr<<"Error: [HikDecEnc] Could not stop file!\n";		
	}

	pthread_mutex_lock (&instance->decodingAndProcessingMutex);
	pthread_cond_signal(&instance->decodingAndProcessingCond);
	pthread_mutex_unlock (&instance->decodingAndProcessingMutex);	
	pthread_mutex_lock (&instance->decodingAndProcessingMutex1);
	pthread_cond_signal(&instance->decodingAndProcessingCond1);
	pthread_mutex_unlock (&instance->decodingAndProcessingMutex1);	
}

////////////////////////////////////////////////////////////////////////////////

int HikDecEnc::DecodingInit(const char *fileName)
{
	/*for (unsigned int i = 0; i < 0xFFFFFF; i++)
	{
		for (unsigned int j = 0; j < 0xFF; j++)
		{
		}
	}*/
	if (PlayM4_CloseFile(0) < 0)
	{
		cerr<<"Error: [HikDecEnc] Could not close file!\n";
		return 0;
	}
	if (PlayM4_OpenFile(0,(char*)fileName) == -1)
	
	{
		//
		cerr<<"Error: PlayM4_OpenFile error! ("<<PlayM4_GetLastError(0)<<")\n";
		return 0;
	}
	//int fileNameLen = strlen(fileName);
	//PlayM4_SetIndexFile();
	decImgConvertCtx = NULL;
	PlayM4_GetPictureSize(0, &decW, &decH);
	

	decFileEnd = 0;
	
	if (PlayM4_SetDecCallBack(0, &DecCallBack) == -1)
	{
		cerr<<"Error: PlayM4_SetDecCallBack\n";
	}
	
	if (PlayM4_SetFileEndCallback(0, &FileEndCallBack, NULL) == -1)
	{
		cerr<<"Error: PlayM4_SetFileEndCallBack\n";
	}
	
	//PlayM4_OneByOne(0);
	PlayM4_Play(0, 0);
	PlayM4_Fast(0);
	PlayM4_Fast(0);
	PlayM4_Fast(0);
	PlayM4_Fast(0);
	//PlayM4_Pause(0, 1);
	decInitialized = 1;
	
	return 1;
}

////////////////////////////////////////////////////////////////////////////////

int HikDecEnc::DecodingGetW()
{
	return decW;
}


////////////////////////////////////////////////////////////////////////////////


int HikDecEnc::DecodingGetH()
{
	return decH;
}


////////////////////////////////////////////////////////////////////////////////

void HikDecEnc::DecodingSetW( int w)
{
	decW = w;
}


////////////////////////////////////////////////////////////////////////////////

void HikDecEnc::DecodingSetH(int h)
{
	decH = h;
}

////////////////////////////////////////////////////////////////////////////////


int HikDecEnc::DecodingGetNextFrame(Image<unsigned char>* argImg)
{
	//cout<<"DecodingGetNextFrame\n";
	unsigned int readingIndex = instance->imageBufferReadingIndex;
	pthread_mutex_lock (&instance->imageBufferWritingIndexMutex);
	unsigned int writingIndex = instance->imageBufferWritingIndex;
	pthread_mutex_unlock (&instance->imageBufferWritingIndexMutex);
	
	
	if(readingIndex == writingIndex)
	{
		pthread_mutex_lock (&decodingAndProcessingMutex);
		pthread_cond_wait(&decodingAndProcessingCond, &decodingAndProcessingMutex);
		pthread_mutex_unlock(&decodingAndProcessingMutex);
	}
	if (decFileEnd)
	{
		return 0;
	}
	
	if (argImg != NULL)
	{
		argImg->Copy(*imageBuffer[readingIndex]);
		timestamp = timestampBuffer[readingIndex];
	}
	//cout<<"DecodingGetNextFrame "<<timestampBuffer[readingIndex]<<" ri="<<readingIndex<<" wi="<<writingIndex<<endl;
	//cout<<"DecodingGetNextFrame 1\n";
	
	//memcpy(imageData, decImgBuf, argImg->GetArea());

	pthread_mutex_lock (&instance->imageBufferReadingIndexMutex);
	instance->imageBufferReadingIndex = (instance->imageBufferReadingIndex + 1) % instance->ImageBufferSize;
	pthread_mutex_unlock (&instance->imageBufferReadingIndexMutex);

	pthread_mutex_lock (&decodingAndProcessingMutex1);
	pthread_cond_signal(&decodingAndProcessingCond1);
	pthread_mutex_unlock(&decodingAndProcessingMutex1);

	return 1;
}

////////////////////////////////////////////////////////////////////////////////


void HikDecEnc::DecodingEnd()
{
	PlayM4_CloseFile(0);
	//PlayM4_DeInit();
	sws_freeContext(decImgConvertCtx);
	pthread_mutex_destroy(&decodingAndProcessingMutex);
  pthread_cond_destroy(&decodingAndProcessingCond);
	pthread_mutex_destroy(&decodingAndProcessingMutex1);
  pthread_cond_destroy(&decodingAndProcessingCond1);
}


////////////////////////////////////////////////////////////////////////////////

void HikDecEnc::DecodingSkipToFrame(int frameNo)
{
	Image<unsigned char>* tmpImg = new Image<unsigned char>(DecodingGetW(), DecodingGetH());
	while (frameNo > 0)
	{
		DecodingGetNextFrame(tmpImg);
		frameNo--;
	}
	delete tmpImg;
}

////////////////////////////////////////////////////////////////////////////////

void HikDecEnc::DecodingSkipFrames(int count)
{
	
}

