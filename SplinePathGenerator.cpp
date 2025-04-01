// code to generate path data from a given bezier curve

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#include <SDL.h>
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL_opengles2.h>
#else
#include <SDL_opengl.h>
#endif

// #define ROTATE_360       // change path rotation from 0-23 into 0-359
#if defined(ROTATE_360) 
#include <cmath> 
#endif
#include <corecrt_math.h>
#include <algorithm>
#include "imfilebrowser.h"

// Visual Studio warnings
#ifdef _MSC_VER
#pragma warning (disable: 5056) // c++20 deprecated == of array types
#endif

struct Point
{
    float x, y;

    Point()
        : x(0.0), y(0.0)
    {}

    Point(const float NewX, const float NewY)
        : x(NewX), y(NewY)
    {}

    Point operator += (const Point& other)
    {
        x += other.x;
        y += other.y;
        return { x,y };
    }
    Point operator -= (const Point& other)
    {
        x -= other.x;
        y -= other.y;
        return { x,y };
    }
};

float Length(const Point v)
{
    return sqrtf(v.x * v.x + v.y * v.y);
}

Point Normalize(const Point v)
{
    Point Result;

    const float ThisLength = Length(v);

    Result.x = v.x / ThisLength;
    Result.y = v.y / ThisLength;

    return Result;
}

// Function to calculate the Catmull-Rom interpolation
Point catmull_rom(const ImVec2 p0, const ImVec2 p1, const ImVec2 p2, const ImVec2 p3, const float t)
{
    const float t2 = t * t;
    const float t3 = t2 * t;

    Point result;
    result.x = 0.5f * ((2 * p1.x) +
                      (-p0.x + p2.x) * t +
                      (2 * p0.x - 5 * p1.x + 4 * p2.x - p3.x) * t2 +
                      (-p0.x + 3 * p1.x - 3 * p2.x + p3.x) * t3);
    result.y = 0.5f * ((2 * p1.y) +
                      (-p0.y + p2.y) * t +
                      (2 * p0.y - 5 * p1.y + 4 * p2.y - p3.y) * t2 +
                      (-p0.y + 3 * p1.y - 3 * p2.y + p3.y) * t3);
    
    return result;
}

// Function to calculate the first derivative of the Catmull-Rom spline
Point catmull_rom_derivative(const ImVec2 p0, const ImVec2 p1, const ImVec2 p2, const ImVec2 p3, const float t)
{
    const float t2 = t * t;

    Point result;
    result.x = 0.5f * ((-p0.x + p2.x) +
                      2 * (2 * p0.x - 5 * p1.x + 4 * p2.x - p3.x) * t +
                      3 * (-p0.x + 3 * p1.x - 3 * p2.x + p3.x) * t2);
    result.y = 0.5f * ((-p0.y + p2.y) +
                      2 * (2 * p0.y - 5 * p1.y + 4 * p2.y - p3.y) * t +
                      3 * (-p0.y + 3 * p1.y - 3 * p2.y + p3.y) * t2);

    return result;
}

struct PathPoint
{
    int command;
    int command_data;
    int deltaX;
    int deltaY;
    int rotation;
    ImVec2 pos;
};

PathPoint Path[5000];
PathPoint CondensedPath[5000];
 
int GeneratePath(ImVector<ImVec2>& points, const int numPoints, const float StepT, const float MinDelta)
{
    Point LastPoint = {0.0f,0.0f};
    Point LastTangent = {0.0f,0.0f};
    Point LastUsedPoint = {0.0f,0.0f};
    Point TotalDelta = {0.0f,0.0f};
    float PathDirection = 0.0f;
    int n = 0;

    for (int i = 1; i < numPoints - 2; i++)
    {
        for (float t = 0.0f; t <= 1.0000f; t += StepT)
        {
            Point CurrentPoint = catmull_rom(points[i-1], points[i], points[i+1], points[i+2], t);
            Point PathTangent = catmull_rom_derivative(points[i-1], points[i], points[i+1], points[i+2], t);

            if (t == 0.0f)
            {
                LastPoint = CurrentPoint;
                LastUsedPoint = CurrentPoint;
                LastTangent = PathTangent;
            }
            const Point Delta = Point(CurrentPoint.x - LastPoint.x, CurrentPoint.y - LastPoint.y);

            const float D1 = MinDelta - Length(TotalDelta);
            TotalDelta += Delta;
            const float D2 = Length(TotalDelta) - MinDelta; 
            if (Length(TotalDelta) >= MinDelta)
            {
                Point UsePoint;
                Point UseTangent;
                if (D1 > D2)
                {
                    UsePoint = LastPoint;
                    UseTangent = LastTangent;
                    TotalDelta -= Delta;
                }
                else
                {
                    UsePoint = CurrentPoint;
                    UseTangent = PathTangent;
                }
#if defined(ROTATE_360)  // set the direction in degrees  0-359 
                PathDirection = 90 - static_cast<float>(atan2f(UseTangent.y, UseTangent.x) * (180.0f / M_PI)); // PathDirection in Degrees 0 - 359 ... North 0 positive clockwise
                if (PathDirection < 0.0f)  // convert any negative degrees into positive degrees
                {
                    PathDirection += 360.0f;
                }
#else
                PathDirection = atan2f(UseTangent.y, UseTangent.x);
                PathDirection *= 3.8197186f;
                PathDirection += 6.0f;
                if (PathDirection < 0.0f)
                {
                    PathDirection += 24.0f;
                }
                if (PathDirection > 24.0f)
                {
                    PathDirection -= 24.0f;
                }
#endif
                if (n < 5000)
                {
                    Path[n++] = {1, 1, static_cast<int>(TotalDelta.x), static_cast<int>(TotalDelta.y), (static_cast<int>(PathDirection - 0.5f)),
                                    {static_cast<float>(LastUsedPoint.x), static_cast<float>(LastUsedPoint.y)}};
                }
                if (D1 > D2)
                {
                    TotalDelta = Delta;
                }
                else
                {
                    TotalDelta = { 0.0f, 0.0f };
                }
                LastUsedPoint = UsePoint;
            }
            LastPoint = CurrentPoint;
            LastTangent = PathTangent;
        }
    }
    if (Length(TotalDelta) > 0.0f)
    {
        Path[n++] = {1, 1, static_cast<int>(TotalDelta.x), static_cast<int>(TotalDelta.y), (static_cast<int>(PathDirection)),
                            {static_cast<float>(LastPoint.x), static_cast<float>(LastPoint.y)}};
    }
    Path[n]= {0,0,0,0,0,{0.0f, 0.0f}};

    return n;
}

int CondensePath(int numPathPoints)
{
    CondensedPath[0] = Path[0];
    int condensedIndex = 0;
    for (int i = 1; i < numPathPoints; i++)
    {
        if ((Path[i].deltaX == CondensedPath[condensedIndex].deltaX) && (Path[i].deltaY == CondensedPath[condensedIndex].deltaY) && (Path[i].rotation == CondensedPath[condensedIndex].rotation))
        {
            CondensedPath[condensedIndex].command_data++;
        }
        else
        {
            condensedIndex++;
            CondensedPath[condensedIndex] = Path[i];
        }
    }
    condensedIndex++;
    CondensedPath[condensedIndex] = {0,0,0,0,0, Path[numPathPoints-1].pos};
    return condensedIndex;
}

char PathText[30 * 1000];

void WritePath(int numCondensedPoints)
{

#if defined(ROTATE_360)  // rotation expands to 2 bytes
    int result = sprintf_s(PathText, sizeof(PathText), "char path[%d * 6] =\n{\n", numCondensedPoints + 1);
#else
    int result = sprintf_s(PathText, sizeof(PathText), "char path[%d * 5] =\n{\n", numCondensedPoints + 1);
#endif
    for (int i = 0; i < numCondensedPoints; i++)
    {
#if defined(ROTATE_360)  // rotation expands to 2 bytes
        result += sprintf_s(&PathText[result], sizeof(PathText) - result, "    %1d, %2d, %3d, %3d, %2d, %2d,\n", CondensedPath[i].command, CondensedPath[i].command_data, CondensedPath[i].deltaX, CondensedPath[i].deltaY, (CondensedPath[i].rotation & 255), (CondensedPath[i].rotation >> 8) & 255);
#else
        result += sprintf_s(&PathText[result], sizeof(PathText) - result, "    %1d, %2d, %3d, %3d, %2d,\n", CondensedPath[i].command, CondensedPath[i].command_data, CondensedPath[i].deltaX, CondensedPath[i].deltaY, CondensedPath[i].rotation);
#endif
    }
#if defined(ROTATE_360)  // rotation expands to 2 bytes
    result += sprintf_s(&PathText[result], sizeof(PathText) - result, "    0,  0,   0,   0,  0, 0\n");
#else
    result += sprintf_s(&PathText[result], sizeof(PathText) - result, "    0,  0,   0,   0,  0\n");
#endif
    result += sprintf_s(&PathText[result], sizeof(PathText) - result, "};\n");
    PathText[result] = 0;
    (void)fflush(stdout);
}

void Export360Path(int numCondensedPoints)
{
    FILE* output;
    errno_t error = fopen_s(&output, "PATH-360.BIN", "wb");
    if (error == 0 && output != nullptr)
    {
        auto bytesWritten = fwrite(&numCondensedPoints, 1, 2, output);
        for (int i = 0; i < numCondensedPoints; i++)
        {
            bytesWritten += fwrite(&CondensedPath[i].command, 1, 1, output);
            bytesWritten += fwrite(&CondensedPath[i].command_data, 1, 1, output);
            bytesWritten += fwrite(&CondensedPath[i].deltaX, 1, 1, output);
            bytesWritten += fwrite(&CondensedPath[i].deltaY, 1, 1, output);
            bytesWritten += fwrite(&CondensedPath[i].rotation, 1, 2, output);
        }
        __int32 zip = 0;
        bytesWritten += fwrite(&zip, 1, 4, output);  // command 0, command_data 0, deltaX 0, deltaY 0
        bytesWritten += fwrite(&zip, 1, 2, output);  // rotation 0

        int result = fclose(output);
        printf("Bytes Written: %zu  fclose result: %d \n", bytesWritten, result);
    }
}

void SavePoints(const ImVector<ImVec2>& points, float MinDelta, const char* filename)
{
    FILE *output;
    errno_t error = fopen_s(&output, filename, "wb");
    if (error == 0 && output != nullptr)
    {
        fwrite( &points.Size, sizeof(points.Size), 1, output);
        fwrite(points.begin(), sizeof(float), points.Size * 2, output);
        fwrite(&MinDelta, sizeof(float), 1, output);
        fclose(output);
    }
}

void LoadPoints(ImVector<ImVec2>& points, float& MinDelta, const char* filename)
{
    FILE *input;
    errno_t error = fopen_s(&input, filename, "rb");
    if (error == 0 && input != nullptr)
    {
        int temp;
        fread( &temp, sizeof(int), 1, input);
        points.resize(temp);
        fread(points.begin(), sizeof(float), temp * 2, input);
        fread(&MinDelta, sizeof(float), 1, input);
        fclose(input);
    }
}

void SaveProject(const ImVector<char[MAX_PATH]>& projectFiles, const char* filename)
{
    FILE *output;
    errno_t error = fopen_s(&output, filename, "wb");
    if (error == 0 && output != nullptr)
    {
        int temp = projectFiles.Size;
        fwrite( &temp, sizeof(int), 1, output);
        fwrite(projectFiles.begin(), sizeof(char[MAX_PATH]), projectFiles.Size, output);
        fclose(output);
    }
    
}
void LoadProject(ImVector<char[MAX_PATH]>& projectFiles, const char* filename)
{
    FILE *input;
    errno_t error = fopen_s(&input, filename, "rb");
    if (error == 0 && input != nullptr)
    {
        int temp;
        fread( &temp, sizeof(int), 1, input);
        projectFiles.resize(temp);
        fread(projectFiles.begin(), sizeof(char[MAX_PATH]), temp, input);
        fclose(input);
    }
}

void FlipPointsX(ImVector<ImVec2>& points)
{
    float max_x = -99999.9f;
    float min_x = 99999.9f;
    for (auto& point : points)
    {
        max_x = std::max(point.x, max_x);
        min_x = std::min(point.x, min_x);
    }
    for (auto& point : points)
    {
        point.x = (max_x - point.x) + min_x;
    }
}
void FlipPointsY(ImVector<ImVec2>& points)
{
    float max_y = -99999.9f;
    float min_y = 99999.9f;
    for (auto& point : points)
    {
        max_y = std::max(point.y, max_y);
        min_y = std::min(point.y, min_y);
    }
    for (auto& point : points)
    {
        point.y = (max_y - point.y) + min_y;
    }
}

void MovePoints(ImVector<ImVec2>& points, const ImVec2& delta)
{
    for (auto& point : points)
    {
        point.x += delta.x;
        point.y += delta.y;
    }
}

void ScalePoints(ImVector<ImVec2>& points, const float ScaleFactor)
{
    for (auto& point : points)
    {
        point.x *= ScaleFactor;
        point.y *= ScaleFactor;
    }
}


void ExportPaths(unsigned short memoryBase, const ImVector<char[MAX_PATH]>& files)
{
    ImVector<unsigned short> PathSizes;
    ImVector<unsigned char> PathData;

    float MinDelta = 10.0f;
    ImVector<ImVec2> points;

    // read in the paths, generate and condense them, and save the data in a buffers and the size 
    int numPaths = files.size();
    PathSizes.resize(numPaths);
    for (int i = 0; i < numPaths; i++)
    {
        LoadPoints(points, MinDelta, files[i]);
        if (points.size() > 3)
        {
            const int numPathPoints = GeneratePath(points, points.size(), 0.0001f, MinDelta);
            int numCondensedPoints = CondensePath(numPathPoints) + 1;
            for (int j = 0; j < numCondensedPoints; j++)
            {
                PathData.push_back((unsigned char)CondensedPath[j].command);
                PathData.push_back((unsigned char)CondensedPath[j].command_data);
                PathData.push_back((unsigned char)CondensedPath[j].deltaX);
                PathData.push_back((unsigned char)CondensedPath[j].deltaY);
                PathData.push_back((unsigned char)CondensedPath[j].rotation);
            }
            PathSizes[i] = (unsigned short)(numCondensedPoints * 5);
        }
    }

    FILE* output;
    errno_t error = fopen_s(&output, "PATHS.BIN", "wb");
    if (error == 0 && output != nullptr)
    {
        auto bytesWritten = fwrite(&numPaths, 1, 2, output);

        unsigned short offset = (unsigned short)(memoryBase + 2 + (numPaths * 2));
        for (int i = 0; i < numPaths; i++)
        {
            bytesWritten += fwrite(&offset, 1, 2, output);
            offset += PathSizes[i];
        }

        bytesWritten += fwrite(PathData.Data, 1, PathData.Size, output);

        int result = fclose(output);
        printf("Bytes Written: %zu  fclose result: %d \n", bytesWritten, result);
    }
}

// Main code
int main(int, char**)
{
    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char* glsl_version = "#version 100";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(__APPLE__)
    // GL 3.2 Core + GLSL 150
    const char* glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

    // From 2.0.18: Enable native IME.
#ifdef SDL_HINT_IME_SHOW_UI
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window = SDL_CreateWindow("Spline Path Generator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1920, 1080, window_flags);
    if (window == nullptr)
    {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return -1;
    }

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
    //io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows
    //io.ConfigViewportsNoAutoMerge = true;
    //io.ConfigViewportsNoTaskBarIcon = true;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    // - Our Emscripten build process allows embedding fonts to be accessible at runtime from the "fonts/" folder. See Makefile.emscripten for details.
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != nullptr);

    // Our state
    bool show_demo_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    ImVector<char[MAX_PATH]> files;
    
    // Main loop
    bool done = false;
#ifdef __EMSCRIPTEN__
    // For an Emscripten build we are disabling file-system access, so let's not attempt to do a fopen() of the imgui.ini file.
    // You may manually call LoadIniSettingsFromMemory() to load settings from your own storage.
    io.IniFilename = nullptr;
    EMSCRIPTEN_MAINLOOP_BEGIN
#else
    while (!done)
#endif
    {
        const ImU32 col = ImColor(0.4f, 0.4f, 0.4f, 1.0f);
        const ImU32 col2 = ImColor(1.0f, 1.0f, 1.0f, 1.0f);
        const ImU32 col3 = ImColor(0.0f, 1.0f, 1.0f, 1.0f);

        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // create an ImGui window that covers the entire viewport, so that we can have a menu bar at the top of the applications
        int width, height;
        SDL_GetWindowSize(window, &width, &height);
        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2((float)width + 2.0f, (float)height + 2.0f), ImGuiCond_Always);        

        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoBringToFrontOnFocus |                 // we just want to use this window as a host for the menubar and docking
            ImGuiWindowFlags_NoNavFocus |                                                      // so turn off everything that would make it act like a window
            ImGuiWindowFlags_NoDocking |
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_MenuBar |
            ImGuiWindowFlags_NoBackground;                                                      // we want our game content to show through this window, so turn off the background.

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));                          // we don't want any padding for windows docked to this window frame

        bool show = (ImGui::Begin("Main", NULL, windowFlags));                                   // show the "window"
        ImGui::PopStyleVar();                                                                    // restore the style so inner windows have fames

        static float MinDelta = 10.0f;
        static float ScaleFactor = 1.0f;
        int numCondensedPoints = 0;
        static ImVector<ImVec2> points;
        
        // create a docking space inside our inner window
        ImGui::DockSpace(ImGui::GetID("Dockspace"), ImVec2(0.0f, 0.0f),  ImGuiDockNodeFlags_None);
        if (show)
        {
            ImGui::Begin("Curve Params");
            ImGui::DragFloat("MinDelta", &MinDelta, 1.0f, 2.00f, 200.0f);
            ImGui::DragFloat("Scale Factor", &ScaleFactor, 0.05f, 0.1f, 2.0f);
            if (points.size() > 3)
            {
                const int numPathPoints = GeneratePath(points, points.size(), 0.0001f, MinDelta);
                ImGui::Text("num points: %d", numPathPoints);
                numCondensedPoints = CondensePath(numPathPoints);
                ImGui::Text("num condensed points: %d", numCondensedPoints);
            }
            ImGui::End();

            ImGui::Begin("Path");
            ImVec2 PathWindowSize = ImGui::GetWindowSize();
            PathWindowSize.x -= 16;
            PathWindowSize.y -= 36;
            ImGui::InputTextMultiline("Path", PathText, IM_ARRAYSIZE(PathText), PathWindowSize, ImGuiInputTextFlags_ReadOnly);
            ImGui::End();
            WritePath(numCondensedPoints);

#if !defined(ROTATE_360)
            static int item_selected_idx = -1; // Here we store our selected data as an index.
            int item_highlighted_idx = -1; // Here we store our highlighted data as an index.
            ImGui::Begin("Project");
            ImVec2 ProjectWindowSize = ImGui::GetWindowSize();
            ProjectWindowSize.x -= 16;
            ProjectWindowSize.y -= 36;
            if (ImGui::BeginListBox("Files", ProjectWindowSize))
            {
                for (int n = 0; n < files.size(); n++)
                {
                    const bool is_selected = (item_selected_idx == n);
                    if (ImGui::Selectable(files[n], is_selected))
                    {
                        item_selected_idx = n;
                        LoadPoints(points, MinDelta, files[n]);
                    }

                    if (ImGui::IsItemHovered())
                    {
                        item_highlighted_idx = n;
                    }

                    // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                    if (is_selected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }

                    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
                    {
                        // Set payload to carry the index of our item (could be anything)
                        ImGui::SetDragDropPayload("SPLINE_PATH_GENERATOR_PATH", &n, sizeof(int));

                        // Display preview (could be anything, e.g. when dragging an image we could decide to display
                        // the filename and a small preview of the image, etc.)
                        ImGui::EndDragDropSource();
                    }
                    if (ImGui::BeginDragDropTarget())
                    {
                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SPLINE_PATH_GENERATOR_PATH"))
                        {
                            IM_ASSERT(payload->DataSize == sizeof(int));
                            int payload_n = *(const int*)payload->Data;
                            char temp[MAX_PATH];
                            strcpy_s(temp, files[payload_n]);
                            auto temp2 = files.find(files[n]);
                            files.find_erase(files[payload_n]);
                            files.insert(temp2, temp);
                        }
                        ImGui::EndDragDropTarget();
                    }

                }        
                ImGui::EndListBox();
            }
            ImGui::End();
#endif
            static bool bLoad = false;
            static bool bSave = false;
            static bool bLoadProject = false;
            static bool bSaveProject = false;
            static bool bAddToProject = false;

            // Do a menu bar with an exit menu
            if (ImGui::BeginMenuBar())
            {
                if (ImGui::BeginMenu("File"))
                {
                    if (ImGui::MenuItem("Load Points"))
                    {
                        bLoad = true;
                    }
                    if (ImGui::MenuItem("Save Points"))
                    {
                        bSave = true;
                    }
                    if (ImGui::MenuItem("Exit"))
                    {
                        done = true;
                    }
                    ImGui::EndMenu();
                }

#if !defined(ROTATE_360)
                if (ImGui::BeginMenu("Project"))
                {
                    if (ImGui::MenuItem("Load Project"))
                    {
                        bLoadProject = true;
                    }
                    if (ImGui::MenuItem("Save Project"))
                    {
                        bSaveProject = true;
                    }
                    if (ImGui::MenuItem("Add Spline"))
                    {
                        bAddToProject = true;
                    }
                    if (ImGui::MenuItem("Remove Spline"))
                    {
                        // remove selected file from project
                        if (item_selected_idx != -1)
                        {
                            files.find_erase(files[item_selected_idx]);
                            points.clear();
                        }
                    }
                    ImGui::EndMenu();
                }
#endif                
                if (ImGui::BeginMenu("Export"))
                {
#if defined(ROTATE_360)
                    if (ImGui::MenuItem("PATH-360.BIN"))
#else
                    if (ImGui::MenuItem("PATHS.BIN"))
#endif
                    {
#if defined(ROTATE_360)
                        Export360Path(numCondensedPoints);
#else
                        ExportPaths(0xB400, files);
#endif
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }

            static ImGui::FileBrowser* fileDialog = nullptr;
            if (bLoad || bSave || bAddToProject)
            {
                if (fileDialog == nullptr)
                {
                    // create a file browser instance
                    fileDialog = new ImGui::FileBrowser((bLoad || bAddToProject) ? ImGuiFileBrowserFlags_EnterNewFilename : ImGuiFileBrowserFlags_EnterNewFilename | ImGuiFileBrowserFlags_CreateNewDir);
            
                    // (optional) set browser properties
                    fileDialog->SetTitle(bAddToProject ? "Add Spline to Project" : bLoad ? "Load Spline" : "Save Spline");
                    fileDialog->SetTypeFilters({ ".spg" });
                    fileDialog->Open();
                }

                if (fileDialog != nullptr)
                {
                    fileDialog->Display();
                    if(fileDialog->HasSelected())
                    {
                        if (bLoad)
                        {
                            LoadPoints(points, MinDelta, fileDialog->GetSelected().string().c_str());
                        }
                        else if (bSave)
                        {
                            SavePoints(points, MinDelta, fileDialog->GetSelected().string().c_str());
                        }
                        else if (bAddToProject)
                        {
                            char temp[MAX_PATH];
                            strcpy_s(temp, fileDialog->GetSelected().string().c_str());
                            files.push_back(temp);
                        }
                        fileDialog->Close();
                    }
                }
                if (fileDialog->IsOpened() == false)
                {
                    fileDialog->ClearSelected();
                    bLoad = bSave = bAddToProject = false;
                    delete(fileDialog);
                    fileDialog = nullptr;
                }
            }
            else if (bLoadProject || bSaveProject)
            {
                if (fileDialog == nullptr)
                {
                    // create a file browser instance
                    fileDialog = new ImGui::FileBrowser(bLoadProject ? ImGuiFileBrowserFlags_EnterNewFilename : ImGuiFileBrowserFlags_EnterNewFilename | ImGuiFileBrowserFlags_CreateNewDir);
            
                    // (optional) set browser properties
                    fileDialog->SetTitle(bLoad ? "Load Project" : "Save Project");
                    fileDialog->SetTypeFilters({ ".prj" });
                    fileDialog->Open();
                }

                if (fileDialog != nullptr)
                {
                    fileDialog->Display();
                    if(fileDialog->HasSelected())
                    {
                        if (bLoadProject)
                        {
                            files.clear();
                            LoadProject(files, fileDialog->GetSelected().string().c_str());
                        }
                        else if (bSaveProject)
                        {
                            auto temp = fileDialog->GetSelected().string();
                            if (temp.ends_with(".prj") == false)
                            {
                                temp.append(".prj");
                            }
                            SaveProject(files, temp.c_str());
                        }
                        fileDialog->Close();
                    }
                }
                if (fileDialog->IsOpened() == false)
                {
                    fileDialog->ClearSelected();
                    bLoadProject = bSaveProject = false;
                    delete(fileDialog);
                    fileDialog = nullptr;
                }
                
            }
        }
        ImGui::End();

        if (show_demo_window)
        {
            ImGui::ShowDemoWindow(&show_demo_window);
        }

        {
            ImGui::Begin("Canvas");
            static ImVec2 scrolling(0.0f, 0.0f);
            static bool opt_enable_grid = true;
            static bool opt_enable_context_menu = true;
            static bool adding_line = false;
            static bool moving_point = false;
            static bool context_open = false;
            static bool moving_points = false;

            // Using InvisibleButton() as a convenience 1) it will advance the layout cursor and 2) allows us to use IsItemHovered()/IsItemActive()
            ImVec2 canvas_p0 = ImGui::GetCursorScreenPos();      // ImDrawList API uses screen coordinates!
            ImVec2 canvas_sz = ImGui::GetContentRegionAvail();   // Resize canvas to what's available
            if (canvas_sz.x < 50.0f) canvas_sz.x = 50.0f;
            if (canvas_sz.y < 50.0f) canvas_sz.y = 50.0f;
            ImVec2 canvas_p1 = ImVec2(canvas_p0.x + canvas_sz.x, canvas_p0.y + canvas_sz.y);

            // Draw border and background color
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            draw_list->AddRectFilled(canvas_p0, canvas_p1, IM_COL32(50, 50, 50, 255));
            draw_list->AddRect(canvas_p0, canvas_p1, IM_COL32(255, 255, 255, 255));

            // This will catch our interactions
            ImGui::InvisibleButton("canvas", canvas_sz, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
            const bool is_hovered = ImGui::IsItemHovered(); // Hovered
            const bool is_active = ImGui::IsItemActive();   // Held
            const ImVec2 origin(canvas_p0.x + scrolling.x, canvas_p0.y + scrolling.y); // Lock scrolled origin
            const ImVec2 mouse_pos_in_canvas(io.MousePos.x - origin.x, io.MousePos.y - origin.y);

            static int hovered_point = -1;
            if (moving_point == false && context_open == false)
            {
                hovered_point = -1;
                int curr_point = 0;
                for (const ImVec2& point : points)
                {
                    if ((fabsf(point.x - mouse_pos_in_canvas.x) < 4.0f) && (fabsf(point.y - mouse_pos_in_canvas.y) < 4.0f))
                    {
                        hovered_point = curr_point;
                    }
                    curr_point++;
                }
            }

            if (moving_points)
            {
                MovePoints(points, io.MouseDelta);
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                {
                    moving_points = false;
                }
            }
            else
            {
                // Add first and second point
                if (is_hovered && !adding_line && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                {
                    if (hovered_point != -1)
                    {
                        moving_point = true;
                    }
                    else
                    {
                        if (points.empty())
                        {
                            points.push_back(mouse_pos_in_canvas);
                        }
                        points.push_back(mouse_pos_in_canvas);
                        adding_line = true;
                    }
                }
                if (moving_point)
                {
                    points[hovered_point] = mouse_pos_in_canvas;
                    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
                    {
                        moving_point = false;
                    }
                }
                if (adding_line)
                {
                    points.back() = mouse_pos_in_canvas;
                    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
                    {
                        adding_line = false;
                    }
                }

                // Pan (we use a zero mouse threshold when there's no context menu)
                // You may decide to make that threshold dynamic based on whether the mouse is hovering something etc.
                const float mouse_threshold_for_pan = opt_enable_context_menu ? -1.0f : 0.0f;
                if (is_active && ImGui::IsMouseDragging(ImGuiMouseButton_Right, mouse_threshold_for_pan))
                {
                    scrolling.x += io.MouseDelta.x;
                    scrolling.y += io.MouseDelta.y;
                }

                // Context menu (under default mouse threshold)
                ImVec2 drag_delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Right);
                if (opt_enable_context_menu && drag_delta.x == 0.0f && drag_delta.y == 0.0f)
                {
                    ImGui::OpenPopupOnItemClick("context", ImGuiPopupFlags_MouseButtonRight);
                }
                if (ImGui::BeginPopup("context"))
                {
                    context_open = true;
                    if (adding_line)
                    {
                        points.resize(points.size() - 1);
                    }
                    adding_line = false;
                    moving_point = false;
                    if (ImGui::MenuItem("Remove this", NULL, false, hovered_point != -1 && points.Size > hovered_point)) { points.erase(&points[hovered_point]); }
                    if (ImGui::MenuItem("Remove one", NULL, false, points.Size > 0)) { points.resize(points.size() - 1); }
                    if (ImGui::MenuItem("Remove all", NULL, false, points.Size > 0)) { points.clear(); scrolling.x = 0; scrolling.y = 0; }
                    if (ImGui::MenuItem("Reverse points array", NULL, false, points.Size > 0)) { std::ranges::reverse(points); scrolling.x = 0; scrolling.y = 0; }
                    if (ImGui::MenuItem("Flip X", NULL, false, points.Size > 0)) { FlipPointsX(points); scrolling.x = 0; scrolling.y = 0; }
                    if (ImGui::MenuItem("Flip Y", NULL, false, points.Size > 0)) { FlipPointsY(points); scrolling.x = 0; scrolling.y = 0; }
                    if (ImGui::MenuItem("Move Spline", NULL, false, points.Size > 0)) { moving_points = true; }
                    if (ImGui::MenuItem("Scale Spline", NULL, false, points.Size > 0)) { ScalePoints(points, ScaleFactor); }
                    ImGui::EndPopup();
                    if (ImGui::IsPopupOpen("context") == false)
                    {
                        context_open = false;
                    }
                }
            }

            // Draw grid + all lines in the canvas
            draw_list->PushClipRect(canvas_p0, canvas_p1, true);
            if (opt_enable_grid)
            {
                const float GRID_STEP = 64.0f;
                for (float x = fmodf(scrolling.x, GRID_STEP); x < canvas_sz.x; x += GRID_STEP)
                {
                    draw_list->AddLine(ImVec2(canvas_p0.x + x, canvas_p0.y), ImVec2(canvas_p0.x + x, canvas_p1.y), IM_COL32(200, 200, 200, 40));
                }
                for (float y = fmodf(scrolling.y, GRID_STEP); y < canvas_sz.y; y += GRID_STEP)
                {
                    draw_list->AddLine(ImVec2(canvas_p0.x, canvas_p0.y + y), ImVec2(canvas_p1.x, canvas_p0.y + y), IM_COL32(200, 200, 200, 40));
                }
            }

            for (int n = 0; n < points.Size-1; n += 1)
            {
                draw_list->AddLine(ImVec2(origin.x + points[n].x, origin.y + points[n].y), ImVec2(origin.x + points[n + 1].x, origin.y + points[n + 1].y), col, 2.0f);
            }

            if (points.size() > 3)
            {
                ImVector<ImVec2> linePoints;
                for (int i = 0; i < numCondensedPoints; i++)
                {
                    linePoints.push_back({CondensedPath[i].pos.x + origin.x, CondensedPath[i].pos.y + origin.y});
                }
                if (CondensedPath[numCondensedPoints-1].command_data > 1)
                {
                    linePoints.push_back({CondensedPath[numCondensedPoints].pos.x + origin.x, CondensedPath[numCondensedPoints].pos.y + origin.y});
                }
                draw_list->AddPolyline(linePoints.begin(), linePoints.size(), col3, 0, 1);
                int curr_point = 0;
                for (const auto& point : points)
                {
                    ImVec2 new_point;
                    new_point.x = point.x + origin.x;
                    new_point.y = point.y + origin.y;

                    float thickness = 1.0f;
                    if (hovered_point == curr_point)
                    {
                        thickness = 3.0f;
                    }
                    draw_list->AddCircle(new_point, 3.0f, col2, 16, thickness);
                    curr_point++;
                }
            }
            draw_list->PopClipRect();
            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Update and Render additional Platform Windows
        // (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
        //  For this specific demo app we could also call SDL_GL_MakeCurrent(window, gl_context) directly)
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
            SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
        }

        SDL_GL_SwapWindow(window);
    }
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_END;
#endif

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
