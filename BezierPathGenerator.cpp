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

// This example can also compile and run with Emscripten! See 'Makefile.emscripten' for details.
#ifdef __EMSCRIPTEN__
#include "../libs/emscripten/emscripten_mainloop_stub.h"
#endif

//#define _CRT_SECURE_NO_WARNINGS    
#include <corecrt_math.h>
//#include <cstdio>

#include "Paths.h"

struct Point
{
    double x, y;

    Point()
        : x(0.0), y(0.0)
    {}

    Point(const double NewX, const double NewY)
        : x(NewX), y(NewY)
    {}

    Point operator += (const Point& other)
    {
        x += other.x;
        y += other.y;
        return { x,y };
    }
};

double Length(const Point v)
{
    return sqrt(v.x * v.x + v.y * v.y);
}

Point Normalize(const Point v)
{
    Point Result;

    const double ThisLength = Length(v);

    Result.x = v.x / ThisLength;
    Result.y = v.y / ThisLength;

    return Result;
}

Point cubic_bezier(Point p0, Point p1, Point p2, Point p3, double t) {
    Point result;
    double u = 1 - t;
    double tt = t * t;
    double uu = u * u;
    double uuu = uu * u;
    double ttt = tt * t;

    result.x = uuu * p0.x; //beginning influence
    result.x += 3 * uu * t * p1.x; // first middle point influence
    result.x += 3 * u * tt * p2.x; // second middle point influence
    result.x += ttt * p3.x; // ending influence

    result.y = uuu * p0.y;
    result.y += 3 * uu * t * p1.y;
    result.y += 3 * u * tt * p2.y;
    result.y += ttt * p3.y;

    return result;
}

Point cubic_bezier_derivative(Point p0, Point p1, Point p2, Point p3, double t) {
    Point result;
    double u = 1 - t;
    double uu = u * u;
    double tt = t * t;

    result.x = 3 * uu * (p1.x - p0.x) +
        6 * u * t * (p2.x - p1.x) +
        3 * tt * (p3.x - p2.x);

    result.y = 3 * uu * (p1.y - p0.y) +
        6 * u * t * (p2.y - p1.y) +
        3 * tt * (p3.y - p2.y);

    return result;
}

struct PathPoint
{
    int command;
    int command_data;
    ImVec2 pos;
    int deltaX;
    int deltaY;
    int rotation;
};

PathPoint Path[500];

int GeneratePath(const Point p1, const Point c1, const Point c2, const Point p2, const double StepT, const double MinDelta)
{
    Point LastPoint;
    Point TotalDelta;
    int i = 0;

    for (double t = 0.0f; t <= 1.00001f; t += StepT)
    {
        const Point CurrentPoint = cubic_bezier(p1, c1, c2, p2, t);
        const Point PathTangent = cubic_bezier_derivative(p1, c1, c2, p2, t);

        double PathDirection = (atan2(PathTangent.y, PathTangent.x) * 3.82f); // DotProduct(Point(0.0f, 1.0f), PathTangent);
        if (PathDirection > 12.0f)
        {
            PathDirection -= 24.0f;
        }
        if (t == 0.0f)
        {
            LastPoint = CurrentPoint;
        }
        const Point Delta = Point(CurrentPoint.x - LastPoint.x, CurrentPoint.y - LastPoint.y);

        TotalDelta += Delta;
        if (Length(TotalDelta) > MinDelta)
        {
            //printf("    1, 1, %3d, %3d, %2d,    %3d, %3d, %3d %3d, %3d\n", static_cast<int>(TotalDelta.y * 50.0f), -static_cast<int>(TotalDelta.x * 50.0f), (static_cast<int>(PathDirection) + 12),
            //    static_cast<int>(CurrentPoint.x * 1.0f), static_cast<int>(CurrentPoint.y * 1.0f), static_cast<int>(PathTangent.x * 100.0f), static_cast<int>(PathTangent.y * 100.0f), static_cast<int>(PathDirection));
            if (i < 500)
            {
                Path[i++] = {1, 1,  {static_cast<float>(CurrentPoint.y * 50.0f), -static_cast<float>(CurrentPoint.x * 50.0f)}, static_cast<int>(TotalDelta.y * 50.0f), -static_cast<int>(TotalDelta.x * 50.0f), (static_cast<int>(PathDirection) + 12)};
            }
            TotalDelta = { 0.0f, 0.0f };
        }

        LastPoint = CurrentPoint;
    }

    return i;
}


void ExportPaths()
{
    FILE* output = fopen("PATHS.BIN", "wb");
    if (output != nullptr)
    {
        int bytesWritten = fwrite(&num_paths, 2, 1, output);

        unsigned short offset = 0xA800 + 2 + (num_paths * 2);
        bytesWritten += fwrite(&offset, 2, 1, output);
        offset += sizeof(path0);
        bytesWritten += fwrite(&offset, 2, 1, output);
        offset += sizeof(path1);
        bytesWritten += fwrite(&offset, 2, 1, output);
        offset += sizeof(path2);
        bytesWritten += fwrite(&offset, 2, 1, output);
        offset += sizeof(path3);
        bytesWritten += fwrite(&offset, 2, 1, output);
        offset += sizeof(path4);
        bytesWritten += fwrite(&offset, 2, 1, output);

        bytesWritten += fwrite(&path0, 1, sizeof(path0), output);
        bytesWritten += fwrite(&path1, 1, sizeof(path1), output);
        bytesWritten += fwrite(&path2, 1, sizeof(path2), output);
        bytesWritten += fwrite(&path3, 1, sizeof(path3), output);
        bytesWritten += fwrite(&path4, 1, sizeof(path4), output);
        bytesWritten += fwrite(&path5, 1, sizeof(path5), output);

        int result = fclose(output);
        printf("Bytes Written: %d  fclose result: %d \n", bytesWritten, result);
    }
}

int CalcPath(float in_p1[2], float in_c1[2], float in_c2[2], float in_p2[2], float MinDelta)
{
    // const Point p1 = Point(0.0, 1.0);
    // const Point c1 = Point(3.0, 1.0);
    // const Point c2 = Point(3.0, 4.0);
    // const Point p2 = Point(6.0, 4.0);
    //const Point p1 = Point(6.0, 3.0);
    //const Point c1 = Point(2.0, 5.0);
    //const Point c2 = Point(-1.0, 1.0);
    //const Point p2 = Point(4.0, 2.0);
    //const Point p1 = Point(10.0, 6.0);
    //const Point c1 = Point(2.0, 9.0);
    //const Point c2 = Point(-1.0, 0.0);
    //const Point p2 = Point(6.0, 3.0);
    const Point p1 = Point(in_p1[0], in_p1[1]);
    const Point c1 = Point(in_c1[0], in_c1[1]);
    const Point c2 = Point(in_c2[0], in_c2[1]);
    const Point p2 = Point(in_p2[0], in_p2[1]);
    return GeneratePath(p1, c1, c2, p2, 0.0001, MinDelta);
}

void WritePath(int numPathPoints)
{
    PathPoint Condensed[500];
    Condensed[0] = Path[0];
    int condensedIndex = 0;
    for (int i = 1; i < numPathPoints; i++)
    {
        if (Path[i].deltaX == Condensed[condensedIndex].deltaX && Path[i].deltaY == Condensed[condensedIndex].deltaY && Path[i].rotation == Condensed[condensedIndex].rotation)
        {
            Condensed[condensedIndex].command_data++;
        }
        else
        {
            condensedIndex++;
            Condensed[condensedIndex] = Path[i];
        }
    }
    printf("%d\n", condensedIndex);
    for (int i = 0; i < condensedIndex; i++)
    {
        printf("%1d, %1d, %3d, %3d, %2d\n", Condensed[i].command, Condensed[i].command_data, Condensed[i].deltaX, Condensed[i].deltaY, Condensed[i].rotation);
    }
    printf("0, 0,   0,   0,  0\n");
    (void)fflush(stdout);
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
    SDL_Window* window = SDL_CreateWindow("Dear ImGui SDL2+OpenGL3 example", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
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
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows
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
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

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

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
        {
            static float f = 0.0f;
            static int counter = 0;

            ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

            ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
            ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
            ImGui::Checkbox("Another Window", &show_another_window);

            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
            ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

            if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::End();
        }

        // 3. Show another simple window.
        if (show_another_window)
        {
            ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
            ImGui::Text("Hello from another window!");
            if (ImGui::Button("Close Me"))
                show_another_window = false;
            ImGui::End();
        }

        {
            static float p1[2] = {10.0f, 6.0f};
            static float c1[2] = {2.0f, 9.0f};
            static float c2[2] = {-1.0f, 0.0f};
            static float p2[2] = {6.0f, 3.0f};
            static float MinDelta = 0.25f;
            int numPathPoints = CalcPath(p1,c1,c2,p2,MinDelta);
            ImGui::Begin("Curve Params");
            ImGui::DragFloat2("p1", p1, 0.01f, -20.0f, 20.0f);
            ImGui::DragFloat2("c1", c1, 0.01f, -20.0f, 20.0f);
            ImGui::DragFloat2("c2", c2, 0.01f, -20.0f, 20.0f);
            ImGui::DragFloat2("p2", p2, 0.01f, -20.0f, 20.0f);
            ImGui::DragFloat("MinDelta", &MinDelta, 0.01f, 0.1f, 2.0f);
            ImGui::DragInt("num points", &numPathPoints);
            static bool clicked = false;
            if (ImGui::Button("Write Path"))
            {
                clicked = true;
            }
            if (clicked == true)
            {
                WritePath(numPathPoints);
                clicked = false;
            }            
            ImGui::End();

            ImGui::Begin("Curve!");
            const ImVec2 p = ImGui::GetCursorScreenPos();
            float x = p.x + 4.0f;
            float y = p.y + 504.0f;
            ImVec2 linePoints[500];
            for (int i = 0; i < numPathPoints; i++)
            {
                linePoints[i].x = Path[i].pos.x + x;
                linePoints[i].y = Path[i].pos.y + y;
            }
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            static float sz = 10.0f;
            //const ImVec2 cp4[4] = { ImVec2(0.0f, 0.0f), ImVec2(sz * 1.3f, sz * 0.3f), ImVec2(sz - sz * 1.3f, sz - sz * 0.3f), ImVec2(sz, sz) };
            const ImVec2 cp4[4] = { ImVec2(10.0f * sz, 6.0f * sz), ImVec2(2.f * sz,  9.f * sz), ImVec2( -1.f * sz, 0.f * sz), ImVec2(6.f * sz, 3.f * sz) };
            const ImU32 col = ImColor(1.0f, 1.0f, 0.4f, 1.0f);
            const ImU32 col2 = ImColor(1.0f, 1.0f, 1.0f, 1.0f);
            //draw_list->AddBezierCubic(ImVec2(x + cp4[0].x, y + cp4[0].y), ImVec2(x + cp4[1].x, y + cp4[1].y), ImVec2(x + cp4[2].x, y + cp4[2].y), ImVec2(x + cp4[3].x, y + cp4[3].y), col, 1, 0);
            draw_list->AddPolyline(linePoints, numPathPoints, col, 0, 1);
            draw_list->AddLine(ImVec2(p1[1] * 50.f + x, -p1[0] * 50.f + y), ImVec2(c1[1] * 50.f + x, -c1[0] * 50.f + y), col2, 1);
            draw_list->AddLine(ImVec2(p2[1] * 50.f + x, -p2[0] * 50.f + y), ImVec2(c2[1] * 50.f + x, -c2[0] * 50.f + y), col2, 1);
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
