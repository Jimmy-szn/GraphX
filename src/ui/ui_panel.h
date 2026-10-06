#pragma once
#include <string>
#include "app_state.h"

struct GLFWwindow;

namespace ui {

// Width of the left panel
constexpr float PANEL_WIDTH = 300.0f;

// Smallest allowed window size
constexpr int MIN_WINDOW_W = 800;
constexpr int MIN_WINDOW_H = 500;

// Call AFTER any other GLFW callbacks are installed
void init(GLFWwindow* window);
void shutdown();

void beginFrame();
void endFrame();

// Scene area
SceneViewport computeSceneViewport(GLFWwindow* window);

void drawPanel(AppState& state);            // left sidebar
void drawViewportOverlay(AppState& state);  // zoom, reset, snapshot buttons

// Show a message at the panel
void setStatus(AppState& state, const std::string& msg, double seconds = 4.0);

bool wantsMouse();
bool wantsKeyboard();

}
