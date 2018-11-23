#include "ImGradient.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <stdint.h>
#include <set>

namespace ImGradient
{
   static int DrawPoint(ImDrawList* draw_list, ImVec4 color, const ImVec2 size, bool editing, ImVec2 pos)
   {
      int ret = 0;
      ImGuiIO& io = ImGui::GetIO();

      ImVec2 p1 = ImLerp(pos, ImVec2(pos+ImVec2(size.x-size.y, 0.f)), color.w);
      ImVec2 p2 = ImLerp(pos+ImVec2(size.y, size.y), ImVec2(pos + size), color.w);

      ImRect rc(p1, p2);

	  color.w = 1.f;
      draw_list->AddRectFilled(p1, p2, ImColor(color));
      draw_list->AddRect(p1, p2, editing?(0xFFFFFFFF- ImColor(color)):0xFFFFFFFF, 4.f);

      if (rc.Contains(io.MousePos))
      {
         if (io.MouseClicked[0])
            return 2;
         return 1;
      }
      return 0;
   }
   
   bool Edit(Delegate &delegate, const ImVec2& size, int& selection)
   {
      bool ret = false;
      ImGuiIO& io = ImGui::GetIO();
      ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0,0));
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
         float t = io.MousePos.x / size.x;
         delegate.AddPoint(delegate.GetPoint(t));
         ret = true;
      }
      ImGui::EndChildFrame();
      ImGui::PopStyleVar();
      
      selection = currentSelection;
      return ret;
   }
}
