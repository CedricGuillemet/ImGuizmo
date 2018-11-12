#include "ImCurveEdit.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <stdint.h>
#include <set>

namespace ImCurveEdit
{

   static float smoothstep(float edge0, float edge1, float x)
   {
      x = ImClamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
      return x * x * (3 - 2 * x);
   }

   static float distance(float x, float y, float x1, float y1, float x2, float y2)
   {
      float A = x - x1;
      float B = y - y1;
      float C = x2 - x1;
      float D = y2 - y1;

      float dot = A * C + B * D;
      float len_sq = C * C + D * D;
      float param = -1.f;
      if (len_sq > FLT_EPSILON)
         param = dot / len_sq;

      float xx, yy;

      if (param < 0.f) {
         xx = x1;
         yy = y1;
      }
      else if (param > 1.f) {
         xx = x2;
         yy = y2;
      }
      else {
         xx = x1 + param * C;
         yy = y1 + param * D;
      }

      float dx = x - xx;
      float dy = y - yy;
      return sqrtf(dx * dx + dy * dy);
   }

   static int DrawPoint(ImDrawList* draw_list, ImVec2 pos, const ImVec2 size, const ImVec2 offset, bool edited)
   {
      int ret = 0;
      ImGuiIO& io = ImGui::GetIO();

      static const ImVec2 localOffsets[4] = { ImVec2(1,0), ImVec2(0,1), ImVec2(-1,0), ImVec2(0,-1) };
      ImVec2 offsets[4];
      for (int i = 0; i < 4; i++)
      {
         offsets[i] = pos * size + localOffsets[i]*4.5f + offset;
      }
      
      const ImVec2 center = pos * size + offset;
      const ImRect anchor(center - ImVec2(5, 5), center + ImVec2(5, 5));
      draw_list->AddConvexPolyFilled(offsets, 4, 0xFF000000);
      if (anchor.Contains(io.MousePos))
      {
         ret = 1;
         if (io.MouseDown[0])
            ret = 2;
      }
      if (edited)
         draw_list->AddPolyline(offsets, 4, 0xFFFFFFFF, true, 3.0f);
      else if (ret)
         draw_list->AddPolyline(offsets, 4, 0xFF80B0FF, true, 2.0f);
      else
         draw_list->AddPolyline(offsets, 4, 0xFF0080FF, true, 2.0f);
      
      return ret;
   }

   int Edit(Delegate &delegate, const ImVec2& size)
   {
      struct EditPoint
      {
         int curveIndex;
         int pointIndex;
         bool operator <(const EditPoint& other) const
         {
            if (curveIndex < other.curveIndex)
               return true;
            if (curveIndex > other.curveIndex)
               return false;

            if (pointIndex < other.pointIndex)
               return true;
            return false;
         }
      };


      static bool selectingQuad = false;
      static ImVec2 quadSelection;
      static int overCurve = -1;
      static int movingCurve = -1;
      static float scaleV = 1.0f;
      static float scaleVTarget = 1.f;
      static const size_t subStepCount = 20;
      static const float step = 1.f / float(subStepCount - 1);
      static std::set<EditPoint> selection;
      static bool overSelectedPoint = false;

      ImGuiIO& io = ImGui::GetIO();
      ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0,0));
      ImGui::BeginChildFrame(137, size);

      ImDrawList* draw_list = ImGui::GetWindowDrawList();

      const ImVec2 offset = ImGui::GetCursorScreenPos() + ImVec2(0.f, size.y);
      const ImVec2 ssize(size.x, -size.y);
      const ImVec2 ssizeScaled(size.x, -size.y * scaleV);
      const ImVec2 sizeOfPixel = ImVec2(1.f, 1.f) / ssizeScaled;
      const ImRect container(offset + ImVec2(0.f, ssize.y), offset + ImVec2(ssize.x, 0.f));
      
      if (container.Contains(io.MousePos))
      {
         if (io.MouseWheel > FLT_EPSILON)
            scaleVTarget *= 0.95f;
         if (io.MouseWheel < -FLT_EPSILON)
            scaleVTarget *= 1.05f;
      }
      scaleV = ImLerp(scaleV, scaleVTarget, 0.5f);

      draw_list->AddRectFilled(offset, offset + ssize, 0xFF202020);
      

      const size_t curveCount = delegate.GetCurveCount();
      bool overCurveOrPoint = false;

      int localOverCurve = -1;
      for (size_t c = 0; c < curveCount; c++)
      {
         const size_t ptCount = delegate.GetPointCount(c);
         if (ptCount < 1)
            continue;

         const ImVec2* pts = delegate.GetPoints(c);
         uint32_t curveColor = delegate.GetCurveColor(c);
         if ((overCurve == c && selection.empty() && !selectingQuad) || movingCurve == c)
            curveColor = 0xFFFFFFFF;

         for (size_t p = 0; p < ptCount - 1; p++)
         {
            const ImVec2 p1 = pts[p];
            const ImVec2 p2 = pts[p+1];
            for (size_t substep = 0; substep < subStepCount-1; substep++)
            {
               float t = float(substep) * step;

               const ImVec2 sp1 = ImLerp(p1, p2, t);
               const ImVec2 sp2 = ImLerp(p1, p2, t+step);

               const float rt1 = smoothstep(p1.x, p2.x, sp1.x);
               const float rt2 = smoothstep(p1.x, p2.x, sp2.x);

               const ImVec2 pos1 = ImVec2(sp1.x, ImLerp(p1.y, p2.y, rt1)) * ssizeScaled + offset;
               const ImVec2 pos2 = ImVec2(sp2.x, ImLerp(p1.y, p2.y, rt2)) * ssizeScaled + offset;

               if (distance(io.MousePos.x, io.MousePos.y, pos1.x, pos1.y, pos2.x, pos2.y) < 8.f && localOverCurve == -1)
               {
                  localOverCurve = c;
                  overCurve = c;
                  overCurveOrPoint = true;
               }

               draw_list->AddLine(pos1, pos2, curveColor, 1.3f);
            } // substep
         } // point loop

         for (size_t p = 0; p < ptCount; p++)
         {
            const int drawState = DrawPoint(draw_list, pts[p], ssizeScaled, offset, (selection.find({int(c), int(p)}) != selection.end() && movingCurve == -1));
            if (drawState && movingCurve == -1 && !selectingQuad)
            {
               overCurveOrPoint = true;
               overSelectedPoint = true;
               overCurve = -1;
               if (drawState == 2)
               {
                  if (!io.KeyShift && selection.find({ int(c), int(p) }) == selection.end())
                     selection.clear();
                  selection.insert({ int(c), int(p) });
               }
            }
         }
      } // curves loop

      if (localOverCurve == -1)
         overCurve = -1;

      // move selection
      if (overSelectedPoint && io.MouseDown[0])
      {
         auto prevSelection = selection;
         for (auto& sel : prevSelection)
         {
            const ImVec2* pts = delegate.GetPoints(sel.curveIndex);
            const ImVec2 p = pts[sel.pointIndex] + io.MouseDelta * sizeOfPixel;
            const int newIndex = delegate.EditPoint(sel.curveIndex, sel.pointIndex, p);
            if (newIndex != sel.pointIndex)
            {
               selection.erase(sel);
               selection.insert({ sel.curveIndex, newIndex });
            }
         }
      }


      if (overSelectedPoint && !io.MouseDown[0])
         overSelectedPoint = false;

      // add point
      if (overCurve != -1 && io.MouseDoubleClicked[0])
      {
         const ImVec2 np = (io.MousePos - offset) / ssizeScaled;
         delegate.AddPoint(overCurve, np);
      }

      // move curve
      if (movingCurve != -1)
      {
         const size_t ptCount = delegate.GetPointCount(movingCurve);
         const ImVec2* pts = delegate.GetPoints(movingCurve);
         if (ptCount >= 1)
         {
            for (size_t p = 0; p < ptCount; p++)
            {
               delegate.EditPoint(movingCurve, p, pts[p] + io.MouseDelta * sizeOfPixel);
            }
         }
         if (!io.MouseDown[0])
            movingCurve = -1;
      }
      if (movingCurve == -1 && overCurve != -1 && io.MouseDown[0] && selection.empty() && !selectingQuad)
      {
         movingCurve = overCurve;
      }

      // quad selection
      if (selectingQuad)
      {
         const ImVec2 bmin = ImMin(quadSelection, io.MousePos);
         const ImVec2 bmax = ImMax(quadSelection, io.MousePos);
         draw_list->AddRectFilled(bmin, bmax, 0x40FF0000, 1.f);
         draw_list->AddRect(bmin, bmax, 0xFFFF0000, 1.f);
         const ImRect selectionQuad(bmin, bmax);
         if (!io.MouseDown[0])
         {
            if (!io.KeyShift)
               selection.clear();
            // select everythnig is quad
            for (size_t c = 0; c < curveCount; c++)
            {
               const size_t ptCount = delegate.GetPointCount(c);
               if (ptCount < 1)
                  continue;

               const ImVec2* pts = delegate.GetPoints(c);
               for (size_t p = 0; p < ptCount - 1; p++)
               {
                  const ImVec2 center = pts[p] * ssizeScaled + offset;
                  if (selectionQuad.Contains(center))
                     selection.insert({int(c), int(p)});
               }
            }
            // done
            selectingQuad = false;
         }
      }
      if (!overCurveOrPoint && io.MouseDown[0] && !selectingQuad && movingCurve == -1 && !overSelectedPoint)
      {
         selectingQuad = true;
         quadSelection = io.MousePos;
      }

      ImGui::EndChildFrame();
      ImGui::PopStyleVar();
      return 0;
   }
}
