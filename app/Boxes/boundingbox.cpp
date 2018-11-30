#include "Boxes/boundingbox.h"
#include "canvas.h"
#include "undoredo.h"
#include "Boxes/boxesgroup.h"
#include <QDebug>
#include "GUI/mainwindow.h"
#include "GUI/keysview.h"
#include "GUI/BoxesList/boxscrollwidget.h"
#include "singlewidgetabstraction.h"
#include "GUI/BoxesList/OptimalScrollArea/scrollwidgetvisiblepart.h"
#include "durationrectangle.h"
#include "Animators/animatorupdater.h"
#include "pointhelpers.h"
#include "skqtconversions.h"
#include "global.h"

QList<BoundingBoxQPtr> BoundingBox::mLoadedBoxes;
QList<FunctionWaitingForBoxLoadSPtr> BoundingBox::mFunctionsWaitingForBoxLoad;

BoundingBox::BoundingBox(const BoundingBoxType &type) :
    ComplexAnimator("box") {
    mTransformAnimator = SPtrCreate(BoxTransformAnimator)(this);
    ca_addChildAnimator(mTransformAnimator);

    mEffectsAnimators = SPtrCreate(EffectAnimators)(this);
    mEffectsAnimators->prp_setUpdater(SPtrCreate(PixmapEffectUpdater)(this));
    mEffectsAnimators->prp_blockUpdater();
    ca_addChildAnimator(mEffectsAnimators);
    mEffectsAnimators->SWT_hide();

    mType = type;

    mTransformAnimator->reset();
}

BoundingBox::~BoundingBox() {}

void BoundingBox::prp_updateAfterChangedAbsFrameRange(const int &minFrame,
                                                      const int &maxFrame) {
    Property::prp_updateAfterChangedAbsFrameRange(minFrame,
                                                  maxFrame);
    if(anim_mCurrentAbsFrame >= minFrame) {
        if(anim_mCurrentAbsFrame <= maxFrame) {
            scheduleUpdate(Animator::USER_CHANGE);
        }
    }
}

void BoundingBox::ca_childAnimatorIsRecordingChanged() {
    ComplexAnimator::ca_childAnimatorIsRecordingChanged();
    SWT_scheduleWidgetsContentUpdateWithRule(SWT_Animated);
    SWT_scheduleWidgetsContentUpdateWithRule(SWT_NotAnimated);
}

SingleWidgetAbstraction* BoundingBox::SWT_getAbstractionForWidget(
            ScrollWidgetVisiblePart *visiblePartWidget) {
    Q_FOREACH(const SingleWidgetAbstractionSPtr &abs,
              mSWT_allAbstractions) {
        if(abs->getParentVisiblePartWidget() == visiblePartWidget) {
            return abs.get();
        }
    }
    SingleWidgetAbstraction* abs = SWT_createAbstraction(visiblePartWidget);
    if(visiblePartWidget->getCurrentRulesCollection().rule == SWT_Selected) {
        abs->setContentVisible(true);
    }
    return abs;
}

#include "linkbox.h"
BoundingBoxQSPtr BoundingBox::createLink() {
    BoundingBoxQSPtr linkBox = SPtrCreate(InternalLinkBox)(this);
    copyBoundingBoxDataTo(linkBox.get());
    return linkBox;
}

BoundingBoxQSPtr BoundingBox::createLinkForLinkGroup() {
    BoundingBoxQSPtr box = createLink();
    box->clearEffects();
    return box;
}

void BoundingBox::clearEffects() {
    mEffectsAnimators->ca_removeAllChildAnimators();
}

void BoundingBox::setFont(const QFont &) {}

void BoundingBox::setSelectedFontSize(const qreal &) {}

void BoundingBox::setSelectedFontFamilyAndStyle(const QString &, const QString &) {}

QPointF BoundingBox::getRelCenterPosition() {
    return mRelBoundingRect.center();
}

void BoundingBox::centerPivotPosition(const bool &saveUndoRedo) {
    mTransformAnimator->setPivotWithoutChangingTransformation(
                getRelCenterPosition(), saveUndoRedo);
}

void BoundingBox::setPivotPosition(const QPointF &pos, const bool &saveUndoRedo) {
    mTransformAnimator->setPivotWithoutChangingTransformation(
                pos, saveUndoRedo);
}

void BoundingBox::copyBoundingBoxDataTo(BoundingBox *targetBox) {
    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite);
    BoundingBox::writeBoundingBoxDataForLink(&buffer);
    if(buffer.seek(sizeof(BoundingBoxType)) ) {
        targetBox->BoundingBox::readBoundingBoxDataForLink(&buffer);
    }
    buffer.close();
}

void BoundingBox::drawHoveredSk(SkCanvas *canvas, const SkScalar &invScale) {
    drawHoveredPathSk(canvas, mSkRelBoundingRectPath, invScale);
}

void BoundingBox::drawHoveredPathSk(SkCanvas *canvas,
                                    const SkPath &path,
                                    const SkScalar &invScale) {
    canvas->save();
    SkPath mappedPath = path;
    mappedPath.transform(QMatrixToSkMatrix(
                             mTransformAnimator->getCombinedTransform()));
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setColor(SK_ColorBLACK);
    paint.setStrokeWidth(2*invScale);
    paint.setStyle(SkPaint::kStroke_Style);
    canvas->drawPath(mappedPath, paint);

    paint.setColor(SK_ColorRED);
    paint.setStrokeWidth(invScale);
    canvas->drawPath(mappedPath, paint);
    canvas->restore();
}

void BoundingBox::applyPaintSetting(const PaintSetting &setting) {
    Q_UNUSED(setting);
}

void BoundingBox::setFillColorMode(const ColorMode &colorMode) {
    Q_UNUSED(colorMode);
}

void BoundingBox::setStrokeColorMode(const ColorMode &colorMode) {
    Q_UNUSED(colorMode);
}

bool BoundingBox::isAncestor(BoxesGroup *box) const {
    if(mParentGroup == box) return true;
    if(mParentGroup == nullptr) return false;
    return mParentGroup->isAncestor(box);
}

bool BoundingBox::isAncestor(BoundingBox *box) const {
    if(box->SWT_isBoxesGroup()) {
        return isAncestor(box);
    }
    return false;
}

Canvas *BoundingBox::getParentCanvas() {
    if(mParentGroup == nullptr) return nullptr;
    return mParentGroup->getParentCanvas();
}

void BoundingBox::reloadCacheHandler() { clearAllCache(); }

bool BoundingBox::SWT_isBoundingBox() { return true; }

void BoundingBox::updateAllBoxes(const UpdateReason &reason) {
    scheduleUpdate(reason);
}

void BoundingBox::prp_updateInfluenceRangeAfterChanged() {
    int minAbsUpdateFrame;
    int maxAbsUpdateFrame;
    getVisibleAbsFrameRange(&minAbsUpdateFrame, &maxAbsUpdateFrame);
    prp_updateAfterChangedAbsFrameRange(minAbsUpdateFrame,
                                        maxAbsUpdateFrame);
}

void BoundingBox::clearAllCache() {
    prp_updateInfluenceRangeAfterChanged();
}

void BoundingBox::drawSelectedSk(SkCanvas *canvas,
                                 const CanvasMode &currentCanvasMode,
                                 const SkScalar &invScale) {
    if(isVisibleAndInVisibleDurationRect()) {
        canvas->save();
        drawBoundingRectSk(canvas, invScale);
        if(currentCanvasMode == MOVE_PATH) {
            mTransformAnimator->getPivotMovablePoint()->
                    drawSk(canvas, invScale);
        }
        canvas->restore();
    }
}

void BoundingBox::drawPixmapSk(SkCanvas *canvas) {
    if(isVisibleAndInVisibleDurationRect() &&
        mTransformAnimator->getOpacity() > 0.001) {
        canvas->save();

        SkPaint paint;
        int intAlpha = qRound(mTransformAnimator->getOpacity()*2.55);
        paint.setAlpha(static_cast<U8CPU>(intAlpha));
        paint.setBlendMode(mBlendModeSk);
        //paint.setFilterQuality(kHigh_SkFilterQuality);
        drawPixmapSk(canvas, &paint);
        canvas->restore();
    }
}

void BoundingBox::drawPixmapSk(SkCanvas *canvas, SkPaint *paint) {
    if(mTransformAnimator->getOpacity() < 0.001) { return; }
    //paint->setFilterQuality(kHigh_SkFilterQuality);
    mDrawRenderContainer->drawSk(canvas, paint);
}

void BoundingBox::setBlendModeSk(const SkBlendMode &blendMode) {
    mBlendModeSk = blendMode;
    prp_updateInfluenceRangeAfterChanged();
}

const SkBlendMode &BoundingBox::getBlendMode() {
    return mBlendModeSk;
}

void BoundingBox::resetScale() {
    mTransformAnimator->resetScale(true);
}

void BoundingBox::resetTranslation() {
    mTransformAnimator->resetTranslation(true);
}

void BoundingBox::resetRotation() {
    mTransformAnimator->resetRotation(true);
}

void BoundingBox::prp_setAbsFrame(const int &frame) {
    int lastRelFrame = anim_mCurrentRelFrame;
    ComplexAnimator::prp_setAbsFrame(frame);
    bool isInVisRange = isRelFrameInVisibleDurationRect(
                anim_mCurrentRelFrame);
    if(mUpdateDrawOnParentBox != isInVisRange) {
        if(mUpdateDrawOnParentBox) {
            if(mParentGroup != nullptr) mParentGroup->scheduleUpdate(Animator::FRAME_CHANGE);
        } else {
            scheduleUpdate(Animator::FRAME_CHANGE);
        }
        mUpdateDrawOnParentBox = isInVisRange;
    }
    if(prp_differencesBetweenRelFrames(lastRelFrame,
                                       anim_mCurrentRelFrame)) {
        scheduleUpdate(Animator::FRAME_CHANGE);
    }
}

void BoundingBox::startAllPointsTransform() {}

void BoundingBox::finishAllPointsTransform() {}

void BoundingBox::setStrokeCapStyle(const Qt::PenCapStyle &capStyle) {
    Q_UNUSED(capStyle); }

void BoundingBox::setStrokeJoinStyle(const Qt::PenJoinStyle &joinStyle) {
    Q_UNUSED(joinStyle); }

void BoundingBox::setStrokeWidth(const qreal &strokeWidth, const bool &finish) {
    Q_UNUSED(strokeWidth); Q_UNUSED(finish); }

void BoundingBox::startSelectedStrokeWidthTransform() {}

void BoundingBox::startSelectedStrokeColorTransform() {}

void BoundingBox::startSelectedFillColorTransform() {}

VectorPathEdge *BoundingBox::getEdge(const QPointF &absPos, const qreal &canvasScaleInv) {
    Q_UNUSED(absPos);
    Q_UNUSED(canvasScaleInv);
    return nullptr;
}

bool BoundingBox::prp_differencesBetweenRelFrames(const int &relFrame1,
                                                  const int &relFrame2) {
    bool differences =
            ComplexAnimator::prp_differencesBetweenRelFrames(relFrame1,
                                                             relFrame2);
    if(differences || mDurationRectangle == nullptr) return differences;
    return mDurationRectangle->hasAnimationFrameRange();
}

bool BoundingBox::prp_differencesBetweenRelFramesIncludingInherited(
        const int &relFrame1, const int &relFrame2) {
    bool diffThis = prp_differencesBetweenRelFrames(relFrame1, relFrame2);
    if(mParentGroup == nullptr || diffThis) return diffThis;
    int absFrame1 = prp_relFrameToAbsFrame(relFrame1);
    int absFrame2 = prp_relFrameToAbsFrame(relFrame2);
    int parentRelFrame1 = mParentGroup->prp_absFrameToRelFrame(absFrame1);
    int parentRelFrame2 = mParentGroup->prp_absFrameToRelFrame(absFrame2);

    bool diffInherited =
            mParentGroup->prp_differencesBetweenRelFramesIncludingInheritedExcludingContainedBoxes(
                parentRelFrame1, parentRelFrame2);
    return diffThis || diffInherited;
}

void BoundingBox::setParentGroup(BoxesGroup *parent) {
    mParentGroup = parent;

    mParentTransform = parent->getTransformAnimator();
    mTransformAnimator->setParentTransformAnimator(mParentTransform);

    prp_setAbsFrame(mParentGroup->anim_getCurrentAbsFrame());
    mTransformAnimator->updateCombinedTransform(Animator::USER_CHANGE);
}

void BoundingBox::setParent(BasicTransformAnimator *parent) {
    if(parent == mParentTransform) return;
    mParentTransform = parent;
    mTransformAnimator->setParentTransformAnimator(mParentTransform);

    mTransformAnimator->updateCombinedTransform(Animator::USER_CHANGE);
}

void BoundingBox::clearParent() {
    setParent(mParentGroup->getTransformAnimator());
}

BoxesGroup *BoundingBox::getParentGroup() {
    return mParentGroup;
}

void BoundingBox::setPivotRelPos(const QPointF &relPos,
                                 const bool &saveUndoRedo,
                                 const bool &pivotAutoAdjust) {
    mPivotAutoAdjust = pivotAutoAdjust;
    mTransformAnimator->setPivotWithoutChangingTransformation(relPos,
                                                              saveUndoRedo);
    schedulePivotUpdate();
}

void BoundingBox::startPivotTransform() {
    mTransformAnimator->startPivotTransform();
}

void BoundingBox::finishPivotTransform() {
    mTransformAnimator->finishPivotTransform();
}

void BoundingBox::setPivotAbsPos(const QPointF &absPos,
                                 const bool &saveUndoRedo,
                                 const bool &pivotChanged) {
    setPivotRelPos(mapAbsPosToRel(absPos), saveUndoRedo, pivotChanged);
    //updateCombinedTransform();
}

QPointF BoundingBox::getPivotAbsPos() {
    return mTransformAnimator->getPivotAbs();
}

void BoundingBox::select() {
    mSelected = true;

    SWT_scheduleWidgetsContentUpdateWithRule(SWT_Selected);
}

void BoundingBox::updateRelBoundingRectFromRenderData(
        BoundingBoxRenderData* renderData) {
    mRelBoundingRect = renderData->relBoundingRect;
    mRelBoundingRectSk = QRectFToSkRect(mRelBoundingRect);
    mSkRelBoundingRectPath = SkPath();
    mSkRelBoundingRectPath.addRect(mRelBoundingRectSk);

    if(mPivotAutoAdjust &&
       !mTransformAnimator->rotOrScaleOrPivotRecording()) {
        setPivotPosition(renderData->getCenterPosition(), false);
    }
}

void BoundingBox::updateCurrentPreviewDataFromRenderData(
        BoundingBoxRenderData* renderData) {
    updateRelBoundingRectFromRenderData(renderData);
}

bool BoundingBox::shouldScheduleUpdate() {
    if(mParentGroup == nullptr) return false;
    if((isVisibleAndInVisibleDurationRect()) ||
            (isRelFrameInVisibleDurationRect(anim_mCurrentRelFrame))) {
        return true;
    }
    return false;
}

void BoundingBox::scheduleUpdate(const UpdateReason &reason) {
    scheduleUpdate(anim_mCurrentRelFrame, reason);
}

void BoundingBox::scheduleUpdate(const int &relFrame, const UpdateReason& reason) {
    Q_ASSERT(!mBlockedSchedule);
    if(!shouldScheduleUpdate()) return;
    mExpiredPixmap = 1;
//    if(SWT_isCanvas()) {
        auto currentRenderData = getCurrentRenderData(relFrame);
        if(currentRenderData == nullptr) {
            currentRenderData = updateCurrentRenderData(relFrame, reason);
            if(currentRenderData == nullptr) return;
        } else {
            if(!currentRenderData->redo && !currentRenderData->copied &&
                    reason != UpdateReason::FRAME_CHANGE) {
                currentRenderData->redo = currentRenderData->isAwaitingUpdate();
            }
            return;
        }
        auto currentReason = currentRenderData->reason;
        if(reason == USER_CHANGE &&
                (currentReason == CHILD_USER_CHANGE ||
                 currentReason == FRAME_CHANGE)) {
            currentRenderData->reason = reason;
        } else if(reason == CHILD_USER_CHANGE &&
                  currentReason == FRAME_CHANGE) {
            currentRenderData->reason = reason;
        }
        currentRenderData->redo = false;
        currentRenderData->addScheduler();
//    }

    //mUpdateDrawOnParentBox = isVisibleAndInVisibleDurationRect();

    if(mParentGroup != nullptr) {
        mParentGroup->scheduleUpdate(reason == USER_CHANGE ? CHILD_USER_CHANGE : reason);
    }

    emit scheduledUpdate();
}

BoundingBoxRenderData *BoundingBox::updateCurrentRenderData(const int& relFrame,
                                                            const UpdateReason& reason) {
    auto renderData = createRenderData();
    if(renderData == nullptr) return nullptr;
    renderData->relFrame = relFrame;
    renderData->reason = reason;
    mCurrentRenderDataHandler.addItemAtRelFrame(renderData);
    return renderData.get();
}

BoundingBoxRenderData *BoundingBox::getCurrentRenderData(const int& relFrame) {
    BoundingBoxRenderData* currentRenderData =
            mCurrentRenderDataHandler.getItemAtRelFrame(relFrame);
    if(currentRenderData == nullptr && mExpiredPixmap == 0) {
        currentRenderData = mDrawRenderContainer->getSrcRenderData();
        if(currentRenderData == nullptr) return nullptr;
//        if(currentRenderData->relFrame == relFrame) {
        if(!prp_differencesBetweenRelFramesIncludingInherited(
                    currentRenderData->relFrame, relFrame)) {
            auto copy = currentRenderData->makeCopy();
            copy->relFrame = relFrame;
            mCurrentRenderDataHandler.addItemAtRelFrame(copy);
            return copy.get();
        }
        return nullptr;
    }
    return currentRenderData;
}


void BoundingBox::nullifyCurrentRenderData(const int& relFrame) {
    mCurrentRenderDataHandler.removeItemAtRelFrame(relFrame);
}

void BoundingBox::deselect() {
    mSelected = false;

    SWT_scheduleWidgetsContentUpdateWithRule(SWT_Selected);
}

bool BoundingBox::isContainedIn(const QRectF &absRect) {
    return absRect.contains(getCombinedTransform().mapRect(mRelBoundingRect));
}

BoundingBox *BoundingBox::getPathAtFromAllAncestors(const QPointF &absPos) {
    if(absPointInsidePath(absPos)) {
        return this;
    } else {
        return nullptr;
    }
}

QPointF BoundingBox::mapAbsPosToRel(const QPointF &absPos) {
    return mTransformAnimator->mapAbsPosToRel(absPos);
}

PaintSettings *BoundingBox::getFillSettings() {
    return nullptr;
}

StrokeSettings *BoundingBox::getStrokeSettings() {
    return nullptr;
}

void BoundingBox::drawAsBoundingRectSk(SkCanvas *canvas,
                                       const SkPath &path,
                                       const SkScalar &invScale,
                                       const bool &dashes) {
    canvas->save();

    SkPaint paint;
    if(dashes) {
        SkScalar intervals[2] = {MIN_WIDGET_HEIGHT*0.25f*invScale,
                                 MIN_WIDGET_HEIGHT*0.25f*invScale};
        paint.setPathEffect(SkDashPathEffect::Make(intervals, 2, 0));
    }
    paint.setAntiAlias(true);
    paint.setStrokeWidth(1.5f*invScale);
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setColor(SK_ColorBLACK);
    SkPath mappedPath = path;
    mappedPath.transform(QMatrixToSkMatrix(getCombinedTransform()));
    canvas->drawPath(mappedPath, paint);
    paint.setStrokeWidth(0.75f*invScale);
    paint.setColor(SK_ColorWHITE);
    canvas->drawPath(mappedPath, paint);

//    SkPaint paint;
//    SkScalar intervals[2] = {MIN_WIDGET_HEIGHT*0.25f*invScale,
//                             MIN_WIDGET_HEIGHT*0.25f*invScale};
//    paint.setPathEffect(SkDashPathEffect::Make(intervals, 2, 0));
//    paint.setColor(SkColorSetARGB(125, 0, 0, 0));
//    paint.setStyle(SkPaint::kStroke_Style);
//    paint.setStrokeWidth(invScale);
//    SkPath mappedPath = path;
//    mappedPath.transform(QMatrixToSkMatrix(getCombinedTransform()));
//    canvas->drawPath(mappedPath, paint);

    canvas->restore();
}

void BoundingBox::drawBoundingRectSk(SkCanvas *canvas,
                                     const SkScalar &invScale) {
    drawAsBoundingRectSk(canvas, mSkRelBoundingRectPath,
                         invScale, true);
}

const SkPath &BoundingBox::getRelBoundingRectPath() {
    return mSkRelBoundingRectPath;
}

QMatrix BoundingBox::getCombinedTransform() const {
    return mTransformAnimator->getCombinedTransform();
}

QMatrix BoundingBox::getRelativeTransformAtCurrentFrame() {
    return getRelativeTransformAtRelFrame(anim_mCurrentRelFrame);
}

void BoundingBox::applyTransformation(BoxTransformAnimator *transAnimator) {
    Q_UNUSED(transAnimator);
}

void BoundingBox::applyTransformationInverted(BoxTransformAnimator *transAnimator) {
    Q_UNUSED(transAnimator);
}

void BoundingBox::scale(const qreal &scaleBy) {
    scale(scaleBy, scaleBy);
}

void BoundingBox::scale(const qreal &scaleXBy, const qreal &scaleYBy) {
    mTransformAnimator->scale(scaleXBy, scaleYBy);
}

NodePoint *BoundingBox::createNewPointOnLineNear(const QPointF &absPos, const bool &adjust, const qreal &canvasScaleInv) {
    Q_UNUSED(absPos);
    Q_UNUSED(adjust);
    Q_UNUSED(canvasScaleInv);
    return nullptr;
}

void BoundingBox::rotateBy(const qreal &rot) {
    mTransformAnimator->rotateRelativeToSavedValue(rot);
}

void BoundingBox::rotateRelativeToSavedPivot(const qreal &rot) {
    mTransformAnimator->rotateRelativeToSavedValue(rot,
                                                   mSavedTransformPivot);
}

void BoundingBox::scaleRelativeToSavedPivot(const qreal &scaleXBy,
                                            const qreal &scaleYBy) {
    mTransformAnimator->scaleRelativeToSavedValue(scaleXBy, scaleYBy,
                                                 mSavedTransformPivot);
}

void BoundingBox::scaleRelativeToSavedPivot(const qreal &scaleBy) {
    scaleRelativeToSavedPivot(scaleBy, scaleBy);
}

QPointF BoundingBox::mapRelPosToAbs(const QPointF &relPos) const {
    return mTransformAnimator->mapRelPosToAbs(relPos);
}

QRectF BoundingBox::getRelBoundingRect() const {
    return mRelBoundingRect;
}

QRectF BoundingBox::getRelBoundingRectAtRelFrame(const int &relFrame) {
    Q_UNUSED(relFrame);
    return getRelBoundingRect();
}

void BoundingBox::applyCurrentTransformation() {}

void BoundingBox::moveByAbs(const QPointF &trans) {
    mTransformAnimator->moveByAbs(trans);
    //    QPointF by = mParent->mapAbsPosToRel(trans) -
//                 mParent->mapAbsPosToRel(QPointF(0., 0.));
// //    QPointF by = mapAbsPosToRel(
// //                trans - mapRelativeToAbsolute(QPointF(0., 0.)));

//    moveByRel(by);
}

void BoundingBox::moveByRel(const QPointF &trans) {
    mTransformAnimator->moveRelativeToSavedValue(trans.x(), trans.y());
}

void BoundingBox::setAbsolutePos(const QPointF &pos,
                                 const bool &saveUndoRedo) {
    setRelativePos(mParentTransform->mapAbsPosToRel(pos), saveUndoRedo);
}

void BoundingBox::setRelativePos(const QPointF &relPos,
                                 const bool &saveUndoRedo) {
    mTransformAnimator->setPosition(relPos.x(), relPos.y(), saveUndoRedo);
}

void BoundingBox::saveTransformPivotAbsPos(const QPointF &absPivot) {
    mSavedTransformPivot =
            mParentTransform->mapAbsPosToRel(absPivot) -
            mTransformAnimator->getPivot();
}

void BoundingBox::startPosTransform() {
    mTransformAnimator->startPosTransform();
}

void BoundingBox::startRotTransform() {
    mTransformAnimator->startRotTransform();
}

void BoundingBox::startScaleTransform() {
    mTransformAnimator->startScaleTransform();
}

void BoundingBox::startTransform() {
    mTransformAnimator->prp_startTransform();
}

void BoundingBox::finishTransform() {
    mTransformAnimator->prp_finishTransform();
    //updateCombinedTransform();
}

qreal BoundingBox::getEffectsMarginAtRelFrame(const int &relFrame) {
    return mEffectsAnimators->getEffectsMarginAtRelFrame(relFrame);
}

qreal BoundingBox::getEffectsMarginAtRelFrameF(const qreal &relFrame) {
    return mEffectsAnimators->getEffectsMarginAtRelFrameF(relFrame);
}

void BoundingBox::setupBoundingBoxRenderDataForRelFrameF(
                        const qreal &relFrame,
                        BoundingBoxRenderData* data) {
    data->relFrame = qRound(relFrame);
    data->renderedToImage = false;
    data->relTransform = getRelativeTransformAtRelFrameF(relFrame);
    data->parentTransform = mTransformAnimator->
            getParentCombinedTransformMatrixAtRelFrameF(relFrame);
    data->transform = data->relTransform*data->parentTransform;
    data->opacity = mTransformAnimator->getOpacityAtRelFrameF(relFrame);
    data->resolution = getParentCanvas()->getResolutionFraction();
    bool effectsVisible = getParentCanvas()->getRasterEffectsVisible();
    if(effectsVisible) {
        data->effectsMargin = getEffectsMarginAtRelFrameF(relFrame)*
                data->resolution + 2.;
    } else {
        data->effectsMargin = 2.;
    }
    data->blendMode = getBlendMode();

    Canvas* parentCanvas = getParentCanvas();
    data->maxBoundsRect = parentCanvas->getMaxBoundsRect();
    if(data->opacity > 0.001 && effectsVisible) {
        setupEffectsF(relFrame, data);
    }
    if(data->parentIsTarget && (data->reason == USER_CHANGE ||
            data->reason != BoundingBox::CHILD_USER_CHANGE)) {
        nullifyCurrentRenderData(data->relFrame);
    }
}

BoundingBoxRenderDataSPtr BoundingBox::createRenderData() { return nullptr; }

void BoundingBox::setupEffectsF(const qreal &relFrame,
                                BoundingBoxRenderData *data) {
    mEffectsAnimators->addEffectRenderDataToListF(relFrame, data);
}

void BoundingBox::addLinkingBox(BoundingBox *box) {
    mLinkingBoxes << box;
}

void BoundingBox::removeLinkingBox(BoundingBox *box) {
    mLinkingBoxes.removeOne(box);
}

const QList<BoundingBoxQPtr> &BoundingBox::getLinkingBoxes() const {
    return mLinkingBoxes;
}

EffectAnimators *BoundingBox::getEffectsAnimators() {
    return mEffectsAnimators.data();
}

void BoundingBox::incReasonsNotToApplyUglyTransform() {
    mNReasonsNotToApplyUglyTransform++;
}

void BoundingBox::decReasonsNotToApplyUglyTransform() {
    mNReasonsNotToApplyUglyTransform--;
}

bool BoundingBox::isSelected() { return mSelected; }

bool BoundingBox::relPointInsidePath(const QPointF &point) {
    return mRelBoundingRect.contains(point.toPoint());
}

bool BoundingBox::absPointInsidePath(const QPointF &absPoint) {
    return relPointInsidePath(mapAbsPosToRel(absPoint));
}

MovablePoint *BoundingBox::getPointAtAbsPos(const QPointF &absPtPos,
                                            const CanvasMode &currentCanvasMode,
                                            const qreal &canvasScaleInv) {
    if(currentCanvasMode == MOVE_PATH) {
        MovablePoint* pivotMovable = mTransformAnimator->getPivotMovablePoint();
        if(pivotMovable->isPointAtAbsPos(absPtPos, canvasScaleInv)) {
            return pivotMovable;
        }
    }
    return nullptr;
}

void BoundingBox::cancelTransform() {
    mTransformAnimator->prp_cancelTransform();
    //updateCombinedTransform();
}

void BoundingBox::moveUp() {
    mParentGroup->increaseContainedBoxZInList(this);
}

void BoundingBox::moveDown() {
    mParentGroup->decreaseContainedBoxZInList(this);
}

void BoundingBox::bringToFront() {
    mParentGroup->bringContainedBoxToEndList(this);
}

void BoundingBox::bringToEnd() {
    mParentGroup->bringContainedBoxToFrontList(this);
}

void BoundingBox::setZListIndex(const int &z,
                                const bool &saveUndoRedo) {
    if(saveUndoRedo) {
//        addUndoRedo(new SetBoundingBoxZListIndexUnoRedo(mZListIndex, z, this));
    }
    mZListIndex = z;
}

void BoundingBox::selectAndAddContainedPointsToList(const QRectF &, QList<MovablePointPtr> &) {}

int BoundingBox::getZIndex() {
    return mZListIndex;
}

QPointF BoundingBox::getAbsolutePos() {
    return QPointF(mTransformAnimator->getCombinedTransform().dx(),
                   mTransformAnimator->getCombinedTransform().dy());
}

void BoundingBox::updateDrawRenderContainerTransform() {
    if(mNReasonsNotToApplyUglyTransform == 0) {
        mDrawRenderContainer->updatePaintTransformGivenNewCombinedTransform(
                    mTransformAnimator->getCombinedTransform());
    }

}

void BoundingBox::setPivotAutoAdjust(const bool &bT) {
    mPivotAutoAdjust = bT;
}

const BoundingBoxType &BoundingBox::getBoxType() { return mType; }

void BoundingBox::startDurationRectPosTransform() {
    if(hasDurationRectangle()) {
        mDurationRectangle->startPosTransform();
    }
}

void BoundingBox::finishDurationRectPosTransform() {
    if(hasDurationRectangle()) {
        mDurationRectangle->finishPosTransform();
    }
}

void BoundingBox::moveDurationRect(const int &dFrame) {
    if(hasDurationRectangle()) {
        mDurationRectangle->changeFramePosBy(dFrame);
    }
}

void BoundingBox::startMinFramePosTransform() {
    if(hasDurationRectangle()) {
        mDurationRectangle->startMinFramePosTransform();
    }
}

void BoundingBox::finishMinFramePosTransform() {
    if(hasDurationRectangle()) {
        mDurationRectangle->finishMinFramePosTransform();
    }
}

void BoundingBox::moveMinFrame(const int &dFrame) {
    if(hasDurationRectangle()) {
        mDurationRectangle->moveMinFrame(dFrame);
    }
}

void BoundingBox::startMaxFramePosTransform() {
    if(hasDurationRectangle()) {
        mDurationRectangle->startMaxFramePosTransform();
    }
}

void BoundingBox::finishMaxFramePosTransform() {
    if(hasDurationRectangle()) {
        mDurationRectangle->finishMaxFramePosTransform();
    }
}

void BoundingBox::moveMaxFrame(const int &dFrame) {
    if(hasDurationRectangle()) {
        mDurationRectangle->moveMaxFrame(dFrame);
    }
}

DurationRectangle *BoundingBox::getDurationRectangle() {
    return mDurationRectangle.get();
}

void BoundingBox::getMotionBlurProperties(QList<Property *> &list) {
    list.append(mTransformAnimator->getScaleAnimator());
    list.append(mTransformAnimator->getPosAnimator());
    list.append(mTransformAnimator->getPivotAnimator());
    list.append(mTransformAnimator->getRotAnimator());
}

BoxTransformAnimator *BoundingBox::getTransformAnimator() {
    return mTransformAnimator.get();
}

VectorPath *BoundingBox::objectToVectorPathBox() { return nullptr; }

VectorPath *BoundingBox::strokeToVectorPathBox() { return nullptr; }

void BoundingBox::selectionChangeTriggered(const bool &shiftPressed) {
    Canvas* parentCanvas = getParentCanvas();
    if(shiftPressed) {
        if(mSelected) {
            parentCanvas->removeBoxFromSelection(this);
        } else {
            parentCanvas->addBoxToSelection(this);
        }
    } else {
        parentCanvas->clearBoxesSelection();
        parentCanvas->addBoxToSelection(this);
    }
}

bool BoundingBox::isAnimated() { return prp_isDescendantRecording(); }

void BoundingBox::addEffect(const PixmapEffectQSPtr& effect) {
    //effect->setUpdater(SPtrCreate(PixmapEffectUpdater)(this));

    if(!mEffectsAnimators->hasChildAnimators()) {
        mEffectsAnimators->SWT_show();
    }
    mEffectsAnimators->ca_addChildAnimator(effect);
    effect->setParentEffectAnimators(mEffectsAnimators.data());

    clearAllCache();
}

void BoundingBox::removeEffect(const PixmapEffectQSPtr& effect) {
    mEffectsAnimators->ca_removeChildAnimator(effect);
    if(!mEffectsAnimators->hasChildAnimators()) {
        mEffectsAnimators->SWT_hide();
    }

    clearAllCache();
}

//int BoundingBox::prp_getParentFrameShift() const {
//    if(mParentGroup == nullptr) {
//        return 0;
//    } else {
//        return mParentGroup->prp_getFrameShift();
//    }
//}

bool BoundingBox::hasDurationRectangle() const {
    return mDurationRectangle != nullptr;
}

void BoundingBox::createDurationRectangle() {
    DurationRectangleQSPtr durRect =
            SPtrCreate(DurationRectangle)(this);
    durRect->setMinFrame(0);
    Canvas* parentCanvas = getParentCanvas();
    if(parentCanvas != nullptr) {
        durRect->setFramesDuration(getParentCanvas()->getFrameCount());
    }
    setDurationRectangle(durRect);
}

void BoundingBox::shiftAll(const int &shift) {
    if(hasDurationRectangle()) {
        mDurationRectangle->changeFramePosBy(shift);
    } else {
        anim_shiftAllKeys(shift);
    }
}

QMatrix BoundingBox::getRelativeTransformAtRelFrame(const int &relFrame) {
    return mTransformAnimator->getRelativeTransformAtRelFrame(relFrame);
}

QMatrix BoundingBox::getRelativeTransformAtRelFrameF(const qreal &relFrame) {
    return mTransformAnimator->getRelativeTransformAtRelFrameF(relFrame);
}

int BoundingBox::prp_getRelFrameShift() const {
    if(mDurationRectangle == nullptr) return 0;
    return mDurationRectangle->getFrameShift();
}

void BoundingBox::setDurationRectangle(const DurationRectangleQSPtr& durationRect) {
    if(durationRect == mDurationRectangle) return;
    if(mDurationRectangle != nullptr) {
        disconnect(mDurationRectangle.data(), nullptr, this, nullptr);
    }
    DurationRectangleQSPtr oldDurRect = mDurationRectangle;
    mDurationRectangle = durationRect;
    updateAfterDurationRectangleShifted();
    if(durationRect == nullptr) {
        shiftAll(oldDurRect->getFrameShift());
    }

    if(mDurationRectangle == nullptr) return;
    connect(mDurationRectangle.data(), SIGNAL(posChangedBy(int)),
            this, SLOT(updateAfterDurationRectangleShifted(int)));
    connect(mDurationRectangle.data(), SIGNAL(rangeChanged()),
            this, SLOT(updateAfterDurationRectangleRangeChanged()));

    connect(mDurationRectangle.data(), SIGNAL(minFrameChangedBy(int)),
            this, SLOT(updateAfterDurationMinFrameChangedBy(int)));
    connect(mDurationRectangle.data(), SIGNAL(maxFrameChangedBy(int)),
            this, SLOT(updateAfterDurationMaxFrameChangedBy(int)));
}

void BoundingBox::updateAfterDurationRectangleShifted(const int &dFrame) {
    prp_setParentFrameShift(prp_mParentFrameShift);
    prp_setAbsFrame(anim_mCurrentAbsFrame);
    int minAbsUpdateFrame;
    int maxAbsUpdateFrame;
    getVisibleAbsFrameRange(&minAbsUpdateFrame, &maxAbsUpdateFrame);
    if(dFrame > 0) {
        prp_updateAfterChangedAbsFrameRange(minAbsUpdateFrame - dFrame,
                                            maxAbsUpdateFrame);
    } else {
        prp_updateAfterChangedAbsFrameRange(minAbsUpdateFrame,
                                            maxAbsUpdateFrame - dFrame);
    }
}

void BoundingBox::updateAfterDurationMinFrameChangedBy(const int &by) {
    int minAbsUpdateFrame;
    int maxAbsUpdateFrame;
    getVisibleAbsFrameRange(&minAbsUpdateFrame, &maxAbsUpdateFrame);
    if(by > 0) {
        prp_updateAfterChangedAbsFrameRange(minAbsUpdateFrame - by,
                                            minAbsUpdateFrame - 1);
    } else {
        prp_updateAfterChangedAbsFrameRange(minAbsUpdateFrame,
                                            minAbsUpdateFrame - by - 1);
    }
}

void BoundingBox::updateAfterDurationMaxFrameChangedBy(const int &by) {
    int minAbsUpdateFrame;
    int maxAbsUpdateFrame;
    getVisibleAbsFrameRange(&minAbsUpdateFrame, &maxAbsUpdateFrame);
    if(by > 0) {
        prp_updateAfterChangedAbsFrameRange(maxAbsUpdateFrame - by + 1,
                                            maxAbsUpdateFrame);
    } else {
        prp_updateAfterChangedAbsFrameRange(maxAbsUpdateFrame + 1,
                                            maxAbsUpdateFrame - by);
    }
}

void BoundingBox::updateAfterDurationRectangleRangeChanged() {}

DurationRectangleMovable *BoundingBox::anim_getRectangleMovableAtPos(
        const int &relX, const int &minViewedFrame,
        const qreal &pixelsPerFrame) {
    if(mDurationRectangle == nullptr) return nullptr;
    return mDurationRectangle->getMovableAt(relX, pixelsPerFrame,
                                            minViewedFrame);
}

void BoundingBox::prp_drawKeys(QPainter *p,
                               const qreal &pixelsPerFrame,
                               const qreal &drawY,
                               const int &startFrame,
                               const int &endFrame) {
    if(mDurationRectangle != nullptr) {
        p->save();
        p->translate(prp_getParentFrameShift()*pixelsPerFrame, 0);
        mDurationRectangle->draw(p, pixelsPerFrame,
                                drawY, startFrame);
        p->restore();
    }

    Animator::prp_drawKeys(p,
                           pixelsPerFrame, drawY,
                           startFrame, endFrame);
}

void BoundingBox::addPathEffect(const PathEffectQSPtr &) {}

void BoundingBox::addFillPathEffect(const PathEffectQSPtr &) {}

void BoundingBox::addOutlinePathEffect(const PathEffectQSPtr &) {}

void BoundingBox::removePathEffect(const PathEffectQSPtr &) {}

void BoundingBox::removeFillPathEffect(const PathEffectQSPtr &) {}

void BoundingBox::removeOutlinePathEffect(const PathEffectQSPtr &) {}

void BoundingBox::addActionsToMenu(QMenu *) {}

bool BoundingBox::handleSelectedCanvasAction(QAction *) {
    return false;
}

void BoundingBox::setName(const QString &name) {
    if(name == prp_mName) return;
    prp_mName = name;

    emit nameChanged(name);
}

const QString &BoundingBox::getName() {
    return prp_mName;
}

bool BoundingBox::isVisibleAndInVisibleDurationRect() {
    return isRelFrameInVisibleDurationRect(anim_mCurrentRelFrame) && mVisible;
}

bool BoundingBox::isRelFrameInVisibleDurationRect(const int &relFrame) {
    if(mDurationRectangle == nullptr) return true;
    return relFrame <= mDurationRectangle->getMaxFrameAsRelFrame() &&
           relFrame >= mDurationRectangle->getMinFrameAsRelFrame();
}

bool BoundingBox::isRelFrameFInVisibleDurationRect(const qreal &relFrame) {
    if(mDurationRectangle == nullptr) return true;
    return relFrame <= mDurationRectangle->getMaxFrameAsRelFrame() &&
           relFrame >= mDurationRectangle->getMinFrameAsRelFrame();
}

bool BoundingBox::isRelFrameVisibleAndInVisibleDurationRect(
        const int &relFrame) {
    return isRelFrameInVisibleDurationRect(relFrame) && mVisible;
}

bool BoundingBox::isRelFrameFVisibleAndInVisibleDurationRect(
        const qreal &relFrame) {
    return isRelFrameFInVisibleDurationRect(relFrame) && mVisible;
}

void BoundingBox::prp_getFirstAndLastIdenticalRelFrame(int *firstIdentical,
                                                       int *lastIdentical,
                                                       const int &relFrame) {
    if(mVisible) {
        if(isRelFrameInVisibleDurationRect(relFrame)) {
            ComplexAnimator::prp_getFirstAndLastIdenticalRelFrame(
                                        firstIdentical,
                                        lastIdentical,
                                        relFrame);
            if(mDurationRectangle != nullptr) {
                if(relFrame == mDurationRectangle->getMinFrameAsRelFrame()) {
                    *firstIdentical = relFrame;
                }
                if(relFrame == mDurationRectangle->getMaxFrameAsRelFrame()) {
                    *lastIdentical = relFrame;
                }
            }
        } else {
            if(relFrame > mDurationRectangle->getMaxFrameAsRelFrame()) {
                *firstIdentical = mDurationRectangle->getMaxFrameAsRelFrame() + 1;
                *lastIdentical = INT_MAX;
            } else if(relFrame < mDurationRectangle->getMinFrameAsRelFrame()) {
                *firstIdentical = INT_MIN;
                *lastIdentical = mDurationRectangle->getMinFrameAsRelFrame() - 1;
            }
        }
    } else {
        *firstIdentical = INT_MIN;
        *lastIdentical = INT_MAX;
    }
}


void BoundingBox::getFirstAndLastIdenticalForMotionBlur(int *firstIdentical,
                                                        int *lastIdentical,
                                                        const int &relFrame,
                                                        const bool &takeAncestorsIntoAccount) {
    if(mVisible) {
        if(isRelFrameInVisibleDurationRect(relFrame)) {
            int fId = INT_MIN;
            int lId = INT_MAX;
            {
                int fId_ = INT_MIN;
                int lId_ = INT_MAX;

                QList<Property*> propertiesT;
                getMotionBlurProperties(propertiesT);
                Q_FOREACH(Property* child, propertiesT) {
                    if(fId_ > lId_) {
                        break;
                    }
                    int fIdT_;
                    int lIdT_;
                    child->prp_getFirstAndLastIdenticalRelFrame(
                                &fIdT_, &lIdT_,
                                relFrame);
                    if(fIdT_ > fId_) {
                        fId_ = fIdT_;
                    }
                    if(lIdT_ < lId_) {
                        lId_ = lIdT_;
                    }
                }

                if(lId_ > fId_) {
                    fId = fId_;
                    lId = lId_;
                } else {
                    fId = relFrame;
                    lId = relFrame;
                }
            }
            *firstIdentical = fId;
            *lastIdentical = lId;
            if(mDurationRectangle != nullptr) {
                if(fId < mDurationRectangle->getMinFrameAsRelFrame()) {
                    *firstIdentical = relFrame;
                }
                if(lId > mDurationRectangle->getMaxFrameAsRelFrame()) {
                    *lastIdentical = relFrame;
                }
            }
        } else {
            if(relFrame > mDurationRectangle->getMaxFrameAsRelFrame()) {
                *firstIdentical = mDurationRectangle->getMaxFrameAsRelFrame() + 1;
                *lastIdentical = INT_MAX;
            } else if(relFrame < mDurationRectangle->getMinFrameAsRelFrame()) {
                *firstIdentical = INT_MIN;
                *lastIdentical = mDurationRectangle->getMinFrameAsRelFrame() - 1;
            }
        }
    } else {
        *firstIdentical = INT_MIN;
        *lastIdentical = INT_MAX;
    }
    if(mParentGroup == nullptr || takeAncestorsIntoAccount) return;
    if(*firstIdentical == relFrame && *lastIdentical == relFrame) return;
    int parentFirst;
    int parentLast;
    int parentRel = mParentGroup->prp_absFrameToRelFrame(
                prp_relFrameToAbsFrame(relFrame));
    mParentGroup->BoundingBox::getFirstAndLastIdenticalForMotionBlur(
                  &parentFirst,
                  &parentLast,
                  parentRel);
    if(parentFirst > *firstIdentical) {
        *firstIdentical = parentFirst;
    }
    if(parentLast < *lastIdentical) {
        *lastIdentical = parentLast;
    }
}

void BoundingBox::processSchedulers() {
    mBlockedSchedule = true;
    foreach(const std::shared_ptr<_ScheduledExecutor> &updatable, mSchedulers) {
        updatable->schedulerProccessed();
    }
}

void BoundingBox::addSchedulersToProcess() {
    foreach(const _ScheduledExecutorSPtr &updatable, mSchedulers) {
        MainWindow::getInstance()->addUpdateScheduler(updatable);
    }

    mSchedulers.clear();
    mBlockedSchedule = false;
}

const int &BoundingBox::getLoadId() {
    return mLoadId;
}

int BoundingBox::setBoxLoadId(const int &loadId) {
    mLoadId = loadId;
    return loadId + 1;
}

void BoundingBox::clearBoxLoadId() {
    mLoadId = -1;
}

BoundingBox *BoundingBox::getLoadedBoxById(const int &loadId) {
    foreach(const BoundingBoxQPtr& box, mLoadedBoxes) {
        if(box == nullptr) return nullptr;
        if(box->getLoadId() == loadId) {
            return box;
        }
    }
    return nullptr;
}

void BoundingBox::addFunctionWaitingForBoxLoad(const FunctionWaitingForBoxLoadSPtr &func) {
    mFunctionsWaitingForBoxLoad << func;
}

void BoundingBox::addLoadedBox(BoundingBox *box) {
    mLoadedBoxes << box;
    for(int i = 0; i < mFunctionsWaitingForBoxLoad.count(); i++) {
        FunctionWaitingForBoxLoadSPtr funcT =
                mFunctionsWaitingForBoxLoad.at(i);
        if(funcT->loadBoxId == box->getLoadId()) {
            funcT->boxLoaded(box);
            mFunctionsWaitingForBoxLoad.removeAt(i);
            i--;
        }
    }
}

int BoundingBox::getLoadedBoxesCount() {
    return mLoadedBoxes.count();
}

void BoundingBox::clearLoadedBoxes() {
    foreach(const BoundingBoxQPtr& box, mLoadedBoxes) {
        box->clearBoxLoadId();
    }
    mLoadedBoxes.clear();
    mFunctionsWaitingForBoxLoad.clear();
}

void BoundingBox::selectAllPoints(Canvas *canvas) {
    Q_UNUSED(canvas);
}

void BoundingBox::addScheduler(const _ScheduledExecutorSPtr& updatable) {
    mSchedulers << updatable;
}

void BoundingBox::setVisibile(const bool &visible,
                              const bool &saveUndoRedo) {
    if(mVisible == visible) return;
    if(mSelected) {
        removeFromSelection();
    }
    if(saveUndoRedo) {
        //        addUndoRedo(new SetBoxVisibleUndoRedo(this, mVisible, visible));
    }
    mVisible = visible;

    clearAllCache();

    scheduleUpdate(Animator::USER_CHANGE);

    SWT_scheduleWidgetsContentUpdateWithRule(SWT_Visible);
    SWT_scheduleWidgetsContentUpdateWithRule(SWT_Invisible);
    Q_FOREACH(BoundingBox* box, mLinkingBoxes) {
        if(box->isParentLinkBox()) {
            box->setVisibile(visible, false);
        }
    }
}

void BoundingBox::switchVisible() {
    setVisibile(!mVisible);
}

bool BoundingBox::isParentLinkBox() {
    return mParentGroup->SWT_isLinkBox();
}

void BoundingBox::switchLocked() {
    setLocked(!mLocked);
}

void BoundingBox::hide()
{
    setVisibile(false);
}

void BoundingBox::show()
{
    setVisibile(true);
}

bool BoundingBox::isVisibleAndUnlocked() {
    return isVisible() && !mLocked;
}

bool BoundingBox::isVisible() {
    return mVisible;
}

bool BoundingBox::isLocked() {
    return mLocked;
}

void BoundingBox::lock() {
    setLocked(true);
}

void BoundingBox::unlock() {
    setLocked(false);
}

void BoundingBox::setLocked(const bool &bt) {
    if(bt == mLocked) return;
    if(mSelected) {
        getParentCanvas()->removeBoxFromSelection(this);
    }
    mLocked = bt;
    SWT_scheduleWidgetsContentUpdateWithRule(SWT_Locked);
    SWT_scheduleWidgetsContentUpdateWithRule(SWT_Unlocked);
}

bool BoundingBox::SWT_shouldBeVisible(const SWT_RulesCollection &rules,
                                      const bool &parentSatisfies,
                                      const bool &parentMainTarget) {
    const SWT_Rule &rule = rules.rule;
    bool satisfies = false;
    bool alwaysShowChildren = rules.alwaysShowChildren;
    if(rules.type == &SingleWidgetTarget::SWT_isSingleSound) return false;
    if(alwaysShowChildren) {
        if(rule == SWT_NoRule) {
            satisfies = parentSatisfies;
        } else if(rule == SWT_Selected) {
            satisfies = isSelected() ||
                    (parentSatisfies && !parentMainTarget);
        } else if(rule == SWT_Animated) {
            satisfies = isAnimated() ||
                    (parentSatisfies && !parentMainTarget);
        } else if(rule == SWT_NotAnimated) {
            satisfies = !isAnimated() ||
                    (parentSatisfies && !parentMainTarget);
        } else if(rule == SWT_Visible) {
            satisfies = isVisible() ||
                    (parentSatisfies && !parentMainTarget);
        } else if(rule == SWT_Invisible) {
            satisfies = !isVisible() ||
                    (parentSatisfies && !parentMainTarget);
        } else if(rule == SWT_Locked) {
            satisfies = isLocked() ||
                    (parentSatisfies && !parentMainTarget);
        } else if(rule == SWT_Unlocked) {
            satisfies = !isLocked() ||
                    (parentSatisfies && !parentMainTarget);
        }
    } else {
        if(rule == SWT_NoRule) {
            satisfies = parentSatisfies;
        } else if(rule == SWT_Selected) {
            satisfies = isSelected();
        } else if(rule == SWT_Animated) {
            satisfies = isAnimated();
        } else if(rule == SWT_NotAnimated) {
            satisfies = !isAnimated();
        } else if(rule == SWT_Visible) {
            satisfies = isVisible() && parentSatisfies;
        } else if(rule == SWT_Invisible) {
            satisfies = !isVisible() ||
                    (parentSatisfies && !parentMainTarget);
        } else if(rule == SWT_Locked) {
            satisfies = isLocked() ||
                    (parentSatisfies && !parentMainTarget);
        } else if(rule == SWT_Unlocked) {
            satisfies = !isLocked() && parentSatisfies;
        }
    }
    if(satisfies) {
        const QString &nameSearch = rules.searchString;
        if(!nameSearch.isEmpty()) {
            satisfies = prp_mName.contains(nameSearch);
        }
    }
    return satisfies;
}

bool BoundingBox::SWT_visibleOnlyIfParentDescendant() {
    return false;
}

void BoundingBox::SWT_addToContextMenu(
        QMenu *menu) {
    menu->addAction("Apply Transformation");
    menu->addAction("Create Link");
    menu->addAction("Center Pivot");
    menu->addAction("Copy");
    menu->addAction("Cut");
    menu->addAction("Duplicate");
    menu->addAction("Group");
    menu->addAction("Ungroup");
    menu->addAction("Delete");

    QMenu *effectsMenu = menu->addMenu("Effects");
    effectsMenu->addAction("Blur");
    effectsMenu->addAction("Shadow");
//            effectsMenu->addAction("Brush");
    effectsMenu->addAction("Lines");
    effectsMenu->addAction("Circles");
    effectsMenu->addAction("Swirl");
    effectsMenu->addAction("Oil");
    effectsMenu->addAction("Implode");
    effectsMenu->addAction("Desaturate");
}

void BoundingBox::removeFromParent() {
    mParentGroup->removeContainedBox(GetAsSPtr(this, BoundingBox));
}

void BoundingBox::removeFromSelection() {
    if(mSelected) {
        Canvas* parentCanvas = getParentCanvas();
        parentCanvas->removeBoxFromSelection(this);
    }
}

bool BoundingBox::SWT_handleContextMenuActionSelected(
        QAction *selectedAction) {
    if(selectedAction != nullptr) {
        if(selectedAction->text() == "Delete") {
            mParentGroup->removeContainedBox(GetAsSPtr(this, BoundingBox));
        } else if(selectedAction->text() == "Apply Transformation") {
            applyCurrentTransformation();
        } else if(selectedAction->text() == "Create Link") {
            mParentGroup->addContainedBox(createLink());
        } else if(selectedAction->text() == "Group") {
            getParentCanvas()->groupSelectedBoxes();
            return true;
//        } else if(selectedAction->text() == "Ungroup") {
//            ungroupSelected();
//        } else if(selectedAction->text() == "Center Pivot") {
//            mCurrentBoxesGroup->centerPivotForSelected();
//        } else if(selectedAction->text() == "Blur") {
//            mCurrentBoxesGroup->applyBlurToSelected();
//        } else if(selectedAction->text() == "Shadow") {
//            mCurrentBoxesGroup->applyShadowToSelected();
//        } else if(selectedAction->text() == "Brush") {
//            mCurrentBoxesGroup->applyBrushEffectToSelected();
//        } else if(selectedAction->text() == "Lines") {
//            mCurrentBoxesGroup->applyLinesEffectToSelected();
//        } else if(selectedAction->text() == "Circles") {
//            mCurrentBoxesGroup->applyCirclesEffectToSelected();
//        } else if(selectedAction->text() == "Swirl") {
//            mCurrentBoxesGroup->applySwirlEffectToSelected();
//        } else if(selectedAction->text() == "Oil") {
//            mCurrentBoxesGroup->applyOilEffectToSelected();
//        } else if(selectedAction->text() == "Implode") {
//            mCurrentBoxesGroup->applyImplodeEffectToSelected();
//        } else if(selectedAction->text() == "Desaturate") {
//            mCurrentBoxesGroup->applyDesaturateEffectToSelected();
        }
    } else {

    }
    return false;
}

QMimeData *BoundingBox::SWT_createMimeData() {
    return new BoundingBoxMimeData(this);
}

void BoundingBox::renderDataFinished(BoundingBoxRenderData *renderData) {
    mExpiredPixmap = 0;
    if(renderData->redo) {
        scheduleUpdate(renderData->relFrame, Animator::USER_CHANGE);
    }
    mDrawRenderContainer->setSrcRenderData(renderData);
    updateDrawRenderContainerTransform();
}

void BoundingBox::getVisibleAbsFrameRange(int *minFrame, int *maxFrame) {
    if(mDurationRectangle == nullptr) {
        *minFrame = INT_MIN;
        *maxFrame = INT_MAX;
    } else {
        *minFrame = mDurationRectangle->getMinFrameAsAbsFrame();
        *maxFrame = mDurationRectangle->getMaxFrameAsAbsFrame();
    }
}

FunctionWaitingForBoxLoad::FunctionWaitingForBoxLoad(const int &boxIdT) {
    loadBoxId = boxIdT;
}