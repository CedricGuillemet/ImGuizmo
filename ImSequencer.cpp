#include "ImSequencer.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <cstdlib>

namespace ImSequencer
{

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
		ImGui::BeginGroup();
		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		ImVec2 canvas_pos = ImGui::GetCursorScreenPos();            // ImDrawList API uses screen coordinates!
		ImVec2 canvas_size = ImGui::GetContentRegionAvail();        // Resize canvas to what's available
		static const int scrollBarHeight = 14;
		int firstFrameUsed = firstFrame ? *firstFrame : 0;

		int sequenceCount = sequence->GetItemCount();
		int controlHeight = (sequenceCount + 1) * ItemHeight;
      for (int i = 0; i < sequenceCount; i++)
         controlHeight += sequence->GetCustomHeight(i);
		int frameCount = sequence->GetFrameCount();

		static bool MovingScrollBar = false;
		static bool MovingCurrentFrame = false;
      struct CustomDraw
      {
         int index;
         ImRect customArea;
      };
      ImVector<CustomDraw> customDraws;
		// zoom in/out
      int frameOverCursor = 0;
      const int visibleFrameCount = (int)floorf((canvas_size.x - legendWidth) / framePixelWidth);
      const float barWidthRatio = visibleFrameCount / (float)frameCount;
      const float barWidthInPixels = barWidthRatio * (canvas_size.x - legendWidth);

		ImRect regionRect(canvas_pos, canvas_pos + canvas_size);
		if (regionRect.Contains(io.MousePos))
		{
         
         frameOverCursor = *firstFrame + (int)(visibleFrameCount * ((io.MousePos.x - (float)legendWidth - canvas_pos.x) / (canvas_size.x - legendWidth)));
         //frameOverCursor = max(min(*firstFrame - visibleFrameCount / 2, frameCount - visibleFrameCount), 0);

         /**firstFrame -= frameOverCursor;
         *firstFrame *= framePixelWidthTarget / framePixelWidth;
         *firstFrame += frameOverCursor;*/
         if (io.MouseWheel < -FLT_EPSILON)
         {
            *firstFrame -= frameOverCursor;
            *firstFrame = int(*firstFrame * 1.1f);
            framePixelWidthTarget *= 0.9f;
            *firstFrame += frameOverCursor;
         }

         if (io.MouseWheel > FLT_EPSILON)
         {
            *firstFrame -= frameOverCursor;
            *firstFrame = int(*firstFrame * 0.9f);
            framePixelWidthTarget *= 1.1f;
            *firstFrame += frameOverCursor;
         }
		}
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
      }
      if (panningView && !io.MouseDown[2])
      {
         panningView = false;
      }
		framePixelWidthTarget = ImClamp(framePixelWidthTarget, 0.1f, 50.f);

		framePixelWidth = ImLerp(framePixelWidth, framePixelWidthTarget, 0.33f);


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
			bool hasScrollBar(false);
			int framesPixelWidth = int(frameCount * framePixelWidth);
			if ((framesPixelWidth + legendWidth) >= canvas_size.x)
			{
				hasScrollBar = true;
				controlHeight += scrollBarHeight;
			}

			ImRect backgroundRect(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + controlHeight));
			ImGui::InvisibleButton("canvas", ImVec2(canvas_size.x, (float)controlHeight));

			// full background
			draw_list->AddRectFilled(backgroundRect.Min, backgroundRect.Max, 0xFF262222, 0);

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
					if (*currentFrame < 0)
						*currentFrame = 0;
					if (*currentFrame >= frameCount)
						*currentFrame = frameCount - 1;
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

         size_t customHeight = 0;
			for (int i = 0; i < sequenceCount; i++)
			{
				int type;
				sequence->Get(i, NULL, NULL, &type, NULL);
				ImVec2 tpos(canvas_pos.x + 3, canvas_pos.y + (i + 1) * ItemHeight + 2 + customHeight);
				draw_list->AddText(tpos, 0xFFFFFFFF, sequence->GetItemLabel(i));

				if (sequenceOptions&SEQUENCER_DEL)
				{
					bool overDel = SequencerAddDelButton(draw_list, ImVec2(canvas_pos.x + legendWidth - ItemHeight + 2 - 10, tpos.y + 2), false);
					if (overDel && io.MouseReleased[0])
						delEntry = i;

					bool overDup = SequencerAddDelButton(draw_list, ImVec2(canvas_pos.x + legendWidth - ItemHeight - ItemHeight + 2 - 10, tpos.y + 2), true);
					if (overDup && io.MouseReleased[0])
						dupEntry = i;
				}
            customHeight += sequence->GetCustomHeight(i);
			}

			// clipping rect so items bars are not visible in the legend on the left when scrolled
			draw_list->PushClipRect(ImVec2(canvas_pos.x + legendWidth, canvas_pos.y), ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + controlHeight), true);
			
			// slots background
         customHeight = 0;
			for (int i = 0; i < sequenceCount; i++)
			{
				unsigned int col = (i & 1) ? 0xFF3A3636 : 0xFF413D3D;

            size_t localCustomHeight = sequence->GetCustomHeight(i);
				ImVec2 pos = ImVec2(canvas_pos.x + legendWidth, canvas_pos.y + ItemHeight * (i + 1) + 1 + customHeight);
				ImVec2 sz = ImVec2(canvas_size.x + canvas_pos.x, pos.y + ItemHeight - 1 + localCustomHeight);
				if (!popupOpened && cy >= pos.y && cy < pos.y + ItemHeight && movingEntry == -1 && cx>canvas_pos.x && cx < canvas_pos.x + canvas_size.x)
				{
					col += 0x80201008;
					pos.x -= legendWidth;
				}
				draw_list->AddRectFilled(pos, sz, col, 0);
            customHeight += localCustomHeight;
			}

			int modFrameCount = 10;
			int frameStep = 1;
			while ((modFrameCount * framePixelWidth) < 150)
			{
				modFrameCount *= 2;
				frameStep *= 2;
			};
			int halfModFrameCount = modFrameCount / 2;

			auto drawLine = [&](int i) {
				bool baseIndex = ((i % modFrameCount) == 0) || (i == frameCount);
				bool halfIndex = (i % halfModFrameCount) == 0;
				int px = (int)canvas_pos.x + int(i * framePixelWidth) + legendWidth - int(firstFrameUsed * framePixelWidth);
				int tiretStart = baseIndex ? 4 : (halfIndex ? 10 : 14);
				int tiretEnd = baseIndex ? controlHeight : ItemHeight;

				if (px <= (canvas_size.x + canvas_pos.x) && px >= (canvas_pos.x + legendWidth))
				{
					draw_list->AddLine(ImVec2((float)px, canvas_pos.y + (float)tiretStart), ImVec2((float)px, canvas_pos.y + (float)tiretEnd - 1), 0xFF606060, 1);

					draw_list->AddLine(ImVec2((float)px, canvas_pos.y + (float)ItemHeight), ImVec2((float)px, canvas_pos.y + (float)controlHeight - 1), 0x30606060, 1);
				}

				if (baseIndex)
				{
					char tmps[512];
					sprintf(tmps, "%d", i);
					draw_list->AddText(ImVec2((float)px + 3.f, canvas_pos.y), 0xFFBBBBBB, tmps);
				}

			};
			for (int i = 0; i <= frameCount; i += frameStep)
			{
				drawLine(i);
			}
			drawLine(frameCount);

			draw_list->AddLine(canvas_pos, ImVec2(canvas_pos.x, canvas_pos.y + controlHeight), 0xFF000000, 1);
			draw_list->AddLine(ImVec2(canvas_pos.x, canvas_pos.y + ItemHeight), ImVec2(canvas_size.x, canvas_pos.y + ItemHeight), 0xFF000000, 1);

			// selection
			bool selected = selectedEntry && (*selectedEntry >= 0);
			if (selected)
			{
				draw_list->AddRectFilled(ImVec2(canvas_pos.x, canvas_pos.y + ItemHeight * (*selectedEntry + 1)), ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + ItemHeight * (*selectedEntry + 2)), 0x801080FF, 1.f);
			}

			// slots
			customHeight = 0;
			for (int i = 0; i < sequenceCount; i++)
			{
				int *start, *end;
				unsigned int color;
				sequence->Get(i, &start, &end, NULL, &color);
            size_t localCustomHeight = sequence->GetCustomHeight(i);

				ImVec2 pos = ImVec2(canvas_pos.x + legendWidth - firstFrameUsed * framePixelWidth, canvas_pos.y + ItemHeight * (i + 1) + 1 + customHeight);
				ImVec2 slotP1(pos.x + *start * framePixelWidth, pos.y + 2);
				ImVec2 slotP2(pos.x + *end * framePixelWidth + framePixelWidth, pos.y + ItemHeight - 2 + localCustomHeight);
				unsigned int slotColor = color | 0xFF000000;
				
				if (slotP1.x <= (canvas_size.x + canvas_pos.x) && slotP2.x >= (canvas_pos.x + legendWidth))
				{
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
				if (movingEntry == -1 && (sequenceOptions&SEQUENCER_EDIT_STARTEND) && backgroundRect.Contains(io.MousePos))
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
						if (io.MouseDown[0] && !MovingScrollBar && !MovingCurrentFrame)
						{
							movingEntry = i;
							movingPos = cx;
							movingPart = j + 1;
							break;
						}
					}
				}
            if (localCustomHeight > 0)
            {
               ImRect customArea(pos - ImVec2(legendWidth, 0), ImVec2(canvas_pos.x + canvas_size.x, slotP2.y));
               customDraws.push_back({ i, customArea });
            }
				customHeight += localCustomHeight;
			}
			//ImGui::PopClipRect();

			// moving
			if (backgroundRect.Contains(io.MousePos) && movingEntry >= 0)
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
					movingPos += int(diffFrame * framePixelWidth);
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
				}
			}
         for (auto& customDraw : customDraws)
            sequence->CustomDraw(customDraw.index, draw_list, customDraw.customArea);
			// cursor
			if (currentFrame && *currentFrame >= 0)
			{
				float cursorOffset = canvas_pos.x + legendWidth + (*currentFrame - firstFrameUsed) * framePixelWidth + framePixelWidth / 2;
				draw_list->AddLine(ImVec2(cursorOffset, canvas_pos.y), ImVec2(cursorOffset, canvas_pos.y + controlHeight), 0xA02A2AFF, 8);
			}
			draw_list->PopClipRect();
			// copy paste
			if (sequenceOptions&SEQUENCER_COPYPASTE)
			{
				ImRect rectCopy(ImVec2(canvas_pos.x + 100, canvas_pos.y + 2)
					, ImVec2(canvas_pos.x + 100 + 30, canvas_pos.y + ItemHeight - 2));
				bool inRectCopy = rectCopy.Contains(io.MousePos);
				unsigned int copyColor = inRectCopy ? 0xFF1080FF : 0xFF000000;
				draw_list->AddText(rectCopy.Min, copyColor, "Copy");

				ImRect rectPaste(ImVec2(canvas_pos.x + 140, canvas_pos.y + 2)
					, ImVec2(canvas_pos.x + 140 + 30, canvas_pos.y + ItemHeight - 2));
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
			
			if (hasScrollBar)
			{
				int scrollBarStartHeight = controlHeight - scrollBarHeight;
				// ratio = number of frames visible in control / number to total frames
				
				float startFrameOffset = ((float)firstFrameUsed / (float)frameCount) * (canvas_size.x - legendWidth);
				ImVec2 scrollBarA(canvas_pos.x + legendWidth, canvas_pos.y + scrollBarStartHeight);
				ImVec2 scrollBarB(canvas_pos.x + legendWidth + canvas_size.x, canvas_pos.y + controlHeight);
				draw_list->AddRectFilled(scrollBarA, scrollBarB, 0xFF222222, 0);

				ImRect scrollBarRect(scrollBarA, scrollBarB);
				bool inScrollBar = scrollBarRect.Contains(io.MousePos);
				ImVec2 scrollBarC(canvas_pos.x + legendWidth + startFrameOffset, canvas_pos.y + scrollBarStartHeight + 2);
				ImVec2 scrollBarD(canvas_pos.x + legendWidth + barWidthInPixels + startFrameOffset, canvas_pos.y + controlHeight - 2);
				draw_list->AddRectFilled(scrollBarC, scrollBarD, (inScrollBar|| MovingScrollBar) ? 0xFF606060 : 0xFF505050, 2);
            ImRect scrollBarThumb(scrollBarC, scrollBarD);
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
