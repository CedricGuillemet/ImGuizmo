# ImGuizmo
Immediate mode 3D gizmo for scene editing.
Dear Imgui is the only dependancy. Only world translation supported for now. Local mode and rotation/scale might come in the future.
Add both .h and .cpp to your project then:

Call BeginFrame right after ImGui_XXXX_NewFrame();

void BeginFrame();

Return true if mouse cursor is over any gizmo control (axis, plan or screen component)

bool IsOver();

Return true if mouse IsOver or if the gizmo is in moving state

bool IsUsing();

Enable/disable the gizmo. Stay in the state until next call to Enable.
Gizmo is rendered with gray half transparent color when disabled

void Enable(bool enable);

Call it when you want a gizmo
Needs view and projection matrices. 
Matrix parameter is the source matrix (where will be gizmo be drawn) and might be transformed by the function. Return deltaMatrix is optional
Translation is applied in world space

void Translate(const float *view, const float *projection, float *matrix, float *deltaMatrix = 0);
