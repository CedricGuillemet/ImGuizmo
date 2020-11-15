#include "ImGradient.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <stdint.h>
#include <set>

namespace ImGradient
{
#ifndef IMGUI_DEFINE_MATH_OPERATORS
   static inline ImVec2 operator*(const ImVec2& lhs, const float rhs) { return ImVec2(lhs.x * rhs, lhs.y * rhs); }
   static inline ImVec2 operator/(const ImVec2& lhs, const float rhs) { return ImVec2(lhs.x / rhs, lhs.y / rhs); }
   static inline ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y); }
   static inline ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x - rhs.x, lhs.y - rhs.y); }
   static inline ImVec2 operator*(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x * rhs.x, lhs.y * rhs.y); }
   static inline ImVec2 operator/(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x / rhs.x, lhs.y / rhs.y); }
#endif

   static int DrawPoint(ImDrawList* draw_list, ImVec4 color, const ImVec2 size, bool editing, ImVec2 pos)
   {
      int ret = 0;
      ImGuiIO& io = ImGui::GetIO();

      ImVec2 p1 = ImLerp(pos, ImVec2(pos + ImVec2(size.x - size.y, 0.f)), color.w) + ImVec2(3, 3);
      ImVec2 p2 = ImLerp(pos + ImVec2(size.y, size.y), ImVec2(pos + size), color.w) - ImVec2(3, 3);
      ImRect rc(p1, p2);

      color.w = 1.f;
      draw_list->AddRectFilled(p1, p2, ImColor(color));
      if (editing)
         draw_list->AddRect(p1, p2, 0xFFFFFFFF, 2.f, 15, 2.5f);
      else
         draw_list->AddRect(p1, p2, 0x80FFFFFF, 2.f, 15, 1.25f);

      if (rc.Contains(io.MousePos))
      {
         if (io.MouseClicked[0])
            return 2;
         return 1;
      }
      return 0;
   }

   bool Edit(Delegate& delegate, const ImVec2& size, int& selection)
   {
      bool ret = false;
      ImGuiIO& io = ImGui::GetIO();
      ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
      ImGui::BeginChildFrame(137, size);

      ImDrawList* draw_list = ImGui::GetWindowDrawList();
      const ImVec2 offset = ImGui::GetCursorScreenPos();

      const ImVec4* pts = delegate.GetPoints();
      static int currentSelection = -1;
      static int movingPt = -1;
      if (currentSelection >= int(delegate.GetPointCount()))
         currentSelection = -1;
      if (movingPt != -1)
      {
         ImVec4 current = pts[movingPt];
         current.w += io.MouseDelta.x / size.x;
         current.w = ImClamp(current.w, 0.f, 1.f);
         delegate.EditPoint(movingPt, current);
         ret = true;
         if (!io.MouseDown[0])
            movingPt = -1;
      }
      for (size_t i = 0; i < delegate.GetPointCount(); i++)
      {
         int ptSel = DrawPoint(draw_list, pts[i], size, i == currentSelection, offset);
         if (ptSel == 2)
         {
            currentSelection = int(i);
            ret = true;
         }
         if (ptSel == 1 && io.MouseDown[0] && movingPt == -1)
         {
            movingPt = int(i);
         }
      }
      ImRect rc(offset, offset + size);
      if (rc.Contains(io.MousePos) && io.MouseDoubleClicked[0])
      {
         float t = (io.MousePos.x - offset.x) / size.x;
         delegate.AddPoint(delegate.GetPoint(t));
         ret = true;
      }
      ImGui::EndChildFrame();
      ImGui::PopStyleVar();

      selection = currentSelection;
      return ret;
   }
}
