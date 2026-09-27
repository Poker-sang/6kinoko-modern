#include "kinoko/critical_section.h"
#include "kinoko/render_target.h"
#include "kinoko/graphics_device.h"
#include "kinoko/diagnostics.h"
#include <cstring>
#include <cstddef>
extern "C" void kinoko_trace_i32(const char *,int32_t);

// 4011B0: preserve the original HAL/HW -> HAL/SW -> REF/SW fallback order.
extern "C" int32_t kinoko_graphics_create(HWND window,int32_t width,int32_t height) {
    if (!window) return 0; // inherited invalid-window boundary
    auto &state=kinoko_graphics;
    state.original_window_style=GetWindowLongA(window,GWL_STYLE);
    kinoko_trace("4011b0:pre-d3d-create");
    state.factory=kinoko::graphics::create_factory(kinoko::graphics::sdk_version);
    if (!state.factory) {
        MessageBoxA(window,"kinoko::graphics::create_factory failed","DirectX-Error",MB_OK);return 0;
    }
    state.display={};
    if (FAILED(state.factory->GetAdapterDisplayMode(kinoko::graphics::adapter_default,&state.display))) {
        // Retain the inherited startup-failure cleanup boundary.
        state.factory->Release();state.factory=nullptr;
        MessageBoxA(window,"GetAdapterDisplayMode failed","DirectX-Error",MB_OK);return 0;
    }
    if (width<=0 || height<=0) {
        WINDOWINFO info{};info.cbSize=sizeof(info);
        GetWindowInfo(window,&info);
        width=info.rcClient.right-info.rcClient.left;
        height=info.rcClient.bottom-info.rcClient.top;
    }
    state.present={};
    auto &parameters=state.present;
    parameters.BackBufferWidth=width;
    parameters.BackBufferHeight=height;
    parameters.BackBufferFormat=state.display.Format;
    parameters.BackBufferCount=1;
    parameters.MultiSampleType=kinoko::graphics::multisample_none;
    parameters.SwapEffect=kinoko::graphics::swap_discard;
    parameters.hDeviceWindow=window;
    parameters.Windowed=TRUE;
    parameters.EnableAutoDepthStencil=TRUE;
    parameters.AutoDepthStencilFormat=kinoko::graphics::format_d24s8;
    parameters.Flags=kinoko::graphics::presentation_flag_discard_depthstencil;
    parameters.PresentationInterval=kinoko::graphics::presentation_interval_one;
    kinoko_trace_i32("4011b0:client-width",width);
    kinoko_trace_i32("4011b0:client-height",height);
    struct Attempt { kinoko::graphics::DeviceKind type; DWORD behavior; };
    constexpr Attempt attempts[]={
        {kinoko::graphics::device_hal,kinoko::graphics::creation_hardware_vertexprocessing|kinoko::graphics::creation_multithreaded},
        {kinoko::graphics::device_hal,kinoko::graphics::creation_software_vertexprocessing|kinoko::graphics::creation_multithreaded},
        {kinoko::graphics::device_ref,kinoko::graphics::creation_software_vertexprocessing|kinoko::graphics::creation_multithreaded}
    };
    HRESULT status=E_FAIL;
    for (const auto attempt:attempts) {
        status=state.factory->CreateDevice(kinoko::graphics::adapter_default,attempt.type,window,
            attempt.behavior,&parameters,&state.device);
        kinoko_trace_hresult("4011b0:create-device-hr",status);
        if (SUCCEEDED(status)) break;
    }
    if (FAILED(status) || !state.device) {
        state.factory->Release();state.factory=nullptr;state.device=nullptr;
        MessageBoxA(window,"CreateDevice failed","DirectX-Error",MB_OK);return 0;
    }
    state.capabilities={};
    state.device->GetDeviceCaps(&state.capabilities);
    state.device->GetSwapChain(0,&state.swap_chain);
    state.cooperative_status=kinoko::graphics::ok;
    kinoko_trace("4011b0:done");
    return 1;
}

// 401600: swap chain, device, then factory; consumers borrow those interfaces.
extern "C" int32_t kinoko_graphics_release(void) {
    auto &state=kinoko_graphics;
    if (state.swap_chain) { state.swap_chain->Release();state.swap_chain=nullptr; }
    if (state.device) { state.device->Release();state.device=nullptr; }
    ULONG result=0;
    if (state.factory) { result=state.factory->Release();state.factory=nullptr; }
    return static_cast<int32_t>(result);
}

// 401040, invoked once by the reconstructed CRT initialization sequence.
extern "C" void kinoko_graphics_initialize_runtime() {
    kinoko_critical_section_construct(&kinoko_graphics_lock);
    kinoko_initialize_device_listeners();
    kinoko_graphics.factory = nullptr;
    kinoko_graphics.device = nullptr;
    kinoko_graphics.swap_chain = nullptr;
    kinoko_graphics.unknown_state = 0;
}
