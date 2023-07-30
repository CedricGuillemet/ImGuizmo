FROM ubuntu:20.04

ARG DEBIAN_FRONTEND=noninteractive

# nvidia docker runtime env
ENV NVIDIA_VISIBLE_DEVICES \
        ${NVIDIA_VISIBLE_DEVICES:-all}
ENV NVIDIA_DRIVER_CAPABILITIES \
        ${NVIDIA_DRIVER_CAPABILITIES:+$NVIDIA_DRIVER_CAPABILITIES,}graphics,compat32,utility

RUN apt-get update &&\
    apt-get install -y \
        build-essential \
        cmake \
        gdb \
        git \
        libglfw3-dev \
        libglew-dev \
        libsdl2-dev \
        xorg-dev \
        apt-utils \
        dialog \
        x11-apps

WORKDIR /usr/local/src

RUN git clone https://github.com/skaslev/gl3w.git &&\
    cd gl3w && mkdir build && cd build &&\
    cmake .. && make && make install

RUN git clone https://github.com/aniongithub/imgui-cmake.git &&\
    cd imgui-cmake &&\
    git submodule update --init --recursive &&\
    mkdir build && cd build &&\
    cmake -DIMGUI_GLLOADER=gl3w -DIMGUI_GLFW_IMPL=OFF -DBUILD_GLFW_OPENGL3_SAMPLE=OFF -DBUILD_SDL2_OPENGL3_SAMPLE=OFF .. &&\
    make && make install