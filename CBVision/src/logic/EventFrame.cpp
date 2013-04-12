#include "EventFrame.h"
using namespace std;

const int EventFrame::FlagUsed = 1 << 0;
const int EventFrame::FlagPainted = 1 << 1;

int EventFrame::VideoW = 0;
int EventFrame::VideoH = 0;

EventFrame::EventFrame()
{
	box[0] = 0x7FFFFFFF;
	box[1] = 0x7FFFFFFF;
	box[2] = 0;
	box[3] = 0;
	weightCenter[0] = 0;
	weightCenter[1] = 0;
	flags = 0;
	area = 0;
	label = 0;
	dbgMsg[0]='\0';
}

EventFrame::~EventFrame()
{
}

void EventFrame::AddPoint(int i, int j)
{
	if (box[0] > i)
		box[0] = i;
	if (box[1] > j)
		box[1] = j;
	if (box[2] < i)
		box[2] = i;
	if (box[3]< j)
		box[3] = j;
	
	weightCenter[0] += i;
	weightCenter[1] += j;
}

void EventFrame::Close()
{
	extendedBox[0] = box[0] - 20;
	if (extendedBox[0] < 0)
		extendedBox[0] = 0;
	extendedBox[1] = box[1] - 20;
	if (extendedBox[1] < 0)
		extendedBox[1] = 0;
	extendedBox[2] = box[2] + 20;
	if (extendedBox[2] >= VideoW)
		extendedBox[2] = VideoW - 1;
	extendedBox[3] = box[3] + 20;
	if (extendedBox[3] >= VideoH)
		extendedBox[3] = VideoH - 1;
	
	weightCenter[0] /= area;
	weightCenter[1] /= area;
}

