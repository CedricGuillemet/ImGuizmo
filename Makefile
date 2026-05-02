CXXFLAGS=-std=c++11
CPPFLAGS=-Isrc -Iexample

LIB_OBJS = src/ImGuizmo.o src/GraphEditor.o src/ImCurveEdit.o src/ImGradient.o src/ImSequencer.o
EXAMPLE_OBJS = example/imgui.o example/imgui_draw.o example/imgui_tables.o example/imgui_widgets.o example/main.o

EXAMPLE_NAME = example.exe
LDFLAGS=-mwindows -static-libgcc -static-libstdc++
LIBS=-limm32 -lopengl32 -lgdi32

$(EXAMPLE_NAME): $(LIB_OBJS) $(EXAMPLE_OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

example/main.o: CXXFLAGS := -std=c++17

clean:
	$(RM) $(LIB_OBJS)
	$(RM) $(EXAMPLE_OBJS)
	$(RM) $(EXAMPLE_NAME)
