#include "geometry.h"
#include <SFML/Graphics/Rect.hpp>
#include <math.h>
#include <vector>
#include <stdio.h>


Line Segment::toLine() const {
	Line res;
	res.a = (pt2.y - pt1.y);
	res.b = (pt1.x - pt2.x);
	res.c = -(pt1.x*res.a + pt1.y*res.b);
	return res;
}
double pointsDistance(Vector2d p1, Vector2d p2) {
	Vector2d v=p2-p1;
	return sqrt(v.x*v.x + v.y*v.y);
}
static Line parallelLine(const Line &line, const Vector2d &pt) {
	/* computes a line parallel to the input line, going through the specified pt point */
	Line res = line;
	res.c = -(res.a * pt.x + res.b * pt.y);
	return res;
}
static Line orthoLine(const Line &line, const Vector2d &pt) {
	/* computes a line orthogonal to the input line, going through the specified pt point */
	Line res;
	res.a = -line.b;
	res.b = line.a;
	res.c = -(res.a * pt.x + res.b * pt.y);
	return res;
}
static bool solve2nd(double *res1, double *res2, double a, double b, double c) {/* solves equation : a*x^2 + b*x + c = 0 */
	double delta = b*b-4*a*c;
	if (delta < 0) return false;
	delta = sqrt(delta);
	*res1 = (-b+delta)/(2*a);
	*res2 = (-b-delta)/(2*a);
	return true;
}

static bool circleIntersectsLine(Vector2d *res1, Vector2d *res2, const Line &line, const Circle &ci) {
	Vector2d res;
	/* equation is: x^2(1-a^2/b^2)+x(2*y0-2*x0)+(x0^2+y0^2-r^2)=0 */
	double x0, y0, a, b;
	double coeff1, coeff2;
	double con1, con2;
	if (line.b > line.a) {a = line.a; b = line.b; x0 = ci.center.x + line.c/b; y0 = ci.center.y; coeff1 = 1; con1 = 0; coeff2 = -(a/b); con2 = -(line.c/b);}
	else		     {a = line.b; b = line.a; x0 = ci.center.y + line.c/b; y0 = ci.center.x; coeff2 = 1; con2 = 0; coeff1 = -(a/b); con1 = -(line.c/b);}
	double x1, x2;
	if (!solve2nd(&x1, &x2, 1-(a*a)/(b*b), 2*(y0 - x0), (x0*x0+y0*y0 - ci.radius*ci.radius)))
		return false;
	(*res1) = Vector2d(coeff1*x1+con1, coeff2*x1+con2);
	(*res2) = Vector2d(coeff1*x2+con1, coeff2*x2+con2);
	return true;
}
static bool is_between(double x, double a, double b) {
	return (x >= a && x < b) || (x >= b && x < a);
}
static bool isLPointOnSegment(const Vector2d &res, const Segment &segt) { /* point must already be on the Line */
	double d1, d2;
	d1 = segt.pt2.x - segt.pt1.x;
	d2 = segt.pt2.y - segt.pt2.y;
	if (fabs(d1) > fabs(d2)) return is_between(res.x, segt.pt1.x, segt.pt2.x);
	return is_between(res.y, segt.pt1.y, segt.pt2.y);
}
static bool circleIntersectsSegment(Vector2d &res, const Segment &segt, const Circle &ci) {
	Vector2d p1, p2;
	if (!circleIntersectsLine(&p1, &p2, segt.toLine(), ci)) return false;
	if (pointsDistance(p1, segt.pt1) < pointsDistance(p2, segt.pt1)) res = p1; else res = p2;
	return isLPointOnSegment(res, segt);
}
static bool intersectLines(Vector2d &res, const Line &line1, const Line &line2) {
	double d = line1.a * line2.b - line2.a * line1.b;
	if (fabs(d) < 1e-8) return false;
	res.y = (line2.a*line1.c - line1.a*line2.c) / d;
	res.x = (line1.b*line2.c - line2.b*line1.c) / d;
	return true;
}
static bool intersectSegments(Vector2d &res0, const Segment &s1, const Segment &s2) {
	Vector2d res;
	if (!intersectLines(res, s1.toLine(), s2.toLine())) return false;
        if (!isLPointOnSegment(res, s1)) return false;
	if (!isLPointOnSegment(res, s2)) return false;
	res0 = res;
	return true;
}
static double angle_from_dxdy(double dx, double dy) {
	double angle=0;
	angle = atan(dy/dx);
	if (dx < 0) angle += M_PI;
	return angle+M_PI/2;
}
static bool trigoAngleFromSegment(const Segment &segt) { /* oriented segment */
	return angle_from_dxdy(segt.pt2.x - segt.pt1.x, segt.pt2.y - segt.pt1.y);
}
static bool pointMovesToCircleArc(Segment &vect, const CircleArc &arc) { /* oriented segment */
	return false;
	Vector2d A;
	Vector2d B = vect.pt2;
	Vector2d C;
	if (!circleIntersectsSegment(A, vect, arc.circle)) return false;
	Segment AB, OA;
	OA.pt2 = A; OA.pt1 = arc.circle.center;
	double angle = trigoAngleFromSegment(OA);
	if (!(angle >= arc.start && angle < arc.end)) return false;
	AB.pt1 = A; AB.pt2 = B;
	Line BC = parallelLine(OA.toLine(), B);
	Line AC = orthoLine(OA.toLine(), A);
	if (!intersectLines(C, BC, AC))
		{vect.pt2 = vect.pt1;return true;} /* In theory, it's impossible as BC and AC are orthogonal */
	vect.pt2 = C;
	return true;
}
static void dispLine(const Line &line) {
	fprintf(stderr, "[line %lg*x+%lg*y+%lg = 0]\n", line.a, line.b, line.c);
}
static void dispPoint(Vector2d pt) {
	fprintf(stderr, "[point %lg %lg]\n", pt.x, pt.y);
}
static bool pointMovesToSegment(Segment &vect, const Segment &segt) {
	Vector2d A, C, B = vect.pt2;
	if (!intersectSegments(A, vect, segt)) {
		return false;
	} else {
	fprintf(stderr, "[pmt (%lg,%lg)-(%lg,%lg) (%lg,%lg)-(%lg,%lg)]\n"
		,vect.pt1.x, vect.pt1.y, vect.pt2.x, vect.pt2.y
		,segt.pt1.x, segt.pt1.y, segt.pt2.x, segt.pt2.y);
		fprintf(stderr, "[intersects at %lg,%lg]\n", A.x, A.y);
		asm("int $3");
	}
	Line BC = orthoLine(segt.toLine(), B);
	Line AC = segt.toLine();
	if (!intersectLines(C, BC, AC)) {
		fprintf(stderr, "[This should never happen]\n");
		vect.pt2 = vect.pt1;
		return true;
	}
	vect.pt2 = C;
	return true;
}
static bool pointMovesToComplexShape(Segment &vect, const ComplexShape &shape) {
	if (shape.type == CSIT_ARC) return pointMovesToCircleArc(vect, shape.arc);
	else if (shape.type == CSIT_SEGMENT) return pointMovesToSegment(vect, shape.segment);
	return false;
}
static void roundAugmentRectangle(const DoubleRect &r0, double radius, std::vector<ComplexShape> &shapes) {
	unsigned i;
	DoubleRect r = r0;
	shapes.resize(8);
	r.left -= radius;
	r.top  -= radius;
	r.width  += 2*radius;
	r.height += 2*radius;
	for(i=0; i < 4;i++) {
		Segment s;
		CircleArc c;
		s.pt1.x = ((i==0 || i==3) ? r.left : r.left+r.width);
		s.pt1.y = ((i==0 || i==1) ? r.top  : r.top+r.height);
		s.pt2.x = ((i==2 || i==3) ? r.left : r.left+r.width);
		s.pt2.y = ((i==0 || i==3) ? r.top  : r.top+r.height);
		shapes[2*i].type = CSIT_SEGMENT;
		shapes[2*i].segment = s;

		c.circle.radius = radius;
		s.pt1.x = ((i==0 || i==3) ? r0.left : r0.left+r0.width);
		s.pt1.y = ((i==0 || i==1) ? r0.top  : r0.top+r0.height);
		c.circle.center = s.pt1;
		c.start = M_PI/2 - i*M_PI/2;
		if (c.end < 0) c.end += 2*M_PI;
		c.end = c.start + M_PI/2;
		shapes[2*i+1].type = CSIT_ARC;
		shapes[2*i+1].arc = c;
	}
}
bool moveCircleToRectangle(double radius, Segment &vect, const DoubleRect &r) {
	std::vector<ComplexShape> shapes;
	roundAugmentRectangle(r, radius, shapes);
	for(unsigned i=0; i < shapes.size(); i++) {
		if (pointMovesToComplexShape(vect, shapes[i])) {
			return true;
		}
	}
}
bool moveCircleToCircle(double radius, Segment &vect, const Circle &colli) {
	CircleArc arc;
	arc.circle.center = colli.center;
	arc.circle.radius = radius + colli.radius;
	arc.start = 0;
	arc.end   = M_PI*2+1e-3;
	return pointMovesToCircleArc(vect, arc);
}

void test_geometry_cpp() {
	Segment s1;
	Segment v;
	s1.pt1.x = 100;
	s1.pt1.y = 0;
	s1.pt2.x = 200;
	s1.pt2.y = 100;
	v.pt1.x = 10;
	v.pt1.y = 10;
	v.pt2.x = 80;
	v.pt2.y = 10;
	if (pointMovesToSegment(v, s1)) {
		fprintf(stderr, "[point moves to segt %lg x %lg]\n", v.pt2.x, v.pt2.y);
	} else {
		fprintf(stderr, "[point doesn't move to segt]\n");
	}
}
