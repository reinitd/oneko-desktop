// TODO: Comment explanations
// #define GLAD_GL_IMPLEMENTATION
// #include <glad/glad.h>
#ifdef __APPLE__
/* Defined before OpenGL and GLUT includes to avoid deprecation messages */
#define GL_SILENCE_DEPRECATION
#endif

#ifdef _WIN32
#define PATH_STYLE "windows"
#else
#define PATH_STYLE "unix"
#endif

#include <GLFW/glfw3.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <json.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define DEBUG true
#define CONFIG_VERSION 1.0;

// Thanks https://stackoverflow.com/a/4681398
static char *utilCat(char *dest, char *end, const char *str)
{
    while (dest < end && *str)
        *dest++ = *str++;
    return dest;
}

size_t joinStr(char *out_string, size_t out_bufsz, const char *delim, char **chararr)
{
    char *ptr = out_string;
    char *strend = out_string + out_bufsz - 1;
    while (ptr < strend && *chararr)
    {
        ptr = utilCat(ptr, strend, *chararr);
        chararr++;
        if (*chararr)
            ptr = utilCat(ptr, strend, delim);
    }
    *ptr = '\0';
    return ptr - out_string;
}

char *pathBuilder(int num_parts, ...)
{
    va_list args;
    va_start(args, num_parts);

    size_t total_length = 1; // 1 for the null terminator
    for (int i = 0; i < num_parts; i++)
    {
        char *part = va_arg(args, char *);
        total_length += strlen(part) + 1; // 1 for the delimiter
    }
    va_end(args);

    char *full_path = malloc(total_length);
    if (!full_path)
    {
        perror("Failed to allocate memory");
        return NULL;
    }

    const char *delim = "/";
    if (strcmp("windows", PATH_STYLE) == 0)
    {
        delim = "\\";
    }

    va_start(args, num_parts);
    char **parts = malloc(num_parts * sizeof(char *));
    for (int i = 0; i < num_parts; i++)
    {
        parts[i] = va_arg(args, char *);
    }
    va_end(args);

    joinStr(full_path, total_length, delim, parts);

    free(parts);
    return full_path;
}

char *resolveDegreeToImage(float deg)
{
    static char image_name[11];

    if (deg >= 247.5 && deg < 292.5)
    {
        strcpy(image_name, "up");
    }
    else if (deg >= 292.5 && deg < 337.5)
    {
        strcpy(image_name, "up-right");
    }
    else if (deg >= 337.5 || deg < 22.5)
    {
        strcpy(image_name, "right");
    }
    else if (deg >= 22.5 && deg < 67.5)
    {
        strcpy(image_name, "down-right");
    }
    else if (deg >= 67.5 && deg < 112.5)
    {
        strcpy(image_name, "down");
    }
    else if (deg >= 112.5 && deg < 157.5)
    {
        strcpy(image_name, "down-left");
    }
    else if (deg >= 157.5 && deg < 202.5)
    {
        strcpy(image_name, "left");
    }
    else if (deg >= 202.5 && deg < 247.5)
    {
        strcpy(image_name, "up-left");
    }

    return image_name;
}

void initTextures(GLuint **texture, int txWidth, int txHeight, unsigned char *txData)
{
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, txWidth, txHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, txData);
    stbi_image_free(txData);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glEnable(GL_TEXTURE_2D);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
}

void handleGLFWError(const char *error, bool glfw_terminate)
{
    glfwGetError(&error);
    printf("There's been an error:\n%s", error);
    if (glfw_terminate)
    {
        glfwTerminate();
    }
    exit(EXIT_FAILURE);
}

int start(char image_directory_path[], int target_fps)
{
    const char *error;
    GLFWwindow *window;

    if (!glfwInit())
    {
        handleGLFWError(error, false);
    }

    int count;
    int winWidth, winHeight;
    int monitorX, monitorY;
    GLFWmonitor **monitors = glfwGetMonitors(&count);
    const GLFWvidmode *videoMode = glfwGetVideoMode(monitors[0]);

    winWidth = videoMode->width / 1.5;
    winHeight = winWidth / 16 * 9; // TODO: detect aspect ratio, not just 16:9

    glfwGetMonitorPos(monitors[0], &monitorX, &monitorY);

    // TODO: Revisit these
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
    glfwWindowHint(GLFW_MOUSE_PASSTHROUGH, GLFW_TRUE);

    window = glfwCreateWindow(70, 70, "Kot", NULL, NULL);

    if (!window)
    {
        handleGLFWError(error, true);
    }

    glfwMakeContextCurrent(window);

    // For transparent textures
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // stbi_set_flip_vertically_on_load(true);

    char *initial_image_path = NULL;
    const char *path_to_add = pathBuilder(3, "actions", "sitting", "1.png");
    initial_image_path = malloc(strlen(path_to_add) + 1);
    strcpy(initial_image_path, path_to_add);

    char *image_path = malloc(strlen(image_directory_path) + strlen(initial_image_path));
    strcpy(image_path, image_directory_path);
    strcat(image_path, initial_image_path);

    if (DEBUG)
    {
        printf("Image path: %s\n", image_path);
    }

    while (!glfwWindowShouldClose(window))
    {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        if (DEBUG && count % 15 == 0)
        {
            printf("Mouse: (%f, %f)\n", xpos, ypos);
        }

        // -- Texture loading
        int txWidth, txHeight, nrChannels;
        unsigned char *txData = stbi_load(image_path, &txWidth, &txHeight, &nrChannels, STBI_rgb_alpha);
        if (!txData)
        {
            stbi_image_free(txData);
            perror("Failed to load texture.");
            exit(EXIT_FAILURE);
        }

        GLuint texture;
        initTextures(*texture, txWidth, txHeight, &txData);

        double currX = monitorX + (videoMode->width - winWidth);
        double currY = monitorY + (videoMode->height - winHeight);

        static const float speed = 1.25f;  // TODO: Move to config file.
        static const int maxDistance = 75; // TODO: Move to config file.

        int counter = 0;
        double lastSwitchTime = glfwGetTime();
        double switchInterval = .15; // TODO: Move to config file.
        bool isImage1 = true;

        // -- Render shit here

        glClear(GL_COLOR_BUFFER_BIT);
        glfwSwapBuffers(window); // TODO: What does this do? FIND OUT!
        glfwPollEvents();

        count++;
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

int main(void)
{
    if (DEBUG)
    {
        printf("PATH STYLE: %s\n", PATH_STYLE);
        printf("Path builder test, should be (hello/world): %s\n", pathBuilder(2, "hello", "world"));
    }
    // config check
    // TODO: trim trailing slash in config if present
    if (strcmp("windows", PATH_STYLE) == 0)
    {
        start("C:\\users\\kerba\\dev\\clang\\oneko-desktop\\images", 60);
    }
    else
    {
        start("/Users/reinitd/Developer/clang/oneko-desktop/images", 60);
    }

    return 0;
}
