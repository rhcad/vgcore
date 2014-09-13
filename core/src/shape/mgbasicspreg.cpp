// mgbasicspreg.cpp: 实现基本图形的工厂类 MgBasicShapes
// Copyright (c) 2004-2013, Zhang Yungui
// License: LGPL, https://github.com/rhcad/touchvg

#include "mgbasicspreg.h"
#include "mgshapet.h"
#include "mgcomposite.h"
#include "mgimagesp.h"
#include "mgbasicsps.h"

void MgBasicShapes::registerShapes(MgShapeFactory* factory)
{
    MgShapeT<MgGroup>::registerCreator(factory);
    MgShapeT<MgDot>::registerCreator(factory);
    MgShapeT<MgLine>::registerCreator(factory);
    MgShapeT<MgRect>::registerCreator(factory);
    MgShapeT<MgEllipse>::registerCreator(factory);
    MgShapeT<MgRoundRect>::registerCreator(factory);
    MgShapeT<MgDiamond>::registerCreator(factory);
    MgShapeT<MgParallel>::registerCreator(factory);
    MgShapeT<MgLines>::registerCreator(factory);
    MgShapeT<MgSplines>::registerCreator(factory);
    MgShapeT<MgGrid>::registerCreator(factory);
    MgShapeT<MgImageShape>::registerCreator(factory);
    MgShapeT<MgArc>::registerCreator(factory);
    MgShapeT<MgGrid>::registerCreator(factory);
    MgShapeT<MgPathShape>::registerCreator(factory);
}

int MgEllipse::crossCircle(Point2d& pt1, Point2d& pt2, const MgBaseShape* sp1,
                           const MgBaseShape* sp2, const Point2d& hitpt)
{
    const bool c1 = isCircle(sp1);
    const bool c2 = isCircle(sp2);
    const bool a1 = sp1->isKindOf(MgArc::Type());
    const bool a2 = sp2->isKindOf(MgArc::Type());
    Point2d cen1, cen2;
    float r1 = 0, r2 = 0;
    int n = -1;
    
    if (c1) {
        cen1 = ((MgEllipse*)sp1)->getCenter();
        r1 = ((MgEllipse*)sp1)->getRadiusX();
    }
    if (c2) {
        cen2 = ((MgEllipse*)sp2)->getCenter();
        r2 = ((MgEllipse*)sp2)->getRadiusX();
    }
    if (a1) {
        cen1 = ((MgArc*)sp1)->getCenter();
        r1 = ((MgArc*)sp1)->getRadius();
    }
    if (a2) {
        cen2 = ((MgArc*)sp2)->getCenter();
        r2 = ((MgArc*)sp2)->getRadius();
    }
    
    const bool ca1 = c1 || a1;
    const bool ca2 = c2 || a2;
    const MgBaseShape* line = ca1 ? sp2 : sp1;
    
    if (ca1 && ca2) {
        n = mgcurv::crossTwoCircles(pt1, pt2, cen1, r1, cen2, r2);
    } else if ((ca1 || ca2) && line->isKindOf(MgLine::Type())) {
        n = mgcurv::crossLineCircle(pt1, pt2, line->getPoint(0), line->getPoint(1),
                                    ca1 ? cen1 : cen2, ca1 ? r1 : r2);
    } else if ((ca1 || ca2) && line->isKindOf(MgLines::Type())) {
        int edges = line->getPointCount() - (line->isClosed() ? 0 : 1);
        Point2d pt1r, pt2r;
        float dist = _FLT_MAX;
        int n2;
        
        for (int i = 0; i < edges; i++) {
            n2 = mgcurv::crossLineCircle(pt1r, pt2r, line->getHandlePoint(i),
                                        line->getHandlePoint((i + 1) % line->getPointCount()),
                                        ca1 ? cen1 : cen2, ca1 ? r1 : r2);
            if (n2 > 0) {
                Point2d pt(pt2r.distanceTo(hitpt) < pt1r.distanceTo(hitpt) ? pt2r : pt1r);
                float d = hitpt.distanceTo(pt);
                
                if (dist > d) {
                    dist = d;
                    n = n2;
                    pt1 = pt1r;
                    pt2 = pt2r;
                }
            }
        }
    }
    
    return n;
}

int MgEllipse::crossCircle(Point2d& pt1, Point2d& pt2, const MgBaseShape* sp)
{
    int n = -1;
    
    if (isCircle(sp) && pt1 != pt2) {
        n = mgcurv::crossLineCircle(pt1, pt2, pt1, pt2,
                                    ((MgEllipse*)sp)->getCenter(),
                                    ((MgEllipse*)sp)->getRadiusX());
    }
    else if (sp->isKindOf(MgArc::Type()) && pt1 != pt2) {
        n = mgcurv::crossLineCircle(pt1, pt2, pt1, pt2,
                                    ((MgArc*)sp)->getCenter(),
                                    ((MgArc*)sp)->getRadius());
    }
    return n;
}


static bool drawRect(const MgRect& sp, int, GiGraphics& gs, const GiContext& ctx, int)
{
    return gs.drawPolygon(&ctx, 4, sp.getPoints());
}

static bool drawEllipse(const MgEllipse& sp, int, GiGraphics& gs, const GiContext& ctx, int)
{
    bool ret = false;
    
    if (sp.isOrtho()) {
        ret = gs.drawEllipse(&ctx, sp.getRect());
    }
    else {
        ret = gs.drawBeziers(&ctx, 13, sp.getBeziers(), true);
    }
    
    return ret;
}

static bool drawRoundRect(const MgRoundRect& sp, int, GiGraphics& gs, const GiContext& ctx, int)
{
    if (sp.isOrtho()) {
        return gs.drawRoundRect(&ctx, sp.getRect(), sp.getRadiusX(), sp.getRadiusY());
    } else {
        GiSaveModelTransform xf(&gs.xf(), Matrix2d::rotation(sp.getAngle(), sp.getCenter()));
        return gs.drawRoundRect(&ctx, sp.getRect(), sp.getRadiusX(), sp.getRadiusY());
    }
}

static bool drawDiamond(const MgDiamond& sp, int, GiGraphics& gs, const GiContext& ctx, int)
{
    Point2d pts[] = { sp.getHandlePoint(0), sp.getHandlePoint(1),
        sp.getHandlePoint(2), sp.getHandlePoint(3) };
    return gs.drawPolygon(&ctx, 4, pts);
}

static bool drawArc(const MgArc& sp, int mode, GiGraphics& gs, const GiContext& ctx, int)
{
    bool ret = gs.drawArc(&ctx, sp.getCenter(), sp.getRadius(), 0,
                          sp.getStartAngle(), sp.getSweepAngle());
    if (mode > 0) {
        GiContext ctxln(0, GiColor(0, 126, 0, 64), GiContext::kDashDot);
        gs.drawLine(&ctxln, sp.getCenter(), sp.getStartPoint());
        gs.drawLine(&ctxln, sp.getCenter(), sp.getEndPoint());
        gs.drawLine(&ctxln, sp.getStartPoint(), sp.getStartPoint() + sp.getStartTangent());
        gs.drawLine(&ctxln, sp.getEndPoint(), sp.getEndPoint() + sp.getEndTangent());
    }
    return ret;
}

static bool drawDot(const MgDot& sp, int, GiGraphics& gs, const GiContext& ctx, int)
{
    bool ret = false;
    int type = sp.getPointType();
    
    if (type <= 0) {
        GiContext ctx2(0, GiColor::Invalid(), GiContext::kNullLine,
                       ctx.hasFillColor() ? ctx.getFillColor() : ctx.getLineColor());
        float w = gs.calcPenWidth(1.5f * ctx.getLineWidth(), false);
        
        ret = gs.drawCircle(&ctx2, sp.getPoint(0), gs.xf().displayToModel(w));
    } else {
        ret = gs.drawHandle(sp.getPoint(0), type < kGiHandleCustom ? type - 1 : type);
    }
    
    return ret;
}

static bool drawLine(const MgLine& sp, int, GiGraphics& gs, const GiContext& ctx, int)
{
    return (sp.isRayline() ? gs.drawRayline(&ctx, sp.getPoint(0), sp.getPoint(1))
            : sp.isBeeline() ? gs.drawBeeline(&ctx, sp.getPoint(0), sp.getPoint(1))
            : gs.drawLine(&ctx, sp.getPoint(0), sp.getPoint(1)));
}

static bool drawParallel(const MgParallel& sp, int, GiGraphics& gs, const GiContext& ctx, int)
{
    return gs.drawPolygon(&ctx, 4, sp.getPoints());
}

static bool drawLines(const MgLines& sp, int, GiGraphics& gs, const GiContext& ctx, int)
{
    return (sp.isClosed() ? gs.drawPolygon(&ctx, sp.getPointCount(), sp.getPoints())
            : gs.drawLines(&ctx, sp.getPointCount(), sp.getPoints()));
}

static bool drawSplines(const MgSplines& sp, int, GiGraphics& gs, const GiContext& ctx, int)
{
    int n = sp.getPointCount();
    
    if (n == 2) {
        return gs.drawLine(&ctx, sp.getPoint(0), sp.getPoint(1));
    }
    if (sp.getVectors()) {
        return gs.drawBeziers(&ctx, n, sp.getPoints(), sp.getVectors(), sp.isClosed());
    }
    return gs.drawQuadSplines(&ctx, n, sp.getPoints(), sp.isClosed());
}

static bool drawPath(const MgPathShape& sp, int mode, GiGraphics& gs, const GiContext& ctx, int)
{
    return gs.drawPath(&ctx, sp.pathc(), mode == 0);
}

static bool drawGrid(const MgGrid& sp, int, GiGraphics& gs, const GiContext& ctx, int)
{
    Vector2d cell(sp.getCellSize() / 2);
    
    if (!sp.isValid(_MGZERO)) {
        Box2d rect(sp.getRect());
        
        GiContext ctxedge(ctx);
        ctxedge.setNoFillColor();
        
        gs.drawRect(&ctxedge, rect);
        
        if (!sp.getCellSize().isZeroVector()) {
            GiContext ctxerr(ctx);
            ctxerr.setLineColor(255, 0, 0);
            ctxerr.setLineStyle(GiContext::kDashLine);
            ctxerr.setLineWidth(0, false);
            
            gs.drawLine(&ctxerr, rect.leftTop(), rect.rightBottom());
            gs.drawLine(&ctxerr, rect.leftBottom(), rect.rightTop());
        }
        return !rect.isEmpty();
    }
    
    int nx = (int)(sp.getWidth() / cell.x + _MGZERO);
    int ny = (int)(sp.getHeight() / cell.y + _MGZERO);
    Box2d rect(sp.getPoint(3), sp.getPoint(3)
               + Vector2d((float)(cell.x * nx), (float)(cell.y * ny)));
    
    float w = gs.calcPenWidth(ctx.getLineWidth(), ctx.isAutoScale()) / -2.f;
    GiContext ctxgrid(w, ctx.getLineColor());
    
    int ret = gs.drawRect(&ctxgrid, rect) ? 1 : 0;
    
    bool switchx = (nx >= 10 && cell.x < gs.xf().displayToModel(20, true));
    bool switchy = (ny >= 10 && cell.y < gs.xf().displayToModel(20, true));
    Point2d pts[2] = { rect.leftTop(), rect.leftBottom() };
    
    for (int i = 1; i < nx; i++) {
        pts[0].x += cell.x;
        pts[1].x += cell.x;
        ctxgrid.setLineWidth(!switchx || i%5 > 0 ? w/2 : w, false);
        ctxgrid.setLineAlpha(-w < 0.9f && (!switchx || i%5 > 0) ?
                             ctx.getLineAlpha() / 2 : ctx.getLineAlpha());
        ret += gs.drawLine(&ctxgrid, pts[0], pts[1]) ? 1 : 0;
    }
    
    pts[0] = rect.leftBottom();
    pts[1] = rect.rightBottom();
    for (int j = 1; j < ny; j++) {
        pts[0].y += cell.y;
        pts[1].y += cell.y;
        ctxgrid.setLineWidth(!switchy || j%5 > 0 ? w/2 : w, false);
        ctxgrid.setLineAlpha(-w < 0.9f && (!switchy || j%5 > 0) ?
                             ctx.getLineAlpha() / 2 : ctx.getLineAlpha());
        ret += gs.drawLine(&ctxgrid, pts[0], pts[1]) ? 1 : 0;
    }
    
    return ret > 0;
}

bool MgShape::drawShape(const MgBaseShape& sp, int mode, GiGraphics& gs, const GiContext& ctx, int segment)
{
    switch (sp.getType()) {
        case kMgShapeGrid:
            return drawGrid((MgGrid&)sp, mode, gs, ctx, segment);
        case kMgShapeDot:
            return drawDot((MgDot&)sp, mode, gs, ctx, segment);
        case kMgShapeLine:
            return drawLine((MgLine&)sp, mode, gs, ctx, segment);
        case kMgShapeRect:
            return drawRect((MgRect&)sp, mode, gs, ctx, segment);
        case kMgShapeEllipse:
            return drawEllipse((MgEllipse&)sp, mode, gs, ctx, segment);
        case kMgShapeRoundRect:
            return drawRoundRect((MgRoundRect&)sp, mode, gs, ctx, segment);
        case kMgShapeDiamond:
            return drawDiamond((MgDiamond&)sp, mode, gs, ctx, segment);
        case kMgShapeParallel:
            return drawParallel((MgParallel&)sp, mode, gs, ctx, segment);
        case kMgShapeLines:
            return drawLines((MgLines&)sp, mode, gs, ctx, segment);
        case kMgShapeSplines:
            return drawSplines((MgSplines&)sp, mode, gs, ctx, segment);
        case kMgShapeArc:
            return drawArc((MgArc&)sp, mode, gs, ctx, segment);
        case kMgShapePath:
            return drawPath((MgPathShape&)sp, mode, gs, ctx, segment);
        default:
            return sp.draw(mode, gs, ctx, segment);
    }
}
