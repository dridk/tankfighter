#ifndef __GEOMETRY_H__
#define __GEOMETRY_H__
#include "coretypes.h"

struct Line {
	double a,b,c; /* Line equation a*x+b*y+c = 0 */
};
struct Segment {
	Vector2d pt1, pt2;
	Line toLine() const;
};
struct Circle {
	Vector2d center;
	double radius;
};
struct CircleArc {
	Circle circle;
	double start, end; /* RADIANS angle */
};
enum ComplexShapeItemType {
	CSIT_NONE,
	CSIT_ARC,
	CSIT_SEGMENT
};
struct ComplexShape {
	ComplexShapeItemType type;
		Segment segment;
		CircleArc arc;
};
enum InteractionType {
	IT_GHOST, /* Don't interact. Used for testing whether a collision exists. */
	IT_CANCEL,
	IT_STICK,
	IT_BOUNCE,
	IT_SLIDE
};
struct MoveContext {
	MoveContext();
	MoveContext(InteractionType interaction, const Segment &vect);
	InteractionType interaction;
	Segment vect;
	Vector2d nmove;
};
bool moveCircleToRectangle(double radius, MoveContext &ctx, const DoubleRect &r);
bool moveCircleToCircle(double radius, MoveContext &ctx, const Circle &colli);
double pointsDistance(Vector2d p1, Vector2d p2);
#endif
