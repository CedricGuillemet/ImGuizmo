#include "imgui.h"
#define IMAPP_IMPL
#include "ImApp.h"

#include "ImGuizmo.h"
#include "ImSequencer.h"

#include <math.h>
#include <vector>

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

void EditTransform(const float *cameraView, float *cameraProjection, float* matrix)
{
	static ImGuizmo::OPERATION mCurrentGizmoOperation(ImGuizmo::ROTATE);
	static ImGuizmo::MODE mCurrentGizmoMode(ImGuizmo::WORLD);
	static bool useSnap = false;
	static float snap[3] = { 1.f, 1.f, 1.f };

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
	ImGuiIO& io = ImGui::GetIO();
	ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
	ImGuizmo::Manipulate(cameraView, cameraProjection, mCurrentGizmoOperation, mCurrentGizmoMode, matrix, NULL, useSnap ? &snap[0] : NULL);
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
		sprintf(tmps, "[%02d] %s", index, SequencerItemTypeNames[myItems[index].mType]);
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


	// return true if selection is made
	
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

	float cameraView[16] = 
	  { 1.f, 0.f, 0.f, 0.f,
		0.f, 1.f, 0.f, 0.f,
		0.f, 0.f, 1.f, 0.f,
		1.f, -1.f, -4.f, 1.f };

	float cameraProjection[16];

	// sequence with default values
	MySequence mySequence;
	mySequence.mFrameCount = 100;
	mySequence.myItems.push_back(MySequence::MySequenceItem{ 0, 10, 30 });
	mySequence.myItems.push_back(MySequence::MySequenceItem{ 1, 20, 30 });
	mySequence.myItems.push_back(MySequence::MySequenceItem{ 3, 12, 60 });
	mySequence.myItems.push_back(MySequence::MySequenceItem{ 2, 61, 90 });
	mySequence.myItems.push_back(MySequence::MySequenceItem{ 4, 90, 99 });
	
	// Main loop
	while (!imApp.Done())
	{
		imApp.NewFrame();

		ImGuiIO& io = ImGui::GetIO();
		Perspective(27.f, io.DisplaySize.x / io.DisplaySize.y, 0.1f, 100.f, cameraProjection);

		ImGuizmo::BeginFrame();

		// debug
		ImGuizmo::DrawCube(cameraView, cameraProjection, objectMatrix);

		// create a window and insert the inspector
		ImGui::SetNextWindowPos(ImVec2(10, 10));
		ImGui::SetNextWindowSize(ImVec2(320, 240));
		ImGui::Begin("Matrix Inspector");
		EditTransform(cameraView, cameraProjection, objectMatrix);
		ImGui::End();

		// let's create the sequencer
		static int selectedEntry = -1;
		static int firstFrame = 0;
		static bool expanded = true;
		ImGui::SetNextWindowPos(ImVec2(10, 260));
		ImGui::SetNextWindowSize(ImVec2(740, 380));
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