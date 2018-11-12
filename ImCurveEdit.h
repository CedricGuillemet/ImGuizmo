#pragma once
#include <stdint.h>

struct ImVec2;

namespace ImCurveEdit
{
   struct Delegate
   {
      virtual size_t GetCurveCount() = 0;
      virtual size_t GetPointCount(size_t curveIndex) = 0;
      virtual uint32_t GetCurveColor(size_t curveIndex) = 0;
      virtual ImVec2* GetPoints(size_t curveIndex) = 0;
      virtual int EditPoint(size_t curveIndex, int pointIndex, ImVec2 value) = 0;
      virtual void AddPoint(size_t curveIndex, ImVec2 value) = 0;
   };

   int Edit(Delegate &delegate, const ImVec2& size);
}
