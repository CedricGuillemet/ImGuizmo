#pragma once
#include <cstddef>

struct ImVec4;
struct ImVec2;

namespace ImGradient
{
   struct Delegate
   {
      virtual size_t GetPointCount() = 0;
      virtual ImVec4* GetPoints() = 0;
      virtual int EditPoint(int pointIndex, ImVec4 value) = 0;
      virtual ImVec4 GetPoint(float t) = 0;
      virtual void AddPoint(ImVec4 value) = 0;
   };

   bool Edit(Delegate& delegate, const ImVec2& size, int& selection);
}
