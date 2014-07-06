// mgpathsp.cpp: 实现路径图形 MgPathShape
// Copyright (c) 2004-2014, Zhang Yungui
// License: LGPL, https://github.com/touchvg/vgcore

#include "mgpathsp.h"
#include "mgshape_.h"
#include <sstream>
#include <stdlib.h>

MG_IMPLEMENT_CREATE(MgPathShape)

MgPathShape::MgPathShape()
{
}

MgPathShape::~MgPathShape()
{
}

int MgPathShape::_getPointCount() const
{
    return _path.getCount();
}

Point2d MgPathShape::_getPoint(int index) const
{
    return _path.getPoint(index);
}

void MgPathShape::_setPoint(int index, const Point2d& pt)
{
    _path.setPoint(index, pt);
}

void MgPathShape::_copy(const MgPathShape& src)
{
    _path.copy(src._path);
    __super::_copy(src);
}

bool MgPathShape::_equals(const MgPathShape& src) const
{
    if (_path.getCount() != src._path.getCount() || !__super::_equals(src)) {
        return false;
    }
    for (int i = 0; i < _path.getCount(); i++) {
        if (_path.getPoint(i) != src._path.getPoint(i) ||
            _path.getNodeType(i) != src._path.getNodeType(i)) {
            return false;
        }
    }
    return true;
}

void MgPathShape::_update()
{
    _extent.set(_path.getCount(), _path.getPoints());
    __super::_update();
}

void MgPathShape::_transform(const Matrix2d& mat)
{
    for (int i = 0; i < _path.getCount(); i++) {
        _path.setPoint(i, _path.getPoint(i) * mat);
    }
    __super::_transform(mat);
}

void MgPathShape::_clear()
{
    _path.clear();
    __super::_clear();
}

bool MgPathShape::_isClosed() const
{
    return !!(_path.getNodeType(_path.getCount() - 1) & kGiCloseFigure);
}

bool MgPathShape::isCurve() const
{
    for (int i = 0; i < _path.getCount(); i++) {
        if (_path.getNodeType(i) & (kGiBezierTo | kGiQuadTo))
            return true;
    }
    return false;
}

float MgPathShape::_hitTest(const Point2d& pt, float tol, MgHitResult& res) const
{
    int n = _path.getCount();
    const Point2d* pts = _path.getPoints();
    const char* types = _path.getTypes();
    Point2d pos, ends, bz[7], nearpt;
    bool err = false;
    const Box2d rect (pt, 2 * tol, 2 * tol);
    
    res.dist = _FLT_MAX - tol;
    for (int i = 0; i < n && !err; i++) {
        float dist = _FLT_MAX;
        pos = ends;
        
        switch (types[i] & ~kGiCloseFigure) {
            case kGiMoveTo:
                ends = pts[i];
                break;
                
            case kGiLineTo:
                ends = pts[i];
                if (rect.isIntersect(Box2d(pos, ends))) {
                    dist = mglnrel::ptToLine(pos, ends, pt, nearpt);
                }
                break;
                
            case kGiBezierTo:
                if (i + 2 >= n) {
                    err = true;
                    break;
                }
                bz[0] = pos;
                bz[1] = pts[i];
                bz[2] = pts[i+1];
                bz[3] = pts[i+2];
                ends = bz[3];
                
                if (rect.isIntersect(mgnear::bezierBox1(bz))) {
                    dist = mgnear::nearestOnBezier(pt, bz, nearpt);
                }
                i += 2;
                break;
                
            case kGiQuadTo:
                if (i + 1 >= n) {
                    err = true;
                    break;
                }
                bz[0] = pos;
                bz[1] = pts[i];
                bz[2] = pts[i+1];
                ends = bz[2];
                
                mgcurv::quadBezierToCubic(bz, bz + 3);
                if (rect.isIntersect(mgnear::bezierBox1(bz + 3))) {
                    dist = mgnear::nearestOnBezier(pt, bz, nearpt);
                }
                i++;
                break;
                
            default:
                err = true;
                break;
        }
        if (res.dist > dist) {
            res.dist = dist;
            res.segment = i;
        }
    }
    
    return res.dist;
}

bool MgPathShape::_hitTestBox(const Box2d& rect) const
{
    if (!__super::_hitTestBox(rect))
        return false;
    
    int n = _path.getCount();
    const Point2d* pts = _path.getPoints();
    const char* types = _path.getTypes();
    Point2d pos, ends, bz[7];
    int ret = 0;
    
    for (int i = 0; i < n && !ret; i++) {
        pos = ends;
        
        switch (types[i] & ~kGiCloseFigure) {
            case kGiMoveTo:
                ends = pts[i];
                break;
                
            case kGiLineTo:
                ends = pts[i];
                ret = rect.isIntersect(Box2d(pos, ends));
                break;
                
            case kGiBezierTo:
                if (i + 2 >= n) {
                    ret |= 2;
                    break;
                }
                bz[0] = pos;
                bz[1] = pts[i];
                bz[2] = pts[i+1];
                bz[3] = pts[i+2];
                ends = bz[3];
                ret |= rect.isIntersect(mgnear::bezierBox1(bz)) ? 1 : 0;
                i += 2;
                break;
                
            case kGiQuadTo:
                if (i + 1 >= n) {
                    ret |= 2;
                    break;
                }
                bz[0] = pos;
                bz[1] = pts[i];
                bz[2] = pts[i+1];
                ends = bz[2];
                mgcurv::quadBezierToCubic(bz, bz + 3);
                ret |= rect.isIntersect(mgnear::bezierBox1(bz + 3)) ? 1 : 0;
                i++;
                break;
                
            default:
                ret |= 2;
                break;
        }
    }
    
    return ret == 1;
}

bool MgPathShape::_draw(int mode, GiGraphics& gs, const GiContext& ctx, int segment) const
{
    bool ret = gs.drawPath(&ctx, _path, mode == 0);
    return __super::_draw(mode, gs, ctx, segment) || ret;
}

bool MgPathShape::_save(MgStorage* s) const
{
    bool ret = __super::_save(s);
    std::stringstream ss;
    
    int n = _path.getCount();
    const Point2d* pts = _path.getPoints();
    const char* types = _path.getTypes();
    char cmd = 0;
    
    for (int i = 0; i < n; i++) {
        switch (types[i] & ~kGiCloseFigure) {
            case kGiMoveTo:
                ss << "M" << pts[i].x << " " << pts[i].y << " ";
                cmd = 'L';
                break;
                
            case kGiLineTo:
                if (cmd != 'L') {
                    cmd = 'L';
                    ss << cmd;
                }
                ss << pts[i].x << " " << pts[i].y << " ";
                break;
                
            case kGiBezierTo:
                if (i + 2 >= n) {
                    break;
                }
                cmd = 'C';
                ss << cmd << pts[i].x << " " << pts[i].y;
                ss << " " << pts[i+1].x << " " << pts[i+1].y;
                ss << " " << pts[i+2].x << " " << pts[i+2].y << " ";
                i += 2;
                break;
                
            case kGiQuadTo:
                if (i + 1 >= n) {
                    break;
                }
                cmd = 'Q';
                ss << cmd << pts[i].x << " " << pts[i].y;
                ss << " " << pts[i+1].x << " " << pts[i+1].y << " ";
                i++;
                break;
                
            default:
                break;
        }
        if (types[i] & kGiCloseFigure) {
            cmd = 'Z';
            ss << cmd;
        }
    }
    
    s->writeString("d", ss.str().c_str());
    
    return ret;
}

static void parsePath(const char* s, GiPath& path);

bool MgPathShape::_load(MgShapeFactory* factory, MgStorage* s)
{
    bool ret = __super::_load(factory, s);
    int len = s->readString("d", NULL, 0);
    
    if (!ret || len < 1)
        return false;
    
    char *buf = new char[1 + len];
    
    len = s->readString("d", buf, len);
    buf[len] = 0;
    ret = importSVGPath(buf);
    
    delete[] buf;
    
    return ret;
}

bool MgPathShape::importSVGPath(const char* d)
{
    _path.clear();
    parsePath(d, _path);
    return true;
}

// parsePath() is based on nanosvg.h (https://github.com/memononen/nanosvg)
// NanoSVG is a simple stupid single-header-file SVG parse.
// Copyright (c) 2013-14 Mikko Mononen memon@inside.org

static int nsvg__isspace(char c)
{
	return strchr(" \t\n\v\f\r", c) != 0;
}

static int nsvg__isdigit(char c)
{
	return strchr("0123456789", c) != 0;
}

static int nsvg__isnum(char c)
{
	return strchr("0123456789+-.eE", c) != 0;
}

static const char* nsvg__getNextPathItem(const char* s, char* it)
{
	int i = 0;
	it[0] = '\0';
	// Skip white spaces and commas
	while (*s && (nsvg__isspace(*s) || *s == ',')) s++;
	if (!*s) return s;
	if (*s == '-' || *s == '+' || nsvg__isdigit(*s)) {
		// sign
		if (*s == '-' || *s == '+') {
			if (i < 63) it[i++] = *s;
			s++;
		}
		// integer part
		while (*s && nsvg__isdigit(*s)) {
			if (i < 63) it[i++] = *s;
			s++;
		}
		if (*s == '.') {
			// decimal point
			if (i < 63) it[i++] = *s;
			s++;
			// fraction part
			while (*s && nsvg__isdigit(*s)) {
				if (i < 63) it[i++] = *s;
				s++;
			}
		}
		// exponent
		if (*s == 'e' || *s == 'E') {
			if (i < 63) it[i++] = *s;
			s++;
			if (*s == '-' || *s == '+') {
				if (i < 63) it[i++] = *s;
				s++;
			}
			while (*s && nsvg__isdigit(*s)) {
				if (i < 63) it[i++] = *s;
				s++;
			}
		}
		it[i] = '\0';
	} else {
		// Parse command
		it[0] = *s++;
		it[1] = '\0';
		return s;
	}
    
	return s;
}

static int nsvg__getArgsPerElement(char cmd)
{
	switch (cmd) {
		case 'v':
		case 'V':
		case 'h':
		case 'H':
			return 1;
		case 'm':
		case 'M':
		case 'l':
		case 'L':
		case 't':
		case 'T':
			return 2;
		case 'q':
		case 'Q':
		case 's':
		case 'S':
			return 4;
		case 'c':
		case 'C':
			return 6;
		case 'a':
		case 'A':
			return 7;
	}
	return 0;
}

static void parsePath(const char* s, GiPath& path)
{
    char item[64];
    char cmd = 0;
	float args[10];
	int nargs = 0;
	int rargs = 0;
    
    while (*s) {
        s = nsvg__getNextPathItem(s, item);
        if (!*item) break;
        if (!nsvg__isnum(item[0])) {
            cmd = item[0];
            rargs = nsvg__getArgsPerElement(cmd);
            nargs = 0;
        } else {
            if (nargs < 10)
                args[nargs++] = (float)atof(item);
            if (nargs >= rargs) {
                switch (cmd) {
                    case 'm':
                    case 'M':
                        path.moveTo(Point2d(args[0], args[1]));
                        // Moveto can be followed by multiple coordinate pairs,
                        // which should be treated as linetos.
                        cmd = (cmd == 'm') ? 'l' : 'L';
                        rargs = nsvg__getArgsPerElement(cmd);
                        break;
                    case 'l':
                    case 'L':
                        path.lineTo(Point2d(args[0], args[1]));
                        break;
                    case 'H':
                    case 'h':
                        break;
                    case 'V':
                    case 'v':
                        break;
                    case 'C':
                    case 'c':
                        path.bezierTo(Point2d(args[0], args[1]),
                                      Point2d(args[2], args[3]),
                                      Point2d(args[4], args[5]));
                        break;
                    case 'S':
                    case 's':
                        break;
                    case 'Q':
                    case 'q':
                        path.quadTo(Point2d(args[0], args[1]),
                                    Point2d(args[2], args[3]));
                        break;
                    case 'T':
                    case 't':
                        break;
                    case 'A':
                    case 'a':
                        path.arcTo(Point2d(args[0], args[1]));
                        break;
                    default:
                        break;
                }
                nargs = 0;
            }
        }
    }
}
