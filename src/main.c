#ifdef __APPLE__
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
#include <math.h>
#include <json-c/json.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define DEBUG true
#define CONFIG_VERSION 2.0

typedef struct
{
    float config_version;
    char *images_directory;
    float target_fps;
    float movement_speed;
    float distance_from_cursor_to_stop_movement;
    float image_switch_interval;
    float image_scale_factor;
} Config;

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

void initTextures(GLuint *texture, int txWidth, int txHeight, unsigned char *txData)
{
    glGenTextures(1, texture);
    glBindTexture(GL_TEXTURE_2D, *texture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, txWidth, txHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, txData);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glEnable(GL_TEXTURE_2D);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR)
    {
        printf("OpenGL error in initTextures: %d\n", err);
    }
}

void handleGLFWError(bool glfw_terminate)
{
    const char *error_description;
    glfwGetError(&error_description);
    printf("There's been an error:\n%s\n", error_description);
    if (glfw_terminate)
    {
        glfwTerminate();
    }
    exit(EXIT_FAILURE);
}

void loopGlInit(GLuint *texture)
{
    glClear(GL_COLOR_BUFFER_BIT);

    // For transparency
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, *texture);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

char *buildActionImagePath(char *image_root_dir, char *action, bool isImage1)
{
    char *image;
    const char *filename = isImage1 ? "1.png" : "2.png";
    size_t length = strlen(filename) + 1;

    image = malloc(length);
    if (image == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    strcpy(image, filename);
    char *image_actions_path = pathBuilder(2, image_root_dir, "actions");
    return pathBuilder(3, image_actions_path, action, image);
}

float lerp(float start, float target, float t)
{
    return start + t * (target - start);
}

int start(const Config *cfg)
{
    GLFWwindow *window;

    if (!glfwInit())
    {
        handleGLFWError(false);
    }

    int count;
    GLFWmonitor **monitors = glfwGetMonitors(&count);

    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
    glfwWindowHint(GLFW_MOUSE_PASSTHROUGH, GLFW_TRUE);

    window = glfwCreateWindow(70, 70, "Kot", NULL, NULL);

    if (!window)
    {
        handleGLFWError(true);
    }

    glfwMakeContextCurrent(window);

    // For transparent textures
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    stbi_set_flip_vertically_on_load(true);

    char *first_sitting = pathBuilder(3, "actions", "sitting", "1.png");

    size_t initial_length = strlen(cfg->images_directory);
    size_t additional_length = strlen(first_sitting);
    size_t total_length = initial_length + additional_length + 1;

    char *initial_image_path = (char *)malloc(total_length);

    if (initial_image_path == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }

    strcpy(initial_image_path, cfg->images_directory);
    strcat(initial_image_path, first_sitting);

    if (DEBUG)
    {
        printf("Image path: %s\n", initial_image_path);
    }

    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    // -- Texture loading
    int txWidth, txHeight, nrChannels;
    unsigned char *txData = stbi_load(initial_image_path, &txWidth, &txHeight, &nrChannels, STBI_rgb_alpha);
    free(initial_image_path);
    if (!txData)
    {
        perror("Failed to load texture.");
        exit(EXIT_FAILURE);
    }

    GLuint texture;
    initTextures(&texture, txWidth, txHeight, txData);
    stbi_image_free(txData);

    const float speed = cfg->movement_speed;
    const int maxDistance = cfg->distance_from_cursor_to_stop_movement;

    double lastSwitchTime = glfwGetTime();
    const double switchInterval = cfg->image_switch_interval;
    bool isImage1 = true;
    const float vertex_scale_factor = cfg->image_scale_factor;

    while (!glfwWindowShouldClose(window))
    {
        loopGlInit(&texture);

        // Render textured quad
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f);
        glVertex2f(-vertex_scale_factor, -vertex_scale_factor);
        glTexCoord2f(1.0f, 0.0f);
        glVertex2f(vertex_scale_factor, -vertex_scale_factor);
        glTexCoord2f(1.0f, 1.0f);
        glVertex2f(vertex_scale_factor, vertex_scale_factor);
        glTexCoord2f(0.0f, 1.0f);
        glVertex2f(-vertex_scale_factor, vertex_scale_factor);
        glEnd();

        glfwGetCursorPos(window, &xpos, &ypos);

        int winXPos, winYPos;
        glfwGetWindowPos(window, &winXPos, &winYPos);
        const GLFWvidmode *videoMode = glfwGetVideoMode(monitors[0]);

        float centeredCursorX = (winXPos + xpos) - (videoMode->width / 2);
        float centeredCursorY = (winYPos + ypos) - (videoMode->height / 2);

        // TODO: Fix bullshit lerping.

        float timeFactor = (float)glfwGetTime() / 10.0f;
        if (timeFactor > 1.0f)
            timeFactor = 1.0f;

        int targetX = (int)xpos;
        int targetY = (int)ypos;

        float lerpX = lerp((float)winXPos, (float)targetX, timeFactor);
        float lerpY = lerp((float)winYPos, (float)targetY, timeFactor);

        glfwSetWindowPos(window, (int)lerpX, (int)lerpY);

        if (DEBUG && count % 15 == 0)
        {
            printf("Mouse: (%f,%f)\n", centeredCursorX, centeredCursorY);
            printf("Win..: (%d,%d)\n", winXPos, winYPos);
            printf("Lerp.: (%f,%f)\n", lerpX, lerpY);
            printf("Trgt.: (%f,%f)\n", targetX, targetY);
        }

        char *action = "sitting";

        txData = stbi_load(buildActionImagePath(cfg->images_directory, action, isImage1), &txWidth, &txHeight, &nrChannels, STBI_rgb_alpha);

        if (txData)
        {
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, txWidth, txHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, txData);
            stbi_image_free(txData);
        }

        double currentTime = glfwGetTime();
        if (currentTime - lastSwitchTime >= switchInterval)
        {
            isImage1 = !isImage1;
            lastSwitchTime = currentTime;
        }

        glfwSwapBuffers(window);
        glfwPollEvents();

        count++;
    }

    glfwTerminate();
    return 0;
}

int are_floats_equal(float a, float b)
{
    return fabs(a - b) < 1e-6;
}

char *lastN(const char *str, size_t n)
{
    size_t len = strlen(str);
    if (len >= n)
    {
        return (char *)str + len - n;
    }
}

void load_config(const char *filename, Config *config)
{
    struct json_object *parsed_json;
    struct json_object *json_value;
    char *file_contents;
    long length;
    FILE *file = fopen(filename, "r");

    if (file == NULL)
    {
        perror("Failed to open config file");
        exit(EXIT_FAILURE);
    }

    fseek(file, 0, SEEK_END);
    length = ftell(file);
    fseek(file, 0, SEEK_SET);
    file_contents = malloc(length);
    fread(file_contents, 1, length, file);
    fclose(file);

    parsed_json = json_tokener_parse(file_contents);
    free(file_contents);

    if (!json_object_object_get_ex(parsed_json, "config_version", &json_value))
    {
        fprintf(stderr, "Config version not found in config.\n");
        exit(1);
    }
    config->config_version = json_object_get_double(json_value);

    if (!are_floats_equal(CONFIG_VERSION, config->config_version))
    {
        fprintf(stderr, "This program supports config version %f, not version %f.\n", CONFIG_VERSION, config->config_version);
        exit(1);
    }

    if (!json_object_object_get_ex(parsed_json, "images_directory", &json_value))
    {
        fprintf(stderr, "Images directory KVP not found in config.\n");
        exit(1);
    }
    const char *images_directory = json_object_get_string(json_value);
    size_t len = strlen(images_directory);

    char *id = (char *)malloc(len + 2);
    if (id == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }

    strcpy(id, images_directory);

    if (strcmp(lastN(id, 1), "/") != 0)
    {
        strcat(id, "/");
    }
    config->images_directory = id;

    if (!json_object_object_get_ex(parsed_json, "target_fps", &json_value))
    {
        config->target_fps = 60.0;
    }
    else
    {
        config->target_fps = json_object_get_double(json_value);
    }

    if (!json_object_object_get_ex(parsed_json, "movement_speed", &json_value))
    {
        config->movement_speed = 1.25;
    }
    else
    {
        config->movement_speed = json_object_get_double(json_value);
    }

    if (!json_object_object_get_ex(parsed_json, "distance_from_cursor_to_stop_movement", &json_value))
    {
        config->distance_from_cursor_to_stop_movement = 75.0;
    }
    else
    {
        config->distance_from_cursor_to_stop_movement = json_object_get_double(json_value);
    }

    if (!json_object_object_get_ex(parsed_json, "image_switch_interval", &json_value))
    {
        config->image_switch_interval = 0.25;
    }
    else
    {
        config->image_switch_interval = json_object_get_double(json_value);
    }

    if (!json_object_object_get_ex(parsed_json, "image_scale_factor", &json_value))
    {
        config->image_scale_factor = 0.5;
    }
    else
    {
        config->image_scale_factor = json_object_get_double(json_value);
    }

    json_object_put(parsed_json); // clean up, clean up, everyone clean up.
}

int main(void)
{
    FILE *confptr;
    confptr = fopen("oneko-config.json", "r");
    if (confptr == NULL)
    {
        fclose(confptr);
        printf("No config file found in the current directory, creating default config file.\n");
        json_object *jobj = json_object_new_object();

        json_object_object_add(jobj, "config_version", json_object_new_double(CONFIG_VERSION));
        json_object_object_add(jobj, "images_directory", json_object_new_string("images"));
        json_object_object_add(jobj, "target_fps", json_object_new_double(60.0));
        json_object_object_add(jobj, "movement_speed", json_object_new_double(1.25));
        json_object_object_add(jobj, "distance_from_cursor_to_stop_movement", json_object_new_int(75));
        json_object_object_add(jobj, "image_switch_interval", json_object_new_double(0.25));
        json_object_object_add(jobj, "image_scale_factor", json_object_new_double(0.5));

        const char *json_string = json_object_to_json_string_ext(jobj, JSON_C_TO_STRING_PRETTY);

        FILE *confptr = fopen("oneko-config.json", "w");
        if (confptr == NULL)
        {
            printf(stderr, "There's been an error opening the config file.\n");
            fclose(confptr);
            exit(1);
        }
        fprintf(confptr, "%s\n", json_string);
        fclose(confptr);
        json_object_put(jobj);

        printf("Please verify the image directory in ./oneko-config.json and rerun the application.\n");
        exit(1);
    }

    fclose(confptr);

    Config config;
    load_config("oneko-config.json", &config);

    start(&config);

    free(config.images_directory);
    return 0;
}
