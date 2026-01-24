#include "AutomationCurveEditor.hpp"

#include <algorithm>
#include <cmath>

namespace magda {

AutomationCurveEditor::AutomationCurveEditor(AutomationLaneId laneId) : laneId_(laneId) {
    setName("AutomationCurveEditor");

    // Register listeners
    AutomationManager::getInstance().addListener(this);
    SelectionManager::getInstance().addListener(this);

    rebuildPointComponents();
}

AutomationCurveEditor::~AutomationCurveEditor() {
    AutomationManager::getInstance().removeListener(this);
    SelectionManager::getInstance().removeListener(this);
}

void AutomationCurveEditor::paint(juce::Graphics& g) {
    // Background
    g.fillAll(juce::Colour(0xFF1A1A1A));

    // Grid
    paintGrid(g);

    // Curve
    paintCurve(g);

    // Drawing preview
    if (isDrawing_) {
        paintDrawingPreview(g);
    }
}

void AutomationCurveEditor::resized() {
    updatePointPositions();
}

void AutomationCurveEditor::paintGrid(juce::Graphics& g) {
    auto bounds = getLocalBounds();

    // Subtle horizontal grid lines (value levels at 25%, 50%, 75%)
    g.setColour(juce::Colour(0x15FFFFFF));
    for (int i = 1; i < 4; ++i) {
        int y = bounds.getHeight() * i / 4;
        g.drawHorizontalLine(y, 0.0f, static_cast<float>(bounds.getWidth()));
    }
}

void AutomationCurveEditor::paintCurve(juce::Graphics& g) {
    const auto& points = getPoints();
    if (points.empty())
        return;

    // Helper to get effective position (preview or actual)
    auto getEffectivePos = [this](const AutomationPoint& p) -> std::pair<double, double> {
        if (previewPointId_ != INVALID_AUTOMATION_POINT_ID && p.id == previewPointId_) {
            return {previewTime_, previewValue_};
        }
        return {p.time, p.value};
    };

    // Create path for curve
    juce::Path curvePath;
    bool pathStarted = false;

    // Draw from left edge to first point
    if (!points.empty()) {
        auto [firstTime, firstValue] = getEffectivePos(points.front());
        int firstX = timeToPixel(firstTime);
        int firstY = valueToPixel(firstValue);

        if (firstX > 0) {
            // Draw horizontal line from left edge at first point's value
            curvePath.startNewSubPath(0.0f, static_cast<float>(firstY));
            curvePath.lineTo(static_cast<float>(firstX), static_cast<float>(firstY));
            pathStarted = true;
        }
    }

    // Draw between points
    for (size_t i = 0; i < points.size(); ++i) {
        const auto& p = points[i];
        auto [time, value] = getEffectivePos(p);
        int x = timeToPixel(time);
        int y = valueToPixel(value);

        if (!pathStarted) {
            curvePath.startNewSubPath(static_cast<float>(x), static_cast<float>(y));
            pathStarted = true;
        } else if (i > 0) {
            const auto& prevP = points[i - 1];

            switch (prevP.curveType) {
                case AutomationCurveType::Linear: {
                    // Check if tension is applied
                    if (std::abs(prevP.tension) < 0.001) {
                        // Pure linear
                        curvePath.lineTo(static_cast<float>(x), static_cast<float>(y));
                    } else {
                        // Tension-based curve - draw as series of line segments
                        auto [prevTime, prevValue] = getEffectivePos(prevP);
                        int prevX = timeToPixel(prevTime);
                        int prevY = valueToPixel(prevValue);

                        const int NUM_SEGMENTS = 16;
                        for (int seg = 1; seg <= NUM_SEGMENTS; ++seg) {
                            double t = static_cast<double>(seg) / NUM_SEGMENTS;

                            // Apply tension curve
                            double curvedT;
                            if (prevP.tension > 0) {
                                curvedT = std::pow(t, 1.0 + prevP.tension * 2.0);
                            } else {
                                curvedT = 1.0 - std::pow(1.0 - t, 1.0 - prevP.tension * 2.0);
                            }

                            double segValue = prevValue + curvedT * (value - prevValue);
                            double segTime = prevTime + t * (time - prevTime);

                            float segX = static_cast<float>(timeToPixel(segTime));
                            float segY = static_cast<float>(valueToPixel(segValue));

                            curvePath.lineTo(segX, segY);
                        }
                    }
                    break;
                }

                case AutomationCurveType::Bezier: {
                    // Calculate control points using effective positions
                    auto [prevTime, prevValue] = getEffectivePos(prevP);
                    int prevX = timeToPixel(prevTime);
                    int prevY = valueToPixel(prevValue);

                    float cp1X =
                        prevX + static_cast<float>(prevP.outHandle.time * pixelsPerSecond_);
                    float cp1Y =
                        prevY - static_cast<float>(prevP.outHandle.value * getPixelsPerValue());
                    float cp2X = x + static_cast<float>(p.inHandle.time * pixelsPerSecond_);
                    float cp2Y = y - static_cast<float>(p.inHandle.value * getPixelsPerValue());

                    curvePath.cubicTo(cp1X, cp1Y, cp2X, cp2Y, static_cast<float>(x),
                                      static_cast<float>(y));
                    break;
                }

                case AutomationCurveType::Step:
                    // Step: horizontal then vertical
                    curvePath.lineTo(static_cast<float>(x), curvePath.getCurrentPosition().y);
                    curvePath.lineTo(static_cast<float>(x), static_cast<float>(y));
                    break;
            }
        }
    }

    // Draw from last point to right edge
    if (!points.empty()) {
        auto [lastTime, lastValue] = getEffectivePos(points.back());
        juce::ignoreUnused(lastTime);
        int lastY = valueToPixel(lastValue);
        int width = getWidth();
        curvePath.lineTo(static_cast<float>(width), static_cast<float>(lastY));
    }

    // Draw the curve
    g.setColour(juce::Colour(0xFF6688CC));
    g.strokePath(curvePath, juce::PathStrokeType(2.0f));

    // Optional: fill under curve
    juce::Path fillPath = curvePath;
    fillPath.lineTo(static_cast<float>(getWidth()), static_cast<float>(getHeight()));
    fillPath.lineTo(0.0f, static_cast<float>(getHeight()));
    fillPath.closeSubPath();
    g.setColour(juce::Colour(0x226688CC));
    g.fillPath(fillPath);
}

void AutomationCurveEditor::paintDrawingPreview(juce::Graphics& g) {
    if (drawMode_ == AutomationDrawMode::Pencil && !drawingPath_.empty()) {
        g.setColour(juce::Colour(0xAAFFFFFF));
        for (size_t i = 1; i < drawingPath_.size(); ++i) {
            g.drawLine(static_cast<float>(drawingPath_[i - 1].x),
                       static_cast<float>(drawingPath_[i - 1].y),
                       static_cast<float>(drawingPath_[i].x), static_cast<float>(drawingPath_[i].y),
                       2.0f);
        }
    } else if (drawMode_ == AutomationDrawMode::Line && isDrawing_) {
        g.setColour(juce::Colour(0xAAFFFFFF));
        auto mousePos = getMouseXYRelative();
        g.drawLine(static_cast<float>(lineStartPoint_.x), static_cast<float>(lineStartPoint_.y),
                   static_cast<float>(mousePos.x), static_cast<float>(mousePos.y), 2.0f);
    }
}

void AutomationCurveEditor::mouseDown(const juce::MouseEvent& e) {
    if (e.mods.isLeftButtonDown()) {
        switch (drawMode_) {
            case AutomationDrawMode::Select:
                // Click on empty area clears selection
                if (!getComponentAt(e.getPosition())) {
                    SelectionManager::getInstance().clearAutomationPointSelection();
                }
                break;

            case AutomationDrawMode::Pencil:
                isDrawing_ = true;
                drawingPath_.clear();
                drawingPath_.push_back(e.getPosition());
                break;

            case AutomationDrawMode::Line:
                isDrawing_ = true;
                lineStartPoint_ = e.getPosition();
                break;

            case AutomationDrawMode::Curve:
                // Similar to pencil but creates bezier points
                isDrawing_ = true;
                drawingPath_.clear();
                drawingPath_.push_back(e.getPosition());
                break;
        }
    }
}

void AutomationCurveEditor::mouseDrag(const juce::MouseEvent& e) {
    if (!isDrawing_)
        return;

    if (drawMode_ == AutomationDrawMode::Pencil || drawMode_ == AutomationDrawMode::Curve) {
        drawingPath_.push_back(e.getPosition());
        repaint();
    } else if (drawMode_ == AutomationDrawMode::Line) {
        repaint();  // Redraw line preview
    }
}

void AutomationCurveEditor::mouseUp(const juce::MouseEvent& e) {
    if (isDrawing_) {
        isDrawing_ = false;

        switch (drawMode_) {
            case AutomationDrawMode::Pencil:
                createPointsFromDrawingPath();
                break;

            case AutomationDrawMode::Line: {
                // Create two points: start and end
                double startTime = pixelToTime(lineStartPoint_.x);
                double startValue = pixelToValue(lineStartPoint_.y);
                double endTime = pixelToTime(e.x);
                double endValue = pixelToValue(e.y);

                addPointAt(startTime, startValue, AutomationCurveType::Linear);
                addPointAt(endTime, endValue, AutomationCurveType::Linear);
                break;
            }

            case AutomationDrawMode::Curve:
                createPointsFromDrawingPath();
                break;

            default:
                break;
        }

        drawingPath_.clear();
        repaint();
    }
}

void AutomationCurveEditor::mouseDoubleClick(const juce::MouseEvent& e) {
    // Double-click to add a point
    double time = pixelToTime(e.x);
    double value = pixelToValue(e.y);

    // Snap time if enabled
    if (snapTimeToGrid) {
        time = snapTimeToGrid(time);
    }

    AutomationCurveType curveType = (drawMode_ == AutomationDrawMode::Curve)
                                        ? AutomationCurveType::Bezier
                                        : AutomationCurveType::Linear;

    addPointAt(time, value, curveType);
}

bool AutomationCurveEditor::keyPressed(const juce::KeyPress& key) {
    if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey) {
        deleteSelectedPoints();
        return true;
    }
    return false;
}

// AutomationManagerListener
void AutomationCurveEditor::automationLanesChanged() {
    rebuildPointComponents();
}

void AutomationCurveEditor::automationLanePropertyChanged(AutomationLaneId laneId) {
    if (laneId == laneId_) {
        repaint();
    }
}

void AutomationCurveEditor::automationPointsChanged(AutomationLaneId laneId) {
    if (laneId == laneId_) {
        // Clear preview when points are committed
        previewPointId_ = INVALID_AUTOMATION_POINT_ID;
        rebuildPointComponents();
    }
}

void AutomationCurveEditor::automationPointDragPreview(AutomationLaneId laneId,
                                                       AutomationPointId pointId,
                                                       double previewTime, double previewValue) {
    if (laneId != laneId_)
        return;

    previewPointId_ = pointId;
    previewTime_ = previewTime;
    previewValue_ = previewValue;

    // Update the point component position for visual feedback
    for (auto& pc : pointComponents_) {
        if (pc->getPointId() == pointId) {
            int x = timeToPixel(previewTime);
            int y = valueToPixel(previewValue);
            pc->setCentrePosition(x, y);
            break;
        }
    }

    repaint();
}

// SelectionManagerListener
void AutomationCurveEditor::selectionTypeChanged(SelectionType newType) {
    juce::ignoreUnused(newType);
    syncSelectionState();
}

void AutomationCurveEditor::automationPointSelectionChanged(
    const AutomationPointSelection& selection) {
    juce::ignoreUnused(selection);
    syncSelectionState();
}

void AutomationCurveEditor::setLaneId(AutomationLaneId laneId) {
    if (laneId_ != laneId) {
        laneId_ = laneId;
        rebuildPointComponents();
    }
}

double AutomationCurveEditor::pixelToTime(int x) const {
    return (x / pixelsPerSecond_) + clipOffset_;
}

int AutomationCurveEditor::timeToPixel(double time) const {
    return static_cast<int>((time - clipOffset_) * pixelsPerSecond_);
}

double AutomationCurveEditor::pixelToValue(int y) const {
    int height = getHeight();
    if (height <= 0)
        return 0.5;
    return 1.0 - (static_cast<double>(y) / height);
}

int AutomationCurveEditor::valueToPixel(double value) const {
    return static_cast<int>((1.0 - value) * getHeight());
}

const std::vector<AutomationPoint>& AutomationCurveEditor::getPoints() const {
    static std::vector<AutomationPoint> empty;

    if (clipId_ != INVALID_AUTOMATION_CLIP_ID) {
        const auto* clip = AutomationManager::getInstance().getClip(clipId_);
        if (clip)
            return clip->points;
        return empty;
    }

    const auto* lane = AutomationManager::getInstance().getLane(laneId_);
    if (lane && lane->isAbsolute())
        return lane->absolutePoints;

    return empty;
}

void AutomationCurveEditor::addPointAt(double time, double value, AutomationCurveType curveType) {
    auto& manager = AutomationManager::getInstance();

    if (clipId_ != INVALID_AUTOMATION_CLIP_ID) {
        manager.addPointToClip(clipId_, time - clipOffset_, value, curveType);
    } else {
        manager.addPoint(laneId_, time, value, curveType);
    }
}

void AutomationCurveEditor::deleteSelectedPoints() {
    auto& selectionManager = SelectionManager::getInstance();
    if (!selectionManager.hasAutomationPointSelection())
        return;

    const auto& selection = selectionManager.getAutomationPointSelection();
    if (selection.laneId != laneId_)
        return;

    auto& manager = AutomationManager::getInstance();

    // Delete in reverse order to maintain indices
    auto pointIds = selection.pointIds;
    for (auto it = pointIds.rbegin(); it != pointIds.rend(); ++it) {
        if (selection.clipId != INVALID_AUTOMATION_CLIP_ID) {
            manager.deletePointFromClip(selection.clipId, *it);
        } else {
            manager.deletePoint(laneId_, *it);
        }
    }

    selectionManager.clearAutomationPointSelection();
}

void AutomationCurveEditor::rebuildPointComponents() {
    pointComponents_.clear();
    handleComponents_.clear();
    tensionHandles_.clear();

    const auto& points = getPoints();
    for (const auto& point : points) {
        auto pc = std::make_unique<AutomationPointComponent>(point.id, this);
        pc->updateFromPoint(point);

        // Set callbacks
        pc->onPointSelected = [this](AutomationPointId pointId) {
            SelectionManager::getInstance().selectAutomationPoint(laneId_, pointId, clipId_);
        };

        pc->onPointMoved = [this](AutomationPointId pointId, double newTime, double newValue) {
            auto& manager = AutomationManager::getInstance();
            if (clipId_ != INVALID_AUTOMATION_CLIP_ID) {
                manager.movePointInClip(clipId_, pointId, newTime - clipOffset_, newValue);
            } else {
                manager.movePoint(laneId_, pointId, newTime, newValue);
            }
        };

        pc->onPointDragPreview = [this](AutomationPointId pointId, double newTime,
                                        double newValue) {
            AutomationManager::getInstance().notifyPointDragPreview(laneId_, pointId, newTime,
                                                                    newValue);
        };

        pc->onPointDeleted = [this](AutomationPointId pointId) {
            auto& manager = AutomationManager::getInstance();
            if (clipId_ != INVALID_AUTOMATION_CLIP_ID) {
                manager.deletePointFromClip(clipId_, pointId);
            } else {
                manager.deletePoint(laneId_, pointId);
            }
        };

        pc->onHandlesChanged = [this](AutomationPointId pointId, const BezierHandle& inHandle,
                                      const BezierHandle& outHandle) {
            auto& manager = AutomationManager::getInstance();
            if (clipId_ != INVALID_AUTOMATION_CLIP_ID) {
                manager.setPointHandlesInClip(clipId_, pointId, inHandle, outHandle);
            } else {
                manager.setPointHandles(laneId_, pointId, inHandle, outHandle);
            }
        };

        addAndMakeVisible(pc.get());
        pointComponents_.push_back(std::move(pc));
    }

    // Create tension handles for each curve segment (between consecutive points)
    // Only for Linear curve type - Bezier uses handles, Step has no curve
    for (size_t i = 0; i < points.size(); ++i) {
        const auto& point = points[i];

        // Only create tension handle if this isn't the last point
        // and the curve type is Linear (tension applies to Linear curves)
        if (i < points.size() - 1 && point.curveType == AutomationCurveType::Linear) {
            auto th = std::make_unique<TensionHandleComponent>(point.id);
            th->setTension(point.tension);

            th->onTensionChanged = [this](AutomationPointId pointId, double tension) {
                auto& manager = AutomationManager::getInstance();
                if (clipId_ != INVALID_AUTOMATION_CLIP_ID) {
                    manager.setPointTensionInClip(clipId_, pointId, tension);
                } else {
                    manager.setPointTension(laneId_, pointId, tension);
                }
            };

            th->onTensionDragPreview = [this](AutomationPointId /*pointId*/, double /*tension*/) {
                // Just repaint to show curve preview
                repaint();
            };

            addAndMakeVisible(th.get());
            tensionHandles_.push_back(std::move(th));
        }
    }

    updatePointPositions();
    syncSelectionState();
}

void AutomationCurveEditor::updatePointPositions() {
    const auto& points = getPoints();

    for (size_t i = 0; i < pointComponents_.size() && i < points.size(); ++i) {
        const auto& point = points[i];
        int x = timeToPixel(point.time);
        int y = valueToPixel(point.value);

        pointComponents_[i]->setCentrePosition(x, y);
        pointComponents_[i]->updateFromPoint(point);
    }

    // Position tension handles at the midpoint of each curve segment
    size_t tensionIdx = 0;
    for (size_t i = 0; i < points.size() - 1 && tensionIdx < tensionHandles_.size(); ++i) {
        const auto& p1 = points[i];
        const auto& p2 = points[i + 1];

        // Only position for Linear curves (tension handles only exist for those)
        if (p1.curveType == AutomationCurveType::Linear) {
            // Calculate midpoint position
            double midTime = (p1.time + p2.time) / 2.0;
            double midValue = (p1.value + p2.value) / 2.0;

            // Apply tension to get the actual curve position at midpoint
            // This makes the handle follow the curve as tension changes
            if (std::abs(p1.tension) > 0.001) {
                double t = 0.5;
                double curvedT;
                if (p1.tension > 0) {
                    curvedT = std::pow(t, 1.0 + p1.tension * 2.0);
                } else {
                    curvedT = 1.0 - std::pow(1.0 - t, 1.0 - p1.tension * 2.0);
                }
                midValue = p1.value + curvedT * (p2.value - p1.value);
            }

            int x = timeToPixel(midTime);
            int y = valueToPixel(midValue);

            tensionHandles_[tensionIdx]->setCentrePosition(x, y);
            tensionHandles_[tensionIdx]->setTension(p1.tension);
            ++tensionIdx;
        }
    }
}

void AutomationCurveEditor::syncSelectionState() {
    auto& selectionManager = SelectionManager::getInstance();
    const auto& selection = selectionManager.getAutomationPointSelection();

    bool isOurSelection = selectionManager.getSelectionType() == SelectionType::AutomationPoint &&
                          selection.laneId == laneId_ &&
                          (clipId_ == INVALID_AUTOMATION_CLIP_ID || selection.clipId == clipId_);

    for (auto& pc : pointComponents_) {
        bool isSelected = false;
        if (isOurSelection) {
            isSelected = std::find(selection.pointIds.begin(), selection.pointIds.end(),
                                   pc->getPointId()) != selection.pointIds.end();
        }
        pc->setSelected(isSelected);
    }

    repaint();
}

void AutomationCurveEditor::createPointsFromDrawingPath() {
    if (drawingPath_.size() < 2)
        return;

    // Simplify path - don't create a point for every pixel
    const int MIN_PIXEL_DISTANCE = 10;

    std::vector<juce::Point<int>> simplifiedPath;
    simplifiedPath.push_back(drawingPath_.front());

    for (size_t i = 1; i < drawingPath_.size(); ++i) {
        const auto& lastAdded = simplifiedPath.back();
        const auto& current = drawingPath_[i];
        int dx = current.x - lastAdded.x;
        int dy = current.y - lastAdded.y;
        int distSq = dx * dx + dy * dy;

        if (distSq >= MIN_PIXEL_DISTANCE * MIN_PIXEL_DISTANCE) {
            simplifiedPath.push_back(current);
        }
    }

    // Always include last point
    if (simplifiedPath.back() != drawingPath_.back()) {
        simplifiedPath.push_back(drawingPath_.back());
    }

    // Create automation points
    AutomationCurveType curveType = (drawMode_ == AutomationDrawMode::Curve)
                                        ? AutomationCurveType::Bezier
                                        : AutomationCurveType::Linear;

    for (const auto& pixelPoint : simplifiedPath) {
        double time = pixelToTime(pixelPoint.x);
        double value = pixelToValue(pixelPoint.y);

        if (snapTimeToGrid) {
            time = snapTimeToGrid(time);
        }

        addPointAt(time, value, curveType);
    }
}

}  // namespace magda
