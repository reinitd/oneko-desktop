#define GLAD_GL_IMPLEMENTATION
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#include "logger/logger.hpp"
#include "json.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <filesystem>
#include <cstdio>

#include <Windows.h>
#include <winuser.h>
#include <shellapi.h>
#include <thread>
#include <cstring>
#define APPWM_ICONNOTIFY (WM_APP + 1)
#define IDM_EXIT 777

#include <cmath>
#include <iostream>

using namespace std;
using json = nlohmann::json;

const bool DEBUG = false;
const double CONFIG_VERSION = 1.0;

filesystem::path get_executable_path()
{
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    return path;
}

filesystem::path EXE_PATH = get_executable_path().parent_path();
filesystem::path FILE_PATH = EXE_PATH / "oneko-config.json";
filesystem::path EXAMPLE_IMAGE_PATH = filesystem::canonical(EXE_PATH / "../images");

SimpleLogger logger("./oneko-desktop.log");

pair<int, int> getWindowPosition(GLFWwindow *window)
{
    int x, y;
    glfwGetWindowPos(window, &x, &y);
    return pair<int, int>(x, y);
}

string resolveDegreeToImage(float deg)
{
    string image_name;
    // this shit was cancerous to make
    if (deg >= 247.5 && deg < 292.5)
    {
        image_name = "up";
    }
    else if (deg >= 292.5 && deg < 337.5)
    {
        image_name = "up-right";
    }
    else if (deg >= 337.5 || deg < 22.5)
    {
        image_name = "right";
    }
    else if (deg >= 22.5 && deg < 67.5)
    {
        image_name = "down-right";
    }
    else if (deg >= 67.5 && deg < 112.5)
    {
        image_name = "down";
    }
    else if (deg >= 112.5 && deg < 157.5)
    {
        image_name = "down-left";
    }
    else if (deg >= 157.5 && deg < 202.5)
    {
        image_name = "left";
    }
    else if (deg >= 202.5 && deg < 247.5)
    {
        image_name = "up-left";
    }

    return image_name;
}

int start(string image_directory_path)
{
    const char *description;
    GLFWwindow *mainWindow;

    if (!glfwInit())
    {
        glfwGetError(&description);
        cout << "Error: " << description << endl;
        exit(EXIT_FAILURE);
    }

    // for getting mouse position
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
    glfwWindowHint(GLFW_MOUSE_PASSTHROUGH, GLFW_TRUE);

    mainWindow = glfwCreateWindow(70, 70, "Oneko Desktop", NULL, NULL);

    if (!mainWindow)
    {
        glfwGetError(&description);
        logger.log(description, "ERROR");
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwSetWindowPos(mainWindow,
                     monitorX + (videoMode->width - windowWidth) / 2,
                     monitorY + (videoMode->height - windowHeight) / 2);

    glfwSetWindowSizeLimits(mainWindow, 1, 1, GLFW_DONT_CARE, GLFW_DONT_CARE);

    HWND hwnd = glfwGetWin32Window(mainWindow);
    ShowWindow(hwnd, SW_HIDE);
    SetWindowLong(hwnd, GWL_EXSTYLE, WS_EX_TOOLWINDOW);
    ShowWindow(hwnd, SW_SHOW);

    glfwMakeContextCurrent(mainWindow);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    // make texture transparent
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    stbi_set_flip_vertically_on_load(true);

    int width, height, channels;
    string image_path = image_directory_path + "\\actions\\sitting\\1.png";
    unsigned char *image = stbi_load(image_path.c_str(), &width, &height, &channels, STBI_rgb_alpha);

    if (!image)
    {
        logger.log("Failed to load image (initial).", "ERROR");
        return -1;
    }

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    stbi_image_free(image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glEnable(GL_TEXTURE_2D);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // black

    double currentX = monitorX + (videoMode->width - windowWidth);
    double currentY = monitorY + (videoMode->height - windowHeight);

    const float speed = 1.25f;

    const int maxDistance = 75;

    int counter = 0;
    double lastSwitchTime = glfwGetTime();
    double switchInterval = .15; // seconds
    bool isImage1 = true;

    // loop until the user closes the window
    while (!glfwWindowShouldClose(mainWindow))
    {
        glClear(GL_COLOR_BUFFER_BIT);

        // transparent shit
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texture);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        float vertex_scale_factor = 0.4f;

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

        if (GetCursorPos(&screenPixel))
        {
            double targetX = monitorX + screenPixel.x + 10;
            double targetY = monitorY + screenPixel.y + 10;

            double deltaX = targetX - currentX;
            double deltaY = targetY - currentY;

            double distance = sqrt(deltaX * deltaX + deltaY * deltaY);

            double directionX = deltaX / distance;
            double directionY = deltaY / distance;

            const float offset = (70 / 2); // window width divided by 2

            // move only if the distance is within the threshold
            if (distance > maxDistance)
            {
                currentX = currentX + speed * directionX;
                currentY = currentY + speed * directionY;
            }

            pair<int, int> winPos = getWindowPosition(mainWindow);

            int winXPos = winPos.first;
            int winYPos = winPos.second;

            int trueDistanceX = winXPos - screenPixel.x + offset;
            int trueDistanceY = winYPos - screenPixel.y + offset;

            float rad = atan2(deltaY, deltaX);
            float deg = fmod((rad * (180 / M_PI)) + 360, 360);

            glfwSetWindowPos(mainWindow, static_cast<int>(currentX - offset - 10), static_cast<int>(currentY - offset - 10));
            string image_name;
            image_name = resolveDegreeToImage(deg);

            if (distance < maxDistance)
            {
                image_name = "actions/sitting";
            }

            image_path = image_directory_path + image_name + "/1.png";

            double currentTime = glfwGetTime();
            if (currentTime - lastSwitchTime > switchInterval)
            {
                lastSwitchTime = currentTime;

                if (isImage1)
                {
                    image_path = image_directory_path + image_name + "/2.png";
                }
                else
                {
                    image_path = image_directory_path + image_name + "/1.png";
                }

                image = stbi_load(image_path.c_str(), &width, &height, &channels, STBI_rgb_alpha);

                if (!image)
                {
                    logger.log("Failed to load image (update):\n" + image_path, "ERROR");
                    return -1;
                }

                glBindTexture(GL_TEXTURE_2D, texture);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
                stbi_image_free(image);

                isImage1 = !isImage1;
            }

            if (DEBUG)
            {
                if (counter % 15 == 0)
                {
                    int winXPosFromWinCenter = winXPos - offset;
                    int winYPosFromWinCenter = winYPos - offset;

                    logger.log("Rad: " + std::to_string(rad) + "\nDeg: " + std::to_string(deg), "DEBUG");
                    logger.log("Win X.....: " + std::to_string(winXPosFromWinCenter) + "\nWin Y.....: " + std::to_string(winYPosFromWinCenter), "DEBUG");
                    logger.log("Cur X.....: " + std::to_string(screenPixel.x) + "\nCur Y.....: " + std::to_string(screenPixel.y), "DEBUG");
                    logger.log("Distance X: " + std::to_string(trueDistanceX) + "\nDistance Y: " + std::to_string(trueDistanceY), "DEBUG");
                    logger.log("--------------------------------------", "DEBUG");
                }
            }
        }

        glfwSwapBuffers(mainWindow);
        glfwPollEvents();
        counter++;
    }

    // clean up
    glDeleteTextures(1, &texture);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}

string get_images_directory()
{
    ifstream f(FILE_PATH);
    json data = json::parse(f);
    string images_path = data["images_directory"];
    return images_path + "/";
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
BOOL WINAPI ConsoleRoutine(DWORD dwCtrlType);

LPCSTR lpszClass = "__hidden__";

void message_event_loop()
{
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        cout << "getmsg" << endl;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void setup_system_tray(string images_path)
{
    ::ShowWindow(::GetConsoleWindow(), SW_HIDE); // THANK YOU SO MUCH: https://stackoverflow.com/a/40430801/14363702

    // THANK YOU BIG TIME 🙏. I spent hours trying to find a way to detect click events: https://stackoverflow.com/a/38974976/14363702
    HINSTANCE hInstance = GetModuleHandle(nullptr);

    WNDCLASS wc;
    HWND hWnd;

    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hbrBackground = nullptr;
    wc.hCursor = nullptr;
    wc.hIcon = nullptr;
    wc.hInstance = hInstance;
    wc.lpfnWndProc = WndProc;
    wc.lpszClassName = lpszClass;
    wc.lpszMenuName = nullptr;
    wc.style = 0;
    RegisterClass(&wc);

    hWnd = CreateWindow(lpszClass, lpszClass, WS_OVERLAPPEDWINDOW,
                        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                        nullptr, nullptr, hInstance, nullptr);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    static NOTIFYICONDATA nid;
    static string icon_path = get_images_directory() + "logo.ico";
    static HMENU hPopupMenu = nullptr;

    switch (iMsg)
    {
    case WM_CREATE:
        std::memset(&nid, 0, sizeof(nid));
        nid.cbSize = sizeof(nid);
        nid.hWnd = hWnd;
        nid.uID = 0;
        nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_INFO;
        nid.uCallbackMessage = WM_APP + 1;

        hPopupMenu = CreatePopupMenu();
        if (!hPopupMenu)
        {
            logger.log("Failed to create popup menu. Error code: " + GetLastError(), "ERROR");
            return -1;
        }
        AppendMenu(hPopupMenu, MF_STRING, IDM_EXIT, ":(  Exit");

        nid.hIcon = (HICON)LoadImage(NULL, icon_path.c_str(), IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_SHARED);
        if (!nid.hIcon)
        {
            logger.log("Couldn't load image. nid.hIcon.", "ERROR");
            MessageBox(hWnd, "Couldn't load image. nid.hIcon.", "Error", MB_OK | MB_ICONERROR);
        }
        memcpy(nid.szInfoTitle, "Oneko Desktop", sizeof("Oneko Desktop"));
        memcpy(nid.szInfo, "Started!\nYou may exit by right clicking the system tray icon.", sizeof("Started!\nYou may exit by right clicking the system tray icon."));
        nid.dwInfoFlags = NIIF_NOSOUND; // you're welcome :)
        lstrcpy(nid.szTip, (LPCSTR)"Oneko Desktop");
        Shell_NotifyIcon(NIM_ADD, &nid);
        Shell_NotifyIcon(NIM_SETVERSION, &nid);
        return 0;
    case WM_APP + 1:
        switch (lParam)
        {
        case WM_LBUTTONUP:
            break;
        case WM_RBUTTONDOWN:
        case WM_CONTEXTMENU:
            POINT pt;
            GetCursorPos(&pt);

            if (hPopupMenu)
            {
                SetForegroundWindow(hWnd); // this little bastard makes the context menu "toggleable"
                if (!TrackPopupMenu(hPopupMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL))
                {
                    logger.log("TrackPopupMenu failed. Error code: " + GetLastError(), "ERROR");
                }
            }
            else
            {
                logger.log("hPopupMenu is NULL.", "ERROR");
            }

            break;
        }
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDM_EXIT:
            DestroyMenu(hPopupMenu);
            Shell_NotifyIcon(NIM_DELETE, &nid);
            PostQuitMessage(0);
            break;
        }

        break;
    case WM_DESTROY:
        Shell_NotifyIcon(NIM_DELETE, &nid);
        logger.log("WM Destroyed from context menu, goodbye!", "INFO");
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, iMsg, wParam, lParam);
}

int main()
{

    if (!filesystem::exists(FILE_PATH))
    {
        ofstream outputFile(FILE_PATH);

        logger.log("Couldn't find config file, creating one...", "WARNING");

        if (!outputFile.is_open())
        {
            logger.log("Unable to open the file!", "ERROR");
            return 1;
        }

        outputFile << "{\n"
                   << "    \"config_version\": " << CONFIG_VERSION << ",\n"
                   << "    \"images_directory\": \"" << EXAMPLE_IMAGE_PATH.generic_string() << "\"\n"
                   << "}";

        outputFile.close();
        logger.log("Config file created at:\n" + FILE_PATH.generic_string() + "\n\nPlease update it accordingly and rerun the application.",
                   "INFO");
    }
    else
    {
        string images_path = get_images_directory();

        logger.log("Using images directory: " + images_path, "INFO");

        logger.log("Minimizing to system tray.", "INFO");

        setup_system_tray(images_path);
        thread messageEventLoop(message_event_loop);

        start(images_path);

        messageEventLoop.join();
    }

    return 0;
}