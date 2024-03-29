#include <iostream>
#include <fstream>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <deque>
#include "utils/Utils.h"
#include "utils/CmdLine.h"
#include "media/FFMpegDecEnc.h"
#include "media/HikDecEnc.h"
#include "logic/SegTrkCls.h"
#include "Version.h"
#include "utils/ResultsViewer.h"



using namespace std;


int ProcessingLoop(CmdLine* cmdLine)
{
	
	//encoder definitions
	FFMpegDecEnc* ffmpegDecEnc = NULL;
	HikDecEnc* hikDecEnc = NULL;
	DecEnc* enc = NULL;
	DecEnc* encConvert = NULL;
	//decoding
	DecEnc* dec = NULL;
	FrameProvider* frameProvider = NULL;
	
	//logic
	SegTrkCls* segTrkCls = NULL;
	
	//output
	deque< Image<unsigned char>* > outputImgBuffer;
	ResultsViewer* rv = 0;
	
	//subtitle file
	ofstream subFile;
	int subFileIndex;
	int crtS;
	int crtMS;
	int lastS;
	int lastMS;
	
	//ofstream debugFile;
	//debugFile.open("dbg.log", ios::binary);;
	// print version no
	if ( cmdLine->cmdLineArgs[CmdLine::ArgVersion]->isSet == true )
	{
		cout<<"CBVision v"<<VERSION<<endl;
	}
	
	// create the encoder for the processed frames
	enc = new FFMpegDecEnc();
	// if there is a conversion output file specified create a conversion encoder
	if ( cmdLine->cmdLineArgs[CmdLine::ArgConversionOutputFile]->isSet == true )
	{
		encConvert = new FFMpegDecEnc();	
	}

	//create a decoder
	if (strcmp(cmdLine->cmdLineArgs[CmdLine::ArgDecoder]->strVal,"hikvision") == 0)
	{
		dec = new HikDecEnc();
	}
	else
	{
		dec = enc;
	}

	frameProvider = new FrameProvider(cmdLine, dec);
	
	// if there is a conversion output file specified create a conversion encoder
	if ( cmdLine->cmdLineArgs[CmdLine::ArgConversionOutputFile]->isSet == true)
	{
		int encConvertInitResult = encConvert->EncodingInit(cmdLine->cmdLineArgs[CmdLine::ArgConversionOutputFile]->strVal,
			cmdLine->cmdLineArgs[CmdLine::ArgOutputCodec]->strVal, dec->DecodingGetW(), dec->DecodingGetH(),
			cmdLine->cmdLineArgs[CmdLine::ArgOutputFramerate]->isSet ? cmdLine->cmdLineArgs[CmdLine::ArgOutputFramerate]->floatVal:dec->DecodingGetFrameRate(), cmdLine->cmdLineArgs[CmdLine::ArgOutputBitrate]->intVal, PIX_FMT_YUV420P);
		if (!encConvertInitResult)
		{
			cerr<<"Error: [CBVision] Could not initialize the conversion encoder\n";
			return 1;
		}		
	}

	// if there is a output file specified then create an encoder and execute processing logic
	if (cmdLine->cmdLineArgs[CmdLine::ArgOutputFile]->isSet == true)
	{
		int encInitResult = enc->EncodingInit(cmdLine->cmdLineArgs[CmdLine::ArgOutputFile]->strVal,
			cmdLine->cmdLineArgs[CmdLine::ArgOutputCodec]->strVal, dec->DecodingGetW(), dec->DecodingGetH(),
			cmdLine->cmdLineArgs[CmdLine::ArgOutputFramerate]->isSet ? cmdLine->cmdLineArgs[CmdLine::ArgOutputFramerate]->floatVal:dec->DecodingGetFrameRate(), cmdLine->cmdLineArgs[CmdLine::ArgOutputBitrate]->intVal, PIX_FMT_YUV420P);
		if (!encInitResult)
		{
			cerr<<"Error: [CBVision] Could not initialize the output encoder\n";
			return 1;
		}
	}
	
	if (cmdLine->cmdLineArgs[CmdLine::ArgShowResults]->isSet == true)
	{
		rv = new ResultsViewer(cmdLine);
		rv->Init(dec->DecodingGetW(), dec->DecodingGetH());
	}
	
	if (cmdLine->cmdLineArgs[CmdLine::ArgOutputFile]->isSet == true)
	{
		unsigned int lastClosedEventsSize = 0;
		int lastClosedEventEndFrame = -1;
		//int lastEvtStartFrameIndex = 0;
		int curFrame = 0;
		segTrkCls = new SegTrkCls(frameProvider, cmdLine);	
		time_t processingStartTime;
		time_t startTime;
		time(&processingStartTime);
		if (cmdLine->cmdLineArgs[CmdLine::ArgMakeTimestampSubtitle]->isSet == true)
		{
			int len = strlen(cmdLine->cmdLineArgs[CmdLine::ArgOutputFile]->strVal);
			char* subFileName = new char[len + 1];
			memcpy(subFileName, cmdLine->cmdLineArgs[CmdLine::ArgOutputFile]->strVal, len + 1);
			memcpy(subFileName + len - 3, "srt", 3);
			subFile.open(subFileName, ios::binary);
			subFileIndex = 1;	
			lastS = 0;
			lastMS = 0;
			if (cmdLine->cmdLineArgs[CmdLine::ArgTimestampSubtitleStartTime]->isSet == true)
			{
				Str2Time(cmdLine->cmdLineArgs[CmdLine::ArgTimestampSubtitleStartTime]->strVal, &startTime);
				
			}
			else
			{
				time(&startTime);
			}
		}
		while (segTrkCls->UpdateNext())
		{
			curFrame++;
			unsigned char keyPressed = 0;
			
			//if(false)
			{
				cin>>keyPressed;

				if (!cin.eof())
				{
					if (!cin.fail())
					{
						if (keyPressed == 'q')
						{
							cout<<"Quiting...";
							break;
						}
						char timeStampStr[20];
						unsigned int timeStamp = segTrkCls->GetCrtTimestamp();
						time_t processingCrtTime;
						time(&processingCrtTime);
						unsigned int processingTime = processingCrtTime - processingStartTime;
						unsigned int processingTimeH = processingTime / 3600;
						unsigned int processingTimeM = processingTime % 3600;
						unsigned int processingTimeS = processingTimeM % 60;
						processingTimeM = processingTimeM / 60;
						Time2Str(timeStampStr, timeStamp);
						cout<<"At frame "<<curFrame<<" with timestamp "<<timeStampStr<<" - total processing time "<<processingTimeH<<":"<<processingTimeM<<":"<<processingTimeS<<" esi="<<segTrkCls->GetValidEventStartFrameIndex()<<" "<<segTrkCls->GetActiveEvents()->front()->expirationCounter<<endl;
					}
				}
				else
				{
					cin.clear();
				}
			}
			
			
			Image<unsigned char>* crtFrame = segTrkCls->GetCrtImg();
			//crtFrame->Copy(*segTrkCls->GetSegmentedImg());
			if (cmdLine->cmdLineArgs[CmdLine::ArgShowResults]->isSet == true && segTrkCls->classifyIsFrameInteresting
				 && curFrame >= cmdLine->cmdLineArgs[CmdLine::ArgShowResultsStartFrame]->intVal && curFrame < cmdLine->cmdLineArgs[CmdLine::ArgShowResultsEndFrame]->intVal)
			{
				rv->SetImage(0, crtFrame);
				rv->SetImage(1, segTrkCls->GetBGImg());
				rv->SetImage(2, segTrkCls->GetSegmentedImg());
				rv->SetImage(3, segTrkCls->GetClassifyingImg());
				rv->Refresh();
				if (cmdLine->cmdLineArgs[CmdLine::ArgSaveResults]->isSet == true)
					rv->Save(curFrame);
			}
			if ( cmdLine->cmdLineArgs[CmdLine::ArgConversionOutputFile]->isSet == true )
			{
				encConvert->EncodingAddFrame(*crtFrame);
			}
			outputImgBuffer.push_back(crtFrame);
			int evtStartFrameIndex = segTrkCls->GetValidEventStartFrameIndex();
	
			if (lastClosedEventsSize != segTrkCls->GetClosedEvents()->size())
			{
				int diff = segTrkCls->GetClosedEvents()->size() - lastClosedEventsSize;
				for (list<Event*>::reverse_iterator rEvtIter = segTrkCls->GetClosedEvents()->rbegin(); rEvtIter != segTrkCls->GetClosedEvents()->rend(); rEvtIter++)
				{
					if ((*rEvtIter)->endFrameIndex > lastClosedEventEndFrame)
						lastClosedEventEndFrame = (*rEvtIter)->endFrameIndex;
					diff--;
					if (diff == 0)
						break;
				}					
				lastClosedEventsSize = segTrkCls->GetClosedEvents()->size();
			}
			
			int outBufferStartIndex = curFrame + 1 - outputImgBuffer.size();
			if (evtStartFrameIndex > outBufferStartIndex || evtStartFrameIndex == -1)
			{				
				while( outBufferStartIndex <= lastClosedEventEndFrame && (evtStartFrameIndex == -1 || outBufferStartIndex <= evtStartFrameIndex))
				{
					Image<unsigned char>* tmpImg = outputImgBuffer.front();
					//output frame
					enc->EncodingAddFrame(*tmpImg);
					outputImgBuffer.pop_front();
					delete tmpImg;	

					if (cmdLine->cmdLineArgs[CmdLine::ArgMakeTimestampSubtitle]->isSet == true)
					{
						//output subtitle
						char timeStampStr[20];
						int frameRate = (int)cmdLine->cmdLineArgs[CmdLine::ArgOutputFramerate]->floatVal;
						unsigned int timeStamp = startTime + enc->EncodingGetCrtFrame() / frameRate;
						Time2Str(timeStampStr, (time_t)timeStamp);
						crtS = (int)(enc->EncodingGetCrtFrame() / frameRate);
						crtMS = (enc->EncodingGetCrtFrame() % frameRate) * 1000 / frameRate;
						int startH = lastS / 3600;
						int startM = (lastS / 60) % 60;
						int startS = lastS % 60;
						int endH = crtS / 3600;
						int endM = (crtS / 60) % 60;
						int endS = crtS % 60;
						subFile<<subFileIndex<<"\r\n";
						char intervalStr[32];
						sprintf(intervalStr, "%.2d:%.2d:%.2d,%.3d --> %.2d:%.2d:%.2d,%.3d\r\n", startH, startM, startS, lastMS, endH, endM, endS, crtMS);
						subFile<<intervalStr;
						subFile<<"F: "<<enc->EncodingGetCrtFrame()<<" T: "<<timeStampStr<<"\r\n\r\n";
						lastS = crtS;
						lastMS = crtMS;
						subFileIndex ++;
					}
					/*crtOutFrame++;
					// write out eventSet to XML?
					if (rv->isSaveXMLEventsNameSet() )
					{
						rv->saveVisualEventSetToXML(*segTrkCls->GetClosedEvents(), outBufferStartIndex, crtOutFrame, frTimestamp);
					}*/

					outBufferStartIndex++;
				}
				int frameRate = cmdLine->cmdLineArgs[CmdLine::ArgOutputFramerate]->intVal;
				while ((evtStartFrameIndex > 0 && outBufferStartIndex < evtStartFrameIndex - frameRate) ||
					(evtStartFrameIndex == -1 && outputImgBuffer.size() > (unsigned)frameRate))
				{
					Image<unsigned char>* imgTmp = outputImgBuffer.front();
					outputImgBuffer.pop_front();
					if (imgTmp)
						delete imgTmp;
					outBufferStartIndex++;
					//lastClosedEventsSize = segTrkCls->GetClosedEvents()->size();
				}
			}
		}
	}
	else if ( cmdLine->cmdLineArgs[CmdLine::ArgConversionOutputFile]->isSet == true )
	{
		Image<unsigned char>* newImg = NULL;
		while (frameProvider->GetNextFrame(newImg))
		{	
			//cout<<"new frame\n";		
			encConvert->EncodingAddFrame(*newImg);
			newImg = NULL;
		}
	}
	else
	{
		cerr<<"Error: [CBVision] At least one output file needs to be specified\n"; 
		return 1;
	}
	
	if (cmdLine->cmdLineArgs[CmdLine::ArgMakeTimestampSubtitle]->isSet == true)
	{
		subFile.close();
	}
	
	cout<<"Last event index: "<<Event::lastIndex<<endl;
	
	//close encoders
	if (cmdLine->cmdLineArgs[CmdLine::ArgOutputFile]->isSet == true)
	{
		if (enc->EncodingGetCrtFrame() <= 1)
		{
			Image<unsigned char>* tmpImg = new Image<unsigned char>(dec->DecodingGetW(),dec->DecodingGetH());
			tmpImg->Clear(0xFFFF00);
			for (int i = 0; i < enc->EncodingGetFrameRate(); i++)
				enc->EncodingAddFrame(*tmpImg);
		}
		enc->EncodingEnd();
	}
	
	if (cmdLine->cmdLineArgs[CmdLine::ArgConversionOutputFile]->isSet == true)
	{
		encConvert->EncodingEnd();
	}
	//debugFile.close();
	// clean
	delete rv;
	if (segTrkCls)
		delete segTrkCls;
	if (ffmpegDecEnc)
		delete ffmpegDecEnc;
	if (hikDecEnc)
		delete hikDecEnc;
	if (encConvert)
		delete encConvert;

	return 0;
}

int main(int argc, char* argv[])
{
	//read command line
	set_keypress();
	CmdLine* cmdLine = new CmdLine();
	if (cmdLine->ArgsParse(argc, argv) == 0)
	{
		cmdLine->ArgsPrint();
		return 1;
	}
	ProcessingLoop(cmdLine);
	delete cmdLine;
	reset_keypress();
}

