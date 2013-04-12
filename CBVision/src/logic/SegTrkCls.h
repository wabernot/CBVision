#ifndef SEGTRKCLS_H_
#define SEGTRKCLS_H_

#include "../image/Image.h"
#include "../logic/Event.h"
#include "../logic/EventFrame.h"
#include "../logic/Shape.h"
#include "../media/FrameProvider.h"
#include "../utils/CmdLine.h"
#include "../utils/Utils.h"
#include <deque>
#include <vector>
#include <list>
#include <cairo.h>
#include <gtk/gtk.h>
#include <fstream>
using namespace std;

class SegTrkCls
{
private:
	static const int VariationGridSize;

	static const int AvgDepth;
	static const int SumMinDarkThreshold;
	static const int SumMaxDarkThreshold;
	
	static const int SumImgHalvingImgCnt;

	static const int ReinitAvgThreshold;

	static const int BgImgMaxVariance;
	static const int BGInvalidateDiffPrev;
	static const int BGInvalidateDiffCrt;
	
	static const int ScoringPercentDistance;
	static const int ScoringPercentArea;
	static const int ScoringPercentSpeed;
	
	static const unsigned int ReinitBGMinTimeDiff;
	
	static const int ClassifyImgScale;
	
	typedef struct 
	{
		//double score;
		int sqDistDiffFromPredicted;

		Event* evt;
		EventFrame* evtFr;
	} EventScore;

	int w;
	int h;
	float frameRate;
	Image<int>* sumImg;
	//Image< PixRGB<unsigned char> >* crtImgRGB;
	Image<int>* substractedImg;
	Image<unsigned char>* avgImg;
	Image<unsigned char>* segmentedImg;
	Image<unsigned char>* crtImg;
	Image<unsigned char>* crtImgMasked;
	Image<int>* classifyingImg;

	//Image<unsigned char>* lastEmptyImg;
	unsigned int crtImgTimestamp;
	int crtImgIndex;
	int lastImgTimestamp;
	Image<unsigned char>* tmpByteImg1;
	Image<unsigned char>* tmpByteImg2;
	Image<int>* tmpIntImg1;
	Image<int>* tmpIntImg2;
	Image<int>* workingImg;
	int* workingImgData;
	int sumImgCnt;
	deque< Image< unsigned char >* > imgCache;
	deque<unsigned int> imgTimestampCache;
	int imgCacheFirstFrame;

	int darkThreshold;
	int bgInitFrameCount;
	CmdLine* cmdLine;
	FrameProvider* frameProvider;
	list<Event*> activeEvents;
	list<Event*> closedEvents;
	vector<EventFrame*> currentEventFrames;
	unsigned int currentEventFramesUsedCount;
	int activeEventsCount;
	double substractingCrtDiff;

//segmentation
	void SegmentAndAddToBG();
	void BuildStatsByClustering();
	void Classify();
	int LabelNew(vector<int>& labelCoverage);
	void LabelSwitch(int index1, int index2, vector<int>& labelCoverage);
	void LabelMerge(int index, int to, vector<int>& labelCoverage);
	
//events
	void EventsUpdate();
	list<Event*>::iterator EventClose(list<Event*>::iterator evtIter);
	void EventCloseAll();
	static bool EventScoresCompare(const EventScore &es1, const EventScore &es2);
	void EventAddToClosedList(Event* evt);
	void EventsDraw();

//bg
	//bool IsBGValid();
	bool InitBG();

//frame queue
	bool ReadAndCache();
	bool PeekAtFrameRelative(unsigned int frameNo, Image<unsigned char>* &img, int* timestamp);
	bool GetNextFrame(Image<unsigned char>* &img, unsigned int* timestamp);

	//utils	ut of stock online

	//static bool CompareAbs(int, int);
	static int Sgn(int);
	
public:
	SegTrkCls(FrameProvider* fp, CmdLine* cl);
	~SegTrkCls();
	bool UpdateNext();

	Image<int>* GetSubstractedImg();
	Image<unsigned char>* GetBGImg();
	Image<unsigned char>* GetSegmentedImg();
	Image<unsigned char>* GetCrtImg();
	unsigned int GetCrtTimestamp();
	Image<int>* GetClassifyingImg();
	

	int GetValidEventStartFrameIndex();
	bool EventIsAnyValid();
	list<Event*>* GetActiveEvents();
	list<Event*>* GetClosedEvents();

//debug
	Image<unsigned char>* GetDbgImg();
	//Image<unsigned char>* GetLastEmptyImg();
	Image<unsigned char>* dbgImg;
	vector<float> dbgAvgV;
	vector<float> dbgAbsAvgV;
	vector<double> dbgDiff;
	vector<int> dbgCnt;
	cairo_surface_t *overlaySurface;
	cairo_t *overlayContext;
	
	
	
	
	
	//classification
	
	static const int GridSize = 4;
	static const int FirstZoom = 1 << 2;
	static const int FirstZoomStep = GridSize << 2;
	static const int SecondZoom = 1 << 4;
	static const int SecondZoomStep = GridSize << 4;
	
	static const int OffsetPixel = 0;
	static const int OffsetTheta = 1;
	static const int OffsetGrad = 2;
	static const int OffsetPixelDiff = 3;
	static const int OffsetGradDiff = 4;
	static const int OffsetAngleDiff = 5;
	static const int OffsetResult = 6;
	static const int OffsetFirstZoomPixelDiff = 7;
	static const int OffsetFirstZoomGradDiff = 8;
	static const int OffsetSecondZoomSum = 9;
	static const int OffsetSecondZoomGradSum = 10;
	static const int OffsetShape1 = 11;
	static const int OffsetShape4 = 14;
	static const int OffsetSubstractedIndex = 15;
	static const int OffsetCount = 16;
	
	
	static int* offsets;
	static int xDir[];
	static int yDir[];	
	int* sectionBox;
	int sectionBoxGridAligned[4];
	int maxEdgeValue;
	int minEdgeValue;
	int classificationScore;
	int totalClassificationScore;
	int totalClassificationScoreCount;
	vector<Shape*> shapes;
	ofstream classificationResultsFile;
	
	void ClassifySetSection(const int* box);
	//template <class T, class T_1> void ApplyOperator(const T* mulMat, int mulMatDim, const Image<T_1>& src);
	//template <unsigned char, unsigned char> void ApplyOperator(const T* mulMat, int mulMatDim, const Image<unsigned char>& src);
	
	template <class T, class T_1> void ClassifyComputeGradient(Image<T>* destImg, const Image<T_1>& srcImg);
	template <int, unsigned char> void ClassifyComputeGradient(Image<int>* destImg, const Image<unsigned char>& srcImg);

	template <class T> void ClassifyDrawGradient(Image<T>* destImg);
	template <class T> void ClassifyComputeShape(Image<T>* destImg);
	template <class T> void ClassifyThinEdges(Image<T>* destImg);
	
	template <class T> void ClassifyClearData(Image<T>* destImg);
	template <class T> void ClassifyDrawShapes(Image<T>* destImg);
	
	template <class T> void ClassifySubstract(Image<T>* destImg, const Image<T>& srcImg);
	
	template <class T> void ClassifyTestLamprey(Image<T>* destImg);
	bool classifyIsFrameInteresting;
		
	void ClassifyComputeShape();
	void ClassifyShapeGetWeightCenter(int* weightCenter, int boxX1, int boxY1, int boxX2, int boxY2);
		
	int OverlayDrawText(const char * text, int x, int y);
	int OverlayToImage(Image<unsigned char> * destImg);
};

#endif