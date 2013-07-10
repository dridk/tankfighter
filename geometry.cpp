#include "geometry.h"
#include <SFML/Graphics/Rect.hpp>
#include <math.h>
#include <vector>
#include <stdio.h>


#ifdef DEBUG
static void dispLine(const Line &line) {
	fprintf(stderr, "[line %lg*x+%lg*y+%lg = 0]\n", line.a, line.b, line.c);
}
static void dispPoint(Vector2d pt) {
	fprintf(stderr, "[point %lg %lg]\n", pt.x, pt.y);
}
#endif
static const double minWallDistance = 1e-3;

Line Segment::toLine() const {
	Line res;
	res.a = (pt2.y - pt1.y);
	res.b = (pt1.x - pt2.x);
	res.c = -(pt1.x*res.a + pt1.y*res.b);
	return res;
}
double segmentModule(const Segment &segt) {
	return pointsDistance(segt.pt1, segt.pt2);
}
double vectorModule(const Vector2d &v) {
	return sqrt(v.x*v.x + v.y*v.y);
}
void normalizeVector(Vector2d &v, double new_module) {
	double module = vectorModule(v);
	v.x *= new_module/module;
	v.y *= new_module/module;
}
void normalizeAngle(double &angle) {
	if (angle < 0) angle += M_PI*2;
	if (angle > M_PI*2) angle -= M_PI*2;
}
void normalizeAngle(float &angle) {
	double a = angle;
	normalizeAngle(a);
	angle = a;
}
#if 0
static Vector2d line2Vector(const Line &line) {
	return Vector2d(line.a, -line.b);
}
static Vector2d orthoVector(const Vector2d &v) {
	return Vector2d(v.y, -v.x);
}
static Line parallelLine(const Line &line, const Vector2d &pt) {
	/* computes a line parallel to the input line, going through the specified pt point */
	Line res = line;
	res.c = -(res.a * pt.x + res.b * pt.y);
	return res;
}
static bool orthoProjectOnSegment(Vector2d &res0, const Segment &segt, const Vector2d &pt) {
	Vector2d res;
	if (!orthoProjectOnLine(res, segt.toLine(), pt)) return false;
	if (!isLPointOnSegment(res, segt)) return false;
	res0 = res;
	return true;
}
static bool inRectangle(const Segment &segt, const DoubleRect &r) {
	return inRectangle(segt.pt1, r) && inRectangle(segt.pt2, r);
}
#endif
static Vector2d segment2Vector(const Segment &segt) {
	return segt.pt2 - segt.pt1;
}
double pointsDistance(Vector2d p1, Vector2d p2) {
	Vector2d v=p2-p1;
	return sqrt(v.x*v.x + v.y*v.y);
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

static Vector2d getPointOnLine(const Line &line) {
	Vector2d res;
	double a=line.a, b=line.b, c=line.c;
	if ((fabs(a)+fabs(b)) < 1e-6) {
		/* invalid line */
		res.x = 0; res.y = 0;
		return res;
	}
	if (fabs(b) > fabs(a)) {
		res.x = 0;
		res.y = - c/b;
	} else {
		res.y = 0;
		res.x = - c/a;
	}
	return res;
}
static bool circleIntersectsLine(Vector2d *res1, Vector2d *res2, const Line &line, const Circle &ci) {
	Vector2d res;
	/* equation is: x^2(1-a^2/b^2)+x(2*y0-2*x0)+(x0^2+y0^2-r^2)=0 */
	double x0, y0, a, b;
	double coeff1, coeff2;
	Vector2d repere = getPointOnLine(line); /* Now, ignore c constant, since this line goes through this repere origin */
	Vector2d cic = ci.center - repere;
	if (fabs(line.b) > fabs(line.a)) {a = line.a; b = line.b; x0 = cic.x; y0 = cic.y; coeff1 = 1; coeff2 = -(a/b);}
	else		                 {a = line.b; b = line.a; x0 = cic.y; y0 = cic.x; coeff2 = 1; coeff1 = -(a/b);}
	double x1, x2;
	if (!solve2nd(&x1, &x2, 1+(a*a)/(b*b), 2*(a/b*y0 - x0), (x0*x0+y0*y0 - ci.radius*ci.radius)))
		return false;
	(*res1) = Vector2d(coeff1*x1, coeff2*x1) + repere;
	(*res2) = Vector2d(coeff1*x2, coeff2*x2) + repere;
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
	if (segmentModule(segt) < 1e-6) return false;
	if (!circleIntersectsLine(&p1, &p2, segt.toLine(), ci)) return false;
	/*fprintf(stderr, "[circle intersects line %lg,%lg and %lg,%lg]\n", p1.x, p1.y, p2.x, p2.y);*/
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
double angle_from_dxdy(double dx, double dy) {
	double angle=0;
	angle = atan(dy/dx);
	if (dx < 0) angle += M_PI;
	normalizeAngle(angle);
	return angle;
}
static double trigoAngleFromSegment(const Segment &segt) { /* oriented segment */
	return angle_from_dxdy(segt.pt2.x - segt.pt1.x, segt.pt2.y - segt.pt1.y);
}
static bool orthoProjectOnLine(Vector2d &res, const Line &line, const Vector2d &pt) {
	return intersectLines(res, orthoLine(line, pt), line);
}
static void translateSegment(Segment &segt, Vector2d v) {
	segt.pt1 += v;
	segt.pt2 += v;
}
static bool inCircle(const Vector2d &pt, const Circle &circle) {
	return pointsDistance(pt, circle.center) < circle.radius;
}
static bool inCircle(const Segment &segt, const Circle &circle) {
	return inCircle(segt.pt1, circle) && inCircle(segt.pt2, circle);
}
static bool inRectangle(const Vector2d &p, const DoubleRect &r) {
	return p.x >= r.left && p.x < r.left+r.width && p.y >= r.top && p.y < r.top+r.height;
}
static bool inRoundRectangle(const Vector2d &p, const DoubleRect &r, double radius) {
	Vector2d corner;
	if (!inRectangle(p,r)) return false;
	if (p.x > r.left+radius && p.x < r.left+r.width-radius) return p.y > r.top && p.y  < r.top+r.height;
	if (p.y > r.top+radius &&  p.y < r.top+r.height-radius) return p.x > r.left && p.x < r.left+r.width;
	if (p.x <= r.left+radius) corner.x = r.left+radius; else corner.x = r.left+r.width+radius;
	if (p.y <= r.top+radius)  corner.y = r.top +radius; else corner.y = r.top+r.height+radius;
	return pointsDistance(p, corner) < radius;
}
static bool pointMovesAgainstWall(MoveContext &ctx, const Line &wall, const Vector2d &A) {
	Segment &vect = ctx.vect;
	Vector2d I = vect.pt1;
	Vector2d B = vect.pt2;
	InteractionType interaction = ctx.interaction;
	if (interaction == IT_SLIDE || interaction == IT_BOUNCE) {
		Vector2d C;
		Line BC = orthoLine(wall, B);
		Line AC = wall;
		if (!intersectLines(C, BC, AC)) {
			fprintf(stderr, "[This should never happen]\n");
			vect.pt2 = I;
			return true;
		}
		if (interaction == IT_SLIDE) {
			vect.pt2 = C;
		} else {
			vect.pt2 = C + (C-B);
			Vector2d move = vect.pt2 - A;
			normalizeVector(move, vectorModule(ctx.nmove));
			ctx.nmove = move;
			return true;
		}
	} else if (interaction == IT_STICK) {
		vect.pt2 = A;
	} else if (interaction == IT_CANCEL) {
		vect.pt2 = I;
		return true;
	} else if (interaction == IT_GHOST) {
		return true;
	}
	Vector2d proj;
	if (orthoProjectOnLine(proj, wall, I)) {
		Vector2d v = Vector2d(I.x - proj.x, I.y - proj.y); /* vector orthogonal to wall, that moves the point out of the wall */
		if (vectorModule(v) >= minWallDistance/2) {
			normalizeVector(v, minWallDistance);
			/*translateSegment(vect, v);*/
			vect.pt2 += v;
		}
	}
	return true;
}
static bool pointMovesToCircleArc(MoveContext &ctx, const CircleArc &arc) { /* oriented segment */
	Segment &vect = ctx.vect;
	Vector2d A;
	const Circle &circle = arc.circle;

	if (fabs(arc.start) <= 1e-3 && fabs(arc.end-2*M_PI) <= 1e-3
	    && circle.filled && inCircle(vect.pt1, circle) && !inCircle(vect.pt2, circle)) {
		return false; /* assume exiting something is not interacting */
	}
	if (fabs(arc.start) <= 1e-3 && fabs(arc.end-2*M_PI) <= 1e-3
	    && circle.filled && inCircle(vect, circle)) {
		/* already inside discus */
		if (ctx.interaction == IT_GHOST) return true;
		else if (ctx.interaction == IT_CANCEL) {vect.pt2 = vect.pt1;}
		else if (ctx.interaction == IT_SLIDE || ctx.interaction == IT_STICK || ctx.interaction == IT_BOUNCE) {
			Vector2d OB = (ctx.interaction != IT_STICK ? vect.pt2 : vect.pt1) - circle.center;
			double module = segmentModule(vect);
			normalizeVector(OB, circle.radius+minWallDistance);
			vect.pt2 = circle.center + OB;
			if (ctx.interaction == IT_BOUNCE) {
				normalizeVector(OB, module);
				ctx.nmove = OB;
			}
		}
		return true;
	}
	if (!circleIntersectsSegment(A, vect, circle)) return false;
	/*fprintf(stderr, "[pmc (%lg,%lg)-(%lg,%lg) (%lg,%lg)-%lg [%lg-%lg]]\n"
		,vect.pt1.x, vect.pt1.y, vect.pt2.x, vect.pt2.y
		,arc.circle.center.x, arc.circle.center.y, arc.circle.radius, arc.start, arc.end);
	fprintf(stderr, "[pmc intersects at %lg,%lg]\n", A.x, A.y);*/
	Segment AB, OA;
	OA.pt2 = A; OA.pt1 = circle.center;
	double angle = trigoAngleFromSegment(OA);
	if (!(angle >= arc.start && angle < arc.end)) return false;
	return pointMovesAgainstWall(ctx, orthoLine(OA.toLine(), A), A);
}
static bool pointMovesToSegment(MoveContext &ctx, const Segment &segt0) {
	Segment &vect = ctx.vect;
	Vector2d A, C;
	Vector2d proj;
	Segment segt = segt0;
	if (segmentModule(vect) < 1e-6) return false;
	/*fprintf(stderr, "[pos %lg,%lg]\n", vect.pt1.x, vect.pt1.y);*/
	if (!intersectSegments(A, vect, segt)) {
		return false;
	} /*else {
	fprintf(stderr, "[pmt (%lg,%lg)-(%lg,%lg) (%lg,%lg)-(%lg,%lg)]\n"
		,vect.pt1.x, vect.pt1.y, vect.pt2.x, vect.pt2.y
		,segt.pt1.x, segt.pt1.y, segt.pt2.x, segt.pt2.y);
		fprintf(stderr, "[intersects at %lg,%lg]\n", A.x, A.y);
	}*/
	return pointMovesAgainstWall(ctx, segt.toLine(), A);
}
static bool pointMovesToComplexShape(MoveContext &ctx, const ComplexShape &shape) {
	if (shape.type == CSIT_ARC) return pointMovesToCircleArc(ctx, shape.arc);
	else if (shape.type == CSIT_SEGMENT) return pointMovesToSegment(ctx, shape.segment);
	return false;
}
static void prolongateSegment(Segment &s, double distance) {
	Vector2d pro1 = s.pt1 - s.pt2;
	normalizeVector(pro1, distance);
	s.pt1 += pro1;
	s.pt2 -= pro1;
}
static void augmentRectangle(DoubleRect &r, double augment) {
	r.left -= augment;
	r.top -= augment;
	r.height += 2*augment;
	r.width += 2*augment;
}
static void roundAugmentRectangle(const DoubleRect &r0, double augment, std::vector<ComplexShape> &shapes, bool inside) {
	unsigned i;
	DoubleRect r = r0;
	shapes.resize(inside?4:8);
	double radius = fabs(augment);
	augmentRectangle(r, augment);
	for(i=0; i < 4;i++) {
		Segment s;
		CircleArc c;
		s.pt1.x = ((i==0 || i==3) ? r.left : r.left+r.width);
		s.pt1.y = ((i==0 || i==1) ? r.top  : r.top+r.height);
		s.pt2.x = ((i==2 || i==3) ? r.left : r.left+r.width);
		s.pt2.y = ((i==0 || i==3) ? r.top  : r.top+r.height);
		if (!inside) prolongateSegment(s, -augment+minWallDistance/2);
		else prolongateSegment(s, minWallDistance);
		shapes[i].type = CSIT_SEGMENT;
		shapes[i].segment = s;

		if (inside) continue;
		c.circle.radius = radius;
		s.pt1.x = ((i==0 || i==3) ? r0.left : r0.left+r0.width);
		s.pt1.y = ((i==0 || i==1) ? r0.top  : r0.top+r0.height);
		c.circle.center = s.pt1;
		double start, end;
		start = M_PI/2 - i*M_PI/2;
		normalizeAngle(start);
		end = start + M_PI/2;
		normalizeAngle(end);
		c.start = -end;
		c.end   = -start;
		if (fabs(c.end) <= 1e-3) c.end = M_PI*2;
		normalizeAngle(c.start);
		normalizeAngle(c.end);

#if 0
		c.start = -1000;
		c.end = +1000;
#endif

		shapes[i+4].type = CSIT_ARC;
		shapes[i+4].arc = c;
	}
}
bool moveCircleToRectangle(double radius, MoveContext &ctx, const GeomRectangle &r0) {
	DoubleRect r = r0.r;
	std::vector<ComplexShape> shapes;
	bool sign = inRectangle(ctx.vect.pt1, r) && !r0.filled;
	roundAugmentRectangle(r, (sign?-radius:radius), shapes, inRectangle(ctx.vect.pt1, r));
	augmentRectangle(r, radius);
	if (r0.filled && inRoundRectangle(ctx.vect.pt1, r, radius) && !inRoundRectangle(ctx.vect.pt2, r, radius)) {
		return false; /* assume exiting something is not interacting */
	}
	if (r0.filled && inRoundRectangle(ctx.vect.pt1, r, radius) && inRoundRectangle(ctx.vect.pt2, r, radius)) {
		Segment &vect = ctx.vect;
		/* already inside rectangle */
		if (ctx.interaction == IT_GHOST) return true;
		else if (ctx.interaction == IT_CANCEL) {vect.pt2 = vect.pt1;}
		else if (ctx.interaction == IT_SLIDE || ctx.interaction == IT_STICK || ctx.interaction == IT_BOUNCE) {
			Vector2d B = ctx.interaction != IT_STICK ? vect.pt2 : vect.pt1;
			Vector2d O = Vector2d(r.left + r.width/2, r.top + r.height/2);
			Vector2d C;
			if (B.x <= O.x && r.left > minWallDistance) C.x = r.left - minWallDistance; else C.x = r.left + r.width + minWallDistance;
			if (B.y <= O.y && r.top > minWallDistance) C.y = r.top - minWallDistance;  else C.y = r.top + r.height + minWallDistance;
			if (fabs(C.x - B.x) < fabs(C.y - B.y)) C.y = B.y; else C.x = B.x;
			double module = segmentModule(vect);
			vect.pt2 = C;
			if (ctx.interaction == IT_BOUNCE) {
				Vector2d OB = B - O;
				normalizeVector(OB, module);
				ctx.nmove = OB;
			}
		}
		return true;
	}
	for(unsigned i=0; i < shapes.size(); i++) {
		if (pointMovesToComplexShape(ctx, shapes[i])) {
			return true;
		}
	}
	return false;
}
bool moveCircleToCircle(double radius, MoveContext &ctx, const Circle &colli) {
	CircleArc arc;
	arc.circle.center = colli.center;
	arc.circle.radius = radius + colli.radius;
	arc.start = 0;
	arc.end   = M_PI*2+1e-3;
	return pointMovesToCircleArc(ctx, arc);
}

MoveContext::MoveContext(InteractionType interaction0, const Segment &vect0)
	:interaction(interaction0),vect(vect0) {
	nmove = segment2Vector(vect);
}
MoveContext::MoveContext() {
	interaction = IT_GHOST;
	vect.pt1 = Vector2d(0,0);
	vect.pt2 = vect.pt1;
	nmove = Vector2d(0,0);
}
static void test_segments() {
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
	MoveContext ctx(IT_SLIDE, v);
	if (pointMovesToSegment(ctx, s1)) {
		v = ctx.vect;
		fprintf(stderr, "[point moves to segt %lg x %lg]\n", v.pt2.x, v.pt2.y);
	} else {
		fprintf(stderr, "[point doesn't move to segt]\n");
	}
}
static void test_circles() {
	Segment vect;
	Circle circle;
	Vector2d repere;
	Vector2d A;
	circle.center.x = 100;
	circle.center.y = 0;
	circle.radius = 10;
	vect.pt1.x = 0;
	vect.pt1.y = 0;
	vect.pt2.x = 101;
	vect.pt2.y = 5;

	repere.x = 0; repere.y = 10;
	translateSegment(vect, repere);
	circle.center += repere;
	if (circleIntersectsSegment(A, vect, circle)) {
		fprintf(stderr, "[intersection %lg,%lg]\n", A.x, A.y);
	} else {
		fprintf(stderr, "[no intersection between segment and circle]\n");
	}
	fprintf(stderr, "[radius = %lg]\n", pointsDistance(circle.center, A));
}
void test_geometry_cpp() {
	test_segments();
	test_circles();
	/* abort(); */
}
