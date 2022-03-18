TARGET_OS=$(shell uname | sed -e 's/_.*//' | tr '[:upper:]' '[:lower:]' | sed -e 's/\//_/g')
CXXFLAGS=-std=c++11
CPPFLAGS=-I. -Iexample
EXE_EXT=
LDFLAGS=-static-libgcc -static-libstdc++

ifneq (,$(findstring "$(TARGET_OS)", "linux" "darwin"))
	LIBS=-lGL
else
	EXE_EXT=.exe
	LDFLAGS+=-mwindows
	LIBS=-limm32 -lopengl32 -lgdi32
endif


LIB_OBJS = ImGuizmo.o GraphEditor.o ImCurveEdit.o ImGradient.o ImSequencer.o
EXAMPLE_OBJS = example/imgui.o example/imgui_draw.o example/imgui_tables.o example/imgui_widgets.o example/main.o

EXAMPLE_NAME = example.$(EXE_EXT)

$(EXAMPLE_NAME): $(LIB_OBJS) $(EXAMPLE_OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

example/main.o: CXXFLAGS := -std=c++17

clean:
	$(RM) $(LIB_OBJS)
	$(RM) $(EXAMPLE_OBJS)
	$(RM) $(EXAMPLE_NAME)
