#ifndef EVENT_H_
#define EVENT_H_

#include <list>
#include "EventFrame.h"
#include "../utils/Utils.h"

using namespace std;

class Event
{
public:
	list<EventFrame*> eventFrames;
	list<int> angles1;
	list<int> angles2;
	int expirationCounter;
	int startFrameIndex;
	int endFrameIndex;
	int lastPos[2];
	int lastArea;
	int avgArea;
	int lastSpeed[2];
	double avgSpeed[2];
	int avgSpeedCount;
	int flags;
	int closedIndex;
	int creationIndex;
	int closeReason;
	double lastDirection;
	Event* lastFrameFirstChoiceMate;
	int counterForBeingSecondChoice;
	float classificationScore;
	int classificationScoreFrames;
	int classificationLenIsOverMin;

	static const int ExpirationTime;
	static const int FlagHasSpeed;
	static const int FlagUsed;
	static const int FlagClosed;
	static int lastIndex;
	

	Event();
	void Init();
	~Event();
	void Close();
};

#endif