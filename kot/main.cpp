#define GLAD_GL_IMPLEMENTATION
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#include <stdio.h>
#include <stdlib.h>

#include <windows.h>
#include <valarray>

#include <iostream>

using namespace std;

int main()
{
    const char *description;
    GLFWwindow *mainWindow;

    if (!glfwInit())
    {
        glfwGetError(&description);
        printf("Error: %s\n", description);
        exit(EXIT_FAILURE);
    }

    // For getting mouse position
    POINT screenPixel;

    int count;
    int windowWidth, windowHeight;
    int monitorX, monitorY;
    GLFWmonitor **monitors = glfwGetMonitors(&count);
    const GLFWvidmode *videoMode = glfwGetVideoMode(monitors[0]);
    windowWidth = videoMode->width / 1.5;
    windowHeight = windowWidth / 16 * 9;

    glfwGetMonitorPos(monitors[0], &monitorX, &monitorY);

    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);

    mainWindow = glfwCreateWindow(70, 70, "Kot", NULL, NULL);

    if (!mainWindow)
    {
        glfwGetError(&description);
        printf("Error: %s\n", description);
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwSetWindowPos(mainWindow,
                     monitorX + (videoMode->width - windowWidth) / 2,
                     monitorY + (videoMode->height - windowHeight) / 2);

    glfwSetWindowSizeLimits(mainWindow, 1, 1, GLFW_DONT_CARE, GLFW_DONT_CARE);

    glfwMakeContextCurrent(mainWindow);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    // Make texture transparent
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    stbi_set_flip_vertically_on_load(true);

    int width, height, channels;
    unsigned char *image = stbi_load("C:\\Users\\kerba\\dev\\cpp\\kot-cmake\\kot\\data\\ea nasir.png", &width, &height, &channels, STBI_rgb_alpha);

    if (!image)
    {
        cerr << "Failed to load image" << endl;
        return -1;
    }

    // OpenGL texture
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    stbi_image_free(image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // texture mapping
    glEnable(GL_TEXTURE_2D);

    // black
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    const float smoothFactor = 0.008f;
    double currentX = monitorX + (videoMode->width - windowWidth);
    double currentY = monitorY + (videoMode->height - windowHeight);

    const float speed = 1.5f;

    // Loop until the user closes the window
    while (!glfwWindowShouldClose(mainWindow))
    {
        glClear(GL_COLOR_BUFFER_BIT);

        // more transparent shit
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texture);

        // Enable blending
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Render texture
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f);
        glVertex2f(-1.0f, -1.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex2f(1.0f, -1.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex2f(1.0f, 1.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex2f(-1.0f, 1.0f);
        glEnd();

        if (GetCursorPos(&screenPixel))
        {
            double targetX = monitorX + screenPixel.x + 10;
            double targetY = monitorY + screenPixel.y + 10;

            // difference between target and current positions
            double deltaX = targetX - currentX;
            double deltaY = targetY - currentY;

            double distance = sqrt(deltaX * deltaX + deltaY * deltaY);

            // idk wtf this does but it works frfr
            double directionX = deltaX / distance;
            double directionY = deltaY / distance;

            // same for this, but it works.
            currentX = currentX + speed * directionX;
            currentY = currentY + speed * directionY;

            glfwSetWindowPos(mainWindow, static_cast<int>(currentX), static_cast<int>(currentY));
        }

        glfwSwapBuffers(mainWindow);
        glfwPollEvents();
    }

    // Clean up
    glDeleteTextures(1, &texture);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}
