#include "ImVectorEditor.h"

#include <algorithm>
#include <cmath>

namespace ImVectorEditor {
namespace {

static constexpr float kMinZoom = 0.0001f;

static ImVec2 add(const ImVec2& a, const ImVec2& b) {
    return ImVec2(a.x + b.x, a.y + b.y);
}

static ImVec2 sub(const ImVec2& a, const ImVec2& b) {
    return ImVec2(a.x - b.x, a.y - b.y);
}

static ImVec2 mul(const ImVec2& a, float s) {
    return ImVec2(a.x * s, a.y * s);
}

static ImVec2 mul(const ImVec2& a, const ImVec2& b) {
    return ImVec2(a.x * b.x, a.y * b.y);
}

static float lengthSq(const ImVec2& v) {
    return v.x * v.x + v.y * v.y;
}

static float length(const ImVec2& v) {
    return std::sqrt(lengthSq(v));
}

static ImVec2 rotate(const ImVec2& p, float radians) {
    const float c = std::cos(radians);
    const float s = std::sin(radians);
    return ImVec2(p.x * c - p.y * s, p.x * s + p.y * c);
}

static ImVec2 snapToAngleIncrement(const ImVec2& v, float incrementRadians) {
    const float len = length(v);
    if (len <= kMinZoom) {
        return v;
    }

    const float angle = std::atan2(v.y, v.x);
    const float snappedAngle = std::round(angle / incrementRadians) * incrementRadians;
    return ImVec2(std::cos(snappedAngle) * len, std::sin(snappedAngle) * len);
}

static bool isShiftDown() {
#if IMGUI_VERSION_NUM >= 18700
    return ImGui::IsKeyDown(ImGuiMod_Shift) || ImGui::GetIO().KeyShift;
#else
    return ImGui::GetIO().KeyShift;
#endif
}

static bool alignHandles(Anchor& anchor) {
    if (!anchor.hasHandleIn && !anchor.hasHandleOut) {
        return AddHandles(anchor);
    }

    if (!anchor.hasHandleIn) {
        anchor.handleIn = mul(anchor.handleOut, -1.0f);
        anchor.hasHandleIn = true;
        anchor.handleMode = HandleMode::Aligned;
        return true;
    }

    if (!anchor.hasHandleOut) {
        anchor.handleOut = mul(anchor.handleIn, -1.0f);
        anchor.hasHandleOut = true;
        anchor.handleMode = HandleMode::Aligned;
        return true;
    }

    const float inLength = length(anchor.handleIn);
    const float outLength = length(anchor.handleOut);
    ImVec2 direction = outLength >= inLength ? anchor.handleOut : mul(anchor.handleIn, -1.0f);
    float directionLength = length(direction);
    if (directionLength <= kMinZoom) {
        return false;
    }

    const ImVec2 unit = mul(direction, 1.0f / directionLength);
    anchor.handleOut = mul(unit, outLength);
    anchor.handleIn = mul(unit, -inLength);
    anchor.handleMode = HandleMode::Aligned;
    return true;
}

static ImVec2 alignedOppositeHandle(const ImVec2& handle, float oppositeLength) {
    const float handleLength = length(handle);
    if (handleLength <= kMinZoom) {
        return ImVec2(0.0f, 0.0f);
    }

    return mul(handle, -oppositeLength / handleLength);
}

static bool hasCurveBetween(const Anchor& a, const Anchor& b) {
    return a.hasHandleOut || b.hasHandleIn;
}

static void drawControlPoint(ImDrawList* drawList, const ImVec2& center, float radius,
                             ControlPointShape shape, ImU32 color) {
    switch (shape) {
    case ControlPointShape::Circle:
        drawList->AddCircleFilled(center, radius, color);
        break;
    case ControlPointShape::Square:
        drawList->AddRectFilled(sub(center, ImVec2(radius, radius)),
                                add(center, ImVec2(radius, radius)), color);
        break;
    case ControlPointShape::Diamond:
        drawList->AddQuadFilled(ImVec2(center.x, center.y - radius),
                                ImVec2(center.x + radius, center.y),
                                ImVec2(center.x, center.y + radius),
                                ImVec2(center.x - radius, center.y), color);
        break;
    }
}

static void drawSegment(ImDrawList* drawList, const Config& config,
                        const ImVec2& origin, const Anchor& a, const Anchor& b,
                        ImU32 color, float thickness) {
    const ImVec2 p0 = add(origin, config.transform.LocalToCanvas(a.position));
    const ImVec2 p3 = add(origin, config.transform.LocalToCanvas(b.position));

    if (!hasCurveBetween(a, b)) {
        drawList->AddLine(p0, p3, color, thickness);
        return;
    }

    const ImVec2 cp1Local = add(a.position, a.hasHandleOut ? a.handleOut : ImVec2(0.0f, 0.0f));
    const ImVec2 cp2Local = add(b.position, b.hasHandleIn ? b.handleIn : ImVec2(0.0f, 0.0f));
    const ImVec2 cp1 = add(origin, config.transform.LocalToCanvas(cp1Local));
    const ImVec2 cp2 = add(origin, config.transform.LocalToCanvas(cp2Local));
    drawList->AddBezierCubic(p0, cp1, cp2, p3, color, thickness, 32);
}

} // namespace

bool MakeCorner(Anchor& anchor) {
    const bool changed = DeleteHandles(anchor) || anchor.handleMode != HandleMode::Corner;
    anchor.handleMode = HandleMode::Corner;
    return changed;
}

bool MakeAligned(Anchor& anchor) {
    return alignHandles(anchor);
}

bool MakeMirrored(Anchor& anchor) {
    if (!anchor.hasHandleIn && !anchor.hasHandleOut) {
        const bool changed = AddHandles(anchor);
        anchor.handleMode = HandleMode::Mirrored;
        return changed;
    }

    ImVec2 direction = anchor.hasHandleOut ? anchor.handleOut : mul(anchor.handleIn, -1.0f);
    float directionLength = length(direction);
    if (directionLength <= kMinZoom) {
        direction = anchor.hasHandleIn ? mul(anchor.handleIn, -1.0f) : anchor.handleOut;
        directionLength = length(direction);
    }
    if (directionLength <= kMinZoom) {
        return false;
    }

    const float inLength = anchor.hasHandleIn ? length(anchor.handleIn) : 0.0f;
    const float outLength = anchor.hasHandleOut ? length(anchor.handleOut) : 0.0f;
    const float mirroredLength = std::max(inLength, outLength);
    const ImVec2 unit = mul(direction, 1.0f / directionLength);
    anchor.handleOut = mul(unit, mirroredLength);
    anchor.handleIn = mul(unit, -mirroredLength);
    anchor.hasHandleIn = true;
    anchor.hasHandleOut = true;
    anchor.handleMode = HandleMode::Mirrored;
    return true;
}

bool AddHandles(Anchor& anchor, float handleLength) {
    if (anchor.hasHandleIn && anchor.hasHandleOut) {
        return false;
    }

    if (anchor.hasHandleIn) {
        const float inLength = std::max(length(anchor.handleIn), handleLength);
        const ImVec2 out = alignedOppositeHandle(anchor.handleIn, inLength);
        anchor.handleOut = lengthSq(out) > 0.0f ? out : ImVec2(handleLength, 0.0f);
    } else if (anchor.hasHandleOut) {
        const float outLength = std::max(length(anchor.handleOut), handleLength);
        const ImVec2 in = alignedOppositeHandle(anchor.handleOut, outLength);
        anchor.handleIn = lengthSq(in) > 0.0f ? in : ImVec2(-handleLength, 0.0f);
    } else {
        anchor.handleIn = ImVec2(-handleLength, 0.0f);
        anchor.handleOut = ImVec2(handleLength, 0.0f);
    }

    anchor.hasHandleIn = true;
    anchor.hasHandleOut = true;
    anchor.handleMode = HandleMode::Aligned;
    return true;
}

bool DeleteHandles(Anchor& anchor) {
    if (!anchor.hasHandleIn && !anchor.hasHandleOut &&
        anchor.handleMode == HandleMode::Corner) {
        return false;
    }

    anchor.handleIn = ImVec2(0.0f, 0.0f);
    anchor.handleOut = ImVec2(0.0f, 0.0f);
    anchor.hasHandleIn = false;
    anchor.hasHandleOut = false;
    anchor.handleMode = HandleMode::Corner;
    return true;
}

void ReversePath(Path& path) {
    std::reverse(path.anchors.begin(), path.anchors.end());
    for (Anchor& anchor : path.anchors) {
        std::swap(anchor.handleIn, anchor.handleOut);
        std::swap(anchor.hasHandleIn, anchor.hasHandleOut);
    }
}

ImVec2 Transform::LocalToCanvas(const ImVec2& local) const {
    const float safeZoom = std::max(zoom, kMinZoom);
    ImVec2 p = sub(local, objectPivot);
    p = mul(p, objectScale);
    p = rotate(p, objectRotationRadians);
    p = add(p, objectPivot);
    p = add(p, objectTranslation);
    p = mul(p, safeZoom);
    return add(p, pan);
}

ImVec2 Transform::CanvasToLocal(const ImVec2& canvas) const {
    const float safeZoom = std::max(zoom, kMinZoom);
    ImVec2 p = sub(canvas, pan);
    p = mul(p, 1.0f / safeZoom);
    p = sub(p, objectTranslation);
    p = sub(p, objectPivot);
    p = rotate(p, -objectRotationRadians);
    const float sx = std::abs(objectScale.x) < kMinZoom ? kMinZoom : objectScale.x;
    const float sy = std::abs(objectScale.y) < kMinZoom ? kMinZoom : objectScale.y;
    return add(ImVec2(p.x / sx, p.y / sy), objectPivot);
}

Result Editor::Draw(const char* id, Path& path, const Config& config) {
    Result result;

    ImVec2 canvasSize = config.canvasSize;
    if (canvasSize.x <= 0.0f) {
        canvasSize.x = ImGui::GetContentRegionAvail().x;
    }
    if (canvasSize.y <= 0.0f) {
        canvasSize.y = ImGui::GetContentRegionAvail().y;
    }
    canvasSize.x = std::max(canvasSize.x, 64.0f);
    canvasSize.y = std::max(canvasSize.y, 64.0f);

    const ImVec2 canvasOrigin = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton(id, canvasSize,
                           ImGuiButtonFlags_MouseButtonLeft |
                           ImGuiButtonFlags_MouseButtonMiddle |
                           ImGuiButtonFlags_MouseButtonRight);

    hovered_ = ImGui::IsItemHovered();
    const ImVec2 mouseScreen = ImGui::GetIO().MousePos;
    const Hit inputHit = HitTest(path, config, canvasOrigin, mouseScreen);

    if (!config.readOnly) {
        HandleKeyboard(path, config, result);
        HandleMouse(path, config, canvasOrigin, inputHit, result);
    }

    const Hit hoveredHit = HitTest(path, config, canvasOrigin, mouseScreen);
    const bool viewPanning = ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0f) && hovered_;
    active_ = ImGui::IsItemActive() || dragging_ || boxSelecting_ || viewPanning || penActive_;
    result.hovered = hovered_;
    result.active = active_;

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const ImVec2 canvasEnd = add(canvasOrigin, canvasSize);
    drawList->AddRectFilled(canvasOrigin, canvasEnd,
                            config.style.backgroundColor, 3.0f);

    drawList->PushClipRect(canvasOrigin, canvasEnd, true);
    if (config.showGrid) {
        DrawGrid(drawList, canvasOrigin, canvasSize, config);
    }
    DrawPath(drawList, path, config, canvasOrigin);
    DrawControls(drawList, path, config, canvasOrigin, hoveredHit);
    if (boxSelecting_) {
        drawList->AddRectFilled(boxSelectStartScreen_, boxSelectCurrentScreen_,
                                IM_COL32(112, 174, 255, 35));
        drawList->AddRect(boxSelectStartScreen_, boxSelectCurrentScreen_,
                          IM_COL32(112, 174, 255, 170));
    }
    drawList->PopClipRect();

    drawList->AddRect(canvasOrigin, canvasEnd, IM_COL32(255, 255, 255, hovered_ ? 85 : 35), 3.0f);

    wantsMouseCapture_ = hovered_ || dragging_ || boxSelecting_ || viewPanning || penActive_;
    wantsKeyboardCapture_ = penActive_ || selectedAnchor_ >= 0 || !selectedAnchors_.empty();
    result.wantsMouseCapture = wantsMouseCapture_;
    result.wantsKeyboardCapture = wantsKeyboardCapture_;

    return result;
}

void Editor::ClearSelection() {
    selectedAnchor_ = -1;
    selectedPart_ = HitPart::None;
    selectedAnchors_.clear();
    dragging_ = false;
    newAnchorDrag_ = false;
    boxSelecting_ = false;
    dragEditStarted_ = false;
    dragStartAnchorPositions_.clear();
}

bool Editor::IsAnchorSelected(int anchor) const {
    return std::find(selectedAnchors_.begin(), selectedAnchors_.end(), anchor) != selectedAnchors_.end();
}

void Editor::SelectAnchor(int anchor, HitPart part, bool addToSelection) {
    if (anchor < 0 || part == HitPart::None) {
        if (!addToSelection) {
            ClearSelection();
        }
        return;
    }

    selectedAnchor_ = anchor;
    selectedPart_ = part;

    if (!addToSelection || part != HitPart::Anchor) {
        selectedAnchors_.clear();
        selectedAnchors_.push_back(anchor);
        return;
    }

    const auto existing = std::find(selectedAnchors_.begin(), selectedAnchors_.end(), anchor);
    if (existing == selectedAnchors_.end()) {
        selectedAnchors_.push_back(anchor);
    } else {
        selectedAnchors_.erase(existing);
        if (selectedAnchors_.empty()) {
            selectedAnchor_ = -1;
            selectedPart_ = HitPart::None;
        } else {
            selectedAnchor_ = selectedAnchors_.back();
        }
    }
}

void Editor::RemoveSelectedAnchors(Path& path) {
    if (selectedAnchors_.empty()) {
        return;
    }

    std::sort(selectedAnchors_.begin(), selectedAnchors_.end());
    selectedAnchors_.erase(std::unique(selectedAnchors_.begin(), selectedAnchors_.end()),
                           selectedAnchors_.end());
    for (auto it = selectedAnchors_.rbegin(); it != selectedAnchors_.rend(); ++it) {
        if (*it >= 0 && *it < static_cast<int>(path.anchors.size())) {
            path.anchors.erase(path.anchors.begin() + *it);
        }
    }
    if (path.anchors.size() < 3) {
        path.closed = false;
    }
    ClearSelection();
}

void Editor::BeginAnchorDrag(const Path& path, const ImVec2& mouseLocal) {
    dragStartLocal_ = mouseLocal;
    dragStartAnchorPositions_.clear();
    for (int anchorIndex : selectedAnchors_) {
        if (anchorIndex >= 0 && anchorIndex < static_cast<int>(path.anchors.size())) {
            dragStartAnchorPositions_.push_back(path.anchors[anchorIndex].position);
        }
    }
}

void Editor::UpdateBoxSelection(Path& path, const Config& config, const ImVec2& canvasOrigin,
                                bool addToSelection) {
    const float minX = std::min(boxSelectStartScreen_.x, boxSelectCurrentScreen_.x);
    const float maxX = std::max(boxSelectStartScreen_.x, boxSelectCurrentScreen_.x);
    const float minY = std::min(boxSelectStartScreen_.y, boxSelectCurrentScreen_.y);
    const float maxY = std::max(boxSelectStartScreen_.y, boxSelectCurrentScreen_.y);

    if (!addToSelection) {
        selectedAnchors_.clear();
    }
    for (int i = 0; i < static_cast<int>(path.anchors.size()); ++i) {
        const ImVec2 anchorScreen = LocalToScreen(config, canvasOrigin, path.anchors[i].position);
        if (anchorScreen.x >= minX && anchorScreen.x <= maxX &&
            anchorScreen.y >= minY && anchorScreen.y <= maxY &&
            !IsAnchorSelected(i)) {
            selectedAnchors_.push_back(i);
        }
    }

    selectedAnchor_ = selectedAnchors_.empty() ? -1 : selectedAnchors_.back();
    selectedPart_ = selectedAnchor_ >= 0 ? HitPart::Anchor : HitPart::None;
}

void Editor::BeginEdit(const Config& config, EditKind kind, int anchorIndex) {
    if (editActive_) {
        return;
    }

    editActive_ = true;
    if (config.delegate) {
        config.delegate->BeginEdit(kind, anchorIndex);
    }
}

void Editor::EndEdit(const Config& config) {
    if (!editActive_) {
        return;
    }

    editActive_ = false;
    if (config.delegate) {
        config.delegate->EndEdit();
    }
}

Editor::Hit Editor::HitTest(const Path& path, const Config& config,
                            const ImVec2& canvasOrigin,
                            const ImVec2& mouseScreen) const {
    Hit best;
    best.distance = config.style.hitRadius;

    for (int i = 0; i < static_cast<int>(path.anchors.size()); ++i) {
        const Anchor& anchor = path.anchors[i];
        const ImVec2 anchorScreen = LocalToScreen(config, canvasOrigin, anchor.position);
        const float anchorDistance = length(sub(mouseScreen, anchorScreen));
        if (anchorDistance <= best.distance) {
            best = {i, HitPart::Anchor, anchorDistance};
        }

        if (anchor.hasHandleIn) {
            const ImVec2 handleScreen = LocalToScreen(config, canvasOrigin,
                                                      add(anchor.position, anchor.handleIn));
            const float handleDistance = length(sub(mouseScreen, handleScreen));
            if (handleDistance <= best.distance) {
                best = {i, HitPart::HandleIn, handleDistance};
            }
        }

        if (anchor.hasHandleOut) {
            const ImVec2 handleScreen = LocalToScreen(config, canvasOrigin,
                                                      add(anchor.position, anchor.handleOut));
            const float handleDistance = length(sub(mouseScreen, handleScreen));
            if (handleDistance <= best.distance) {
                best = {i, HitPart::HandleOut, handleDistance};
            }
        }
    }

    return best;
}

ImVec2 Editor::LocalToScreen(const Config& config, const ImVec2& canvasOrigin,
                             const ImVec2& local) const {
    return add(canvasOrigin, config.transform.LocalToCanvas(local));
}

ImVec2 Editor::ScreenToLocal(const Config& config, const ImVec2& canvasOrigin,
                             const ImVec2& screen) const {
    return config.transform.CanvasToLocal(sub(screen, canvasOrigin));
}

void Editor::DrawGrid(ImDrawList* drawList, const ImVec2& origin, const ImVec2& size,
                      const Config& config) const {
    const float step = std::max(4.0f, config.style.gridStep * config.transform.zoom);
    const ImVec2 end = add(origin, size);
    const float startX = origin.x + std::fmod(config.transform.pan.x, step);
    const float startY = origin.y + std::fmod(config.transform.pan.y, step);

    for (float x = startX; x < end.x; x += step) {
        drawList->AddLine(ImVec2(x, origin.y), ImVec2(x, end.y), config.style.gridColor);
    }
    for (float y = startY; y < end.y; y += step) {
        drawList->AddLine(ImVec2(origin.x, y), ImVec2(end.x, y), config.style.gridColor);
    }
}

void Editor::DrawPath(ImDrawList* drawList, const Path& path, const Config& config,
                      const ImVec2& canvasOrigin) const {
    if (path.anchors.empty()) {
        return;
    }

    for (int i = 1; i < static_cast<int>(path.anchors.size()); ++i) {
        const bool activeClickDragSegment =
            dragging_ && newAnchorDrag_ && selectedAnchor_ == i;
        drawSegment(drawList, config, canvasOrigin, path.anchors[i - 1],
                    path.anchors[i],
                    activeClickDragSegment ? config.style.previewColor : config.style.pathColor,
                    config.style.pathThickness);
    }

    if (path.closed && path.anchors.size() > 2) {
        drawSegment(drawList, config, canvasOrigin, path.anchors.back(),
                    path.anchors.front(), config.style.pathColor,
                    config.style.pathThickness);
    }
}

void Editor::DrawControls(ImDrawList* drawList, const Path& path, const Config& config,
                          const ImVec2& canvasOrigin, const Hit& hoveredHit) const {
    for (int i = 0; i < static_cast<int>(path.anchors.size()); ++i) {
        const Anchor& anchor = path.anchors[i];
        const bool selected = IsAnchorSelected(i);
        const bool closeTarget = i == 0 &&
                                 hoveredHit.anchor == 0 &&
                                 hoveredHit.part == HitPart::Anchor &&
                                 path.anchors.size() >= 3 &&
                                 !path.closed;
        const ImVec2 anchorScreen = LocalToScreen(config, canvasOrigin, anchor.position);

        if (anchor.hasHandleIn) {
            const ImVec2 handle = LocalToScreen(config, canvasOrigin,
                                                add(anchor.position, anchor.handleIn));
            drawList->AddLine(anchorScreen, handle, config.style.handleLineColor,
                              config.style.handleLineThickness);
            drawControlPoint(drawList, handle, config.style.handleRadius,
                             config.style.handleShape,
                             selected && selectedPart_ == HitPart::HandleIn
                                 ? config.style.anchorSelectedColor
                                 : config.style.handleColor);
        }

        if (anchor.hasHandleOut) {
            const ImVec2 handle = LocalToScreen(config, canvasOrigin,
                                                add(anchor.position, anchor.handleOut));
            drawList->AddLine(anchorScreen, handle, config.style.handleLineColor,
                              config.style.handleLineThickness);
            drawControlPoint(drawList, handle, config.style.handleRadius,
                             config.style.handleShape,
                             selected && selectedPart_ == HitPart::HandleOut
                                 ? config.style.anchorSelectedColor
                                 : config.style.handleColor);
        }

        ImU32 anchorColor = config.style.anchorColor;
        if (selected && selectedPart_ == HitPart::Anchor) {
            anchorColor = config.style.anchorSelectedColor;
        } else if (closeTarget) {
            anchorColor = config.style.closeTargetColor;
        }

        drawControlPoint(drawList, anchorScreen, config.style.anchorRadius + 1.5f,
                         config.style.anchorShape, IM_COL32(0, 0, 0, 180));
        drawControlPoint(drawList, anchorScreen, config.style.anchorRadius,
                         config.style.anchorShape, anchorColor);
    }
}

bool Editor::HandleKeyboard(Path& path, const Config& config, Result& result) {
    if (!config.allowKeyboardShortcuts || !wantsKeyboardCapture_) {
        return false;
    }

    bool changed = false;
    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        if (penActive_) {
            BeginEdit(config, EditKind::PathOperation, -1);
            path.clear();
            penActive_ = false;
            result.canceled = true;
            changed = true;
            EndEdit(config);
        }
        ClearSelection();
    } else if (ImGui::IsKeyPressed(ImGuiKey_Enter)) {
        if (penActive_ && path.anchors.size() >= 2) {
            penActive_ = false;
            result.committed = true;
        }
    } else if (ImGui::IsKeyPressed(ImGuiKey_Backspace) ||
               ImGui::IsKeyPressed(ImGuiKey_Delete)) {
        if (penActive_ && !path.anchors.empty()) {
            BeginEdit(config, EditKind::DeleteAnchor,
                      static_cast<int>(path.anchors.size()) - 1);
            path.anchors.pop_back();
            selectedAnchor_ = path.anchors.empty()
                ? -1
                : static_cast<int>(path.anchors.size()) - 1;
            selectedPart_ = selectedAnchor_ >= 0 ? HitPart::Anchor : HitPart::None;
            if (path.anchors.empty()) {
                penActive_ = false;
            }
            changed = true;
            EndEdit(config);
        } else if (!selectedAnchors_.empty()) {
            BeginEdit(config, EditKind::DeleteAnchor, selectedAnchor_);
            RemoveSelectedAnchors(path);
            changed = true;
            EndEdit(config);
        } else if (selectedAnchor_ >= 0 &&
                   selectedAnchor_ < static_cast<int>(path.anchors.size())) {
            BeginEdit(config, EditKind::DeleteAnchor, selectedAnchor_);
            path.anchors.erase(path.anchors.begin() + selectedAnchor_);
            selectedAnchor_ = std::min(selectedAnchor_,
                                       static_cast<int>(path.anchors.size()) - 1);
            selectedPart_ = selectedAnchor_ >= 0 ? HitPart::Anchor : HitPart::None;
            changed = true;
            EndEdit(config);
        }
    }

    result.changed = result.changed || changed;
    return changed;
}

bool Editor::HandleMouse(Path& path, const Config& config, const ImVec2& canvasOrigin,
                         const Hit& hoveredHit, Result& result) {
    bool changed = false;
    const ImGuiIO& io = ImGui::GetIO();
    const ImVec2 mouseLocal = ScreenToLocal(config, canvasOrigin, io.MousePos);

    if (hovered_ && io.MouseWheel != 0.0f) {
        result.viewZoomFactor = std::pow(1.1f, io.MouseWheel);
        result.viewZoomCenterCanvas = sub(io.MousePos, canvasOrigin);
    }

    if ((hovered_ || active_) && ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0f)) {
        result.viewPanDelta = io.MouseDelta;
        return changed;
    }

    if ((hovered_ || active_) &&
        ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) &&
        hoveredHit.anchor >= 0 &&
        hoveredHit.anchor < static_cast<int>(path.anchors.size()) &&
        hoveredHit.part == HitPart::Anchor) {
        SelectAnchor(hoveredHit.anchor, HitPart::Anchor, false);
        dragging_ = false;
        newAnchorDrag_ = false;
        const Anchor& beforeAnchor = path.anchors[hoveredHit.anchor];
        BeginEdit(config,
                  (!beforeAnchor.hasHandleIn && !beforeAnchor.hasHandleOut)
                      ? EditKind::AddHandle
                      : EditKind::ChangePointMode,
                  hoveredHit.anchor);
        changed = MakeAligned(path.anchors[hoveredHit.anchor]) || changed;
        EndEdit(config);
        result.changed = result.changed || changed;
        return changed;
    }

    if (hovered_ && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        if (config.tool == Tool::Pen) {
            if (penActive_ &&
                hoveredHit.anchor == 0 && hoveredHit.part == HitPart::Anchor &&
                path.anchors.size() >= 3) {
                BeginEdit(config, EditKind::PathOperation, 0);
                path.closed = true;
                penActive_ = false;
                result.pathClosed = true;
                result.committed = true;
                changed = true;
                EndEdit(config);
            } else {
                BeginEdit(config, EditKind::AddAnchor, static_cast<int>(path.anchors.size()));
                if (!penActive_ && path.closed) {
                    path.clear();
                    ClearSelection();
                }
                Anchor anchor;
                anchor.position = mouseLocal;
                path.anchors.push_back(anchor);
                SelectAnchor(static_cast<int>(path.anchors.size()) - 1, HitPart::Anchor, false);
                penActive_ = true;
                dragging_ = true;
                newAnchorDrag_ = true;
                dragEditStarted_ = true;
                dragStartLocal_ = mouseLocal;
                changed = true;
            }
        } else {
            const bool additiveSelection = isShiftDown() && hoveredHit.part == HitPart::Anchor;
            if (hoveredHit.part != HitPart::None) {
                SelectAnchor(hoveredHit.anchor, hoveredHit.part, additiveSelection);
                dragging_ = selectedPart_ != HitPart::None;
                dragEditStarted_ = false;
                if (dragging_ && selectedPart_ == HitPart::Anchor) {
                    BeginAnchorDrag(path, mouseLocal);
                }
            } else {
                if (!isShiftDown()) {
                    ClearSelection();
                }
                boxSelecting_ = true;
                boxSelectStartScreen_ = io.MousePos;
                boxSelectCurrentScreen_ = io.MousePos;
                dragging_ = false;
            }
            newAnchorDrag_ = false;
        }
    }

    if (boxSelecting_) {
        boxSelectCurrentScreen_ = io.MousePos;
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.0f)) {
            UpdateBoxSelection(path, config, canvasOrigin, isShiftDown());
        }
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            UpdateBoxSelection(path, config, canvasOrigin, isShiftDown());
            boxSelecting_ = false;
        }
    }

    if (dragging_ && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.0f) &&
        selectedAnchor_ >= 0 && selectedAnchor_ < static_cast<int>(path.anchors.size())) {
        Anchor& anchor = path.anchors[selectedAnchor_];
        if (selectedPart_ == HitPart::Anchor) {
            if (newAnchorDrag_) {
                ImVec2 delta = sub(mouseLocal, dragStartLocal_);
                if (isShiftDown()) {
                    delta = snapToAngleIncrement(delta, 3.14159265358979323846f / 4.0f);
                }
                if (lengthSq(delta) > 9.0f) {
                    anchor.handleOut = delta;
                    anchor.handleIn = mul(delta, -1.0f);
                    anchor.hasHandleOut = true;
                    anchor.hasHandleIn = true;
                    anchor.handleMode = HandleMode::Mirrored;
                    changed = true;
                }
            } else if (IsAnchorSelected(selectedAnchor_) &&
                       dragStartAnchorPositions_.size() == selectedAnchors_.size()) {
                if (!dragEditStarted_) {
                    BeginEdit(config, EditKind::MoveAnchor, selectedAnchor_);
                    dragEditStarted_ = true;
                }
                const ImVec2 delta = sub(mouseLocal, dragStartLocal_);
                for (int i = 0; i < static_cast<int>(selectedAnchors_.size()); ++i) {
                    const int anchorIndex = selectedAnchors_[i];
                    if (anchorIndex >= 0 && anchorIndex < static_cast<int>(path.anchors.size())) {
                        path.anchors[anchorIndex].position = add(dragStartAnchorPositions_[i], delta);
                    }
                }
                changed = true;
            } else {
                if (!dragEditStarted_) {
                    BeginEdit(config, EditKind::MoveAnchor, selectedAnchor_);
                    dragEditStarted_ = true;
                }
                anchor.position = mouseLocal;
                changed = true;
            }
        } else if (selectedPart_ == HitPart::HandleIn) {
            if (!dragEditStarted_) {
                BeginEdit(config, EditKind::MoveHandle, selectedAnchor_);
                dragEditStarted_ = true;
            }
            anchor.handleIn = sub(mouseLocal, anchor.position);
            anchor.hasHandleIn = true;
            if (anchor.handleMode == HandleMode::Mirrored) {
                anchor.handleOut = mul(anchor.handleIn, -1.0f);
                anchor.hasHandleOut = true;
            } else if (anchor.handleMode == HandleMode::Aligned) {
                const float outLength = anchor.hasHandleOut ? length(anchor.handleOut) : length(anchor.handleIn);
                anchor.handleOut = alignedOppositeHandle(anchor.handleIn, outLength);
                anchor.hasHandleOut = true;
            } else {
                anchor.handleMode = HandleMode::Free;
            }
            changed = true;
        } else if (selectedPart_ == HitPart::HandleOut) {
            if (!dragEditStarted_) {
                BeginEdit(config, EditKind::MoveHandle, selectedAnchor_);
                dragEditStarted_ = true;
            }
            anchor.handleOut = sub(mouseLocal, anchor.position);
            anchor.hasHandleOut = true;
            if (anchor.handleMode == HandleMode::Mirrored) {
                anchor.handleIn = mul(anchor.handleOut, -1.0f);
                anchor.hasHandleIn = true;
            } else if (anchor.handleMode == HandleMode::Aligned) {
                const float inLength = anchor.hasHandleIn ? length(anchor.handleIn) : length(anchor.handleOut);
                anchor.handleIn = alignedOppositeHandle(anchor.handleOut, inLength);
                anchor.hasHandleIn = true;
            } else {
                anchor.handleMode = HandleMode::Free;
            }
            changed = true;
        }
    }

    if (dragging_ && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        dragging_ = false;
        newAnchorDrag_ = false;
        dragEditStarted_ = false;
        dragStartAnchorPositions_.clear();
        EndEdit(config);
    }

    result.changed = result.changed || changed;
    return changed;
}

} // namespace ImVectorEditor
