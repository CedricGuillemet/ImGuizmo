#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ImSequencer.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <cstdlib>

namespace ImSequencer
{
#ifndef IMGUI_DEFINE_MATH_OPERATORS
    static ImVec2 operator+(const ImVec2 &a, const ImVec2 &b) {
       return ImVec2(a.x + b.x, a.y + b.y);
    }
#endif
    static bool SequencerAddDelButton(ImDrawList* draw_list, ImVec2 pos, bool add = true)
    {
        ImGuiIO& io = ImGui::GetIO();
        ImRect delRect(pos, ImVec2(pos.x + 16, pos.y + 16));
        bool overDel = delRect.Contains(io.MousePos);
        int delColor = overDel ? 0xFFAAAAAA : 0x50000000;
        float midy = pos.y + 16 / 2 - 0.5f;
        float midx = pos.x + 16 / 2 - 0.5f;
        draw_list->AddRect(delRect.Min, delRect.Max, delColor, 4);
        draw_list->AddLine(ImVec2(delRect.Min.x + 3, midy), ImVec2(delRect.Max.x - 3, midy), delColor, 2);
        if (add)
            draw_list->AddLine(ImVec2(midx, delRect.Min.y + 3), ImVec2(midx, delRect.Max.y - 3), delColor, 2);
        return overDel;
    }

    static int min(int a, int b) { return (a < b) ? a : b; }
    static int max(int a, int b) { return (a > b) ? a : b; }

    bool Sequencer(SequenceInterface *sequence, int *currentFrame, bool *expanded, int *selectedEntry, int *firstFrame, int sequenceOptions)
    {
        bool ret = false;
        ImGuiIO& io = ImGui::GetIO();
        int cx = (int)(io.MousePos.x);
        int cy = (int)(io.MousePos.y);
        static float framePixelWidth = 10.f;
        static float framePixelWidthTarget = 10.f;
        int legendWidth = 200;

        static int movingEntry = -1;
        static int movingPos = -1;
        static int movingPart = -1;
        int delEntry = -1;
        int dupEntry = -1;
        int ItemHeight = 20;

        bool popupOpened = false;
        int sequenceCount = sequence->GetItemCount();
        if (!sequenceCount)
            return false;
        ImGui::BeginGroup();
        
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 canvas_pos = ImGui::GetCursorScreenPos();            // ImDrawList API uses screen coordinates!
        ImVec2 canvas_size = ImGui::GetContentRegionAvail();        // Resize canvas to what's available
        static const int scrollBarHeight = 14;
        int firstFrameUsed = firstFrame ? *firstFrame : 0;


        int controlHeight = sequenceCount * ItemHeight;
        for (int i = 0; i < sequenceCount; i++)
            controlHeight += int(sequence->GetCustomHeight(i));
        int frameCount = ImMax(sequence->GetFrameMax() - sequence->GetFrameMin(), 1);

        static bool MovingScrollBar = false;
        static bool MovingCurrentFrame = false;
        struct CustomDraw
        {
            int index;
            ImRect customRect;
            ImRect legendRect;
            ImRect clippingRect;
            ImRect legendClippingRect;
        };
        ImVector<CustomDraw> customDraws;
        ImVector<CustomDraw> compactCustomDraws;
        // zoom in/out
        const int visibleFrameCount = (int)floorf((canvas_size.x - legendWidth) / framePixelWidth);
        const float barWidthRatio = ImMin(visibleFrameCount / (float)frameCount, 1.f);
        const float barWidthInPixels = barWidthRatio * (canvas_size.x - legendWidth);

        ImRect regionRect(canvas_pos, canvas_pos + canvas_size);
        
        static bool panningView = false;
        static ImVec2 panningViewSource;
        static int panningViewFrame;
        if (ImGui::IsWindowFocused() && io.KeyAlt && io.MouseDown[2])
        {
            if (!panningView)
            {
                panningViewSource = io.MousePos;
                panningView = true;
                panningViewFrame = *firstFrame;
            }
            *firstFrame = panningViewFrame - int((io.MousePos.x - panningViewSource.x) / framePixelWidth);
            *firstFrame = ImClamp(*firstFrame, sequence->GetFrameMin(), sequence->GetFrameMax() - visibleFrameCount);
        }
        if (panningView && !io.MouseDown[2])
        {
            panningView = false;
        }
        framePixelWidthTarget = ImClamp(framePixelWidthTarget, 0.1f, 50.f);

        framePixelWidth = ImLerp(framePixelWidth, framePixelWidthTarget, 0.33f);

        frameCount = sequence->GetFrameMax() - sequence->GetFrameMin();
        if (visibleFrameCount >= frameCount && firstFrame)
            *firstFrame = sequence->GetFrameMin();


        // --
        if (expanded && !*expanded)
        {
            ImGui::InvisibleButton("canvas", ImVec2(canvas_size.x - canvas_pos.x, (float)ItemHeight));
            draw_list->AddRectFilled(canvas_pos, ImVec2(canvas_size.x + canvas_pos.x, canvas_pos.y + ItemHeight), 0xFF3D3837, 0);
            char tmps[512];
            sprintf(tmps, "%d Frames / %d entries", frameCount, sequenceCount);
            draw_list->AddText(ImVec2(canvas_pos.x + 26, canvas_pos.y + 2), 0xFFFFFFFF, tmps);
        }
        else
        {
            bool hasScrollBar(true);
            /*
            int framesPixelWidth = int(frameCount * framePixelWidth);
            if ((framesPixelWidth + legendWidth) >= canvas_size.x)
            {
                hasScrollBar = true;
            }
            */
            // test scroll area
            ImVec2 headerSize(canvas_size.x, (float)ItemHeight);
            ImVec2 scrollBarSize(canvas_size.x, 14.f);
            ImGui::InvisibleButton("topBar", headerSize);
            draw_list->AddRectFilled(canvas_pos, canvas_pos + headerSize, 0xFFFF0000, 0);
            ImVec2 childFramePos = ImGui::GetCursorScreenPos();
            ImVec2 childFrameSize(canvas_size.x, canvas_size.y - 8.f - headerSize.y - (hasScrollBar ? scrollBarSize.y : 0));
            ImGui::PushStyleColor(ImGuiCol_FrameBg, 0);
            ImGui::BeginChildFrame(889, childFrameSize);
            sequence->focused = ImGui::IsWindowFocused();
            ImGui::InvisibleButton("contentBar", ImVec2(canvas_size.x, float(controlHeight)));
            const ImVec2 contentMin = ImGui::GetItemRectMin();
            const ImVec2 contentMax = ImGui::GetItemRectMax();
            const ImRect contentRect(contentMin, contentMax);
            const float contentHeight = contentMax.y - contentMin.y;

            // full background
            draw_list->AddRectFilled(canvas_pos, canvas_pos + canvas_size, 0xFF242424, 0);

            // current frame top
            ImRect topRect(ImVec2(canvas_pos.x + legendWidth, canvas_pos.y), ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + ItemHeight));

            if (!MovingCurrentFrame && !MovingScrollBar && movingEntry == -1 && sequenceOptions&SEQUENCER_CHANGE_FRAME && currentFrame && *currentFrame >= 0 && topRect.Contains(io.MousePos) && io.MouseDown[0])
            {
                MovingCurrentFrame = true;
            }
            if (MovingCurrentFrame)
            {
                if (frameCount)
                {
                    *currentFrame = (int)((io.MousePos.x - topRect.Min.x) / framePixelWidth) + firstFrameUsed;
                    if (*currentFrame < sequence->GetFrameMin())
                        *currentFrame = sequence->GetFrameMin();
                    if (*currentFrame >= sequence->GetFrameMax())
                        *currentFrame = sequence->GetFrameMax();
                }
                if (!io.MouseDown[0])
                    MovingCurrentFrame = false;
            }

            //header
            draw_list->AddRectFilled(canvas_pos, ImVec2(canvas_size.x + canvas_pos.x, canvas_pos.y + ItemHeight), 0xFF3D3837, 0);
            if (sequenceOptions&SEQUENCER_ADD)
            {
                if (SequencerAddDelButton(draw_list, ImVec2(canvas_pos.x + legendWidth - ItemHeight, canvas_pos.y + 2), true) && io.MouseReleased[0])
                    ImGui::OpenPopup("addEntry");

                if (ImGui::BeginPopup("addEntry"))
                {
                    for (int i = 0; i < sequence->GetItemTypeCount(); i++)
                        if (ImGui::Selectable(sequence->GetItemTypeName(i)))
                        {
                            sequence->Add(i);
                            *selectedEntry = sequence->GetItemCount() - 1;
                        }

                    ImGui::EndPopup();
                    popupOpened = true;
                }
            }

            //header frame number and lines
            int modFrameCount = 10;
            int frameStep = 1;
            while ((modFrameCount * framePixelWidth) < 150)
            {
                modFrameCount *= 2;
                frameStep *= 2;
            };
            int halfModFrameCount = modFrameCount / 2;

            auto drawLine = [&](int i, int regionHeight) {
                bool baseIndex = ((i % modFrameCount) == 0) || (i == sequence->GetFrameMax() || i == sequence->GetFrameMin());
                bool halfIndex = (i % halfModFrameCount) == 0;
                int px = (int)canvas_pos.x + int(i * framePixelWidth) + legendWidth - int(firstFrameUsed * framePixelWidth);
                int tiretStart = baseIndex ? 4 : (halfIndex ? 10 : 14);
                int tiretEnd = baseIndex ? regionHeight : ItemHeight;

                if (px <= (canvas_size.x + canvas_pos.x) && px >= (canvas_pos.x + legendWidth))
                {
                    draw_list->AddLine(ImVec2((float)px, canvas_pos.y + (float)tiretStart), ImVec2((float)px, canvas_pos.y + (float)tiretEnd - 1), 0xFF606060, 1);

                    draw_list->AddLine(ImVec2((float)px, canvas_pos.y + (float)ItemHeight), ImVec2((float)px, canvas_pos.y + (float)regionHeight - 1), 0x30606060, 1);
                }

                if (baseIndex && px > (canvas_pos.x + legendWidth))
                {
                    char tmps[512];
                    sprintf(tmps, "%d", i);
                    draw_list->AddText(ImVec2((float)px + 3.f, canvas_pos.y), 0xFFBBBBBB, tmps);
                }

            };

            auto drawLineContent = [&](int i, int regionHeight) {
                int px = (int)canvas_pos.x + int(i * framePixelWidth) + legendWidth - int(firstFrameUsed * framePixelWidth);
                int tiretStart = int(contentMin.y);
                int tiretEnd = int(contentMax.y);

                if (px <= (canvas_size.x + canvas_pos.x) && px >= (canvas_pos.x + legendWidth))
                {
                    //draw_list->AddLine(ImVec2((float)px, canvas_pos.y + (float)tiretStart), ImVec2((float)px, canvas_pos.y + (float)tiretEnd - 1), 0xFF606060, 1);

                    draw_list->AddLine(ImVec2(float(px), float(tiretStart)), ImVec2(float(px), float(tiretEnd)), 0x30606060, 1);
                }
            };
            for (int i = sequence->GetFrameMin(); i <= sequence->GetFrameMax(); i += frameStep)
            {
                drawLine(i, ItemHeight);
            }
            drawLine(sequence->GetFrameMin(), ItemHeight);
            drawLine(sequence->GetFrameMax(), ItemHeight);
            /*
                     draw_list->AddLine(canvas_pos, ImVec2(canvas_pos.x, canvas_pos.y + controlHeight), 0xFF000000, 1);
                     draw_list->AddLine(ImVec2(canvas_pos.x, canvas_pos.y + ItemHeight), ImVec2(canvas_size.x, canvas_pos.y + ItemHeight), 0xFF000000, 1);
                     */
                     // clip content

            draw_list->PushClipRect(childFramePos, childFramePos + childFrameSize);

            // draw item names in the legend rect on the left
            size_t customHeight = 0;
            for (int i = 0; i < sequenceCount; i++)
            {
                int type;
                sequence->Get(i, NULL, NULL, &type, NULL);
                ImVec2 tpos(contentMin.x + 3, contentMin.y + i * ItemHeight + 2 + customHeight);
                draw_list->AddText(tpos, 0xFFFFFFFF, sequence->GetItemLabel(i));

                if (sequenceOptions&SEQUENCER_DEL)
                {
                    bool overDel = SequencerAddDelButton(draw_list, ImVec2(contentMin.x + legendWidth - ItemHeight + 2 - 10, tpos.y + 2), false);
                    if (overDel && io.MouseReleased[0])
                        delEntry = i;

                    bool overDup = SequencerAddDelButton(draw_list, ImVec2(contentMin.x + legendWidth - ItemHeight - ItemHeight + 2 - 10, tpos.y + 2), true);
                    if (overDup && io.MouseReleased[0])
                        dupEntry = i;
                }
                customHeight += sequence->GetCustomHeight(i);
            }

            // clipping rect so items bars are not visible in the legend on the left when scrolled
            //

            // slots background
            customHeight = 0;
            for (int i = 0; i < sequenceCount; i++)
            {
                unsigned int col = (i & 1) ? 0xFF3A3636 : 0xFF413D3D;

                size_t localCustomHeight = sequence->GetCustomHeight(i);
                ImVec2 pos = ImVec2(contentMin.x + legendWidth, contentMin.y + ItemHeight * i + 1 + customHeight);
                ImVec2 sz = ImVec2(canvas_size.x + canvas_pos.x, pos.y + ItemHeight - 1 + localCustomHeight);
                if (!popupOpened && cy >= pos.y && cy < pos.y + (ItemHeight + localCustomHeight) && movingEntry == -1 && cx>contentMin.x && cx < contentMin.x + canvas_size.x)
                {
                    col += 0x80201008;
                    pos.x -= legendWidth;
                }
                draw_list->AddRectFilled(pos, sz, col, 0);
                customHeight += localCustomHeight;
            }

            draw_list->PushClipRect(childFramePos + ImVec2(float(legendWidth), 0.f), childFramePos + childFrameSize);

            // vertical frame lines in content area
            for (int i = sequence->GetFrameMin(); i <= sequence->GetFrameMax(); i += frameStep)
            {
                drawLineContent(i, int(contentHeight));
            }
            drawLineContent(sequence->GetFrameMin(), int(contentHeight));
            drawLineContent(sequence->GetFrameMax(), int(contentHeight));

            // selection
            bool selected = selectedEntry && (*selectedEntry >= 0);
            if (selected)
            {
                customHeight = 0;
                for (int i = 0; i < *selectedEntry; i++)
                    customHeight += sequence->GetCustomHeight(i);;
                draw_list->AddRectFilled(ImVec2(contentMin.x, contentMin.y + ItemHeight * *selectedEntry + customHeight), ImVec2(contentMin.x + canvas_size.x, contentMin.y + ItemHeight * (*selectedEntry + 1) + customHeight), 0x801080FF, 1.f);
            }

            // slots
            customHeight = 0;
            for (int i = 0; i < sequenceCount; i++)
            {
                int *start, *end;
                unsigned int color;
                sequence->Get(i, &start, &end, NULL, &color);
                size_t localCustomHeight = sequence->GetCustomHeight(i);

                ImVec2 pos = ImVec2(contentMin.x + legendWidth - firstFrameUsed * framePixelWidth, contentMin.y + ItemHeight * i + 1 + customHeight);
                ImVec2 slotP1(pos.x + *start * framePixelWidth, pos.y + 2);
                ImVec2 slotP2(pos.x + *end * framePixelWidth + framePixelWidth, pos.y + ItemHeight - 2);
                ImVec2 slotP3(pos.x + *end * framePixelWidth + framePixelWidth, pos.y + ItemHeight - 2 + localCustomHeight);
                unsigned int slotColor = color | 0xFF000000;
                unsigned int slotColorHalf = (color & 0xFFFFFF) | 0x40000000;

                if (slotP1.x <= (canvas_size.x + contentMin.x) && slotP2.x >= (contentMin.x + legendWidth))
                {
                    draw_list->AddRectFilled(slotP1, slotP3, slotColorHalf, 2);
                    draw_list->AddRectFilled(slotP1, slotP2, slotColor, 2);
                }
                if (ImRect(slotP1, slotP2).Contains(io.MousePos) && io.MouseDoubleClicked[0])
                {
                    sequence->DoubleClick(i);
                }
                ImRect rects[3] = { ImRect(slotP1, ImVec2(slotP1.x + framePixelWidth / 2, slotP2.y))
                    , ImRect(ImVec2(slotP2.x - framePixelWidth / 2, slotP1.y), slotP2)
                    , ImRect(slotP1, slotP2) };

                const unsigned int quadColor[] = { 0xFFFFFFFF, 0xFFFFFFFF, slotColor + (selected ? 0 : 0x202020) };
                if (movingEntry == -1 && (sequenceOptions&SEQUENCER_EDIT_STARTEND))// TODOFOCUS && backgroundRect.Contains(io.MousePos))
                {
                    for (int j = 2; j >= 0; j--)
                    {
                        ImRect& rc = rects[j];
                        if (!rc.Contains(io.MousePos))
                            continue;
                        draw_list->AddRectFilled(rc.Min, rc.Max, quadColor[j], 2);
                    }

                    for (int j = 0; j < 3; j++)
                    {
                        ImRect& rc = rects[j];
                        if (!rc.Contains(io.MousePos))
                            continue;
                        if (!ImRect(childFramePos, childFramePos+childFrameSize).Contains(io.MousePos))
                            continue;
                        if (ImGui::IsMouseClicked(0) && !MovingScrollBar && !MovingCurrentFrame)
                        {
                            movingEntry = i;
                            movingPos = cx;
                            movingPart = j + 1;
                            sequence->BeginEdit(movingEntry);
                            break;
                        }
                    }
                }

                // custom draw
                if (localCustomHeight > 0)
                {
                   ImVec2 rp(canvas_pos.x, contentMin.y + ItemHeight * i + 1 + customHeight);
                   ImRect customRect(rp + ImVec2(legendWidth - (firstFrameUsed - sequence->GetFrameMin() - 0.5f) * framePixelWidth, float(ItemHeight)),
                      rp + ImVec2(legendWidth + (sequence->GetFrameMax() - firstFrameUsed - 0.5f + 2.f) * framePixelWidth, float(localCustomHeight + ItemHeight)));
                   ImRect clippingRect(rp + ImVec2(float(legendWidth), float(ItemHeight)), rp + ImVec2(canvas_size.x, float(localCustomHeight + ItemHeight)));

                    ImRect legendRect(rp + ImVec2(0.f, float(ItemHeight)), rp + ImVec2(float(legendWidth), float(localCustomHeight)));
                    ImRect legendClippingRect(canvas_pos + ImVec2(0.f, float(ItemHeight)), canvas_pos + ImVec2(float(legendWidth), float(localCustomHeight + ItemHeight)));
                    customDraws.push_back({ i, customRect, legendRect, clippingRect, legendClippingRect });
                }
                else
                {
                   ImVec2 rp(canvas_pos.x, contentMin.y + ItemHeight * i + customHeight);
                   ImRect customRect(rp + ImVec2(legendWidth - (firstFrameUsed - sequence->GetFrameMin() - 0.5f) * framePixelWidth, float(0.f)),
                      rp + ImVec2(legendWidth + (sequence->GetFrameMax() - firstFrameUsed - 0.5f + 2.f) * framePixelWidth, float(ItemHeight)));
                   ImRect clippingRect(rp + ImVec2(float(legendWidth), float(0.f)), rp + ImVec2(canvas_size.x, float(ItemHeight)));

                   compactCustomDraws.push_back({ i, customRect, ImRect(), clippingRect, ImRect() });
                }
                customHeight += localCustomHeight;
            }


            // moving
            if (/*backgroundRect.Contains(io.MousePos) && */movingEntry >= 0)
            {
                ImGui::CaptureMouseFromApp();
                int diffFrame = int((cx - movingPos) / framePixelWidth);
                if (std::abs(diffFrame) > 0)
                {
                    int *start, *end;
                    sequence->Get(movingEntry, &start, &end, NULL, NULL);
                    if (selectedEntry)
                        *selectedEntry = movingEntry;
                    int & l = *start;
                    int & r = *end;
                    if (movingPart & 1)
                        l += diffFrame;
                    if (movingPart & 2)
                        r += diffFrame;
                    if (l < 0)
                    {
                        if (movingPart & 2)
                            r -= l;
                        l = 0;
                    }
                    if (movingPart & 1 && l > r)
                        l = r;
                    if (movingPart & 2 && r < l)
                        r = l;
                    movingPos = cx;
                }
                if (!io.MouseDown[0])
                {
                    // single select
                    if (!diffFrame && movingPart && selectedEntry)
                    {
                        *selectedEntry = movingEntry;
                        ret = true;
                    }

                    movingEntry = -1;
                    sequence->EndEdit();
                }
            }

            // cursor
            if (currentFrame && firstFrame && *currentFrame >= *firstFrame && *currentFrame <= sequence->GetFrameMax())
            {
                static const float cursorWidth = 8.f;
                float cursorOffset = contentMin.x + legendWidth + (*currentFrame - firstFrameUsed) * framePixelWidth + framePixelWidth / 2 - cursorWidth * 0.5f;
                draw_list->AddLine(ImVec2(cursorOffset, canvas_pos.y), ImVec2(cursorOffset, contentMax.y), 0xA02A2AFF, cursorWidth);
                char tmps[512];
                sprintf(tmps, "%d", *currentFrame);
                draw_list->AddText(ImVec2(cursorOffset + 10, canvas_pos.y + 2), 0xFF2A2AFF, tmps);
            }

            draw_list->PopClipRect();
            draw_list->PopClipRect();

            for (auto& customDraw : customDraws)
                sequence->CustomDraw(customDraw.index, draw_list, customDraw.customRect, customDraw.legendRect, customDraw.clippingRect, customDraw.legendClippingRect);
            for (auto& customDraw : compactCustomDraws)
               sequence->CustomDrawCompact(customDraw.index, draw_list, customDraw.customRect, customDraw.clippingRect);

            // copy paste
            if (sequenceOptions&SEQUENCER_COPYPASTE)
            {
                ImRect rectCopy(ImVec2(contentMin.x + 100, canvas_pos.y + 2)
                    , ImVec2(contentMin.x + 100 + 30, canvas_pos.y + ItemHeight - 2));
                bool inRectCopy = rectCopy.Contains(io.MousePos);
                unsigned int copyColor = inRectCopy ? 0xFF1080FF : 0xFF000000;
                draw_list->AddText(rectCopy.Min, copyColor, "Copy");

                ImRect rectPaste(ImVec2(contentMin.x + 140, canvas_pos.y + 2)
                    , ImVec2(contentMin.x + 140 + 30, canvas_pos.y + ItemHeight - 2));
                bool inRectPaste = rectPaste.Contains(io.MousePos);
                unsigned int pasteColor = inRectPaste ? 0xFF1080FF : 0xFF000000;
                draw_list->AddText(rectPaste.Min, pasteColor, "Paste");

                if (inRectCopy && io.MouseReleased[0])
                {
                    sequence->Copy();
                }
                if (inRectPaste && io.MouseReleased[0])
                {
                    sequence->Paste();
                }
            }
            //

            ImGui::EndChildFrame();
            ImGui::PopStyleColor();
            if (hasScrollBar)
            {
                ImGui::InvisibleButton("scrollBar", scrollBarSize);
                ImVec2 scrollBarMin = ImGui::GetItemRectMin();
                ImVec2 scrollBarMax = ImGui::GetItemRectMax();

                // ratio = number of frames visible in control / number to total frames

                float startFrameOffset = ((float)(firstFrameUsed - sequence->GetFrameMin()) / (float)frameCount) * (canvas_size.x - legendWidth);
                ImVec2 scrollBarA(scrollBarMin.x + legendWidth, scrollBarMin.y - 2);
                ImVec2 scrollBarB(scrollBarMin.x + canvas_size.x, scrollBarMax.y - 1);
                draw_list->AddRectFilled(scrollBarA, scrollBarB, 0xFF222222, 0);

                ImRect scrollBarRect(scrollBarA, scrollBarB);
                bool inScrollBar = scrollBarRect.Contains(io.MousePos);

                draw_list->AddRectFilled(scrollBarA, scrollBarB, 0xFF101010, 8);


                ImVec2 scrollBarC(scrollBarMin.x + legendWidth + startFrameOffset, scrollBarMin.y);
                ImVec2 scrollBarD(scrollBarMin.x + legendWidth + barWidthInPixels + startFrameOffset, scrollBarMax.y - 2);
                draw_list->AddRectFilled(scrollBarC, scrollBarD, (inScrollBar || MovingScrollBar) ? 0xFF606060 : 0xFF505050, 6);

                float handleRadius = (scrollBarMax.y - scrollBarMin.y) / 2;
                ImRect barHandleLeft(scrollBarC, ImVec2(scrollBarC.x + 14, scrollBarD.y));
                ImRect barHandleRight(ImVec2(scrollBarD.x - 14, scrollBarC.y), scrollBarD);

                bool onLeft = barHandleLeft.Contains(io.MousePos);
                bool onRight = barHandleRight.Contains(io.MousePos);

                static bool sizingRBar = false;
                static bool sizingLBar = false;

                draw_list->AddRectFilled(barHandleLeft.Min, barHandleLeft.Max, (onLeft || sizingLBar)?0xFFAAAAAA:0xFF666666, 6);
                draw_list->AddRectFilled(barHandleRight.Min, barHandleRight.Max, (onRight || sizingRBar )?0xFFAAAAAA:0xFF666666, 6);

                ImRect scrollBarThumb(scrollBarC, scrollBarD);
                static const float MinBarWidth = 44.f;
                if (sizingRBar)
                {
                   if (!io.MouseDown[0])
                   {
                      sizingRBar = false;
                   }
                   else
                   {
                      float barNewWidth = ImMax(barWidthInPixels + io.MouseDelta.x, MinBarWidth);
                      float barRatio = barNewWidth / barWidthInPixels;
                      framePixelWidthTarget = framePixelWidth = framePixelWidth / barRatio;
                      int newVisibleFrameCount = int((canvas_size.x - legendWidth) / framePixelWidthTarget);
                      int lastFrame = *firstFrame + newVisibleFrameCount;
                      if (lastFrame > sequence->GetFrameMax())
                      {
                         framePixelWidthTarget = framePixelWidth = (canvas_size.x - legendWidth) / float(sequence->GetFrameMax() - *firstFrame);
                      }
                   }
                }
                else if (sizingLBar)
                {
                   if (!io.MouseDown[0])
                   {
                      sizingLBar = false;
                   }
                   else
                   {
                      if (fabsf(io.MouseDelta.x) > FLT_EPSILON)
                      {
                         float barNewWidth = ImMax(barWidthInPixels - io.MouseDelta.x, MinBarWidth);
                         float barRatio = barNewWidth / barWidthInPixels;
                         float previousFramePixelWidthTarget = framePixelWidthTarget;
                         framePixelWidthTarget = framePixelWidth = framePixelWidth / barRatio;
                         int newVisibleFrameCount = int(visibleFrameCount / barRatio);
                         int newFirstFrame = *firstFrame + newVisibleFrameCount - visibleFrameCount;
                         newFirstFrame = ImClamp(newFirstFrame, sequence->GetFrameMin(), ImMax(sequence->GetFrameMax() - visibleFrameCount, sequence->GetFrameMin()));
                         if (newFirstFrame == *firstFrame)
                         {
                            framePixelWidth = framePixelWidthTarget = previousFramePixelWidthTarget;
                         }
                         else
                         {
                            *firstFrame = newFirstFrame;
                         }
                      }
                   }
                }
                else
                {
                   if (MovingScrollBar)
                   {
                      if (!io.MouseDown[0])
                      {
                         MovingScrollBar = false;
                      }
                      else
                      {
                         float framesPerPixelInBar = barWidthInPixels / (float)visibleFrameCount;
                         *firstFrame = int((io.MousePos.x - panningViewSource.x) / framesPerPixelInBar) - panningViewFrame;
                         *firstFrame = ImClamp(*firstFrame, sequence->GetFrameMin(), ImMax(sequence->GetFrameMax() - visibleFrameCount, sequence->GetFrameMin()));
                      }
                   }
                   else
                   {
                      if (scrollBarThumb.Contains(io.MousePos) && ImGui::IsMouseClicked(0) && firstFrame && !MovingCurrentFrame && movingEntry == -1)
                      {
                         MovingScrollBar = true;
                         panningViewSource = io.MousePos;
                         panningViewFrame = -*firstFrame;
                      }
                      if (!sizingRBar && onRight && ImGui::IsMouseClicked(0))
                         sizingRBar = true;
                      if (!sizingLBar && onLeft && ImGui::IsMouseClicked(0))
                         sizingLBar = true;

                   }
                }
            }

            if (regionRect.Contains(io.MousePos))
            {
                bool overCustomDraw = false;
                for (auto&custom : customDraws)
                {
                    if (custom.customRect.Contains(io.MousePos))
                    {
                        overCustomDraw = true;
                    }
                }
                if (overCustomDraw)
                {
                }
                else
                {
#if 0
                    int frameOverCursor = (int)((io.MousePos.x - topRect.Min.x) / framePixelWidth) + firstFrameUsed;
                    if (io.MouseWheel < -FLT_EPSILON)
                    {
                        framePixelWidthTarget *= 0.9f;
                        framePixelWidth *= 0.9f;
                        int newFrameOverCursor = (int)((io.MousePos.x - topRect.Min.x) / framePixelWidth) + firstFrameUsed;
                        *firstFrame += frameOverCursor - newFrameOverCursor;
                    }

                    if (io.MouseWheel > FLT_EPSILON)
                    {
                        framePixelWidthTarget *= 1.1f;
                        framePixelWidth *= 1.1f;
                        int newFrameOverCursor = (int)((io.MousePos.x - topRect.Min.x) / framePixelWidth) + firstFrameUsed;
                        *firstFrame += frameOverCursor - newFrameOverCursor;
                    }
#endif
                }
            }
        }

        ImGui::EndGroup();

        if (expanded)
        {
            bool overExpanded = SequencerAddDelButton(draw_list, ImVec2(canvas_pos.x + 2, canvas_pos.y + 2), !*expanded);
            if (overExpanded && io.MouseReleased[0])
                *expanded = !*expanded;
        }

        if (delEntry != -1)
        {
            sequence->Del(delEntry);
            if (selectedEntry && (*selectedEntry == delEntry || *selectedEntry >= sequence->GetItemCount()))
                *selectedEntry = -1;
        }

        if (dupEntry != -1)
        {
            sequence->Duplicate(dupEntry);
        }
        return ret;
    }
}
