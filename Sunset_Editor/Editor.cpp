#include "Editor.h"
#include "Sunset_Editor_pch.h"

#include <Windows.h>
#include <tchar.h>

#include <d3d12.h>
#include <dxgi1_6.h>

#include "../../Sunset_Graphics/SunsetGraphics/SunsetGraphics.h"
#include "../../Sunset_Graphics/SunsetGraphics/Direct3D12/Direct3D_12.h"

#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_impl_win32.h"
#include "ImGui/imgui_impl_dx12.h"

#pragma comment(lib, "./../x64/Debug/GameEngine/Sunset_Graphics.lib")

static HINSTANCE hinst;
static HWND hwndMain;

static FrameBuffer* frameBuffer = nullptr;
static DX12::Command* pCmd = nullptr;

// Simple free list based allocator
class ExampleDescriptorHeapAllocator : 
    public DX12::ShaderResourceView
{
public:
    ExampleDescriptorHeapAllocator(UINT numSRV) :
        DX12::ShaderResourceView(numSRV), m_numSRV(numSRV)
    {

    }

    void Create(ID3D12Device* device)
    {
        IM_ASSERT(m_heap != nullptr && FreeIndices.empty());

        D3D12_DESCRIPTOR_HEAP_DESC desc = m_heap->GetDesc();
        HeapType = desc.Type;

        HeapStartCpu = this->GetCPUDescriptorHandle();
        HeapStartGpu = this->GetGPUDescriptorHandle();

        HeapHandleIncrement = device->GetDescriptorHandleIncrementSize(HeapType);

        FreeIndices.reserve((int)desc.NumDescriptors);
        for (int n = desc.NumDescriptors; n > 0; n--) {
            FreeIndices.push_back(n);
        }
    }

    void Destroy()
    {
        m_heap.Reset();
        FreeIndices.clear();
    }

    void Alloc(D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_desc_handle)
    {
        IM_ASSERT(FreeIndices.Size > 0);
        int idx = FreeIndices.back();
        FreeIndices.pop_back();
        out_cpu_desc_handle->ptr = HeapStartCpu.ptr + (idx * HeapHandleIncrement);
        out_gpu_desc_handle->ptr = HeapStartGpu.ptr + (idx * HeapHandleIncrement);
    }

    void Free(D3D12_CPU_DESCRIPTOR_HANDLE out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE out_gpu_desc_handle)
    {
        int cpu_idx = (int)((out_cpu_desc_handle.ptr - HeapStartCpu.ptr) / HeapHandleIncrement);
        int gpu_idx = (int)((out_gpu_desc_handle.ptr - HeapStartGpu.ptr) / HeapHandleIncrement);
        IM_ASSERT(cpu_idx == gpu_idx);
        FreeIndices.push_back(cpu_idx);
    }

    void SetImGui_InitInfo_SRV(ImGui_ImplDX12_InitInfo* init_info)
    {
        init_info->SrvDescriptorHeap = m_heap.Get();
    }

    void SetCommandList(ID3D12GraphicsCommandList* pCmdList) 
    {
        pCmdList->SetDescriptorHeaps(1, m_heap.GetAddressOf());
    }

    UINT m_numSRV;

    D3D12_DESCRIPTOR_HEAP_TYPE  HeapType = D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;

    D3D12_CPU_DESCRIPTOR_HANDLE HeapStartCpu;
    D3D12_GPU_DESCRIPTOR_HANDLE HeapStartGpu;

    UINT                        HeapHandleIncrement;

    ImVector<int>               FreeIndices;
};

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#pragma comment(lib, "dxgi.lib")

LRESULT CALLBACK WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

//------------------------------------------------------------------------------------------------
// 内容　　GameEngine メイン関数
// 引数１　HINSTANCE__* インスタンスハンドル
// 引数２　int ウィンドウ Showコマンド
// 戻り値　int
//------------------------------------------------------------------------------------------------
int __stdcall PROJECT_MAIN_FUNC(HINSTANCE__* hInstance, int nShowCmd)
{
    if (EditorEngine_Init() != TRUE) return 4;
    
    WNDCLASS wc;
    wc.style            = 0;
    wc.lpfnWndProc      = (WNDPROC)WndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = hInstance;
    wc.hIcon            = LoadIcon((HINSTANCE)NULL, IDI_APPLICATION);
    wc.hCursor          = LoadCursor((HINSTANCE)NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH)(COLOR_WINDOW);
    wc.lpszMenuName     = L"MainMenu";
    wc.lpszClassName    = L"MainWndClass";

    if (!RegisterClass(&wc))
        return FALSE;

    hinst = hInstance;  // save instance handle 

    hwndMain = CreateWindow(
        _T("MainWndClass"), _T("MainWindow"),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT, 
        (HWND)NULL, (HMENU)NULL, 
        hinst, (LPVOID)NULL
    );

    if (!hwndMain)
        return FALSE;

    /* Command関連　生成 */
    pCmd = new DX12::Command;
    pCmd->Create();

    frameBuffer = new FrameBuffer(hwndMain, pCmd->GetCommandQueue());

    ShowWindow(hwndMain, SW_SHOW);
    UpdateWindow(hwndMain);


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
    ImGui_ImplWin32_Init(hwndMain);

    ImGui_ImplDX12_InitInfo init_info = {};
    init_info.Device = DX12::GetDevice();
    init_info.CommandQueue = pCmd->GetCommandQueue();
    init_info.NumFramesInFlight = 2;
    init_info.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    init_info.DSVFormat = DXGI_FORMAT_UNKNOWN;

    static ExampleDescriptorHeapAllocator g_pd3dSrvDescHeapAlloc(64);
    g_pd3dSrvDescHeapAlloc.Create(DX12::GetDevice());

    g_pd3dSrvDescHeapAlloc.SetImGui_InitInfo_SRV(&init_info);
    init_info.SrvDescriptorAllocFn = [](
        ImGui_ImplDX12_InitInfo*, 
        D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_handle, 
        D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_handle) {
            return g_pd3dSrvDescHeapAlloc.Alloc(out_cpu_handle, out_gpu_handle); 
        };
    init_info.SrvDescriptorFreeFn = [](
        ImGui_ImplDX12_InitInfo*,
        D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle,
        D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle) {
            return g_pd3dSrvDescHeapAlloc.Free(cpu_handle, gpu_handle);
        };

    ImGui_ImplDX12_Init(&init_info);

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    MSG msg{};
    bool done = false;
    while (!done)
    {
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done) break;

        if (!frameBuffer->PresentTest()) {
            Sleep(10);
            continue;
        }

        // Start the Dear ImGui frame
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;

        ImGuiViewport* viewport = ImGui::GetMainViewport();

        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
       
        window_flags |= ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
            window_flags |= ImGuiWindowFlags_NoBackground;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(1.0f, 0.0f));
        ImGui::Begin("Root", nullptr, window_flags);
        ImGui::PopStyleVar();
        ImGui::PopStyleVar(2);

        // Dockspace
        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
        {
            ImGuiID dockspace_id = ImGui::GetID("Root");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

            /*static auto first_time = true;
            if (first_time)
            {
                first_time = false;

                ImGui::DockBuilderRemoveNode(dockspace_id);
                ImGui::DockBuilderAddNode(dockspace_id, dockspace_flags | ImGuiDockNodeFlags_DockSpace);
                ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);

                auto dock_id_left = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.2f, nullptr, &dockspace_id);
                auto dock_id_right = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 1.0f, nullptr, &dockspace_id);

                ImGui::DockBuilderDockWindow("Sidebar", dock_id_left);
                ImGui::DockBuilderDockWindow("Content", dock_id_right);
                ImGui::DockBuilderFinish(dockspace_id);
            }*/
        }

        if (ImGui::BeginMenuBar())
        {
           /* if (ImGui::BeginMenu("Options"))
            {
                static bool a = true;
                if (ImGui::MenuItem("Close", NULL, false, a != NULL)) {

                }

                ImGui::Separator();

                if (ImGui::MenuItem("Flag: NoDockingOverCentralNode", "", (dockspace_flags & ImGuiDockNodeFlags_NoDockingOverCentralNode) != 0)) { dockspace_flags ^= ImGuiDockNodeFlags_NoDockingOverCentralNode; }
                if (ImGui::MenuItem("Flag: NoDockingSplit", "", (dockspace_flags & ImGuiDockNodeFlags_NoDockingSplit) != 0)) { dockspace_flags ^= ImGuiDockNodeFlags_NoDockingSplit; }
                if (ImGui::MenuItem("Flag: NoUndocking", "", (dockspace_flags & ImGuiDockNodeFlags_NoUndocking) != 0)) { dockspace_flags ^= ImGuiDockNodeFlags_NoUndocking; }
                if (ImGui::MenuItem("Flag: NoResize", "", (dockspace_flags & ImGuiDockNodeFlags_NoResize) != 0)) { dockspace_flags ^= ImGuiDockNodeFlags_NoResize; }
                if (ImGui::MenuItem("Flag: AutoHideTabBar", "", (dockspace_flags & ImGuiDockNodeFlags_AutoHideTabBar) != 0)) { dockspace_flags ^= ImGuiDockNodeFlags_AutoHideTabBar; }

                ImGui::Separator();

                ImGui::EndMenu();
            }*/

            ImGui::EndMenuBar();
        }

        ImGui::End();
        
        /* ImGui Render Main */
        {
            if (ImGui::Begin("aaa")) {



                ImGui::End();
            }
        }

        // Rendering
        ImGui::Render();

        pCmd->Begin(frameBuffer);
        ID3D12GraphicsCommandList* cmdList = pCmd->GetCommandList();

        if (frameBuffer->Begin(cmdList)){

            g_pd3dSrvDescHeapAlloc.SetCommandList(cmdList);
            ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList);

            frameBuffer->End(pCmd->GetCommandQueue(), cmdList);
        }
        else {
            pCmd->Close();

            //frameBuffer->ResizeView();
        }

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }

        pCmd->Wait(frameBuffer);
    }

    pCmd->WaitForLastSubmittedFrame(frameBuffer);

    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    g_pd3dSrvDescHeapAlloc.Destroy();

    delete(frameBuffer);
    delete(pCmd);

    if (EditorEngine_CleanUp() != TRUE) return 5;
    
	return 0;
}

BOOL EditorEngine_Init(void)
{
	static BOOL Complete = FALSE;
    if (Complete == TRUE) return TRUE;

    BOOL Success = TRUE;
    Success &= Sunset_Graphics_Init();
    
    //初期化全体の判定
    if (Success == TRUE) {
        //失敗無し
        Complete = TRUE; //完了にする
    }

	return Complete;
}

void EditorEngine_Update(void)
{

}

void EditorEngine_Render(void)
{

}

BOOL EditorEngine_CleanUp(void)
{
    static BOOL Complete = FALSE;
    if (Complete == TRUE) return TRUE;

    BOOL Success = TRUE;

    Success &= Sunset_Graphics_CleanUp();   //グラフィック系の初期化

    /* 解放全体の判定 */
    if (Success == TRUE) {
        /* 失敗無し */

        Complete = TRUE; /* 完了にする */
    }

	return (Complete);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) 
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, Msg, wParam, lParam)) 
        return true;

    switch (Msg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    case WM_CREATE:
        break;

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            DestroyWindow(hWnd);
        }
        break;

    case WM_SIZE:

        if (frameBuffer != nullptr && wParam != SIZE_MINIMIZED) {
            pCmd->WaitForLastSubmittedFrame(frameBuffer);
            frameBuffer->ResizeView();
        }

        break;
    case WM_SIZING:
        break;

    case WM_ENTERSIZEMOVE:
        //isMovingSizing = TRUE;
        break;

    case WM_EXITSIZEMOVE:
        //isMovingSizing = FALSE;
        //PostMessage(hWnd, WM_SIZE, wParam, lParam);
        break;

    default:
        return DefWindowProc(hWnd, Msg, wParam, lParam);
    }

    return 0;
}
