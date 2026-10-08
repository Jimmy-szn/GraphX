#include "UI/ui_panel.h"
#include <cstddef>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"
#include <cstdio>
#include <glad/glad.h>
#include <GLFW/glfw3.h>


#include "Rasterization/rasterizer.h"
#include "app_state.h"
#include "ui/ui_panel.h"
#include <algorithm>
#include <cmath>
#include <imgui.h>
#include <iostream>
#include <vector>
void renderGraph2D(rasterization::PixelBuffer &canvas, const AppState &state) {
  constexpr int pixelsPerUnit = 40;
  const float xMin = -canvas.width() / (2.0f * pixelsPerUnit);
  const float xMax = canvas.width() / (2.0f * pixelsPerUnit);
  const float yMin = -canvas.height() / (2.0f * pixelsPerUnit);
  const float yMax = canvas.height() / (2.0f * pixelsPerUnit);

  const auto toPixelX = [&](float x) {
    return static_cast<int>(
        std::lround((x - xMin) / (xMax - xMin) * (canvas.width() - 1)));
  };
  const auto toPixelY = [&](float y) {
    return static_cast<int>(
        std::lround((yMax - y) / (yMax - yMin) * (canvas.height() - 1)));
  };
  const auto line = [&](int x1, int y1, int x2, int y2,
                        rasterization::Color color) {
    if (state.rasterLineAlgorithm == RasterLineAlgorithm::DDA) {
      rasterization::drawLineDDA(canvas, x1, y1, x2, y2, color);
    } else {
      rasterization::drawLineBresenham(canvas, x1, y1, x2, y2, color);
    }
  };
  canvas.clear({18, 20, 24, 255});
  const auto evaluateY = [&](float x) {
    switch (state.graph2DPlot) {
    case Graph2DPlot::Line:
        return state.lineSlope * x + state.lineIntercept;
    case Graph2DPlot::Parabola:
        return x * x;
    case Graph2DPlot::Sine:
        return std::sin(x);
    case Graph2DPlot::Cosine:
        return std::cos(x);
    case Graph2DPlot::Circle:
        return 0.0f; // Circle is drawn separately with the midpoint algorithm.
    }
    return 0.0f;
};
std::vector<rasterization::Point> curvePoints;
curvePoints.reserve(canvas.width());
for (int pixelX = 0; pixelX < canvas.width(); ++pixelX) {
    float x = xMin + (xMax - xMin) * pixelX / (canvas.width() - 1);
    float y = evaluateY(x);
    curvePoints.push_back({toPixelX(x), toPixelY(y)});
}
  if (state.graph2DPlot!=Graph2DPlot::Circle && state.fillUnderCurve) {
    std::vector<rasterization::Point> fillPoints;
    fillPoints.reserve(curvePoints.size()+2);
    fillPoints.push_back({0, toPixelY(0.0f)});
    fillPoints.insert(fillPoints.end(), curvePoints.begin(), curvePoints.end());
    fillPoints.push_back({toPixelX(xMax), toPixelY(0.0f)});
    rasterization::fillPolygonScanline(canvas, fillPoints, {45, 100, 90, 255});
  }
  const rasterization::Color grid{48, 52, 58, 255};
  const rasterization::Color axis{220, 220, 220, 255};
  if (state.showGrid) {
    for (int x = 0; x < canvas.width(); x += pixelsPerUnit) {
      rasterization::drawLineBresenham(canvas, x, 0, x, canvas.height() - 1,
                                       grid);
      
    }
    for (int y = 0; y < canvas.height(); y += pixelsPerUnit) {
      rasterization::drawLineBresenham(canvas, 0, y, canvas.width() - 1, y,
                                       grid);
    }
  }
  rasterization::drawLineBresenham(canvas, toPixelX(0.0f), 0.0f, toPixelX(0.0f),
                                   canvas.height() - 1, axis);
  rasterization::drawLineBresenham(canvas, 0.0f, toPixelY(0.0f),
                                   canvas.width() - 1, toPixelY(0.0f), axis);
  if (state.graph2DPlot == Graph2DPlot::Circle) {
    rasterization::drawCircleMidpoint(canvas, toPixelX(0), toPixelY(0), state.circleRadius*pixelsPerUnit,{80, 190, 255, 255});
  }
  else{
    for(std::size_t i = 1; i < curvePoints.size(); ++i) {
        line(curvePoints[i-1].x, curvePoints[i-1].y, curvePoints[i].x, curvePoints[i].y, {90, 220, 130, 255});
    }
  }
}

int main() {
  if (!glfwInit()) {
    std::cerr << "Failed to initialize GLFW" << std::endl;
    return -1;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

  GLFWwindow *window =
      glfwCreateWindow(1280, 720, "GraphX - Interactive Math Equation Plotter",
                       nullptr, nullptr);
  if (!window) {
    std::cerr << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
    std::cerr << "Failed to initialize GLAD" << std::endl;
    return -1;
  }
  constexpr int canvasWidth = 640;
  constexpr int canvasHeight = 480;
  rasterization::PixelBuffer canvas(canvasWidth, canvasHeight);
  canvas.clear({18, 20, 24, 255});
  
  GLuint rasterizerTexture = 0;
  glGenTextures(1, &rasterizerTexture);
  glBindTexture(GL_TEXTURE_2D, rasterizerTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, canvasWidth, canvasHeight, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, canvas.data());
  // IMPORTANT: install camera / input callbacks (Naomi) BEFORE this line.
  // ui::init() keeps them and calls them after ImGui. If they are installed
  // later, they replace ImGui's callbacks and the panel stops responding.
  ui::init(window);

  AppState state;

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    ui::beginFrame();
    ui::drawPanel(state);
    ui::drawViewportOverlay(state);
    if (state.mode == AppMode::FunctionPlotter && state.plotView==PlotView::Graph2D) {
        renderGraph2D(canvas, state);   
        glBindTexture(GL_TEXTURE_2D, rasterizerTexture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, canvasWidth, canvasHeight, GL_RGBA,
                        GL_UNSIGNED_BYTE, canvas.data());
        const ImVec2 display = ImGui::GetIO().DisplaySize;
        const float availableWidth = display.x -ui::PANEL_WIDTH;
        const float scale = std::min(availableWidth / canvasWidth, display.y / canvasHeight);
        const ImVec2 size(canvasWidth * scale, canvasHeight * scale);
        const ImVec2 topleft(ui::PANEL_WIDTH +(availableWidth - size.x) * 0.5f, (display.y - size.y) * 0.5f);
        ImGui::GetBackgroundDrawList()->AddImage(
            static_cast<ImTextureID>(rasterizerTexture),
            topleft,
            ImVec2(topleft.x + size.x, topleft.y + size.y),
            ImVec2(0.0f, 1.0f),
            ImVec2(1.0f, 0.0f)
        );
        auto* drawList = ImGui::GetBackgroundDrawList();
        const ImU32 labelColor = IM_COL32(190, 195, 205, 255);
        const float axisScreenX = topleft.x + (canvasWidth / 2.0f)* size.x / canvasWidth;
        const float axisScreenY = topleft.y + (canvasHeight / 2.0f) * size.y / canvasHeight;
        for (int value = -7; value <= 7; ++value) {
            if(value ==0) continue;
            const float pixelX = canvasWidth / 2.0f + value * 40.0f;
            const float screenX = topleft.x + pixelX * size.x / canvasWidth;
            char label[8];
            std::snprintf(label, sizeof(label), "%d", value);
            const ImVec2 textSize = ImGui::CalcTextSize(label);
            drawList->AddText(ImVec2(screenX - textSize.x * 0.5f, axisScreenY + 2.0f), labelColor, label);
        }
        for (int value = -5; value <= 5; ++value) {
            if(value ==0) continue;
            const float pixelY = canvasHeight / 2.0f + value * 40.0f;
            const float screenY = topleft.y + pixelY * size.y / canvasHeight;
            char label[8];
            std::snprintf(label, sizeof(label), "%d", value);
            const ImVec2 textSize = ImGui::CalcTextSize(label);
            drawList->AddText(ImVec2(axisScreenX - textSize.x - 5.0f, screenY - textSize.y * 0.5f), labelColor, label);
        }
        drawList->AddText(ImVec2(axisScreenX + 4.0f, axisScreenY + 4.0f), labelColor, "(0,0)");
    }
    // Full window clear (this is the panel background)
    int fbW = 0, fbH = 0;
    glfwGetFramebufferSize(window, &fbW, &fbH);
    glViewport(0, 0, fbW, fbH);
    glClearColor(0.12f, 0.12f, 0.14f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Scene area. Skipped when the window is minimized.
    SceneViewport vp = ui::computeSceneViewport(window);
    if (vp.valid()) {
      glViewport(vp.x, vp.y, vp.width, vp.height);
      glEnable(GL_SCISSOR_TEST);
      glScissor(vp.x, vp.y, vp.width, vp.height);

      if (state.mode == AppMode::FunctionPlotter) {
        glClearColor(0.05f, 0.09f, 0.17f, 1.0f); // placeholder
      } else {
        glClearColor(0.02f, 0.12f, 0.06f, 1.0f); // placeholder
      } 
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      // TODO (Gerald): switch on state.mode here
      // TODO (Daniel): draw the 3D surface using `vp.aspect()` and the plotter
      // fields
      // TODO (Tracy):  draw the fractal canvas using the fractal fields
      // TODO (Naomi):  use state.zoomRequest / state.resetViewRequested

      glDisable(GL_SCISSOR_TEST);
    }

    glViewport(0, 0, fbW, fbH);
    ui::endFrame();
    if(state.snapshotRequested) {
        std::vector<unsigned char> pixels(
            static_cast<std::size_t>(fbW) * fbH * 4);
        glReadBuffer(GL_BACK);
        glReadPixels(0, 0, fbW, fbH, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());        
        const int saved=stbi_write_png("src/snapshots/graphx_screenshot.png", fbW, fbH, 4, pixels.data(), fbW * 4);
       ui::setStatus(state, saved ? "Screenshot saved: snapshots/graphx_screenshot.png." : "Failed to save screenshot.");
    }

    glfwSwapBuffers(window);
    state.clearRequests();
  }

  ui::shutdown();
  glDeleteTextures(1, &rasterizerTexture);
  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}
