#include "geometry.h"
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/Transform.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/ConvexShape.hpp>
#include <math.h>
#include <vector>
#include <stdio.h>
#include "parameters.h"
#include <algorithm>

using namespace sf;

#ifdef DEBUG
static void dispLine(const Line &line) {
	fprintf(stderr, "[line %g*x+%g*y+%g = 0]\n", line.a, line.b, line.c);
}
static void dispPoint(Vector2d pt) {
	fprintf(stderr, "[point %g %g]\n", pt.x, pt.y);
}
#endif

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
	if (module <= 1e-10) return;
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
static double trigoAngleFromVector(const Vector2d &vect) { /* oriented */
	return angle_from_dxdy(vect.x, vect.y);
}
static bool isTrigoDirect(const Vector2d &pt1, const Vector2d &pt2, const Vector2d &pt3) {
	double a1 = trigoAngleFromVector(pt2 - pt1), a2 = trigoAngleFromVector(pt3 - pt1);
	double a = a2 - a1;
	normalizeAngle(a);
	return a < M_PI;
}

static bool orthoProjectOnLine(Vector2d &res, const Line &line, const Vector2d &pt) {
	return intersectLines(res, orthoLine(line, pt), line);
}
static bool orthoProjectOnSegment(Vector2d &res0, const Segment &segt, const Vector2d &pt) {
	Vector2d res;
	if (!orthoProjectOnLine(res, segt.toLine(), pt)) return false;
	if (!isLPointOnSegment(res, segt)) return false;
	res0 = res;
	return true;
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
static bool angleBetween(double a, double start, double end) {
	normalizeAngle(a);
	double ln = end - start;
	if (fabs(ln - 2*M_PI)<2e-6) return true; /* complete circle included */
	normalizeAngle(ln);
	double xa = a-start;
	normalizeAngle(xa);
	return xa >= 0 && xa <= ln;
}
static bool inDiscusArc(const CircleArc &arc, const Vector2d &pt) {
	if (!inCircle(pt, arc.circle)) return false;
	double a = trigoAngleFromVector(pt - arc.circle.center);
	return angleBetween(a, arc.start, arc.end);
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
	/* interaction == IT_STICK or interaction == IT_SLIDE */
	Vector2d proj;
	if (orthoProjectOnLine(proj, wall, I)) {
		Vector2d v = I - proj; /* vector orthogonal to wall, that moves the point out of the wall */
		if (vectorModule(v) >= parameters.minWallDistance()*0.2) {
			normalizeVector(v, parameters.minWallDistance()*0.2);
		}
		vect.pt2 += v;
	}
	return true;
}
static bool ghostlike(const MoveContext &ctx) {
	return ctx.interaction == IT_GHOST || ctx.interaction == IT_CANCEL;
}
static Vector2d repulseFromCircle(const Vector2d &pt, const Circle &circle, Vector2d &outvect, double extra_repulsion) {
	outvect = pt - circle.center;
	normalizeVector(outvect, circle.radius+extra_repulsion);
	return circle.center+outvect;
}
static bool pointMovesToCircleArc(MoveContext &ctx, const CircleArc &arc) { /* oriented segment */
	Segment &vect = ctx.vect;
	Segment tvect;
	Vector2d A;
	const Circle &circle0 = arc.circle;
	Circle circle = circle0;
	circle.radius = circle0.radius+parameters.minWallDistance()*0.9;

	if (circle.filled && inCircle(vect.pt1, circle) && !inCircle(vect.pt2, circle)) {
		return false; /* assume exiting something is not interacting */
	}
	if (circle.filled && inCircle(vect, circle0)) {
		/* already inside discus */
		Segment OA;
		OA.pt1 = circle.center;
		OA.pt2 = vect.pt2;
		double angle = trigoAngleFromSegment(OA);
		if (angleBetween(angle, arc.start, arc.end)) {
			if (ctx.interaction == IT_GHOST) {
				return true;
			} else if (ctx.interaction == IT_CANCEL) {
				vect.pt2 = vect.pt1;
			}
			else if (ctx.interaction == IT_SLIDE || ctx.interaction == IT_STICK || ctx.interaction == IT_BOUNCE) {
				Vector2d outvect;
				double module = segmentModule(vect);
				vect.pt2 = repulseFromCircle((ctx.interaction != IT_STICK ? vect.pt2 : vect.pt1)
								, circle, outvect, 0.2*parameters.minWallDistance());
				if (ctx.interaction == IT_BOUNCE) {
					normalizeVector(outvect, module);
					ctx.nmove = outvect;
				}
#if 0
				Vector2d OB = (ctx.interaction != IT_STICK ? vect.pt2 : vect.pt1) - circle.center;
				double module = segmentModule(vect);
				normalizeVector(OB, circle0.radius+parameters.minWallDistance()*1.1);
				Vector2d A;
				translateSegment(vect, circle.center + OB - vect.pt2);
				if (ctx.interaction == IT_BOUNCE) {
					normalizeVector(OB, module);
					ctx.nmove = OB;
				}
#endif
			}
			return true;
		}
	}
	tvect = vect;
	if (!ghostlike(ctx)) {
		double mradius = circle0.radius+parameters.minWallDistance();
		if (vectorModule(tvect.pt1 - circle.center) < mradius) {
			Vector2d OP = tvect.pt1 - circle.center;
			normalizeVector(OP, mradius);
			translateSegment(tvect, circle.center + OP - tvect.pt1);
		}
	}
	
	if (!circleIntersectsSegment(A, tvect, circle)) return false; /* main collision detection */
	
	Segment AB, OA;
	OA.pt2 = A; OA.pt1 = circle.center;
	double angle = trigoAngleFromSegment(OA);
	
	if (!angleBetween(angle, arc.start, arc.end)) return false; /* circle arc check */
	
	vect = tvect;
	
	return pointMovesAgainstWall(ctx, orthoLine(OA.toLine(), A), A);
}
static bool pointMovesToSegment(MoveContext &ctx, const Segment &segt0, bool trigoDirect) {
	Segment &vect = ctx.vect;
	Segment tvect;
	Vector2d A, B, C;
	Vector2d proj;
	Segment segt = segt0;
	double mwd = parameters.minWallDistance();
	if (segmentModule(vect) < 1e-6) return false;
	if (!(isTrigoDirect(segt0.pt1, segt0.pt2, ctx.vect.pt1) != trigoDirect
		&& isTrigoDirect(segt0.pt1, segt0.pt2, ctx.vect.pt2) == trigoDirect)) {
			return false;/* doesn't move inside solid half-plane */
	}
	
	A = ctx.vect.pt1;
	orthoProjectOnLine(proj, segt.toLine(), A);
	
	Vector2d bounce_vect = A - proj;
	normalizeVector(bounce_vect, mwd*0.9);
	tvect = vect;
	if (vectorModule(vect.pt1 - proj) <= mwd && !ghostlike(ctx)) { /* the point is already too close to segment */
		Vector2d xbounce_vect = bounce_vect;
		normalizeVector(xbounce_vect, mwd);
		translateSegment(tvect, proj + xbounce_vect - tvect.pt1);
	}
	
	translateSegment(segt, bounce_vect); /* makes segt virtually closer to point, in order to implement minWallDistance */
	if (!intersectSegments(A, tvect, segt)) {
		return false;
	}
	vect = tvect;
	return pointMovesAgainstWall(ctx, segt.toLine(), A);
}
static bool pointMovesToComplexShape(MoveContext &ctx, const ComplexShape &shape) {
	if (shape.type == CSIT_ARC) return pointMovesToCircleArc(ctx, shape.arc);
	else if (shape.type == CSIT_SEGMENT) return pointMovesToSegment(ctx, shape.segment, shape.trigoDirect);
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
static void roundAugmentHorizRectangle(const DoubleRect &r0, double augment, std::vector<ComplexShape> &shapes, bool inside) {
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
		if (!inside) prolongateSegment(s, -augment + parameters.minWallDistance()*0.1);
		else prolongateSegment(s, parameters.minWallDistance()*1.1);
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

typedef std::vector<Vector2d> Polygon;
/* polygons are convex. Last point is implicitly connected to first */
/* consequently, a triangle as a size() of 3 */
Vector2d barycenter(const Polygon &poly) {
	Vector2d c(0,0);
	for(size_t i=0; i < poly.size(); i++) {
		c += poly[i];
	}
	return c*(1.0/poly.size());
}
static bool repulseSegment(Segment &segt, Vector2d &outvect, const Vector2d from_pt, double repulseDistance) {
	/* on output, repulse segt and set outvect to the repulsion vector */
	Vector2d proj;
	if (!orthoProjectOnLine(proj, segt.toLine(), from_pt)) {
		fprintf(stderr, "Unexpected error on poly segt %gx%g-%gx%g\n"
			,segt.pt1.x, segt.pt1.y
			,segt.pt2.x, segt.pt2.y);
		return false;
	}
	outvect = proj - from_pt;
	normalizeVector(outvect, repulseDistance);
	Segment osegt = segt;
	translateSegment(segt, outvect);
	return true;
}
static void drawCS(RenderTarget &target, const std::vector<ComplexShape> &shapes) {
	for(size_t i=0; i < shapes.size(); i++) {
		const ComplexShape &shape = shapes[i];
		if (shape.type == CSIT_SEGMENT) {
			const Segment &segt = shape.segment;
			ConvexShape line(4);
			line.setPoint(0, Vector2f(segt.pt1.x-2, segt.pt1.y-2));
			line.setPoint(1, Vector2f(segt.pt2.x-2, segt.pt2.y-2));
			line.setPoint(2, Vector2f(segt.pt2.x+2, segt.pt2.y+2));
			line.setPoint(3, Vector2f(segt.pt1.x+2, segt.pt1.y+2));
			
			line.setFillColor(Color(255,0,0));
			line.setOutlineColor(Color(0,0,255));
			line.setOutlineThickness(2);
			target.draw(line);
		} else if (shape.type == CSIT_ARC) {
			size_t cnt = 10;
			ConvexShape lines(cnt);
			lines.setFillColor(Color::Transparent);
			lines.setOutlineColor(Color(0,0,255));
			lines.setOutlineThickness(2);
			
			const CircleArc &arc = shape.arc;
			Vector2d center = arc.circle.center;
			double radius = arc.circle.radius;
			for(size_t i=0; i < cnt; i++) {
				double a1 = arc.start + (arc.end - arc.start)*i/(cnt-1);
				lines.setPoint(i, Vector2f(center.x+cos(a1)*radius, center.y+sin(a1)*radius));
			}
			target.draw(lines);
		}
	}
}
static void roundAugmentPolygon(const Polygon &poly, double augment, std::vector<ComplexShape> &shapes) {
	shapes.clear();
	if (poly.size() <= 2 || vectorModule(poly[0] - poly[1]) <= 1e-6) return;
	shapes.reserve(3*poly.size());
	Vector2d bcenter = barycenter(poly);
	size_t sz = poly.size();
	bool trigoDirect = isTrigoDirect(poly[0], poly[1], poly[2]); /* the whole polygon is completely drawn either in direct or in inverse trigo direction */
	
	for(size_t i=0; i < poly.size(); i++) {
		const Vector2d &pt1 = poly[i], &pt2 = poly[(i+1)%sz], &pt3 = poly[(i+2)%sz];
		Segment segt1, segt2;
		Vector2d outvect1, outvect2;
		ComplexShape shape;
		
		segt1.pt1 = pt1;
		segt1.pt2 = pt2;
		segt2.pt1 = pt2;
		segt2.pt2 = pt3;
		
		if (!repulseSegment(segt1, outvect1, bcenter, augment)) continue;
		if (!repulseSegment(segt2, outvect2, bcenter, augment)) continue;
		shape.type = CSIT_SEGMENT;
		shape.segment = segt1;
		prolongateSegment(shape.segment, parameters.minWallDistance()*0.1);
		shape.trigoDirect = trigoDirect;
		shapes.push_back(shape);
		
		/* now, circular transition */
		shape.type = CSIT_ARC;
		shape.trigoDirect = trigoDirect;
		CircleArc &arc = shape.arc;
		arc.circle.center = pt2;
		arc.circle.radius = augment;
		arc.start = trigoAngleFromVector(outvect1);
		arc.end   = trigoAngleFromVector(outvect2);
		if (!trigoDirect) std::swap(arc.start, arc.end);
		if (arc.end < arc.start) arc.end += 2*M_PI;
		shapes.push_back(shape);
		
		/* as well as linear transition (to make figure an included convex polygon) */
		shape.type = CSIT_SEGMENT;
		shape.segment.pt1 = segt1.pt2;
		shape.segment.pt2 = segt2.pt1;
		shape.trigoDirect = trigoDirect;
		shapes.push_back(shape);
		
#if 0
		shape.trigoDirect = trigoDirect;
		shape.type = CSIT_SEGMENT;
		shape.segment = segt2;
		shapes.push_back(shape);
#endif
	}
	return;
}
static bool inLineShadow(const ComplexShape &shape, const Vector2d &pt) {
	const Segment &segt = shape.segment;
	return isTrigoDirect(segt.pt1, segt.pt2, pt) == shape.trigoDirect; /* Point must be inside line-delimited half-plane on the correct side */
}
#if 0
static bool inSegmentShadow(const ComplexShape &shape, const Vector2d &pt) {
	if (!inLineShadow(shape, pt)) return false; /* Point must be inside line-delimited half-plane on the correct side */
	return orthoProjectOnSegment(proj, shape.segment, pt); /* And must be projected on segment */
}
#endif
static bool inComplexShape(const std::vector<ComplexShape> &shapes, const Vector2d &pt) {
	bool inIncludedPolygon = true;
	for(size_t i=0; i < shapes.size(); i++) {
		const ComplexShape &shape =shapes[i];
		if (shape.type == CSIT_SEGMENT && !inLineShadow(shape, pt)) {
			inIncludedPolygon = false;
		} else if (shape.type == CSIT_ARC && inDiscusArc(shape.arc, pt)) {
			return true;
		}
	}
	return inIncludedPolygon;
}

static bool inGRectangle(const GeomRectangle &r0) {
}
static void Rectangle2Polygon(const GeomRectangle &r0, Polygon &poly) {
	const DoubleRect &r=r0.r;
	poly.resize(4);
	Vector2f pt0(r.left, r.top);
	Transform rot;
	/*rot.translate(-pt0);*/
	rot.rotate(180/M_PI*r0.angle);
	/*rot.translate(pt0);*/
	
	for(size_t i=0; i < 4; i++) { /* notice: direct trigo order used here */
		Vector2f p;
		p.x = (i == 0 || i == 3) ? 0 : r.width;
		p.y = (i == 0 || i == 1) ? 0 : r.height;
		p = rot.transformPoint(p);
		p += pt0;
		poly[i] = Vector2d(p.x, p.y);
	}
}
static void roundAugmentRectangle(const GeomRectangle &r0, double augment, std::vector<ComplexShape> &shapes, bool inside) {
	if (inside) {
		roundAugmentHorizRectangle(r0.r, augment, shapes, inside);
	} else {
		Polygon poly;
		Rectangle2Polygon(r0, poly);
		roundAugmentPolygon(poly, augment, shapes);
	}
}
static Vector2d repulseFromComplexShape(const Vector2d pt, std::vector<ComplexShape> &shapes, Vector2d &outvect, double extra_distance) {
	/* get pt out of shape with the shortest orthogonal path */
	Vector2d out = pt;
	double outd = INFINITY;
	for(size_t i=0; i < shapes.size(); i++) {
		const ComplexShape &shape = shapes[i];
		Vector2d proj;
		if (shape.type == CSIT_SEGMENT && orthoProjectOnSegment(proj, shape.segment, pt)) {
			if (vectorModule(proj - pt) < outd) {
				for(size_t j=0; j < shapes.size(); j++) {
					const ComplexShape &xshape = shapes[j];
					if (xshape.type == CSIT_ARC && inDiscusArc(xshape.arc, proj)) {
						Vector2d ovect;
						proj = repulseFromCircle(proj, xshape.arc.circle, ovect, extra_distance);
					}
				}
			}
			if (vectorModule(proj - pt) < outd) {
				Vector2d outvect = proj - pt;
				outd = vectorModule(proj - pt);
				normalizeVector(outvect, outd+extra_distance);
				out = pt + outvect;
			}
		}
	}
	return out;
}
void drawGeomRectangle(RenderTarget &target, const GeomRectangle &geom, double augment) {
	std::vector<ComplexShape> shapes;
	roundAugmentRectangle(geom, augment, shapes, false);
	drawCS(target, shapes);
}
bool moveCircleToRectangle(double radius, MoveContext &ctx, const GeomRectangle &r0) {
	DoubleRect r = r0.r;
	std::vector<ComplexShape> shapes;
#if 0
	bool sign = inRectangle(ctx.vect.pt1, r) && !r0.filled;
	roundAugmentRectangle(r, (sign?-radius:radius), shapes, inRectangle(ctx.vect.pt1, r));
	augmentRectangle(r, radius);
#endif
	roundAugmentRectangle(r0, (r0.filled?radius:-radius), shapes, !r0.filled);
	if (shapes.size() == 0) return false; /* zero-sized object */
	if (r0.filled) {
	bool pt1belongs = inComplexShape(shapes, ctx.vect.pt1);
	bool pt2belongs = inComplexShape(shapes, ctx.vect.pt2);
	if (pt1belongs && !pt2belongs) {
		return false; /* assume exiting something is not interacting */
	}
	if (pt1belongs && pt2belongs) {
		Segment &vect = ctx.vect;
		/* already inside rectangle */
		if (ctx.interaction == IT_GHOST) return true;
		else if (ctx.interaction == IT_CANCEL) {vect.pt2 = vect.pt1;}
		else if (ctx.interaction == IT_SLIDE || ctx.interaction == IT_STICK || ctx.interaction == IT_BOUNCE) {
			Vector2d outvect;
			double module = segmentModule(vect);
			vect.pt2 = repulseFromComplexShape(vect.pt2, shapes, outvect, parameters.minWallDistance());
			if (ctx.interaction == IT_BOUNCE) {
				normalizeVector(outvect, module);
				ctx.nmove = outvect;
			}
		}
		return true;
	}
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
	arc.end   = 2*M_PI+1e-6;
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
	v.pt2.x = 110;
	v.pt2.y = 10;
	MoveContext ctx(IT_SLIDE, v);
	if (pointMovesToSegment(ctx, s1, true)) {
		v = ctx.vect;
		fprintf(stderr, "[point moves to segt %g x %g]\n", v.pt2.x, v.pt2.y);
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
		fprintf(stderr, "[intersection %g,%g]\n", A.x, A.y);
	} else {
		fprintf(stderr, "[no intersection between segment and circle]\n");
	}
	fprintf(stderr, "[radius = %g]\n", pointsDistance(circle.center, A));
}
void test_geometry_cpp() {
	test_segments();
	test_circles();
}
