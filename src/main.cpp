#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <string>

#include "app_state.h"
#include "ui/ui_panel.h"

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

    GLFWwindow* window = glfwCreateWindow(1280, 720,
        "GraphX - Interactive Math Equation Plotter", nullptr, nullptr);
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
                glClearColor(0.05f, 0.09f, 0.17f, 1.0f);   // placeholder
            } else {
                glClearColor(0.02f, 0.12f, 0.06f, 1.0f);   // placeholder
            }
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // TODO (Gerald): switch on state.mode here
            // TODO (Daniel): draw the 3D surface using `vp.aspect()` and the plotter fields
            // TODO (Tracy):  draw the fractal canvas using the fractal fields
            // TODO (Naomi):  use state.zoomRequest / state.resetViewRequested

            glDisable(GL_SCISSOR_TEST);
        }

        glViewport(0, 0, fbW, fbH);
        ui::endFrame();

        glfwSwapBuffers(window);
        state.clearRequests();
    }

    ui::shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
