#ifndef __GEOMETRY_H__
#define __GEOMETRY_H__
#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/Rect.hpp>

struct Line {
	double a,b,c; /* Line equation a*x+b*y+c = 0 */
};
struct Segment {
	sf::Vector2f pt1, pt2;
	Line toLine() const;
};
struct Circle {
	sf::Vector2f center;
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
bool moveCircleToRectangle(double radius, Segment &vect, const sf::FloatRect &r);
bool moveCircleToCircle(double radius, Segment &vect, const Circle &colli);
double pointsDistance(sf::Vector2f p1, sf::Vector2f p2);
#endif
