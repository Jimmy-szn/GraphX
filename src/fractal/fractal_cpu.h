#pragma once
// fractal_cpu.h - pure C++ fractal demos built on the GraphX rasterizer (no OpenGL).
// Place in src/fractal/ next to fractal_cpu.cpp.
#include "../Rasterization/rasterizer.h"

namespace fractal {

using rasterization::Color;
using rasterization::PixelBuffer;

// Which region of the complex plane is visible.
// scale = half the buffer height in complex-plane units (smaller = zoomed in).
struct View {
    double centerX = -0.5;
    double centerY = 0.0;
    double scale   = 1.5;
};

enum class Kind { Mandelbrot, Julia };

// Escape-time algorithm: for every pixel iterate z = z^2 + c and colour the pixel
// by how quickly |z| escapes. Mandelbrot: z0 = 0, c = pixel. Julia: z0 = pixel, c = (juliaRe, juliaIm).
// Pixels that never escape within maxIter are drawn black.
void renderEscapeTime(PixelBuffer& buffer, Kind kind, const View& view,
                      int maxIter, double juliaRe = -0.8, double juliaIm = 0.156);

// Koch snowflake: recursive line subdivision, each segment drawn with Bresenham.
// radius = distance from centre to the triangle's corners (pixels); depth 0 = plain triangle.
void drawKochSnowflake(PixelBuffer& buffer, int centerX, int centerY,
                       int radius, int depth, Color color);

// Sierpinski triangle: recursive subdivision, leaf triangles filled with scanline fill.
void drawSierpinski(PixelBuffer& buffer, int centerX, int centerY,
                    int radius, int depth, Color color);

// View interaction. Pixel coordinates are in the buffer, origin top-left, y down.
// panView: the image follows the mouse drag (dx, dy in buffer pixels).
void panView(View& view, double dxPixels, double dyPixels, int width, int height);
// zoomView: factor < 1 zooms in, > 1 zooms out; the point under (px, py) stays fixed.
void zoomView(View& view, double px, double py, double factor, int width, int height);

} // namespace fractal