#include "Shape.h"
#include <stdio.h>
#include <string.h>
#include <iostream>
#include "../utils/Utils.h"
Shape::Shape():
	totalShapeValue(0),
	lx(0),
	ly(0),
	maxShapeValue(0),
	closeConfidence(0),
	totalConfidence(0)
{

}

Shape::~Shape()
{
	for (list<Point*>::iterator pointIterator = pointList.begin(); pointIterator != pointList.end(); pointIterator++)
	{
		delete *pointIterator;
	}
	pointList.erase(pointList.begin(), pointList.end());
}

Point* Shape::PointAdd(int x, int y, int dx, int dy, int value, bool addAtFront, int confidence)
{
	Point* newPoint = new Point();
	newPoint->x = x;
	newPoint->y = y;
	newPoint->dx = dx;
	newPoint->dy = dy;

	if (addAtFront)
		pointList.push_front(newPoint);
	else
		pointList.push_back(newPoint);

	totalShapeValue += value;

	if(value > maxShapeValue)
		maxShapeValue = value;
	lx += dx;
	ly += dy;
	
	closeConfidence = (closeConfidence + confidence) >> 1;
	totalConfidence += confidence;
	return newPoint;
}

int Shape::PointCount()
{
	return pointList.size();
}

Point* Shape::PointGetLast()
{
	return pointList.back();
}

Point* Shape::PointGetFirst()
{
	return pointList.front();
}

list<Point*> Shape::PointListGet()
{
	return pointList;
}

double Shape::ThetaGet()
{
	return aTan2( (double)ly, (double)lx);
}

int Shape::ValueGet()
{
	if (pointList.size() == 0)
		return 0;
	return totalShapeValue / pointList.size();
}

int Shape::ValueMaxGet()
{
	return maxShapeValue;
}

int Shape::LengthXGet()
{
	return lx;
}

int Shape::LengthYGet()
{
	return ly;
}

int Shape::CloseConfidenceGet()
{
	return closeConfidence;
}

int Shape::AverageConfidenceGet()
{
	return totalConfidence / pointList.size();
}
