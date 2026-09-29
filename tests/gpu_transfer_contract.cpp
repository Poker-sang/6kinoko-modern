// Manual hardware contract: upload padded rows/slices, read back, compare pixels.
// Deliberately needs no shaders or game DAT. Compiled, not automatically run.
#include <SDL3/SDL.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <vector>

static void require(bool ok, const char* what) {
    if (!ok) throw std::runtime_error(what);
}

static void transfer(SDL_GPUDevice* device, Uint32 width, Uint32 height,
                     Uint32 slices, bool padded, bool zero_rows = false) {
    const Uint32 row = width * 4;
    // zero_rows checks that EITHER zero field selects the packed default.
    const Uint32 stride = padded && !zero_rows ? (row + 255u) & ~255u : row;
    const Uint32 rows = padded && !zero_rows ? height + 3 : height;
    const Uint32 offset = 512, payload = stride * rows * slices;
    std::vector<Uint8> input(offset + payload, 0xCD);
    std::vector<Uint8> expected(row * height * slices);
    for (Uint32 z=0; z<slices; ++z) for (Uint32 y=0; y<height; ++y)
        for (Uint32 x=0; x<row; ++x) {
            const auto value=Uint8((x*13+y*31+z*79)%251);
            input[offset+z*stride*rows+y*stride+x]=value;
            expected[(z*height+y)*row+x]=value;
        }
    SDL_GPUTextureCreateInfo ti{};
    ti.type=slices>1?SDL_GPU_TEXTURETYPE_3D:SDL_GPU_TEXTURETYPE_2D;
    ti.format=SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    ti.usage=SDL_GPU_TEXTUREUSAGE_SAMPLER;
    ti.width=width; ti.height=height; ti.layer_count_or_depth=slices;
    ti.num_levels=1; ti.sample_count=SDL_GPU_SAMPLECOUNT_1;
    auto* texture=SDL_CreateGPUTexture(device,&ti); require(texture,"texture");
    SDL_GPUTransferBufferCreateInfo bi{};
    bi.usage=SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD; bi.size=Uint32(input.size());
    auto* upload=SDL_CreateGPUTransferBuffer(device,&bi); require(upload,"upload");
    bi.usage=SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD;
    // Aligned download pitch avoids imposing tight-row support on D3D12.
    const Uint32 read_stride=(row+255u)&~255u;
    bi.size=read_stride*height*slices;
    auto* download=SDL_CreateGPUTransferBuffer(device,&bi); require(download,"download");
    auto* mapped=SDL_MapGPUTransferBuffer(device,upload,false); require(mapped,"map upload");
    std::memcpy(mapped,input.data(),input.size()); SDL_UnmapGPUTransferBuffer(device,upload);
    auto* cmd=SDL_AcquireGPUCommandBuffer(device); require(cmd,"commands");
    auto* copy=SDL_BeginGPUCopyPass(cmd); require(copy,"copy pass");
    SDL_GPUTextureTransferInfo source{};
    source.transfer_buffer=upload; source.offset=offset;
    source.pixels_per_row=padded?(zero_rows?width+64u:stride/4):0;
    source.rows_per_layer=padded&&!zero_rows?rows:0;
    SDL_GPUTextureRegion region{};
    region.texture=texture; region.w=width; region.h=height; region.d=slices;
    SDL_UploadToGPUTexture(copy,&source,&region,false);
    SDL_EndGPUCopyPass(copy);
    require(SDL_SubmitGPUCommandBuffer(cmd),"submit upload");
    cmd=SDL_AcquireGPUCommandBuffer(device); require(cmd,"read commands");
    copy=SDL_BeginGPUCopyPass(cmd); require(copy,"read pass");
    SDL_GPUTextureTransferInfo destination{};
    destination.transfer_buffer=download;
    destination.pixels_per_row=read_stride/4; destination.rows_per_layer=height;
    SDL_DownloadFromGPUTexture(copy,&region,&destination); SDL_EndGPUCopyPass(copy);
    auto* fence=SDL_SubmitGPUCommandBufferAndAcquireFence(cmd); require(fence,"fence");
    require(SDL_WaitForGPUFences(device,true,&fence,1),"wait");
    auto* result=static_cast<const Uint8*>(SDL_MapGPUTransferBuffer(device,download,false));
    require(result,"map download");
    bool equal=true;
    for (Uint32 z=0; z<slices; ++z) for (Uint32 y=0; y<height; ++y)
        equal &= std::memcmp(result+(z*height+y)*read_stride,
                             expected.data()+(z*height+y)*row,row)==0;
    SDL_UnmapGPUTransferBuffer(device,download); SDL_ReleaseGPUFence(device,fence);
    SDL_ReleaseGPUTransferBuffer(device,upload); SDL_ReleaseGPUTransferBuffer(device,download);
    SDL_ReleaseGPUTexture(device,texture);
    if (!equal) {
        std::fprintf(stderr,"Pixel mismatch: %ux%ux%u padded=%d zero_rows=%d\n",width,height,slices,padded,zero_rows);
        throw std::runtime_error("GPU transfer layout mismatch");
    }
}

int main() {
    SDL_GPUDevice* device=nullptr;
    int result=0;
    try {
        require(SDL_Init(SDL_INIT_VIDEO),"SDL video");
        device=SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_DXBC|SDL_GPU_SHADERFORMAT_SPIRV|SDL_GPU_SHADERFORMAT_MSL,true,nullptr);
        require(device,"GPU device");
        std::printf("Backend: %s\n",SDL_GetGPUDeviceDriver(device));
        for (Uint32 w : {1u,16u,32u,64u,96u,160u,544u}) transfer(device,w,7,1,true);
        transfer(device,16,7,2,true); // independent padded slice pitch
        transfer(device,64,7,1,false); // tightly packed defaults
        transfer(device,64,7,1,true,true); // zero rows resets both defaults
        std::puts("GPU transfer pixels match");
    } catch (const std::exception& e) {
        std::fprintf(stderr,"%s: %s\n",e.what(),SDL_GetError()); result=1;
    }
    if (device) SDL_DestroyGPUDevice(device);
    SDL_Quit(); return result;
}
