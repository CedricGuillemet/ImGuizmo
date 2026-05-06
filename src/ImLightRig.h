#pragma once
#include "imgui.h"

namespace ImLightRig
{
   struct Light
   {
      float r, g, b;
      float intensity;
      float x, y;
   };

   inline float Distance(ImVec2 a, ImVec2 b)
   {
      ImVec2 dif = b - a;
      return sqrtf(dif.x * dif.x + dif.y * dif.y);
   }

   inline int Edit(Light* lights, int lightCount, int selected = -1, ImVec2 size = ImVec2(0,0))
   {
      ImDrawList* draw_list = ImGui::GetWindowDrawList();
      ImGuiIO& io = ImGui::GetIO();
      const ImVec2 canvas_pos = ImGui::GetCursorScreenPos();            // ImDrawList API uses screen coordinates!
      const ImVec2 canvas_size = ImGui::GetContentRegionAvail();        // Resize canvas to what's available

      const float sz = (canvas_size.x < canvas_size.y) ? canvas_size.x : canvas_size.y;
      ImVec2 frameSize(
         (size.x < 0.001f) ? sz : size.x,
         (size.y < 0.001f) ? sz : size.y);

      ImGui::InvisibleButton("ImLightRigCanvas", frameSize);
      draw_list->PushClipRect(canvas_pos, canvas_pos + frameSize);
      const float controlWidth = frameSize.x;
      
      draw_list->AddRectFilled(canvas_pos, canvas_pos + frameSize, 0xFF404040);
      draw_list->AddLine(ImVec2(canvas_pos.x + controlWidth * 0.5f, canvas_pos.y), ImVec2(canvas_pos.x + controlWidth * 0.5f, canvas_pos.y + controlWidth), 0xFFAAAAAA);
      draw_list->AddLine(ImVec2(canvas_pos.x, canvas_pos.y + controlWidth * 0.5f), ImVec2(canvas_pos.x + controlWidth, canvas_pos.y + controlWidth * 0.5f), 0xFFAAAAAA);
      draw_list->AddCircle(ImVec2(canvas_pos.x + controlWidth * 0.5f, canvas_pos.y + controlWidth * 0.5f), controlWidth * 0.5f, 0xFFAAAAAA);

      const ImVec2 middle(canvas_pos.x + controlWidth * 0.5f, canvas_pos.y + controlWidth * 0.5f);
      int res = selected;
      static int movingLight = -1;
      for (int i = 0; i < lightCount; i++)
      {
         auto& light = lights[i];
         const auto center(ImVec2(canvas_pos.x + controlWidth * 0.5f + light.x * controlWidth * 0.5f, canvas_pos.y + controlWidth * 0.5f + light.y * controlWidth * 0.5f));
         ImColor color(light.r, light.g, light.b, 1.f);
         ImColor intensity(light.intensity, light.intensity, light.intensity, 1.f);
         const float radius = controlWidth * 0.05f;

         draw_list->AddLine(center, middle, intensity);
         draw_list->AddCircle(center, radius, intensity, 0, 3.f);
         draw_list->AddCircleFilled(center, radius, color);

         if (selected == i)
         {
            draw_list->AddCircle(center, radius * 1.5f, intensity, 0, 2.f);
         }

         const float distance = Distance(io.MousePos, center);
         const bool isOver = distance < radius && movingLight == -1;
         if (isOver)
         {
            draw_list->AddCircle(center, radius * 1.5f, intensity, 0, 4.f);
            if (io.MouseClicked[0])
            {
               res = i;
               movingLight = i;
            }
         }
      }
      if (!io.MouseDown[0])
      {
         movingLight = -1;
      }
      if (movingLight >= 0 && movingLight < lightCount && io.MouseDown[0])
      {
         auto& light = lights[movingLight];
         light.x += (io.MouseDelta.x / controlWidth) * 2.f;
         light.y += (io.MouseDelta.y / controlWidth) * 2.f;

         ImVec2 dir(light.x, light.y);
         float len = Distance(ImVec2(0.f, 0.f), dir);
         if (len > 1.f)
         {
            dir *= 1.f / len;
            light.x = dir.x;
            light.y = dir.y;
         }
      }
      draw_list->PopClipRect();
      return res;
   }
}
