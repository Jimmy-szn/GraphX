#pragma once
// surface.h - 3D surface plotter for z = f(x, y) (Daniel).
// Parses the equation from the panel, builds a grid mesh and draws it with an orbit camera.
#include "app_state.h"

namespace surface {

// Call once after GLAD is loaded / before the GL context is destroyed.
void init();
void shutdown();

// Call inside the scene viewport (glViewport/scissor already set to vp).
// Re-parses / rebuilds the mesh when the panel asks, handles mouse input and draws.
void render(AppState& state, const SceneViewport& vp, double deltaTime);

} // namespace surface
