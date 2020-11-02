[![Linux](https://github.com/encelo/ncTracer/workflows/Linux/badge.svg)](https://github.com/encelo/ncTracer/actions?workflow=Linux)
[![macOS](https://github.com/encelo/ncTracer/workflows/macOS/badge.svg)](https://github.com/encelo/ncTracer/actions?workflow=macOS)
[![Windows](https://github.com/encelo/ncTracer/workflows/Windows/badge.svg)](https://github.com/encelo/ncTracer/actions?workflow=Windows)
[![MinGW](https://github.com/encelo/ncTracer/workflows/MinGW/badge.svg)](https://github.com/encelo/ncTracer/actions?workflow=MinGW)
[![Emscripten](https://github.com/encelo/ncTracer/workflows/Emscripten/badge.svg)](https://github.com/encelo/ncTracer/actions?workflow=Emscripten)
[![Android](https://github.com/encelo/ncTracer/workflows/Android/badge.svg)](https://github.com/encelo/ncTracer/actions?workflow=Android)
[![CodeQL](https://github.com/encelo/ncTracer/workflows/CodeQL/badge.svg)](https://github.com/encelo/ncTracer/actions?workflow=CodeQL)

# ncTracer
An [ImGui](https://github.com/ocornut/imgui) front-end to the [pmTracer](https://github.com/encelo/pmTracer) library made with the [nCine](https://github.com/nCine/nCine).

**Poor Man's Tracer** is a very simple and minimal ray tracing and path tracing library based on [_Ray Tracing from the Ground Up_](http://www.raytracegroundup.com/) by Kevin Suffern.

## Notes
* Don't forget to compile the nCine with `-D NCINE_DYNAMIC_LIBRARY=OFF` so that ncTracer can access the OpenGL and threading private API.
* For testing purposes there are some defines that you can change
  * In `SceneContext.cpp` you can change the value of `THREADING_TYPE` to switch between single threaded, tiled single threaded and tiled multi-threaded
  * In `ThreadManager.h` you can change the value of `STD_THREADS` to switch between `std::thread` and `ncine::Thread`
