#ifndef EVENT_FRAME_H_
#define EVENT_FRAME_H_

#include <list>
class Event;
using namespace std;
class EventFrame
{
public:
	static const int FlagUsed;
	static const int FlagPainted;

	static int VideoW;
	static int VideoH;

	int label;
	int weightCenter[2];
	int area;
	int box[4];
	int extendedBox[4];
	int flags;
	int timestamp;
	int imgIndex;

	char dbgMsg[255];
	
	list<Event*> associatedEvents;

	EventFrame();
	~EventFrame();
	void AddPoint(int i, int j);
	void Close();
};

#endif