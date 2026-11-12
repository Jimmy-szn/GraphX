#include "surface/surface.h"
#include "ui/ui_panel.h"
#include "math_expression.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <vector>

namespace surface {

namespace {


constexpr float kRange = 5.0f; // x and y are sampled over [-kRange, kRange]

// ---------------------------------------------------------------------------
// GPU resources and shader
// Each vertex: position (3), normal (3), height 0..1 for colouring (1)
// ---------------------------------------------------------------------------

const char *kVertexShader = R"(#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in float aHeight;
uniform mat4 uMVP;
out vec3 vNormal;
out float vHeight;
void main() {
    vNormal = aNormal;
    vHeight = aHeight;
    gl_Position = uMVP * vec4(aPos, 1.0);
}
)";

const char *kFragmentShader = R"(#version 330 core
in vec3 vNormal;
in float vHeight;
uniform bool uSolid;      // true: draw in uColor (grid / axes)
uniform vec3 uColor;
uniform bool uLighting;
uniform vec3 uLightDir;
out vec4 FragColor;

// Height colour ramp: blue -> cyan -> green -> yellow -> red
vec3 heightColor(float t) {
    return clamp(vec3(1.5 - abs(4.0 * t - 3.0),
                      1.5 - abs(4.0 * t - 2.0),
                      1.5 - abs(4.0 * t - 1.0)), 0.0, 1.0);
}

void main() {
    vec3 color = uSolid ? uColor : heightColor(vHeight);
    if (uLighting && !uSolid) {
        // abs() lights both sides, so the underside is not black
        float diffuse = abs(dot(normalize(vNormal), uLightDir));
        color *= 0.3 + 0.7 * diffuse;
    }
    FragColor = vec4(color, 1.0);
}
)";

GLuint g_program = 0;
GLuint g_meshVao = 0, g_meshVbo = 0, g_meshEbo = 0;
GLuint g_gridVao = 0, g_gridVbo = 0;
GLsizei g_triangleIndexCount = 0; // triangle indices come first in g_meshEbo,
GLsizei g_lineIndexCount = 0;     // then the wireframe line indices
constexpr int kGridVertexCount = 44; // 11 lines each way on the z = 0 plane

math_expression::NodePtr g_expr;
bool g_needsParse = true;
bool g_needsRebuild = true;

// Orbit camera: eye circles the origin, z is up
struct Orbit {
  float yaw;      // radians around the z axis
  float pitch;    // radians above the xy plane
  float distance; // from the origin
};
const Orbit kDefaultOrbit{-2.35f, 0.5f, 22.0f};
Orbit g_cam = kDefaultOrbit;
Orbit g_targetCam = kDefaultOrbit;
bool g_dragging = false;

GLuint compileShader(GLenum type, const char *source) {
  GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, &source, nullptr);
  glCompileShader(shader);
  GLint ok = 0;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
  if (!ok) {
    char log[1024];
    glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
    std::cerr << "Surface shader error: " << log << std::endl;
  }
  return shader;
}

// Attribute layout shared by the mesh and the grid buffers
void setVertexLayout() {
  const GLsizei stride = 7 * sizeof(float);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void *)0);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void *)(3 * sizeof(float)));
  glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, stride, (void *)(6 * sizeof(float)));
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glEnableVertexAttribArray(2);
}

void pushVertex(std::vector<float> &out, float x, float y, float z) {
  out.insert(out.end(), {x, y, z, 0.0f, 0.0f, 1.0f, 0.0f});
}

// Grid lines on z = 0, then the x, y and z axes (2 vertices each)
void buildGridAndAxes() {
  std::vector<float> v;
  for (int i = -5; i <= 5; ++i) {
    pushVertex(v, float(i), -kRange, 0.0f);
    pushVertex(v, float(i), kRange, 0.0f);
    pushVertex(v, -kRange, float(i), 0.0f);
    pushVertex(v, kRange, float(i), 0.0f);
  }
  const float axisLength = kRange + 1.5f;
  pushVertex(v, -axisLength, 0, 0); pushVertex(v, axisLength, 0, 0);
  pushVertex(v, 0, -axisLength, 0); pushVertex(v, 0, axisLength, 0);
  pushVertex(v, 0, 0, -axisLength); pushVertex(v, 0, 0, axisLength);

  glBindVertexArray(g_gridVao);
  glBindBuffer(GL_ARRAY_BUFFER, g_gridVbo);
  glBufferData(GL_ARRAY_BUFFER, v.size() * sizeof(float), v.data(), GL_STATIC_DRAW);
  setVertexLayout();
  glBindVertexArray(0);
}

// Samples z = f(x, y) on a resolution x resolution grid and uploads the mesh
void rebuildMesh(const AppState &s) {
  const int n = std::max(2, s.resolution);
  const float step = 2.0f * kRange / (n - 1);

  // 1. Sample heights; NaN marks points where f is undefined (e.g. log of a negative)
  std::vector<float> z(n * n);
  float zMin = std::numeric_limits<float>::max();
  float zMax = std::numeric_limits<float>::lowest();
  for (int j = 0; j < n; ++j) {
    for (int i = 0; i < n; ++i) {
      const double value = math_expression::evaluate(g_expr.get(), -kRange + i * step, -kRange + j * step,
                                    s.paramA, s.paramB);
      if (std::isfinite(value)) {
        z[j * n + i] = float(value);
        zMin = std::min(zMin, float(value));
        zMax = std::max(zMax, float(value));
      } else {
        z[j * n + i] = std::numeric_limits<float>::quiet_NaN();
      }
    }
  }
  if (zMin > zMax) zMin = zMax = 0.0f; // nothing was defined

  // Squash very tall surfaces so they fit in the view (colours still use real heights)
  const float zRange = zMax - zMin;
  const float zScale = zRange > 2.0f * kRange ? 2.0f * kRange / zRange : 1.0f;
  const auto valid = [&](int i, int j) { return !std::isnan(z[j * n + i]); };
  const auto height = [&](int i, int j) { return z[j * n + i] * zScale; };

  // 2. Vertices with normals from central differences
  std::vector<float> vertices;
  vertices.reserve(n * n * 7);
  for (int j = 0; j < n; ++j) {
    for (int i = 0; i < n; ++i) {
      const float x = -kRange + i * step;
      const float y = -kRange + j * step;
      if (!valid(i, j)) {
        vertices.insert(vertices.end(), {x, y, 0, 0, 0, 1, 0});
        continue;
      }
      const int il = std::max(i - 1, 0), ir = std::min(i + 1, n - 1);
      const int jd = std::max(j - 1, 0), ju = std::min(j + 1, n - 1);
      const float dzdx = (height(ir, j) - height(il, j)) / ((ir - il) * step);
      const float dzdy = (height(i, ju) - height(i, jd)) / ((ju - jd) * step);
      glm::vec3 normal = glm::normalize(glm::vec3(-dzdx, -dzdy, 1.0f));
      if (!std::isfinite(normal.x) || !std::isfinite(normal.y)) normal = {0, 0, 1};
      const float t = zRange > 0.0f ? (z[j * n + i] - zMin) / zRange : 0.5f;
      vertices.insert(vertices.end(), {x, y, height(i, j), normal.x, normal.y, normal.z, t});
    }
  }

  // 3. Indices: two triangles per grid cell, then wireframe lines along x and y
  std::vector<GLuint> indices;
  const auto index = [&](int i, int j) { return GLuint(j * n + i); };
  for (int j = 0; j + 1 < n; ++j) {
    for (int i = 0; i + 1 < n; ++i) {
      if (!valid(i, j) || !valid(i + 1, j) || !valid(i, j + 1) || !valid(i + 1, j + 1))
        continue;
      indices.insert(indices.end(), {index(i, j), index(i + 1, j), index(i + 1, j + 1),
                                     index(i, j), index(i + 1, j + 1), index(i, j + 1)});
    }
  }
  g_triangleIndexCount = GLsizei(indices.size());
  for (int j = 0; j < n; ++j) {
    for (int i = 0; i < n; ++i) {
      if (!valid(i, j)) continue;
      if (i + 1 < n && valid(i + 1, j)) indices.insert(indices.end(), {index(i, j), index(i + 1, j)});
      if (j + 1 < n && valid(i, j + 1)) indices.insert(indices.end(), {index(i, j), index(i, j + 1)});
    }
  }
  g_lineIndexCount = GLsizei(indices.size()) - g_triangleIndexCount;

  glBindVertexArray(g_meshVao);
  glBindBuffer(GL_ARRAY_BUFFER, g_meshVbo);
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_meshEbo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);
  setVertexLayout();
  glBindVertexArray(0);
}

// Drag to orbit, wheel to zoom, plus the overlay's zoom / reset buttons
void updateCamera(const AppState &s, double deltaTime) {
  ImGuiIO &io = ImGui::GetIO();
  const bool overScene = io.MousePos.x > ui::PANEL_WIDTH && !io.WantCaptureMouse;

  if (io.MouseClicked[0] && overScene) g_dragging = true;
  if (!io.MouseDown[0]) g_dragging = false;
  if (g_dragging) {
    g_targetCam.yaw -= io.MouseDelta.x * 0.01f;
    g_targetCam.pitch += io.MouseDelta.y * 0.01f;
  }
  if (overScene && io.MouseWheel != 0.0f)
    g_targetCam.distance *= std::pow(0.9f, io.MouseWheel);
  if (s.zoomRequest != 0)
    g_targetCam.distance *= s.zoomRequest > 0 ? 0.8f : 1.25f;
  if (s.resetViewRequested)
    g_targetCam = kDefaultOrbit;

  g_targetCam.pitch = std::clamp(g_targetCam.pitch, -1.5f, 1.5f);
  g_targetCam.distance = std::clamp(g_targetCam.distance, 3.0f, 100.0f);

  // Ease toward the target, same idea as the fractal view
  const float ease = float(1.0 - std::exp(-15.0 * deltaTime));
  g_cam.yaw += (g_targetCam.yaw - g_cam.yaw) * ease;
  g_cam.pitch += (g_targetCam.pitch - g_cam.pitch) * ease;
  g_cam.distance += (g_targetCam.distance - g_cam.distance) * ease;
}

} // namespace

void init() {
  const GLuint vs = compileShader(GL_VERTEX_SHADER, kVertexShader);
  const GLuint fs = compileShader(GL_FRAGMENT_SHADER, kFragmentShader);
  g_program = glCreateProgram();
  glAttachShader(g_program, vs);
  glAttachShader(g_program, fs);
  glLinkProgram(g_program);
  glDeleteShader(vs);
  glDeleteShader(fs);

  glGenVertexArrays(1, &g_meshVao);
  glGenBuffers(1, &g_meshVbo);
  glGenBuffers(1, &g_meshEbo);
  glGenVertexArrays(1, &g_gridVao);
  glGenBuffers(1, &g_gridVbo);
  buildGridAndAxes();
}

void shutdown() {
  glDeleteBuffers(1, &g_meshVbo);
  glDeleteBuffers(1, &g_meshEbo);
  glDeleteBuffers(1, &g_gridVbo);
  glDeleteVertexArrays(1, &g_meshVao);
  glDeleteVertexArrays(1, &g_gridVao);
  glDeleteProgram(g_program);
}

void render(AppState &state, const SceneViewport &vp, double deltaTime) {
  // Re-parse on Enter / Plot / preset; keep the old surface if the new equation is invalid
  if (g_needsParse || state.equationChanged) {
    std::string error;
    math_expression::NodePtr expr = math_expression::parse(state.equation, error);
    if (expr) {
      g_expr = std::move(expr);
      state.equationError.clear();
      g_needsRebuild = true;
    } else {
      state.equationError = error;
    }
    g_needsParse = false;
  }
  if (state.paramsChanged) g_needsRebuild = true;
  if (g_needsRebuild && g_expr) {
    rebuildMesh(state);
    g_needsRebuild = false;
  }

  updateCamera(state, deltaTime);

  const glm::vec3 eye = g_cam.distance * glm::vec3(std::cos(g_cam.pitch) * std::cos(g_cam.yaw),
                                                   std::cos(g_cam.pitch) * std::sin(g_cam.yaw),
                                                   std::sin(g_cam.pitch));
  const glm::mat4 mvp =
      glm::perspective(glm::radians(45.0f), vp.aspect(), 0.1f, 500.0f) *
      glm::lookAt(eye, glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

  glEnable(GL_DEPTH_TEST);
  glUseProgram(g_program);
  glUniformMatrix4fv(glGetUniformLocation(g_program, "uMVP"), 1, GL_FALSE, glm::value_ptr(mvp));
  glUniform1i(glGetUniformLocation(g_program, "uLighting"), state.lighting);
  const glm::vec3 light = glm::normalize(glm::vec3(0.4f, 0.3f, 0.85f));
  glUniform3fv(glGetUniformLocation(g_program, "uLightDir"), 1, glm::value_ptr(light));
  const GLint solidLoc = glGetUniformLocation(g_program, "uSolid");
  const GLint colorLoc = glGetUniformLocation(g_program, "uColor");

  // Grid and axes (x red, y green, z blue)
  glUniform1i(solidLoc, 1);
  glBindVertexArray(g_gridVao);
  if (state.showGrid) {
    glUniform3f(colorLoc, 0.25f, 0.27f, 0.32f);
    glDrawArrays(GL_LINES, 0, kGridVertexCount);
  }
  glUniform3f(colorLoc, 0.9f, 0.3f, 0.3f);
  glDrawArrays(GL_LINES, kGridVertexCount, 2);
  glUniform3f(colorLoc, 0.3f, 0.9f, 0.4f);
  glDrawArrays(GL_LINES, kGridVertexCount + 2, 2);
  glUniform3f(colorLoc, 0.35f, 0.55f, 1.0f);
  glDrawArrays(GL_LINES, kGridVertexCount + 4, 2);

  // The surface itself, coloured by height
  if (g_expr) {
    glUniform1i(solidLoc, 0);
    glBindVertexArray(g_meshVao);
    if (state.viewMode == ViewMode::Surface) {
      glDrawElements(GL_TRIANGLES, g_triangleIndexCount, GL_UNSIGNED_INT, (void *)0);
    } else {
      glDrawElements(GL_LINES, g_lineIndexCount, GL_UNSIGNED_INT,
                     (void *)(g_triangleIndexCount * sizeof(GLuint)));
    }
  }

  glBindVertexArray(0);
  glUseProgram(0);
  glDisable(GL_DEPTH_TEST);
}

} // namespace surface
