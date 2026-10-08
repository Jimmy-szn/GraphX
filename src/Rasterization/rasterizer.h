#pragma once
#include <vector>
namespace rasterization{
    struct Color{
        unsigned char r,g,b,a;
    };
    class PixelBuffer{
        public:
            PixelBuffer(int width, int height);
            void clear(Color color);
            int width() const;
            int height() const;
            const unsigned char* data() const;
            void setPixel(int x, int y, Color color);

            
        private:
            int m_width;
            int m_height;
            std::vector<unsigned char> m_pixels;
       
    };
    void drawLineDDA(PixelBuffer& buffer, int x1, int y1, int x2, int y2, Color color);
    void drawLineBresenham(PixelBuffer &buffer, int x1, int y1, int x2, int y2, Color color);
    void drawCircleMidpoint(PixelBuffer &buffer, int centerX, int centerY, int radius, Color color);
    struct Point{
        int x,y;
    };
    void fillPolygonScanline(PixelBuffer& buffer, const std::vector<Point>& vertices, Color color);
}