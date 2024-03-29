#ifndef DECENC_H_
#define DECENC_H_
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavdevice/avdevice.h"
#include "libavutil/opt.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"
}
#include "../image/Image.h"

class DecEnc
{
protected:
	int encInitialized;
	int decInitialized;
	int encCrtFrame;
	int decCrtFrame;
	float encFrameRate;
	float decFrameRate;
	unsigned int timestamp;
public:
	virtual int EncodingInit(const char *filename, const char* codecName, const int w, const int h, const float frameRate, int bitrate, PixelFormat pixFormat) =  0;
	virtual int EncodingAddFrame(const Image<unsigned char>& image) = 0;
	virtual void EncodingEnd() = 0;
	inline int EncodingInitialized() {return encInitialized;};
	inline int EncodingGetCrtFrame() {return encCrtFrame;};
	inline float EncodingGetFrameRate() {return encFrameRate;};
	
	virtual int DecodingInit(const char *filename) = 0;
	virtual int DecodingGetW() = 0;
	virtual int DecodingGetH() = 0;
	virtual void DecodingSetW(int w) = 0;
	virtual void DecodingSetH(int h) = 0;
	virtual int DecodingGetNextFrame(Image<unsigned char>* image) = 0;
	virtual void DecodingEnd() = 0;
	inline int DecodingInitialized() {return decInitialized;};
	inline int DecodingGetTimestamp() {return timestamp;};
	inline int DecodingGetCrtFrame() {return decCrtFrame;};
	inline float DecodingGetFrameRate() {return decFrameRate;};
	virtual void DecodingSkipToFrame(int frameNo) = 0;
	virtual void DecodingSkipFrames(int count) = 0;

};
#endif