#include <iostream>
#include <GLFW/glfw3.h>

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

int main() {
    // 1. Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Configure OpenGL Version (3.3 Core Profile)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // 2. Create Window (Fixed typo: GLFWwindow)
    GLFWwindow* window = glfwCreateWindow(1280, 720, "GraphX - Interactive Math Equation Plotter", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    std::cout << "OpenGL Window Initialized Successfully!" << std::endl;

    // 3. Main Render Loop (Fixed typo: glfwWindowShouldClose)
    while (!glfwWindowShouldClose(window)) {
        // Clear color buffer (Dark slate grey background)
        glClearColor(0.12f, 0.12f, 0.14f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Poll events and swap buffers
        glfwPollEvents();
        glfwSwapBuffers(window);
    }

    // 4. Cleanup
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}