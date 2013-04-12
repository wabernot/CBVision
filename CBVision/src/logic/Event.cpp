#include "Event.h"
#include "EventFrame.h"
#include <iostream>
#include <vector>

using namespace std;

const int Event::ExpirationTime = 10;

const int Event::FlagUsed = 1 << 0;
const int Event::FlagHasSpeed = 1 << 1;
const int Event::FlagClosed = 1 << 2;

int Event::lastIndex = 0;

Event::Event()
{
	Init();
}

void Event::Init()
{
	expirationCounter = ExpirationTime;
	lastPos[0] = 0;
	lastPos[1] = 0;
	lastSpeed[0] = lastSpeed[1] = 0;
	avgSpeed[0] = avgSpeed[1] = 0;
	avgSpeedCount = 0;
	startFrameIndex = 0;
	endFrameIndex = 0;
	lastArea = 0;
	avgArea = 0;
	flags = 0;
	closedIndex = 0;
	closeReason = 0;
	Event::lastIndex++;
	creationIndex = Event::lastIndex;	
	
	counterForBeingSecondChoice = 0;
	lastFrameFirstChoiceMate = NULL;
	classificationScore = 0.0;
	classificationScoreFrames = 0;
	classificationLenIsOverMin = 0;
}

Event::~Event()
{
}

void Event::Close()
{
	expirationCounter = 0;
	flags |= Event::FlagClosed;
	
}
