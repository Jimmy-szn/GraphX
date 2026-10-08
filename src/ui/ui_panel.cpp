#include "ui/ui_panel.h"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <cctype>
#include <cstring>

namespace ui {

namespace {

// Presets
struct Preset {
  const char *name;
  const char *equation;
};

const Preset kPresets[] = {
    {"Sine / Cosine", "sin(x) * cos(y)"},
    {"Ripple", "sin(sqrt(x^2 + y^2))"},
    {"Saddle", "x^2 - y^2"},
    {"Gaussian Hill", "exp(-(x^2 + y^2))"},
    {"Wave (a, b)", "a*sin(x) + b*cos(y)"},
};
const int kPresetCount =
    static_cast<int>(sizeof(kPresets) / sizeof(kPresets[0]));

// Add a name here and in FractalType
const char *kFractalNames[] = {"Mandelbrot", "Julia"};
const int kFractalCount =
    static_cast<int>(sizeof(kFractalNames) / sizeof(kFractalNames[0]));

int g_presetIdx = -1; // custom equation

// Helpers

// True if the equation contains the single-letter name `id` as a whole word.
bool usesIdentifier(const char *eq, char id) {
  for (const char *p = eq; *p; ++p) {
    if (*p != id)
      continue;
    bool leftOk =
        (p == eq) ||
        (!std::isalnum(static_cast<unsigned char>(p[-1])) && p[-1] != '_');
    bool rightOk =
        !std::isalnum(static_cast<unsigned char>(p[1])) && p[1] != '_';
    if (leftOk && rightOk)
      return true;
  }
  return false;
}

void applyTheme() {
  ImGui::StyleColorsDark();
  ImGuiStyle &st = ImGui::GetStyle();
  st.WindowRounding = 0.0f;
  st.FrameRounding = 6.0f;
  st.GrabRounding = 6.0f;
  st.WindowPadding = ImVec2(14, 14);
  st.FramePadding = ImVec2(8, 5);
  st.ItemSpacing = ImVec2(8, 10);

  ImVec4 *c = st.Colors;
  c[ImGuiCol_WindowBg] = ImVec4(0.14f, 0.14f, 0.16f, 1.00f);
  c[ImGuiCol_FrameBg] = ImVec4(0.22f, 0.22f, 0.25f, 1.00f);
  c[ImGuiCol_SliderGrab] = ImVec4(0.45f, 0.62f, 0.85f, 1.00f);
  c[ImGuiCol_SliderGrabActive] = ImVec4(0.55f, 0.72f, 0.95f, 1.00f);
  c[ImGuiCol_CheckMark] = ImVec4(0.55f, 0.72f, 0.95f, 1.00f);
  c[ImGuiCol_Button] = ImVec4(0.26f, 0.38f, 0.55f, 1.00f);
  c[ImGuiCol_ButtonHovered] = ImVec4(0.33f, 0.47f, 0.67f, 1.00f);
  c[ImGuiCol_ButtonActive] = ImVec4(0.20f, 0.30f, 0.45f, 1.00f);
}

// Function plotter section
void drawPlotterControls(AppState &s) {
  ImGui::SeparatorText("Plot View");
  int plotView = static_cast<int>(s.plotView);
  ImGui::RadioButton("2D Plot", &plotView, 0);
  ;
  ImGui::SameLine();
  ImGui::RadioButton("3D Surface", &plotView, 1);
  s.plotView = static_cast<PlotView>(plotView);
  if (s.plotView == PlotView::Graph2D) {
    ImGui::SeparatorText("2D Plot");
    int plotType = static_cast<int>(s.graph2DPlot);
    ImGui::Combo("Plot", &plotType,
                 "Line y=mx+c\0Parabola y=x^2\0Sine y=sin(x)\0Cosine "
                 "y=cos(x)\0Circle\0");
    s.graph2DPlot = static_cast<Graph2DPlot>(plotType);
    if (s.graph2DPlot == Graph2DPlot::Circle) {
      ImGui::SliderInt("Radius", &s.circleRadius, 1, 5);
    } else {
      int algorithm = static_cast<int>(s.rasterLineAlgorithm);
      ImGui::RadioButton("DDA", &algorithm, 0);
      ImGui::SameLine();
      ImGui::RadioButton("Bresenham", &algorithm, 1);
      s.rasterLineAlgorithm = static_cast<RasterLineAlgorithm>(algorithm);
      if (s.graph2DPlot == Graph2DPlot::Line) {
        ImGui::SliderFloat("Slope (m)", &s.lineSlope, -2.0f, 2.0f);
        ImGui::SliderFloat("Intercept (b)", &s.lineIntercept, -5.0f, 5.0f);
      }
    }
    ImGui::Checkbox("Grid", &s.showGrid);
    ImGui::Checkbox("Fill Under Curve", &s.fillUnderCurve);
    return;
  }
  if (g_presetIdx >= 0 &&
      std::strcmp(s.equation, kPresets[g_presetIdx].equation) != 0) {
    g_presetIdx = -1;
  }

  ImGui::SeparatorText("Preset");
  const char *preview =
      g_presetIdx >= 0 ? kPresets[g_presetIdx].name : "Custom";
  ImGui::SetNextItemWidth(-1);
  if (ImGui::BeginCombo("##preset", preview)) {
    for (int i = 0; i < kPresetCount; ++i) {
      if (ImGui::Selectable(kPresets[i].name, g_presetIdx == i)) {
        g_presetIdx = i;
        std::strncpy(s.equation, kPresets[i].equation, sizeof(s.equation) - 1);
        s.equation[sizeof(s.equation) - 1] = '\0';
        s.equationChanged = true;
      }
    }
    ImGui::EndCombo();
  }

  ImGui::SeparatorText("Equation");
  ImGui::AlignTextToFramePadding();
  ImGui::TextUnformatted("z =");
  ImGui::SameLine();
  ImGui::SetNextItemWidth(-1);
  if (ImGui::InputTextWithHint("##equation", "e.g. sin(x) * cos(y)", s.equation,
                               sizeof(s.equation),
                               ImGuiInputTextFlags_EnterReturnsTrue)) {
    s.equationChanged = true;
  }
  if (ImGui::Button("Plot", ImVec2(-1, 0))) {
    s.equationChanged = true;
  }
  if (!s.equationError.empty()) {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.45f, 0.45f, 1.0f));
    ImGui::TextWrapped("%s", s.equationError.c_str());
    ImGui::PopStyleColor();
  }

  // Sliders appear only for parameters the equation uses
  ImGui::SeparatorText("Parameters");
  bool usesA = usesIdentifier(s.equation, 'a');
  bool usesB = usesIdentifier(s.equation, 'b');
  if (usesA && ImGui::SliderFloat("a", &s.paramA, -5.0f, 5.0f))
    s.paramsChanged = true;
  if (usesB && ImGui::SliderFloat("b", &s.paramB, -5.0f, 5.0f))
    s.paramsChanged = true;
  if (!usesA && !usesB) {
    ImGui::TextDisabled("Use a or b in the equation to get sliders.");
  }

  // Resolution rebuilds mesh, so apply only when slider is released.
  ImGui::SetNextItemWidth(-1);
  ImGui::SliderInt("##res", &s.resolution, 20, 400, "Resolution: %d");
  if (ImGui::IsItemDeactivatedAfterEdit())
    s.paramsChanged = true;

  ImGui::SeparatorText("View Mode");
  int vm = static_cast<int>(s.viewMode);
  ImGui::RadioButton("Wireframe", &vm, 0);
  ImGui::SameLine();
  ImGui::RadioButton("Surface", &vm, 1);
  s.viewMode = static_cast<ViewMode>(vm);

  ImGui::SeparatorText("Scene Layout");
  ImGui::Checkbox("Grid", &s.showGrid);
  ImGui::Checkbox("Lighting", &s.lighting);
}

// Fractal section
void drawFractalControls(AppState &s) {
  ImGui::SeparatorText("Fractal");
  int f = static_cast<int>(s.fractal);
  ImGui::SetNextItemWidth(-1);
  ImGui::Combo("##fractal", &f, kFractalNames, kFractalCount);
  s.fractal = static_cast<FractalType>(f);

  ImGui::SeparatorText("Iterations");
  ImGui::SetNextItemWidth(-1);
  ImGui::SliderInt("##iter", &s.maxIterations, 10, 1000, "Max iterations: %d");

  if (s.fractal == FractalType::Julia) {
    ImGui::SeparatorText("Julia Constant");
    ImGui::SliderFloat("Re(c)", &s.juliaRe, -2.0f, 2.0f);
    ImGui::SliderFloat("Im(c)", &s.juliaIm, -2.0f, 2.0f);
  }
}

} // namespace

// Public API

void init(GLFWwindow *window) {
  glfwSetWindowSizeLimits(window, MIN_WINDOW_W, MIN_WINDOW_H, GLFW_DONT_CARE,
                          GLFW_DONT_CARE);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::GetIO().IniFilename = nullptr;
  applyTheme();
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 330");
}

void shutdown() {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}

void beginFrame() {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
}

void endFrame() {
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

SceneViewport computeSceneViewport(GLFWwindow *window) {
  int fbW = 0, fbH = 0, winW = 0, winH = 0;
  glfwGetFramebufferSize(window, &fbW, &fbH);
  glfwGetWindowSize(window, &winW, &winH);

  SceneViewport vp; // all zeros = invalid
  if (fbW <= 0 || fbH <= 0 || winW <= 0 || winH <= 0)
    return vp; // minimized

  float scale =
      static_cast<float>(fbW) / static_cast<float>(winW); // HiDPI factor
  vp.x = static_cast<int>(PANEL_WIDTH * scale + 0.5f);
  vp.y = 0;
  vp.width = fbW - vp.x;
  vp.height = fbH;
  if (vp.width < 0)
    vp.width = 0;
  return vp;
}

void setStatus(AppState &s, const std::string &msg, double seconds) {
  s.statusMessage = msg;
  s.statusExpiresAt = ImGui::GetTime() + seconds;
}

void drawPanel(AppState &s) {
  ImGuiIO &io = ImGui::GetIO();
  ImGui::SetNextWindowPos(ImVec2(0, 0));
  ImGui::SetNextWindowSize(ImVec2(PANEL_WIDTH, io.DisplaySize.y));

  ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                           ImGuiWindowFlags_NoCollapse |
                           ImGuiWindowFlags_NoSavedSettings;
  ImGui::Begin("Control Panel", nullptr, flags);

  ImGui::SeparatorText("Mode");
  int mode = static_cast<int>(s.mode);
  ImGui::RadioButton("Function Plotter", &mode, 0);
  ImGui::RadioButton("Fractal Explorer", &mode, 1);
  s.mode = static_cast<AppMode>(mode);

  if (s.mode == AppMode::FunctionPlotter)
    drawPlotterControls(s);
  else
    drawFractalControls(s);

  ImGui::Spacing();
  ImGui::Separator();
  if (!s.statusMessage.empty() && ImGui::GetTime() < s.statusExpiresAt) {
    ImGui::TextWrapped("%s", s.statusMessage.c_str());
  }
  ImGui::TextDisabled("%.0f FPS", io.Framerate);

  ImGui::End();
}

void drawViewportOverlay(AppState &s) {
  ImGuiIO &io = ImGui::GetIO();
  ImGui::SetNextWindowPos(ImVec2(PANEL_WIDTH + 12.0f, io.DisplaySize.y - 12.0f),
                          ImGuiCond_Always, ImVec2(0.0f, 1.0f));
  ImGui::SetNextWindowBgAlpha(0.65f);

  ImGuiWindowFlags flags =
      ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
      ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
      ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;
  ImGui::Begin("##overlay", nullptr, flags);

  if (ImGui::Button(" - "))
    s.zoomRequest = -1;
  ImGui::SameLine();
  if (ImGui::Button(" + "))
    s.zoomRequest = +1;
  ImGui::SameLine();
  if (ImGui::Button("Reset View"))
    s.resetViewRequested = true;
  ImGui::SameLine();
  if (ImGui::Button("Save Snapshot"))
    s.snapshotRequested = true;

  ImGui::End();
}

bool wantsMouse() { return ImGui::GetIO().WantCaptureMouse; }
bool wantsKeyboard() { return ImGui::GetIO().WantCaptureKeyboard; }

} // namespace ui
