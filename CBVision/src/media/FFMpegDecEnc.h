#ifndef FFMPEGDECENC_H_
#define FFMPEGDECENC_H_

#include "../image/Image.h"
#include "DecEnc.h"

class FFMpegDecEnc : public DecEnc
{
	static const int EncBufSize;
	static const int DecBufSize;
	
	AVCodec *encCodec;
  AVCodecContext *encCodecContext;
	AVFrame *encFrame;
	unsigned char *encBuf;
	int lastEncSize;
	AVOutputFormat *encFormat;
  AVFormatContext *encFormatContext;
	AVStream *encVideoStream;
	struct SwsContext *encImgConvertCtx;
	
	AVCodec *decCodec;
	AVCodecContext *decCodecContext;
	AVFrame *decFrame;
	unsigned char *decBuf;
	int crtDecFrame;
	AVInputFormat *decFormat;
	AVFormatContext * decFormatContext;
	AVStream *decVideoStream;
	int decVideoStreamIndex;	
	struct SwsContext *decImgConvertCtx;
		
public:
	FFMpegDecEnc();
	
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
};
#endif