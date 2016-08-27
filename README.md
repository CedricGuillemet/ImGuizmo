# ImGuizmo

ImGizmo is a small (.h and .cpp) library built ontop of Dear ImGui that allow you to manipulate(Rotate & translate at the moment) 4x4 float matrices. No other dependancies. Coded with Immediate Mode (IM) philosophy in mind.

![Image of Rotation](http://i.imgur.com/y4mcVoT.gif)
![Image of Translation](http://i.imgur.com/o8q8iHq.gif)


// call BeginFrame right after ImGui_XXXX_NewFrame();


**void BeginFrame();**

// return true if mouse cursor is over any gizmo control (axis, plan or screen component)


**bool IsOver();**

// return true if mouse IsOver or if the gizmo is in moving state


**bool IsUsing();**

// enable/disable the gizmo. Stay in the state until next call to Enable.
// gizmo is rendered with gray half transparent color when disabled


**void Enable(bool enable);**

// call it when you want a gizmo
// Needs view and projection matrices. 
// matrix parameter is the source matrix (where will be gizmo be drawn) and might be transformed by the function. Return deltaMatrix is optional
// translation is applied in world space
// rotation is applied in local space
// Note: all matrices are 4x4 float matrix. OpenGL style.

**enum MODE { TRANSLATE, ROTATE };**

**void Mogwai(const float *view, const float *projection, MODE mode, float *matrix, float *deltaMatrix = 0);**


