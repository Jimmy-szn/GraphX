#include "fractal_cpu.h"
#include <cmath>
#include <vector>

namespace fractal {

namespace {

struct P { double x, y; };
struct V {double centerX, centerY, scale;};
View currentView = { 0.0, 0.0, 1.0 };
View targetView = { 0.0, 0.0, 1.0 };

// Smooth cosine palette: t is a continuous iteration count.
Color palette(double t) {
    auto channel = [t](double phase) {
        double v = 0.5 + 0.5 * std::cos(6.28318530718 * (t * 0.02 + phase));
        return static_cast<unsigned char>(255.0 * v);
    };
    return Color{ channel(0.0), channel(0.33), channel(0.67), 255 };
}

int toInt(double v) { return static_cast<int>(std::lround(v)); }

// One Koch step: replace segment A->B by four segments, recursing until depth 0.
// The bump is rotated -60 degrees, which points outward for the vertex order used below
// (screen coordinates, y down).
void kochSegment(PixelBuffer& buf, P a, P b, int depth, Color color) {
    if (depth == 0) {
        rasterization::drawLineBresenham(buf, toInt(a.x), toInt(a.y),
                                              toInt(b.x), toInt(b.y), color);
        return;
    }
    const double dx = (b.x - a.x) / 3.0;
    const double dy = (b.y - a.y) / 3.0;
    const P p1 { a.x + dx, a.y + dy };
    const P p2 { a.x + 2.0 * dx, a.y + 2.0 * dy };
    // rotate (dx, dy) by -60 degrees: cos = 0.5, sin = -sqrt(3)/2
    const double s = 0.86602540378;
    const P peak { p1.x + (0.5 * dx + s * dy), p1.y + (-s * dx + 0.5 * dy) };

    kochSegment(buf, a,  p1,   depth - 1, color);
    kochSegment(buf, p1, peak, depth - 1, color);
    kochSegment(buf, peak, p2, depth - 1, color);
    kochSegment(buf, p2, b,    depth - 1, color);
}

void sierpinskiRec(PixelBuffer& buf, P a, P b, P c, int depth, Color color) {
    if (depth == 0) {
        std::vector<rasterization::Point> tri = {
            { toInt(a.x), toInt(a.y) },
            { toInt(b.x), toInt(b.y) },
            { toInt(c.x), toInt(c.y) }
        };
        rasterization::fillPolygonScanline(buf, tri, color);
        return;
    }
    const P ab { (a.x + b.x) / 2.0, (a.y + b.y) / 2.0 };
    const P bc { (b.x + c.x) / 2.0, (b.y + c.y) / 2.0 };
    const P ca { (c.x + a.x) / 2.0, (c.y + a.y) / 2.0 };
    sierpinskiRec(buf, a,  ab, ca, depth - 1, color);
    sierpinskiRec(buf, ab, b,  bc, depth - 1, color);
    sierpinskiRec(buf, ca, bc, c,  depth - 1, color);
}

} // namespace

void renderEscapeTime(PixelBuffer& buffer, Kind kind, const View& view,
                      int maxIter, double juliaRe, double juliaIm) {
    const int w = buffer.width();
    const int h = buffer.height();
    if (w <= 0 || h <= 0 || maxIter <= 0) return;

    const double aspect = static_cast<double>(w) / h;
    const double bailout = 65536.0;   // large radius^2 gives smooth colouring

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            // Pixel -> complex plane (buffer row 0 is assumed to be the top row)
            const double nx = ((x + 0.5) / w * 2.0 - 1.0) * aspect;
            const double ny = 1.0 - (y + 0.5) / h * 2.0;
            const double px = view.centerX + nx * view.scale;
            const double py = view.centerY + ny * view.scale;

            double zr, zi, cr, ci;
            if (kind == Kind::Julia) { zr = px; zi = py; cr = juliaRe; ci = juliaIm; }
            else                     { zr = 0.0; zi = 0.0; cr = px; ci = py; }

            double zr2 = zr * zr, zi2 = zi * zi;
            int i = 0;
            while (i < maxIter && zr2 + zi2 <= bailout) {
                zi = 2.0 * zr * zi + ci;      // z^2 + c, imaginary part
                zr = zr2 - zi2 + cr;          // z^2 + c, real part
                zr2 = zr * zr;
                zi2 = zi * zi;
                ++i;
            }

            if (i >= maxIter) {
                buffer.setPixel(x, y, Color{ 0, 0, 0, 255 });
            } else {
                const double mu = i - std::log2(std::log2(zr2 + zi2)) + 4.0;
                buffer.setPixel(x, y, palette(mu));
            }
        }
    }
}

void drawKochSnowflake(PixelBuffer& buffer, int centerX, int centerY,
                       int radius, int depth, Color color) {
    const double r = radius;
    const double pi = 3.14159265358979323846;
    // Corners at -90, 30, 150 degrees (one points straight up on screen, y down)
    P v[3];
    for (int k = 0; k < 3; ++k) {
        const double ang = (-90.0 + 120.0 * k) * pi / 180.0;
        v[k] = { centerX + r * std::cos(ang), centerY + r * std::sin(ang) };
    }
    for (int k = 0; k < 3; ++k)
        kochSegment(buffer, v[k], v[(k + 1) % 3], depth, color);
}

void drawSierpinski(PixelBuffer& buffer, int centerX, int centerY,
                    int radius, int depth, Color color) {
    const double r = radius;
    const double pi = 3.14159265358979323846;
    P v[3];
    for (int k = 0; k < 3; ++k) {
        const double ang = (-90.0 + 120.0 * k) * pi / 180.0;
        v[k] = { centerX + r * std::cos(ang), centerY + r * std::sin(ang) };
    }
    sierpinskiRec(buffer, v[0], v[1], v[2], depth, color);
}
void onScroll(double px, double py, double factor, int width, int height) {
    const double aspect = static_cast<double>(width) / height;
    const double nx = (px / width * 2.0 - 1.0) * aspect;
    const double ny = 1.0 - py / height * 2.0;
    const double fx = currentView.centerX + nx * currentView.scale;
    const double fy = currentView.centerY + ny * currentView.scale;
    currentView.scale *= factor;
    currentView.centerX = fx - nx * currentView.scale;
    currentView.centerY = fy - ny * currentView.scale;
}
void updateAnimation(double deltaTime) {
    const double t = 1.0 - std::exp(deltaTime * -5.0); // smooth interpolation to control how snapy the easing feels
    currentView.centerX += (targetView.centerX - currentView.centerX) * t;
    currentView.centerY += (targetView.centerY - currentView.centerY) * t;
    currentView.scale += (targetView.scale - currentView.scale) * t;
}
void panView(View& view, double dxPixels, double dyPixels, int /*width*/, int height) {
    const double unitsPerPixel = 2.0 * view.scale / height;
    view.centerX -= dxPixels * unitsPerPixel;   // image follows the cursor
    view.centerY += dyPixels * unitsPerPixel;   // screen y is down, complex y is up
}

void zoomView(View& view, double px, double py, double factor, int width, int height) {
    const double aspect = static_cast<double>(width) / height;
    const double nx = (px / width * 2.0 - 1.0) * aspect;
    const double ny = 1.0 - py / height * 2.0;
    // complex point currently under the cursor
    const double fx = view.centerX + nx * view.scale;
    const double fy = view.centerY + ny * view.scale;
    view.scale *= factor;
    // move the centre so that same point stays under the cursor
    view.centerX = fx - nx * view.scale;
    view.centerY = fy - ny * view.scale;
}

} // namespace fractal