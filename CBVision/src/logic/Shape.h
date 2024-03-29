#ifndef SHAPE_H_
#define SHAPE_H_

#include <list>
#include "Point.h"


using namespace std;

class Shape
{
public:

	list<Point*> pointList;
	int lx;
	int ly;
	int totalShapeValue;
	int maxShapeValue;

	int totalConfidence;
	int closeConfidence;

	Shape();
	~Shape();
	Point* PointAdd(int x, int y, int dx, int dy, int value, bool addAtFront, int confidence);
	
	int PointCount();
	Point* PointGetFirst();
	Point* PointGetLast();

	list<Point*> PointListGet();
	double ThetaGet();
	int LengthXGet();
	int LengthYGet();
	int ValueMaxGet();
	int ValueGet();
	
	int CloseConfidenceGet();
	int AverageConfidenceGet();
};


#endif
