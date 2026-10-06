#pragma once
#include <string>

enum class AppMode     { FunctionPlotter, FractalExplorer };
enum class ViewMode    { Wireframe, Surface };
enum class FractalType { Mandelbrot, Julia };

// The scene area in framebuffer pixels, origin at the bottom-left
struct SceneViewport {
    int x = 0, y = 0, width = 0, height = 0;
    bool  valid()  const { return width > 0 && height > 0; }
    float aspect() const { return valid() ? float(width) / float(height) : 1.0f; }
};

// Shared state. The UI writes the "controls" and "requests".
struct AppState {
    //Mode
    AppMode mode = AppMode::FunctionPlotter;

    //Function plotter controls
    char     equation[256] = "a*sin(x) + b*cos(y)";   // right side of z = f(x,y)
    float    paramA = 1.5f;
    float    paramB = 0.8f;
    ViewMode viewMode = ViewMode::Wireframe;
    bool     showGrid = true;
    bool     lighting = false;
    int      resolution = 100;                        // grid points per axis

    std::string equationError;

    //Fractal controls
    FractalType fractal = FractalType::Mandelbrot;
    int      maxIterations = 100;
    float    juliaRe = -0.8f;
    float    juliaIm = 0.156f;

    //One-frame requests
    bool equationChanged    = false;   // Enter, Plot button, or a preset was chosen
    bool paramsChanged      = false;   // a/b slider moved, or resolution released
    bool resetViewRequested = false;   // Reset View button
    int  zoomRequest        = 0;       // -1 zoom out, +1 zoom in
    bool snapshotRequested  = false;   // handled in main.cpp

    //Status line
    std::string statusMessage;
    double      statusExpiresAt = 0.0;

    void clearRequests() {
        equationChanged = false;
        paramsChanged = false;
        resetViewRequested = false;
        zoomRequest = 0;
        snapshotRequested = false;
    }
};
