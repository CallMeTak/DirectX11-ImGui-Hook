#include "pch.h"


// TypeDef for D3D11's Present function
typedef  HRESULT(__fastcall* fnPresent)(IDXGISwapChain* pThis, UINT SyncInterval, UINT Flags);

// Global variables for all the thing's we'll need

// Original Present function
fnPresent ogPresent = nullptr;
// DirectX device used for rendering
ID3D11Device* g_pDevice = nullptr;
// DirectX context
ID3D11DeviceContext* g_pContext = nullptr;
// Window of application we're injected into. This gets assigned in hkPresent
HWND g_hWindow = nullptr;
// Address of the original WndProc
LONG_PTR oWndProc = NULL;
// Handle to stdout
HANDLE g_hStdOutput = nullptr;
bool g_bIsImGuiInit = false;
bool g_bShowImGui = true;
// Forward declaration so these methods can be seen before their definition
namespace Hooks {
    void HookWndProc();
    void RestoreWndProc();
    bool HookPresent(uintptr_t presentAddr);
    bool UnHookPresent();

}

// Present hook where we perform our own processing and then call the real Present function
HRESULT hkPresent(IDXGISwapChain* swapchain, UINT SyncInterval, UINT Flags) {

    // Just in case we're given an invalid swapchain. Tbh I have no idea how this could
    // happen or if it does at all
    if (!swapchain) { return ogPresent(swapchain, SyncInterval, Flags); }

    // Ensures ImGui's init functions only run once
    if (!g_bIsImGuiInit) {

        // Gets D3D11 device
        if SUCCEEDED(swapchain->GetDevice(IID_ID3D11Device, (void**)(&g_pDevice))) {
            g_pDevice->GetImmediateContext(&g_pContext);

            // Return if context invalid
            // It likely isn't but it would always be invalid when I incorrectly used IID_DXGIDEVICE
            if (!g_pContext) { return ogPresent(swapchain, SyncInterval, Flags); }

            // SwapChain description contains lots of info we may need
            // For now we only need it to get the OutputWindow below
            // and output resolution
            DXGI_SWAP_CHAIN_DESC sd;
            swapchain->GetDesc(&sd);

            // Gets the application's output window to pass to ImGui
            g_hWindow = sd.OutputWindow;
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();

            // Reference to ImGuiIO allowing us to modify, ImGui's configuration
            ImGuiIO& io = ImGui::GetIO();

            // Set's display size to the swapchain's resolution
            io.DisplaySize = {
                static_cast<float>(sd.BufferDesc.Width),
                static_cast<float>(sd.BufferDesc.Height)
                    };
            // Enables keyboard input
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

            // Enables the WndProc hook so ImGui can receive window input events
            Hooks::HookWndProc();
            ImGui_ImplWin32_Init(g_hWindow);
            ImGui_ImplDX11_Init(g_pDevice, g_pContext);
            g_bIsImGuiInit = true;

        }

    }
    // Only render ImGui if it's been initialized first and if user wants to show
    if (g_bIsImGuiInit && g_bShowImGui) {
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        
        // You can draw whatever you want here
        // For now we just show the Demo window
        ImGui::ShowDemoWindow();

        ImGui::Render();
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }

    // Calls the original Present function
    return ogPresent(swapchain, SyncInterval, Flags);
}

// Our own callback for WndProc where we handle keyboard and mouse events
LRESULT CALLBACK WndProcCallback(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    // Only pass events to ImGui if we want to show the window
    if (g_bShowImGui) {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
            return true;
    }

    switch (uMsg) {
        // Can read mouse and keyboard inputs here
    case WM_KEYDOWN:
        switch (wParam)
        {
            // Delete key toggles whether to show ImGui or not
        case VK_DELETE:
            g_bShowImGui = !g_bShowImGui;
        default:
            break;
        }
    // Disables mouse input to application when ImGui is open
    // However, this may not work in some games as some don't use WndProc for game input
    // Could try to hook PostMessage, SetCursorPos, GetCursorPos, ShowCursor, and ClipCursor
    case WM_CAPTURECHANGED:
    case WM_LBUTTONDBLCLK:
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDBLCLK:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_MOUSEACTIVATE:
    case WM_MOUSEHOVER:
    case WM_MOUSEHWHEEL:
    case WM_MOUSELEAVE:
    case WM_MOUSEMOVE:
    case WM_MOUSEWHEEL:
    case WM_NCHITTEST:
    case WM_NCLBUTTONDBLCLK:
    case WM_NCLBUTTONDOWN:
    case WM_NCLBUTTONUP:
    case WM_NCMBUTTONDBLCLK:
    case WM_NCMBUTTONDOWN:
    case WM_NCMBUTTONUP:
    case WM_NCMOUSEHOVER:
    case WM_NCMOUSELEAVE:
    case WM_NCMOUSEMOVE:
    case WM_NCRBUTTONDBLCLK:
    case WM_NCRBUTTONDOWN:
    case WM_NCRBUTTONUP:
    case WM_NCXBUTTONDBLCLK:
    case WM_NCXBUTTONDOWN:
    case WM_NCXBUTTONUP:
    case WM_RBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_XBUTTONDBLCLK:
    case WM_XBUTTONDOWN:
    case WM_XBUTTONUP:
    case WM_LBUTTONUP:
        if (g_bShowImGui) {
            return true;
        }
        break;
    default:
        break;
    }

    // Call the original WndProc
    return CallWindowProc((WNDPROC)oWndProc, hWnd, uMsg, wParam, lParam);
}

// Not actually needed
// Used in GetPresent to find Present without explicitly stating that it's
// at index 8
enum class IDXGISwapChainVMT {
    QueryInterface,
    AddRef,
    Release,
    SetPrivateData,
    SetPrivateDataInterface,
    GetPrivateData,
    GetParent,
    GetDevice,
    Present,
    GetBuffer,
    SetFullscreenState,
    GetFullscreenState,
    GetDesc,
    ResizeBuffers,
    ResizeTarget,
    GetContainingOutput,
    GetFrameStatistics,
    GetLastPresentCount,
};
namespace Hooks {
    // Replace the application's WndProc handler with our own
    void HookWndProc() {
        oWndProc = SetWindowLongPtr(
            g_hWindow,
            GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&WndProcCallback));
    }

    // Restore original WndProc
    void RestoreWndProc() {
        
        SetWindowLongPtr(g_hWindow, GWLP_WNDPROC, oWndProc);
    }
    bool HookPresent(uintptr_t presentAddr) {
        // Store the original Present in ogPresent
        ogPresent = (fnPresent)presentAddr;
        // Using Detours will Hook the original Present function to redirect to our hkPresent
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttach(&(PVOID&)ogPresent, &hkPresent);
        auto err = DetourTransactionCommit();
        if (err != 0) {
            std::cout << "Error hooking Present: " << err << "\n";
            return false;
        }
        return true;
    }

    bool UnHookPresent() {
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourDetach(&(PVOID&)ogPresent, &hkPresent);
        auto err = DetourTransactionCommit();
        if (err != 0) {
            std::cout << "Error unhooking Present: " << err << "\n";
            return false;
        }
        return true;
    }
 

}
namespace Util {
    void CreateConsoleWindow() {
        // Get original output device handle to restore later
        g_hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
        // Creates a console window
        AllocConsole();
        FILE* f;
        // Redirects output stream to this console window
        freopen_s(&f, "CONOUT$", "w", stdout);
    }
    void CloseConsoleWindow() {
        // Close std output, close console, and set the handle to the original stdout
        fclose(stdout);
        FreeConsole();
        SetStdHandle(STD_OUTPUT_HANDLE, g_hStdOutput);

    }
    uintptr_t GetPresent() {

        DXGI_SWAP_CHAIN_DESC scDesc{ {0} };
        // All of this is just dummy info needed to create the dummy device to get its swapchain
        scDesc.BufferCount = 1;
        scDesc.Windowed = TRUE;
        scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        scDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        scDesc.BufferDesc.Width = 1920;
        scDesc.BufferDesc.Height = 1080;
        scDesc.BufferDesc.RefreshRate = { 120, 1 };
        scDesc.OutputWindow = GetForegroundWindow();
        scDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
        scDesc.SampleDesc.Count = 1;
        scDesc.SampleDesc.Quality = 0;
        IDXGISwapChain* pDummySwapChain = nullptr;
        ID3D11Device* pDummyDevice = nullptr;
        D3D_FEATURE_LEVEL ftLevel;
        HRESULT result = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0, D3D11_SDK_VERSION, &scDesc, &pDummySwapChain, &pDummyDevice, &ftLevel, nullptr);
        if FAILED(result) {
            return 0;
        }
        
        // Gets the address of the swapchain's virtual method table
        // The VMT is the first thing defined inside the object so the address of the object is
        // the same as the VMT
        void** pSwapChainVMT = *(void***)(pDummySwapChain);

        // Releases our dummy SwapChain and Device as they're no longer needed
        pDummySwapChain->Release();
        pDummyDevice->Release();

        // returns the address to the swapchain
        return reinterpret_cast<uintptr_t>(pSwapChainVMT[(UINT)IDXGISwapChainVMT::Present]);
    }
}





BOOL WINAPI HookThread(HMODULE hModule) {
    using namespace std::chrono_literals;

    uintptr_t present = Util::GetPresent();
    Util::CreateConsoleWindow();
    std::cout << "Injected.\n";
    // Exit if we can't get the Present address
    // or if we fail to hook it
    if (present == 0 || !Hooks::HookPresent(present)) {
        // Sleep for 10 seconds so user can read errors in console
        std::this_thread::sleep_for(10s);
        Util::CloseConsoleWindow();
        return false;
    }
    std::cout << "Present Hooked.\n";
    // Unloads dll if END is pressed
    while (true) {
        std::this_thread::sleep_for(1ms);
        if (GetAsyncKeyState(VK_END) & 1) {
            // Restores the original WndProc address
            Hooks::RestoreWndProc();
            // Removes D3D11 Present Hook
            Hooks::UnHookPresent();
            // Close console
            Util::CloseConsoleWindow();
            // Shutdown ImGui
            if (g_bIsImGuiInit) {
                ImGui_ImplDX11_Shutdown();
                ImGui_ImplWin32_Shutdown();
                ImGui::DestroyContext();
            }

            FreeLibraryAndExitThread(hModule, 0);
        }
    }
    return true;
}
BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        CreateThread(
            0,
            0,
            (LPTHREAD_START_ROUTINE)HookThread,
            hModule,
            0,
            0);
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:

        break;
    }
    return TRUE;
}