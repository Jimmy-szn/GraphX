#include "rasterizer.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <vector>

namespace rasterization{
    PixelBuffer::PixelBuffer(int width, int height):
        m_width(width),
        m_height(height),
        m_pixels()
    {
        if(width <=0 || height <=0){
            throw std::invalid_argument("Width and height must be positive");
        }
        m_pixels.resize(
            static_cast<std::size_t>(width) *
            static_cast<std::size_t>(height) * 4    
        );
    }
    void PixelBuffer::clear(Color color){
        for(std::size_t i=0; i<m_pixels.size(); i+=4){
            m_pixels[i] = color.r;
            m_pixels[i+1] = color.g;
            m_pixels[i+2] = color.b;
            m_pixels[i+3] = color.a;
        }
    }
    int PixelBuffer::width() const {
        return m_width;
    }

    int PixelBuffer::height() const {
        return m_height;
    }
    const unsigned char* PixelBuffer::data() const {
        return m_pixels.data();
    }
    void PixelBuffer::setPixel(int x, int y, Color color){
        if(x < 0 || x >= m_width || y < 0 || y >= m_height){
            return;
        }
        const std::size_t index = (
            static_cast<std::size_t>(y) * m_width + x
        ) * 4;
        m_pixels[index] = color.r;
        m_pixels[index+1] = color.g;
        m_pixels[index+2] = color.b;
        m_pixels[index+3] = color.a;
    }
    void drawLineDDA(PixelBuffer& buffer, int x1,int y1, int x2, int y2, Color color){
        const double dx = static_cast<double>(x2 - x1);
        const double dy = static_cast<double>(y2 - y1);
        const int steps = static_cast<int>(std::ceil(std::max(std::abs(dx), std::abs(dy))));

        if(steps==0){
            buffer.setPixel(x1,y1,color);
            return;
        }
        const double xStep = dx / steps;
        const double yStep = dy / steps;

        double x = static_cast<double>(x1);
        double y = static_cast<double>(y1);

        for (int i = 0; i <= steps; ++i){
            buffer.setPixel(
                static_cast<int>(std::lround(x)),
                static_cast<int>(std::lround(y)),
                color
            );
            x+=xStep;
            y+=yStep;
        }
    }
    void drawLineBresenham(PixelBuffer& buffer, int x1, int y1, int x2, int y2, Color color){
        const long long dxRaw = static_cast<long long>(x2) - static_cast<long long>(x1);
        const long long dyRaw = static_cast< long long> (y2) - static_cast<long long>(y1);
        const long long dx = dxRaw < 0 ? -dxRaw: dxRaw;
        const long long dy = dyRaw < 0 ? -dyRaw: dyRaw;

        const int stepx = x1 < x2 ? 1: -1;
        const int stepy = y1 < y2 ? 1: -1;
        const long long deltaY = - dy;
        long long error = dx + deltaY;
        while (true) {
            buffer.setPixel(x1, y1, color);
            if(x1 == x2 && y1 == y2) {
                break;
            }
            const long long twiceError = 2 * error;
            if (twiceError >= deltaY) {
                error += deltaY;
                x1 += stepx;
            }
            if (twiceError <= dx) {
                error += dx;
                y1 += stepy;
            }
        }
    }
    void drawCircleMidpoint(PixelBuffer& buffer, int centerX, int centerY, int radius, Color color){
        if (radius<0){
            return;
        }
        const auto plotIfVisible =[&](long long x, long long y){
            if (x >=0 && x < buffer.width() && y >=0 && y < buffer.height()){
                buffer.setPixel(static_cast<int>(x), static_cast<int>(y), color);
            }
        };
        const auto plotSymmetricPoints = [&](long long x, long long y){ 
            const long long cx = centerX;
            const long long cy = centerY;
            plotIfVisible(cx + x, cy + y);
            plotIfVisible(cx - x, cy + y);
            plotIfVisible(cx + x, cy - y);
            plotIfVisible(cx - x, cy - y);
            plotIfVisible(cx + y, cy + x);
            plotIfVisible(cx - y, cy + x);
            plotIfVisible(cx + y, cy - x);
            plotIfVisible(cx - y, cy - x);

        };
        long long x = 0;
        long long y = radius;
        long long decision = 1 - radius;

        while (x <=y){
            plotSymmetricPoints(x, y);
            ++x;
            if (decision <= 0) {
                decision += 2 * x + 1;
            }
            else{
                --y;
                decision +=  + 2 * (x-y)+ 1;

            }
        }
    }
    void fillPolygonScanline(PixelBuffer& buffer, const std::vector<Point>& vertices, Color color){
        if (vertices.size() < 3){
            return;
        }
        struct Edge {
            int yMax;
            double x;
            double inverseSlope;
        };
        std::vector<std::vector<Edge>> edgeTable(buffer.height());
        for (std::size_t i =0; i < vertices.size(); ++i){
            Point first = vertices[i];
            Point second = vertices[(i + 1) % vertices.size()];
            if (first.y == second.y){
                continue;
            }
            if (first.y > second.y){
                std::swap(first, second);
                
            }
            const double inverseSlope =
            (static_cast<double>(second.x - first.x) / static_cast<double>(second.y - first.y));
            const int startY = std::max(first.y, 0);
            if (startY >= second.y || startY >= buffer.height()){
                continue;
            }
            const double startX = first.x +(static_cast<double>(startY)- first.y) * inverseSlope;
            edgeTable[startY].push_back(
                {second.y, startX, inverseSlope}
            );
        }
        std::vector<Edge> activeEdges;
        for (int y = 0; y < buffer.height(); ++y){
            for (const Edge& edge : edgeTable[y]){
                activeEdges.push_back(edge);
            }
            activeEdges.erase(
                std::remove_if(
                    activeEdges.begin(),
                    activeEdges.end(),
                    [y](const Edge& edge){
                        return y >= edge.yMax;
                    }
                ),
                activeEdges.end()
            );
            std::sort(
                activeEdges.begin(),
                activeEdges.end(),
                [](const Edge& a, const Edge& b){
                    return a.x < b.x;
                }
            );
            for (std::size_t i=0; i+1 < activeEdges.size(); i+=2){
                const double left = std::ceil(activeEdges[i].x);
                const double right = std::ceil(activeEdges[i+1].x);
                const int startX = static_cast<int>(std::max(0.0, left));
                const int endX = static_cast<int>(std::min(static_cast<double>(buffer.width()), right));
                for (int x = startX; x < endX; ++x ) {
                    buffer.setPixel(x, y, color);
                }
            }
            for (Edge& edge : activeEdges) {
                edge.x += edge.inverseSlope;
            }
        }
    }
}