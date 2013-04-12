#ifndef HIKDECENC_H_
#define HIKDECENC_H_

#include "../image/Image.h"
#include "../hikvision/LinuxPlayM4.h"
#include "DecEnc.h"
#include <vector>
using namespace std;

class HikDecEnc : public DecEnc
{
private:
	struct SwsContext *decImgConvertCtx;
	
	int decW;
	int decH;
	static void DecCallBack(int port, char* pBuf, int nSize, FRAME_INFO* pFrameInfo, int nReserved1, int nReserved2);
	static void FileEndCallBack(int port, void* pUser);
	int processing;
public:
	HikDecEnc();
	
	int EncodingInit(const char *filename, const char* codecName, const int w, const int h, const float frameRate, int bitrate, PixelFormat pixFormat);
	int EncodingAddFrame(const Image<unsigned char>& image);
	void EncodingEnd();
	
	int DecodingInit(const char *filename);
	int DecodingGetW();
	int DecodingGetH();
	void DecodingSetW(int w);
	void DecodingSetH(int h);
	int DecodingGetNextFrame(Image<unsigned char>* image);
	void DecodingEnd();
	void DecodingSkipToFrame(int frameNo);
	void DecodingSkipFrames(int count);


	int decFileEnd;
	pthread_mutex_t decodingAndProcessingMutex;
	pthread_cond_t decodingAndProcessingCond;
	pthread_mutex_t decodingAndProcessingMutex1;
	pthread_cond_t decodingAndProcessingCond1;
	//unsigned char* decImgBuf;
	vector< Image<unsigned char>* > imageBuffer;
	unsigned int* timestampBuffer;
	const static unsigned int ImageBufferSize;
	unsigned int imageBufferWritingIndex;
	pthread_mutex_t imageBufferWritingIndexMutex;
	unsigned int imageBufferReadingIndex;
	pthread_mutex_t imageBufferReadingIndexMutex;
	
	static HikDecEnc* instance;
};
#endif