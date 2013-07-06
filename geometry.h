#ifndef __GEOMETRY_H__
#define __GEOMETRY_H__
#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/Rect.hpp>

typedef sf::Vector2<double> Vector2d;
typedef sf::Rect<double> DoubleRect;

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
	IT_CANCEL,
	IT_STICK,
	IT_BOUNCE,
	IT_SLIDE
};
struct MoveContext {
	InteractionType interaction;
	Segment vect;
	Vector2d nmove;
};
bool moveCircleToRectangle(double radius, MoveContext &ctx, const DoubleRect &r);
bool moveCircleToCircle(double radius, MoveContext &ctx, const Circle &colli);
double pointsDistance(Vector2d p1, Vector2d p2);
#endif
