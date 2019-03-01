#ifndef SMARTCTRLPOINT_H
#define SMARTCTRLPOINT_H
#include "movablepoint.h"
class SmartNodePoint;
class SmartCtrlPoint : public NonAnimatedMovablePoint {
    friend class StdSelfRef;
public:
    enum Type { C0, C2 };
    void moveByAbs(const QPointF &absTranslatione);
    void setRelativePos(const QPointF &relPos);
    void moveToAbs(const QPointF &absPos);
    //void moveByWithoutUpdatingTheOther(QPointF absTranslation);
    void moveByRel(const QPointF &relTranslation);
    void startTransform();
    void finishTransform();
    void cancelTransform();

    void removeFromVectorPath();

    bool isHidden() const;
    void scale(const qreal &sx, const qreal &sy);

    void moveToAbsWithoutUpdatingTheOther(const QPointF &absPos);
    void setOtherCtrlPt(SmartCtrlPoint * const ctrlPt);
    void rotate(const qreal &rotate);
protected:
    SmartCtrlPoint(SmartNodePoint * const parentPoint,
                   const Type &type);
private:
    const Type mCtrlType;
    SmartNodePoint* const mParentPoint_k;
    stdptr<SmartCtrlPoint> mOtherCtrlPt_cv;
};

#endif // SMARTCTRLPOINT_H
