#include "FFMpegDecEnc.h"

#include <iostream>

using namespace std;

const int FFMpegDecEnc::DecBufSize = 1<<20;
const int FFMpegDecEnc::EncBufSize = 1<<20;

////////////////////////////////////////////////////////////////////////////////


FFMpegDecEnc::FFMpegDecEnc():
	encCodec(NULL),
	encCodecContext(NULL),
	encFrame(NULL),
	encBuf(NULL),
	encFormat(NULL),
	encFormatContext(NULL),
	encVideoStream(NULL),
	encImgConvertCtx(NULL),
	decCodec(NULL),
	decCodecContext(NULL),
	decFrame(NULL),
	decBuf(NULL),
	decFormat(NULL),
	decFormatContext(NULL),
	decVideoStream(NULL),
	decImgConvertCtx(NULL)
{
	/* initialize libavcodec, and register all codecs and formats */
	av_register_all();
	avdevice_register_all();
	encInitialized = 0;
	decInitialized = 0;
	decFrameRate = 25;
	encFrameRate = 25;
}


////////////////////////////////////////////////////////////////////////////////
	

int FFMpegDecEnc::EncodingInit(const char *fileName, const char* codecName, const int w, const int h, const float frameRate , int bitrate, PixelFormat pixFormat)
{
	const int frameRateNumerator = 2;
	const int frameRateDenominator = frameRate * 2;
	encFrameRate = frameRate;
	/* allocate the output media context */
	avformat_alloc_output_context2(&encFormatContext, NULL, NULL, fileName);

	if (!encFormatContext)
	{
		cerr<<"Error: [FFMpegDecEnc] Could not deduce output container format; using avi.\n";
		avformat_alloc_output_context2(&encFormatContext, NULL, "avi", fileName);
	}
	
	if (!encFormatContext)
	{
		cerr<<"Error: [FFMpegDecEnc] Could not create a container for any output container format\n";
		return 0;
	}

	encFormat = encFormatContext->oformat;
	encCodec = avcodec_find_encoder_by_name(codecName);
	if (!encCodec)
	{
		cerr<<"Error: [FFMpegDecEnc] Codec not found\n";
		return 0;
	}
	encFormat->video_codec = encCodec->id;

  encVideoStream = avformat_new_stream(encFormatContext, encCodec);
	if (!encVideoStream)
	{
		cerr<<"Error: [FFMpegDecEnc] Could not alloc stream\n";
		return 0;
	}

	encCodecContext = encVideoStream->codec;
	encCodecContext->codec_id = encCodec->id;
	encCodecContext->codec_type = AVMEDIA_TYPE_VIDEO;

	/* put sample parameters */
	encCodecContext->rc_max_rate = bitrate;

	encCodecContext->rc_buffer_size = 3 * 1024 * 1024;

	/* resolution must be a multiple of two */
	encCodecContext->width = w;
	encCodecContext->height = h;
	/* time base: this is the fundamental unit of time (in seconds) in terms
		 of which frame timestamps are represented. for fixed-fps content,
		 timebase should be 1/framerate and timestamp increments should be
		 identically 1. */
	encCodecContext->time_base.den = frameRateDenominator;
	encCodecContext->time_base.num = frameRateNumerator;
	encCodecContext->gop_size = 10; /* emit one intra frame every twelve frames at most */
	encCodecContext->pix_fmt = pixFormat;
	if (encCodecContext->codec_id == CODEC_ID_MPEG2VIDEO) {
		/* just for testing, we also add B frames */
		//encCodecContext->max_b_frames=1;
		encCodecContext->max_b_frames = 2;
	}
	
	if (encCodecContext->codec_id == CODEC_ID_MPEG1VIDEO){
		/* Needed to avoid using macroblocks in which some coeffs overflow.
			 This does not happen with normal video, it just happens here as
			 the motion of the chroma plane does not match the luma plane. */
		encCodecContext->mb_decision=2;
	}

	// some formats want stream headers to be separate
	if (encFormat->flags & AVFMT_GLOBALHEADER)
		encCodecContext->flags |= CODEC_FLAG_GLOBAL_HEADER;

	if (encCodec->id == CODEC_ID_H264)
		av_opt_set(encCodecContext->priv_data, "preset", "slow", 0);

	/* open it */
	if (avcodec_open2(encCodecContext, encCodec, NULL) < 0)
	{
		cerr<<"Error: [FFMpegDecEnc] Could not open codec\n";
		return 0;
	}
	
	av_dump_format(encFormatContext, 0, fileName, 1);

	/* allocate output buffer */
	/* XXX: API change will be done */
	/* buffers passed into lav* can be allocated any way you prefer,
		 as long as thaey're aligned enough for the architecture, and
		 they're freed appropriately (such as using av_free for buffers
		 allocated with av_malloc) */
	encBuf = (unsigned char*)av_malloc(EncBufSize);


	/* the image can be allocated by any means and av_image_alloc() is
	 * just the most convenient way if av_malloc() is to be used */
	encFrame = avcodec_alloc_frame();
	if (!encFrame)
	{
		cerr<<"Error: [FFMpegDecEnc] Could not allocate frame\n";
		return 0;
	}
	int size = avpicture_get_size(pixFormat, w, h);
	unsigned char *pictureBuf = (unsigned char*)av_malloc(size);
	if (!pictureBuf)
	{
		cerr<<"Error: [FFMpegDecEnc] Could not alloc picture buf\n";
		av_free(encFrame);
		return 0;
	}
	avpicture_fill((AVPicture *)encFrame, pictureBuf, pixFormat, w, h);

	av_image_alloc(encFrame->data, encFrame->linesize, encCodecContext->width, encCodecContext->height, encCodecContext->pix_fmt, 1);


	/* open the output file, if needed */
	if (!(encFormat->flags & AVFMT_NOFILE))
	{
		if (avio_open(&encFormatContext->pb, fileName, AVIO_FLAG_WRITE) < 0)
		{
			cerr<<"Error: [FFMpegDecEnc] Could not open "<<fileName<<endl;
			return 0;
		}
	}

	encImgConvertCtx = sws_getContext(w, h, PIX_FMT_GRAY8, w, h, pixFormat, SWS_BICUBIC, NULL, NULL, NULL);
	if(encImgConvertCtx == NULL)
	{
		cerr<<"Error: [FFMpegDecEnc] Cannot initialize the conversion context!\n";
		return 0;
	}
	
	/* write the stream header, if any */
	avformat_write_header(encFormatContext, NULL);
	
	encCrtFrame = 0;
	decCrtFrame = 0;
	encInitialized = 1;
	return 1;
}


////////////////////////////////////////////////////////////////////////////////


int FFMpegDecEnc::EncodingAddFrame(const Image<unsigned char>& argImg)
{
	if (!encInitialized)
	{
		cerr<<"Error: [FFMpegDecEnc] Encoding not initialized in EncodingAddFrame\n";
		return 0;
	}
	int hasPacket;
	//video_pts = (double)encVideoStream->pts.val * encVideoStream->time_base.num / encVideoStream->time_base.den;
	AVPacket pkt;
	av_init_packet(&pkt);
	pkt.data = encBuf;
	pkt.size = EncBufSize;
	AVFrame *frame = NULL;
	Image<unsigned char> * image = (Image<unsigned char> *)&argImg;
	const unsigned char* imageData = NULL;
	int width;
	if (image != NULL)
	{
		imageData = image->GetDataR();
		width = image->GetWidth();
		frame = encFrame;
		sws_scale(encImgConvertCtx, (const unsigned char* const*) &imageData, &width, 0, encCodecContext->height, frame->data, frame->linesize);
	}

	/* encode the image */
	if (avcodec_encode_video2(encCodecContext, &pkt, frame, &hasPacket) != 0)
	{
		cerr<<"Error: [FFMpegDecEnc] Could not encode video frame\n";
		return 0;
	}
	if (hasPacket == 1)
	{
		pkt.stream_index = encVideoStream->index;

		/* write the compressed frame in the media file */
		if (av_write_frame(encFormatContext, &pkt) != 0)
		{
			cerr<<"Error: [FFMpegDecEnc] Could not write video frame\n";
			return 0;
		}
	}
	else if (image == NULL)
	{
		//encoder is flushed - return 0 to signal end of encoding
		return 0;
	}
	encCrtFrame++;
	
	return 1;
}


////////////////////////////////////////////////////////////////////////////////

void FFMpegDecEnc::EncodingEnd()
{
	if (!encInitialized)
	{
		cerr<<"Error: [FFMpegDecEnc] Encoding not initialized in EncodingEnd\n";
		return;
	}
	/* get the delayed frames */
	if (encCodec->capabilities & CODEC_CAP_DELAY)
	{
		Image<unsigned char> *image = NULL;
		while(EncodingAddFrame(*image))
		{
			cout<<"Writing delayed frames\n";
		}
	}
	/* write the trailer, if any.  the trailer must be written
	 * before you close the encCodecContexts open when you wrote the
	 * header; otherwise write_trailer may try to use memory that
	 * was freed on av_codec_close() */
	av_write_trailer(encFormatContext);

	//free converting context
	sws_freeContext(encImgConvertCtx);	
	
	/* close each codec */
	if (encVideoStream)
	{
    avcodec_close(encVideoStream->codec);
    av_free(encFrame->data[0]);
    av_free(encFrame);
    av_free(encBuf);
	}
	/* free the streams */
	for (unsigned int i = 0; i < encFormatContext->nb_streams; i++)
	{
		av_freep(&encFormatContext->streams[i]->codec);
		av_freep(&encFormatContext->streams[i]);
	}

	if (!(encFormat->flags & AVFMT_NOFILE)) {
			/* close the output file */
			avio_close(encFormatContext->pb);
	}

	/* free the stream */
	av_free(encFormatContext);	
}


int FFMpegDecEnc::DecodingInit(const char *fileName)
{
	char DeviceName[]="/dev/video";
	if (strncmp(fileName, DeviceName, 8) == 0)
	{
		AVDictionary		*formatParams = NULL;
		AVInputFormat *iformat;
		av_dict_set(&formatParams, "video_size", "640x480", 0);
		av_dict_set(&formatParams, "framerate", "10:1", 0);
		
		decFrameRate = 10;
		iformat = av_find_input_format("video4linux2");
		if (avformat_open_input(&decFormatContext, fileName, iformat, &formatParams) != 0)
		{
			cerr<<"Error: [FFMpegDecEnc] Can not open input file\n";
			return 0;
		}
	}
	else
	{
		if (avformat_open_input(&decFormatContext, fileName, NULL, NULL) != 0)
		{
			cerr<<"Error: [FFMpegDecEnc] Can not open input file\n";
			return 0;
		}
	}
	
	if(avformat_find_stream_info(decFormatContext, NULL) < 0)
	{
    cerr<<"Error: [FFMpegDecEnc] Can not find stream info\n";
		return 0;
	}

	av_dump_format(decFormatContext, 0, fileName, 0);
	
	// Find the first video stream
	decVideoStreamIndex=-1;
	for(unsigned int i=0; i<decFormatContext->nb_streams; i++)
	{
		if(decFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			decVideoStreamIndex=i;
			break;
		}
	}
	if(decVideoStreamIndex==-1)
	{
		cerr<<"Error: [FFMpegDecEnc] Didn't find a video stream\n";
		return 0;
	}

	// Get a pointer to the codec context for the video stream
	decCodecContext=decFormatContext->streams[decVideoStreamIndex]->codec;
	
	// Find the decoder for the video stream
	decCodec=avcodec_find_decoder(decCodecContext->codec_id);
	if(decCodec == NULL)
	{
		cerr<<"Error: [FFMpegDecEnc] Codec not found\n";
		return 0;
	}

	// Inform the codec that we can handle truncated bitstreams -- i.e.,
	// bitstreams where frame boundaries can fall in the middle of packets
	if(decCodec->capabilities & CODEC_CAP_TRUNCATED)
		decCodecContext->flags|=CODEC_FLAG_TRUNCATED;

	// Open codec
	if(avcodec_open2(decCodecContext, decCodec, NULL) < 0)
	{
		cerr<<"Error: [FFMpegDecEnc] Could not open codec\n";
		return 0;
	}

	decFrame = avcodec_alloc_frame();

	int w = decCodecContext->width;
	int h = decCodecContext->height;
	decImgConvertCtx = sws_getContext(w, h, decCodecContext->pix_fmt, w, h, PIX_FMT_GRAY8, SWS_BICUBIC, NULL, NULL, NULL);
	if(decImgConvertCtx == NULL)
	{
		cerr<<"Error: [FFMpegDecEnc] Cannot initialize the conversion context!\n";
		return 0;
	}	
	decInitialized = 1;
	return 1;
}

////////////////////////////////////////////////////////////////////////////////

int FFMpegDecEnc::DecodingGetW()
{
	return decCodecContext->width;
}

////////////////////////////////////////////////////////////////////////////////

int FFMpegDecEnc::DecodingGetH()
{
	return decCodecContext->height;
}

////////////////////////////////////////////////////////////////////////////////

void FFMpegDecEnc::DecodingSetW(int w)
{
	cout<<"Error: [FFMpegDecEnc] DecodingSetW has no effect for ffmpeg\n";
}

////////////////////////////////////////////////////////////////////////////////

void FFMpegDecEnc::DecodingSetH(int h)
{
	cout<<"Error: [FFMpegDecEnc] DecodingSetH has no effect for ffmpeg\n";
}

////////////////////////////////////////////////////////////////////////////////


int FFMpegDecEnc::DecodingGetNextFrame(Image<unsigned char>* argImg)
{
	if (!decInitialized)
	{
		cerr<<"Error: [FFMpegDecEnc] Decoding not initialized in DecodingGetNextFrame\n";
		return 0;
	}
	if (argImg == NULL)
		return 0;
	
  AVPacket packet;
	int frameFinished;

	if(av_read_frame(decFormatContext, &packet)>=0)
	{
		// Is this a packet from the video stream?
		if(packet.stream_index==decVideoStreamIndex)
		{
			// Decode video frame
			avcodec_decode_video2(decCodecContext, decFrame, &frameFinished, &packet);
			
			unsigned char* imageData = argImg->GetDataW();
			int width = argImg->GetWidth();
			if (decFrame->data[0] == 0 || decFrame->linesize[0] == 0)
				return DecodingGetNextFrame(argImg);
			//cout<<"linesize="<<decFrame->linesize[0]<<" "<<decFrame->linesize[1]<<" "<<decFrame->linesize[2]<<" "<<decFrame->linesize[3]<<endl;
			sws_scale(decImgConvertCtx, (unsigned char* const *)&decFrame->data, decFrame->linesize, 0, decCodecContext->height,
				(unsigned char* const *)&imageData, &width);
			decCrtFrame++;
			//cout<<"crtFrame="<<decCrtFrame<<endl;
		}
		// Free the packet that was allocated by av_read_frame
		av_free_packet(&packet);
		return 1;
	}
	else
	{
		decInitialized=0;
		return 0;
	}
}

////////////////////////////////////////////////////////////////////////////////


void FFMpegDecEnc::DecodingEnd()
{
	if (!decInitialized)
	{
		cerr<<"Error: [FFMpegDecEnc] Decoding not initialized in DecodingEnd\n";
		return;
	}
	
	//free the conversion context
	sws_freeContext(decImgConvertCtx);
	
	// Free the YUV frame
	av_free(decFrame);

	// Close the codec
	avcodec_close(decCodecContext);

	// Close the video file
	avformat_close_input(&decFormatContext);		
}


////////////////////////////////////////////////////////////////////////////////

void FFMpegDecEnc::DecodingSkipToFrame(int frameNo)
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

void FFMpegDecEnc::DecodingSkipFrames(int count)
{
	
}

