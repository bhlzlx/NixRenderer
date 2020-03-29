#include <NixApplication.h>
#include <nix/io/archieve.h>
#include <cstdio>
#include <dxgi.h>
#include <dxgi1_4.h>
#include <d3d12.h>

#include "d3dx12.h"

constexpr uint32_t MaxFlightCount = 2;

class HelloWorld : public NixApplication {
private:
    void*                   m_hwnd;
    IDXGIFactory4*          m_dxgiFactory;
    IDXGIAdapter1*          m_dxgiAdapter;
    ID3D12Device*           m_dxgiDevice;
    ID3D12CommandQueue*     m_commandQueue;
    IDXGISwapChain3*        m_swapchain;
    uint32_t                m_flightIndex;
    //
    ID3D12DescriptorHeap*   m_rtvDescriptorHeap;
    uint32_t                m_rtvDescriptorSize;
    ID3D12Resource*         m_renderTargets[MaxFlightCount];
    ID3D12CommandAllocator* m_commandAllocators[MaxFlightCount];
    ID3D12GraphicsCommandList* m_commandList;
    ID3D12Fence*            m_fence[MaxFlightCount];
    //
    HANDLE                  m_fenceEvent;
    UINT64                  m_fenceValue[MaxFlightCount];
    bool                    m_running;
public:

	virtual bool initialize( void* _wnd, Nix::IArchieve* ) {
		// 1. create factory
		HRESULT factoryHandle = CreateDXGIFactory1(IID_PPV_ARGS(&m_dxgiFactory));
		if (FAILED(factoryHandle)) {
			return false;
		}
        // 2.  enum all GPU devices and select a GPU that support DX12
		int adapterIndex = 0;
		bool adapterFound = false;
        while (m_dxgiFactory->EnumAdapters1(adapterIndex, &m_dxgiAdapter) != DXGI_ERROR_NOT_FOUND) {
            DXGI_ADAPTER_DESC1 desc;
            m_dxgiAdapter->GetDesc1(&desc);
            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) { // if it's software
                ++adapterIndex;
                continue;
            }
            // test it can create a device that support dx12 or not
            HRESULT rst = D3D12CreateDevice(m_dxgiAdapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr );
            if (SUCCEEDED(rst)) {
                adapterFound = true;
                break;
            }
            ++adapterIndex;
        }
        if (!adapterFound) {
            return false;
        }
        // 3. create the device
        HRESULT rst = D3D12CreateDevice(
            m_dxgiAdapter,
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(&m_dxgiDevice)
        );
        if (FAILED(rst)){
            return false;
        }
        // 4. create command queue
        D3D12_COMMAND_QUEUE_DESC cmdQueueDesc; {
            cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
            cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
            cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
            cmdQueueDesc.NodeMask = 0;
        }
        rst = m_dxgiDevice->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&m_commandQueue));
        if (FAILED(rst)) {
            return false;
        }
        // 5. swapchain / when windows is resize
        this->m_hwnd = _wnd;// handle of the window

        // 6. create descriptor heap
        // describe an rtv descriptor heap and create
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {}; {
            rtvHeapDesc.NumDescriptors = MaxFlightCount;
            rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
            rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        }
       
        rst = m_dxgiDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvDescriptorHeap));
        if (FAILED(rst)) {
            return false;
        }
        m_rtvDescriptorSize = m_dxgiDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

        // 7. create render targets & render target view
        // Create a RTV for each buffer (double buffering is two buffers, tripple buffering is 3).
        for (int i = 0; i < MaxFlightCount; i++) {
            // first we get the n'th buffer in the swap chain and store it in the n'th
            // position of our ID3D12Resource array
            rst = m_swapchain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i]));
            if (FAILED(rst)) {
                return false;
            }
            // the we "create" a render target view which binds the swap chain buffer (ID3D12Resource[n]) to the rtv handle
            m_dxgiDevice->CreateRenderTargetView(m_renderTargets[i], nullptr, { rtvHandle.ptr + m_rtvDescriptorSize * i } );
        }
        // 8. create command allocators
        for (int i = 0; i < MaxFlightCount; i++) {
            rst = m_dxgiDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocators[i]));
            if (FAILED(rst)) {
                return false;
            }
        }
        // 9. create command list
        // create the command list with the first allocator
        rst = m_dxgiDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocators[0], NULL, IID_PPV_ARGS(&m_commandList));
        if (FAILED(rst)) {
            return false;
        }
        m_commandList->Close();

        // create the fences
        for (int i = 0; i < MaxFlightCount; i++) {
            rst = m_dxgiDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence[i]));
            if (FAILED(rst)) {
                return false;
            }
            m_fenceValue[i] = 0; // set the initial fence value to 0
        }

        // create a handle to a fence event
        m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (m_fenceEvent == nullptr) {
            return false;
        }
    }
    
	virtual void resize(uint32_t _width, uint32_t _height) {

        {// re-create the swapchain!
            if (this->m_swapchain) {
                m_swapchain->Release();
            }
            //
            DXGI_MODE_DESC displayModeDesc = {}; {
                displayModeDesc.Width = _width;
                displayModeDesc.Height = _height;
                displayModeDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // 不用检查支持不支持吗？
            }
            DXGI_SAMPLE_DESC sampleDesc = {}; {
                sampleDesc.Count = 1; // 多重采样，一般这个不开启多重采样。
            }
            // Describe and create the swap chain.
            DXGI_SWAP_CHAIN_DESC swapchainDesc = {}; {
                swapchainDesc.BufferCount = MaxFlightCount;
                swapchainDesc.BufferDesc = displayModeDesc;
                swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
                swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
                swapchainDesc.OutputWindow = (HWND)m_hwnd;
                swapchainDesc.SampleDesc = sampleDesc;
                swapchainDesc.Windowed = true; // fullsceen or not!!!
            }
            //
            IDXGISwapChain* swapchain = nullptr;
            this->m_dxgiFactory->CreateSwapChain(this->m_commandQueue, &swapchainDesc, &swapchain);
            this->m_swapchain = static_cast<IDXGISwapChain3*>(swapchain);
            this->m_flightIndex = m_swapchain->GetCurrentBackBufferIndex();
        }        

        printf("resized!");
    }

	virtual void release() {
        printf("destroyed");
    }

	virtual void tick() {
        if (!m_swapchain) {
            return;
        }
        {// 等待 RT 使用完
            HRESULT rst;
            m_flightIndex = m_swapchain->GetCurrentBackBufferIndex();
            // DX12 用一个预设值来判断 RT 对应的帧有没有渲染、显示完成
            // 而 Vulkan 直接用 Fence 的API 来判断有没有完成
            if (m_fence[m_flightIndex]->GetCompletedValue() < m_fenceValue[m_flightIndex]) {
                rst = m_fence[m_flightIndex]->SetEventOnCompletion(m_fenceValue[m_flightIndex], m_fenceEvent);
                if (FAILED(rst)) {
                    m_running = false;
                }
                WaitForSingleObject(m_fenceEvent, INFINITE);
            }
            m_fenceValue[m_flightIndex]++;
        }

        { //
            ID3D12CommandAllocator* commandAllocator = m_commandAllocators[m_flightIndex];
            HRESULT rst = commandAllocator->Reset();
            if (FAILED(rst)) {
                m_running = false;
            }
            rst = m_commandList->Reset(commandAllocator, NULL);
            if (FAILED(rst)) {
                m_running = false;
            }
            // transform the render target's layout
            m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition( m_renderTargets[m_flightIndex], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
            CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle( m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), m_flightIndex, m_rtvDescriptorSize);
            m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
            const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
            // clear color
            m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
            // transfrom the render target's layout
            m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_flightIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
            //
            rst = m_commandList->Close();
            if (FAILED(rst)) {
                m_running = false;
            }
        }
        {
            ID3D12CommandList* ppCommandLists[] = { m_commandList };
            m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
            // queue is being executed on the GPU
            HRESULT rst = m_commandQueue->Signal(m_fence[m_flightIndex], m_fenceValue[m_flightIndex]);
            if (FAILED(rst)) {
                m_running = false;
            }

            // present the current backbuffer
            rst = m_swapchain->Present(0, 0);
            if (FAILED(rst))
            {
                m_running = false;
            }
        }
    }

	virtual const char * title() {
        return "hello,world!";
    }
	
	virtual uint32_t rendererType() {
		return 0;
	}
};

HelloWorld theapp;

NixApplication* GetApplication() {
    return &theapp;
}