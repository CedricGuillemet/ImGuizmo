#include "imgui.h"
#define IMAPP_IMPL
#include "ImApp.h"

#include "ImGuizmo.h"
#include "ImSequencer.h"

#include <math.h>
#include <vector>
#include <algorithm>
#include "ImCurveEdit.h"

//
//
// ImGuizmo example helper functions
//
//

void Frustum(float left, float right, float bottom, float top, float znear, float zfar, float *m16)
{
	float temp, temp2, temp3, temp4;
	temp = 2.0f * znear;
	temp2 = right - left;
	temp3 = top - bottom;
	temp4 = zfar - znear;
	m16[0] = temp / temp2;
	m16[1] = 0.0;
	m16[2] = 0.0;
	m16[3] = 0.0;
	m16[4] = 0.0;
	m16[5] = temp / temp3;
	m16[6] = 0.0;
	m16[7] = 0.0;
	m16[8] = (right + left) / temp2;
	m16[9] = (top + bottom) / temp3;
	m16[10] = (-zfar - znear) / temp4;
	m16[11] = -1.0f;
	m16[12] = 0.0;
	m16[13] = 0.0;
	m16[14] = (-temp * zfar) / temp4;
	m16[15] = 0.0;
}

void Perspective(float fovyInDegrees, float aspectRatio, float znear, float zfar, float *m16)
{
	float ymax, xmax;
	ymax = znear * tanf(fovyInDegrees * 3.141592f / 180.0f);
	xmax = ymax * aspectRatio;
	Frustum(-xmax, xmax, -ymax, ymax, znear, zfar, m16);
}

void Cross(const float* a, const float* b, float* r)
{
	r[0] = a[1] * b[2] - a[2] * b[1];
	r[1] = a[2] * b[0] - a[0] * b[2];
	r[2] = a[0] * b[1] - a[1] * b[0];
}

float Dot(const float* a, const float* b)
{
	return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

void Normalize(const float* a, float *r)
{
	float il = 1.f / (sqrtf(Dot(a, a)) + FLT_EPSILON);
	r[0] = a[0] * il;
	r[1] = a[1] * il;
	r[2] = a[2] * il;
}

void LookAt(const float* eye, const float* at, const float* up, float *m16)
{
	float X[3], Y[3], Z[3], tmp[3];

	tmp[0] = eye[0] - at[0];
	tmp[1] = eye[1] - at[1];
	tmp[2] = eye[2] - at[2];
	//Z.normalize(eye - at);
	Normalize(tmp, Z);
	Normalize(up, Y);
	//Y.normalize(up);

	Cross(Y, Z, tmp);
	//tmp.cross(Y, Z);
	Normalize(tmp, X);
	//X.normalize(tmp);

	Cross(Z, X, tmp);
	//tmp.cross(Z, X);
	Normalize(tmp, Y);
	//Y.normalize(tmp);

	m16[0] = X[0];
	m16[1] = Y[0];
	m16[2] = Z[0];
	m16[3] = 0.0f;
	m16[4] = X[1];
	m16[5] = Y[1];
	m16[6] = Z[1];
	m16[7] = 0.0f;
	m16[8] = X[2];
	m16[9] = Y[2];
	m16[10] = Z[2];
	m16[11] = 0.0f;
	m16[12] = -Dot(X, eye);
	m16[13] = -Dot(Y, eye);
	m16[14] = -Dot(Z, eye);
	m16[15] = 1.0f;
}

void OrthoGraphic(const float l, float r, float b, const float t, float zn, const float zf, float *m16)
{
	m16[0] = 2 / (r - l);
	m16[1] = 0.0f;
	m16[2] = 0.0f;
	m16[3] = 0.0f;
	m16[4] = 0.0f;
	m16[5] = 2 / (t - b);
	m16[6] = 0.0f;
	m16[7] = 0.0f;
	m16[8] = 0.0f;
	m16[9] = 0.0f;
	m16[10] = 1.0f / (zf - zn);
	m16[11] = 0.0f;
	m16[12] = (l + r) / (l - r);
	m16[13] = (t + b) / (b - t);
	m16[14] = zn / (zn - zf);
	m16[15] = 1.0f;
}

void EditTransform(const float *cameraView, float *cameraProjection, float* matrix)
{
	static ImGuizmo::OPERATION mCurrentGizmoOperation(ImGuizmo::TRANSLATE);
	static ImGuizmo::MODE mCurrentGizmoMode(ImGuizmo::LOCAL);
	static bool useSnap = false;
	static float snap[3] = { 1.f, 1.f, 1.f };
	static float bounds[] = { -0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f };
	static float boundsSnap[] = { 0.1f, 0.1f, 0.1f };
	static bool boundSizing = false;
	static bool boundSizingSnap = false;

	if (ImGui::IsKeyPressed(90))
		mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
	if (ImGui::IsKeyPressed(69))
		mCurrentGizmoOperation = ImGuizmo::ROTATE;
	if (ImGui::IsKeyPressed(82)) // r Key
		mCurrentGizmoOperation = ImGuizmo::SCALE;
	if (ImGui::RadioButton("Translate", mCurrentGizmoOperation == ImGuizmo::TRANSLATE))
		mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
	ImGui::SameLine();
	if (ImGui::RadioButton("Rotate", mCurrentGizmoOperation == ImGuizmo::ROTATE))
		mCurrentGizmoOperation = ImGuizmo::ROTATE;
	ImGui::SameLine();
	if (ImGui::RadioButton("Scale", mCurrentGizmoOperation == ImGuizmo::SCALE))
		mCurrentGizmoOperation = ImGuizmo::SCALE;
	float matrixTranslation[3], matrixRotation[3], matrixScale[3];
	ImGuizmo::DecomposeMatrixToComponents(matrix, matrixTranslation, matrixRotation, matrixScale);
	ImGui::InputFloat3("Tr", matrixTranslation, 3);
	ImGui::InputFloat3("Rt", matrixRotation, 3);
	ImGui::InputFloat3("Sc", matrixScale, 3);
	ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation, matrixRotation, matrixScale, matrix);

	if (mCurrentGizmoOperation != ImGuizmo::SCALE)
	{
		if (ImGui::RadioButton("Local", mCurrentGizmoMode == ImGuizmo::LOCAL))
			mCurrentGizmoMode = ImGuizmo::LOCAL;
		ImGui::SameLine();
		if (ImGui::RadioButton("World", mCurrentGizmoMode == ImGuizmo::WORLD))
			mCurrentGizmoMode = ImGuizmo::WORLD;
	}
	if (ImGui::IsKeyPressed(83))
		useSnap = !useSnap;
	ImGui::Checkbox("", &useSnap);
	ImGui::SameLine();

	switch (mCurrentGizmoOperation)
	{
	case ImGuizmo::TRANSLATE:
		ImGui::InputFloat3("Snap", &snap[0]);
		break;
	case ImGuizmo::ROTATE:
		ImGui::InputFloat("Angle Snap", &snap[0]);
		break;
	case ImGuizmo::SCALE:
		ImGui::InputFloat("Scale Snap", &snap[0]);
		break;
	}
	ImGui::Checkbox("Bound Sizing", &boundSizing);
	if (boundSizing)
	{
		ImGui::PushID(3);
		ImGui::Checkbox("", &boundSizingSnap);
		ImGui::SameLine();
		ImGui::InputFloat3("Snap", boundsSnap);
		ImGui::PopID();
	}

	ImGuiIO& io = ImGui::GetIO();
	ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
	ImGuizmo::Manipulate(cameraView, cameraProjection, mCurrentGizmoOperation, mCurrentGizmoMode, matrix, NULL, useSnap ? &snap[0] : NULL, boundSizing?bounds:NULL, boundSizingSnap?boundsSnap:NULL);
}

//
//
// ImSequencer interface
//
//
static const char* SequencerItemTypeNames[] = { "Camera","Music", "ScreenEffect", "FadeIn", "Animation" };

struct MySequence : public ImSequencer::SequenceInterface
{
	// interface with sequencer
	
	virtual int GetFrameCount() const { return mFrameCount; }
	virtual int GetItemCount() const { return (int)myItems.size(); }

	virtual int GetItemTypeCount() const { return sizeof(SequencerItemTypeNames)/sizeof(char*); }
	virtual const char *GetItemTypeName(int typeIndex) const { return SequencerItemTypeNames[typeIndex]; }
	virtual const char *GetItemLabel(int index) const 
	{ 
		static char tmps[512];
		sprintf_s(tmps, "[%02d] %s", index, SequencerItemTypeNames[myItems[index].mType]);
		return tmps; 
	}

	virtual void Get(int index, int** start, int** end, int *type, unsigned int *color)
	{
		MySequenceItem &item = myItems[index];
		if (color)
			*color = 0xFFAA8080; // same color for everyone, return color based on type
		if (start)
			*start = &item.mFrameStart;
		if (end)
			*end = &item.mFrameEnd;
		if (type)
			*type = item.mType;
	}
	virtual void Add(int type) { myItems.push_back(MySequenceItem{ type, 0, 10 }); };
	virtual void Del(int index) { myItems.erase(myItems.begin() + index); }
	virtual void Duplicate(int index) { myItems.push_back(myItems[index]); }

	// my datas
	MySequence() : mFrameCount(0) {}
	int mFrameCount;
	struct MySequenceItem
	{
		int mType;
		int mFrameStart, mFrameEnd;
	};
	std::vector<MySequenceItem> myItems;
};


inline void rotationY(const float angle, float *m16)
{
	float c = cosf(angle);
	float s = sinf(angle);

	m16[0] = c;
	m16[1] = 0.0f;
	m16[2] = -s;
	m16[3] = 0.0f;
	m16[4] = 0.0f;
	m16[5] = 1.f;
	m16[6] = 0.0f;
	m16[7] = 0.0f;
	m16[8] = s;
	m16[9] = 0.0f;
	m16[10] = c;
	m16[11] = 0.0f;
	m16[12] = 0.f;
	m16[13] = 0.f;
	m16[14] = 0.f;
	m16[15] = 1.0f;
}

struct RampEdit : public ImCurveEdit::Delegate
{
   RampEdit()
   {
      mPts[0][0] = ImVec2(0, 0);
      mPts[0][1] = ImVec2(0.2f, 0.6f);
      mPts[0][2] = ImVec2(0.5f, 0.2f);
      mPts[0][3] = ImVec2(0.7f, 0.4f);
      mPts[0][4] = ImVec2(1.f, 1.f);
      mPointCount[0] = 5;

      mPts[1][0] = ImVec2(0, 0.2f);
      mPts[1][1] = ImVec2(0.35f, 0.7f);
      mPts[1][2] = ImVec2(0.5f, 0.2f);
      mPts[1][3] = ImVec2(1.f, 0.8f);
      mPointCount[1] = 4;


      mPts[2][0] = ImVec2(0, 0);
      mPts[2][1] = ImVec2(0.4f, 0.1f);
      mPts[2][2] = ImVec2(0.55f, 0.82f);
      mPts[2][3] = ImVec2(0.85f, 0.24f);
      mPts[2][4] = ImVec2(0.95f, 0.34f);
      mPts[2][5] = ImVec2(1.f, 0.12f);
      mPointCount[2] = 6;

   }
   size_t GetCurveCount()
   {
      return 3;
   }

   size_t GetPointCount(size_t curveIndex)
   {
      return mPointCount[curveIndex];
   }

   uint32_t GetCurveColor(size_t curveIndex)
   {
      uint32_t cols[] = { 0xFF0000FF, 0xFF00FF00, 0xFFFF0000 };
      return cols[curveIndex];
   }
   ImVec2* GetPoints(size_t curveIndex)
   {
      return mPts[curveIndex];
   }

   virtual int EditPoint(size_t curveIndex, int pointIndex, ImVec2 value)
   {
      mPts[curveIndex][pointIndex] = value;
      SortValues(curveIndex);
      for (size_t i = 0; i < GetPointCount(curveIndex); i++)
      {
         if (mPts[curveIndex][i].x == value.x)
            return i;
      }
      return pointIndex;
   }
   virtual void AddPoint(size_t curveIndex, ImVec2 value)
   {
      if (mPointCount[curveIndex] >= 8)
         return;
      mPts[curveIndex][mPointCount[curveIndex]++] = value;
      SortValues(curveIndex);
   }
   ImVec2 mPts[3][8];
   size_t mPointCount[3];

private:
   void SortValues(size_t curveIndex)
   {
      auto b = std::begin(mPts[curveIndex]);
      auto e = std::begin(mPts[curveIndex]) + GetPointCount(curveIndex);
      std::sort(b, e, [](ImVec2 a, ImVec2 b) { return a.x < b.x; });

   }
};
	
int main(int, char**)
{
	ImApp::ImApp imApp;

	ImApp::Config config;
	config.mWidth = 1280;
	config.mHeight = 720;
	//config.mFullscreen = true;
	imApp.Init(config);

	float objectMatrix[16] = 
	  { 1.f, 0.f, 0.f, 0.f,
		0.f, 1.f, 0.f, 0.f,
		0.f, 0.f, 1.f, 0.f,
		0.f, 0.f, 0.f, 1.f };

	static const float identityMatrix[16] =
	{ 1.f, 0.f, 0.f, 0.f,
		0.f, 1.f, 0.f, 0.f,
		0.f, 0.f, 1.f, 0.f,
		0.f, 0.f, 0.f, 1.f };

	float cameraView[16] = 
	  { 1.f, 0.f, 0.f, 0.f,
		0.f, 1.f, 0.f, 0.f,
		0.f, 0.f, 1.f, 0.f,
		0.f, 0.f, 0.f, 1.f };

	float cameraProjection[16];

	// sequence with default values
	MySequence mySequence;
	mySequence.mFrameCount = 100;
	mySequence.myItems.push_back(MySequence::MySequenceItem{ 0, 10, 30 });
	mySequence.myItems.push_back(MySequence::MySequenceItem{ 1, 20, 30 });
	mySequence.myItems.push_back(MySequence::MySequenceItem{ 3, 12, 60 });
	mySequence.myItems.push_back(MySequence::MySequenceItem{ 2, 61, 90 });
	mySequence.myItems.push_back(MySequence::MySequenceItem{ 4, 90, 99 });
	
	// Camera projection
	bool isPerspective = false;
	float fov = 27.f;
	float viewWidth = 10.f; // for orthographic
	float camYAngle = 165.f / 180.f * 3.14159f;
	float camXAngle = 52.f / 180.f * 3.14159f;
	float camDistance = 8.f;
	rotationY(0.f, objectMatrix);

	
	// Main loop
	while (!imApp.Done())
	{
		imApp.NewFrame();

		ImGuiIO& io = ImGui::GetIO();
		if (isPerspective)
		{
			Perspective(fov, io.DisplaySize.x / io.DisplaySize.y, 0.1f, 100.f, cameraProjection);
		}
		else
		{
			float viewHeight = viewWidth*io.DisplaySize.y / io.DisplaySize.x;
			OrthoGraphic(-viewWidth, viewWidth, -viewHeight, viewHeight, -viewWidth, viewWidth, cameraProjection);
		}
		ImGuizmo::SetOrthographic(!isPerspective);

		float eye[] = { cosf(camYAngle) * cosf(camXAngle) * camDistance, sinf(camXAngle) * camDistance, sinf(camYAngle) * cosf(camXAngle) * camDistance };
		float at[] = { 0.f, 0.f, 0.f };
		float up[] = { 0.f, 1.f, 0.f };
		LookAt(eye, at, up, cameraView);
		ImGuizmo::BeginFrame();


		//static float ng = 0.f;
		//ng += 0.01f;
		//ng = 1.f;
		//rotationY(ng, objectMatrix);
		//objectMatrix[12] = 5.f;
		// debug
		ImGuizmo::DrawCube(cameraView, cameraProjection, objectMatrix);
		ImGuizmo::DrawGrid(cameraView, cameraProjection, identityMatrix, 10.f);

		// create a window and insert the inspector
		ImGui::SetNextWindowPos(ImVec2(10, 10));
		ImGui::SetNextWindowSize(ImVec2(320, 340));
		ImGui::Begin("Editor");
		ImGui::Text("Camera");
		if (ImGui::RadioButton("Perspective", isPerspective)) isPerspective = true;
		ImGui::SameLine();
		if (ImGui::RadioButton("Orthographic", !isPerspective)) isPerspective = false;
		if (isPerspective)
		{
			ImGui::SliderFloat("Fov", &fov, 20.f, 110.f);
		}
		else
		{
			ImGui::SliderFloat("Ortho width", &viewWidth, 1, 20);
		}
		ImGui::SliderAngle("Camera X", &camXAngle, 0.f, 179.f);
		ImGui::SliderAngle("Camera Y", &camYAngle);
		ImGui::SliderFloat("Distance", &camDistance, 1.f, 10.f);
		ImGui::Text("X: %f Y: %f", io.MousePos.x, io.MousePos.y);
		ImGui::Separator();
		EditTransform(cameraView, cameraProjection, objectMatrix);
		ImGui::End();

		// let's create the sequencer
		static int selectedEntry = -1;
		static int firstFrame = 0;
		static bool expanded = true;
		ImGui::SetNextWindowPos(ImVec2(10, 350));
		ImGui::SetNextWindowSize(ImVec2(740, 480));
		ImGui::Begin("Sequencer");
		ImGui::InputInt("Frame count", &mySequence.mFrameCount);
		Sequencer(&mySequence, NULL, &expanded, &selectedEntry, &firstFrame, ImSequencer::SEQUENCER_EDIT_STARTEND | ImSequencer::SEQUENCER_ADD | ImSequencer::SEQUENCER_DEL | ImSequencer::SEQUENCER_COPYPASTE);
		// add a UI to edit that particular item
		if (selectedEntry != -1)
		{
			MySequence::MySequenceItem &item = mySequence.myItems[selectedEntry];
			ImGui::Text("I am a %s, please edit me", SequencerItemTypeNames[item.mType]);
			// switch (type) ....
		}

      static RampEdit rampEdit;
      ImCurveEdit::Edit(rampEdit, ImVec2(600, 300));
		ImGui::End();

		// render everything
		glClearColor(0.45f, 0.4f, 0.4f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui::Render();

		imApp.EndFrame();
	}

	imApp.Finish();

	return 0;
}
