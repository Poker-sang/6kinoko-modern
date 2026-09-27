#include "kinoko/gpu_renderer.hpp"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cstring>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
namespace kinoko::gpu {
namespace {
void require(bool ok,const char* context) {
    if(!ok) throw std::runtime_error(std::string(context)+": "+SDL_GetError());
}
SDL_GPUBlendFactor factor(render::BlendFactor v) {
    using F=render::BlendFactor;
    switch(v) {
    case F::zero:return SDL_GPU_BLENDFACTOR_ZERO;
    case F::one:return SDL_GPU_BLENDFACTOR_ONE;
    case F::source_color:return SDL_GPU_BLENDFACTOR_SRC_COLOR;
    case F::destination_color:return SDL_GPU_BLENDFACTOR_DST_COLOR;
    case F::source_alpha:return SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    case F::inverse_source_alpha:return SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    default:throw std::runtime_error("GPU draw requires a complete blend state");
    }
}
struct Command {
    SDL_GPUCommandBuffer* value;
    bool acquired=false;
    explicit Command(SDL_GPUDevice* d):value(SDL_AcquireGPUCommandBuffer(d)) { require(value,"Acquire command buffer"); }
    ~Command() { if(value) { if(acquired) SDL_SubmitGPUCommandBuffer(value);else SDL_CancelGPUCommandBuffer(value); } }
    void submit() { auto* v=value;value=nullptr;require(SDL_SubmitGPUCommandBuffer(v),"Submit GPU commands"); }
};
struct Transfer {
    SDL_GPUDevice* device;
    SDL_GPUTransferBuffer* value;
    Transfer(SDL_GPUDevice* d,Uint32 size):device(d) {
        SDL_GPUTransferBufferCreateInfo info{};info.usage=SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;info.size=size;
        value=SDL_CreateGPUTransferBuffer(d,&info);require(value,"Create upload buffer");
    }
    ~Transfer() { SDL_ReleaseGPUTransferBuffer(device,value); }
};
}
struct Renderer::State {
    SDL_GPUDevice* device=nullptr;SDL_Window* window=nullptr;
    bool claimed=false;
    SDL_ThreadID thread=SDL_GetCurrentThreadID();
    SDL_GPUShader *vertex=nullptr,*fragment=nullptr;
    SDL_GPUBuffer* vertices=nullptr;SDL_GPUTransferBuffer* upload=nullptr;
    Uint32 capacity=0;
    struct Texture { SDL_GPUTexture* value;uint32_t width,height;bool target,initialized; };
    std::map<std::pair<uint32_t,uint32_t>,SDL_GPUTexture*> depths;
    std::map<TextureId,Texture> textures;
    TextureId next=1,white=0;
    using Key=std::tuple<int,bool,render::BlendOperation,render::BlendFactor,render::BlendFactor,bool,bool,Compare,int>;
    std::map<Key,SDL_GPUGraphicsPipeline*> pipelines;
    std::array<SDL_GPUSampler*,8> samplers{};
    ~State() {
        if(!device)return;
        SDL_WaitForGPUIdle(device);
        for(auto& p:pipelines) SDL_ReleaseGPUGraphicsPipeline(device,p.second);
        for(auto* s:samplers) if(s) SDL_ReleaseGPUSampler(device,s);
        for(auto& t:textures) SDL_ReleaseGPUTexture(device,t.second.value);
        for(auto& d:depths) SDL_ReleaseGPUTexture(device,d.second);
        if(vertices) SDL_ReleaseGPUBuffer(device,vertices);
        if(upload) SDL_ReleaseGPUTransferBuffer(device,upload);
        if(vertex) SDL_ReleaseGPUShader(device,vertex);
        if(fragment) SDL_ReleaseGPUShader(device,fragment);
        if(claimed) SDL_ReleaseWindowFromGPUDevice(device,window);
        SDL_DestroyGPUDevice(device);
    }
    void owner() const { if(thread!=SDL_GetCurrentThreadID()) throw std::runtime_error("GPU renderer called from a different thread"); }
    Texture& texture(TextureId id) {
        const auto found=textures.find(id?id:white);
        if(found==textures.end()) throw std::runtime_error("Unknown GPU texture ID");
        return found->second;
    }
    SDL_GPUGraphicsPipeline* pipeline(SDL_GPUTextureFormat format,const Blend& blend,const Raster& raster) {
        Key key{int(format),blend.enabled,blend.operation,blend.source,blend.destination,raster.depth_test,raster.depth_write,raster.depth_compare,raster.cull};
        auto found=pipelines.find(key);if(found!=pipelines.end())return found->second;
        SDL_GPUColorTargetDescription target{};target.format=format;
        auto& b=target.blend_state;b.enable_blend=blend.enabled;
        b.src_color_blendfactor=factor(blend.source);
        b.dst_color_blendfactor=factor(blend.destination);
        // D3D12 alpha equations require alpha counterparts for color factors.
        const auto alpha=[](render::BlendFactor f) {
            if(f==render::BlendFactor::source_color)return SDL_GPU_BLENDFACTOR_SRC_ALPHA;
            if(f==render::BlendFactor::destination_color)return SDL_GPU_BLENDFACTOR_DST_ALPHA;
            return factor(f);
        };
        b.src_alpha_blendfactor=alpha(blend.source);b.dst_alpha_blendfactor=alpha(blend.destination);
        if(blend.operation==render::BlendOperation::unchanged) throw std::runtime_error("Unresolved blend operation");
        b.color_blend_op=b.alpha_blend_op=blend.operation==render::BlendOperation::add?
            SDL_GPU_BLENDOP_ADD:SDL_GPU_BLENDOP_REVERSE_SUBTRACT;
        SDL_GPUVertexBufferDescription buffer{};buffer.slot=0;buffer.pitch=sizeof(Vertex);buffer.input_rate=SDL_GPU_VERTEXINPUTRATE_VERTEX;
        SDL_GPUVertexAttribute attributes[3]{};
        attributes[0]={0,0,SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,offsetof(Vertex,position)};
        attributes[1]={1,0,SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,offsetof(Vertex,color)};
        attributes[2]={2,0,SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,offsetof(Vertex,uv)};
        SDL_GPUGraphicsPipelineCreateInfo info{};
        info.vertex_shader=vertex;info.fragment_shader=fragment;
        info.vertex_input_state={&buffer,1,attributes,3};
        info.primitive_type=SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
        info.rasterizer_state.fill_mode=SDL_GPU_FILLMODE_FILL;
        info.rasterizer_state.enable_depth_clip=true;
        info.rasterizer_state.cull_mode=raster.cull==0?SDL_GPU_CULLMODE_NONE:raster.cull==1?SDL_GPU_CULLMODE_BACK:SDL_GPU_CULLMODE_FRONT;
        info.rasterizer_state.front_face=SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
        info.multisample_state.sample_count=SDL_GPU_SAMPLECOUNT_1;
        info.target_info.color_target_descriptions=&target;info.target_info.num_color_targets=1;
        info.target_info.has_depth_stencil_target=true;info.target_info.depth_stencil_format=SDL_GPU_TEXTUREFORMAT_D32_FLOAT_S8_UINT;
        info.depth_stencil_state.enable_depth_test=raster.depth_test;
        info.depth_stencil_state.enable_depth_write=raster.depth_write;
        info.depth_stencil_state.compare_op=static_cast<SDL_GPUCompareOp>(int(SDL_GPU_COMPAREOP_NEVER)+int(raster.depth_compare));
        auto* result=SDL_CreateGPUGraphicsPipeline(device,&info);require(result,"Create sprite pipeline");
        try { pipelines.emplace(key,result); } catch(...) {SDL_ReleaseGPUGraphicsPipeline(device,result);throw;}
        return result;
    }
    SDL_GPUSampler* sampler(const Draw& draw) {
        const size_t key=unsigned(draw.linear)|unsigned(draw.wrap_u)*2u|unsigned(draw.wrap_v)*4u;
        auto*& result=samplers[key];if(result)return result;
        SDL_GPUSamplerCreateInfo info{};
        info.min_filter=info.mag_filter=draw.linear?SDL_GPU_FILTER_LINEAR:SDL_GPU_FILTER_NEAREST;
        info.mipmap_mode=SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
        info.address_mode_u=draw.wrap_u?SDL_GPU_SAMPLERADDRESSMODE_REPEAT:SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        info.address_mode_v=draw.wrap_v?SDL_GPU_SAMPLERADDRESSMODE_REPEAT:SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        info.address_mode_w=SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        result=SDL_CreateGPUSampler(device,&info);require(result,"Create sprite sampler");return result;
    }
    SDL_GPUTexture* depth(uint32_t w,uint32_t h) {
        const auto key=std::make_pair(w,h);auto found=depths.find(key);if(found!=depths.end())return found->second;
        SDL_GPUTextureCreateInfo info{};info.type=SDL_GPU_TEXTURETYPE_2D;info.format=SDL_GPU_TEXTUREFORMAT_D32_FLOAT_S8_UINT;
        info.usage=SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;info.width=w;info.height=h;
        info.layer_count_or_depth=1;info.num_levels=1;info.sample_count=SDL_GPU_SAMPLECOUNT_1;
        auto* t=SDL_CreateGPUTexture(device,&info);require(t,"Create depth target");
        try {depths.emplace(key,t);}catch(...){SDL_ReleaseGPUTexture(device,t);throw;}return t;
    }
    void reserve(Uint32 bytes) {
        if(bytes<=capacity)return;
        // Grow geometrically, bounded by SDL's 32-bit buffer sizes.
        const auto size=Uint32(std::min<uint64_t>(std::numeric_limits<Uint32>::max(),std::max<uint64_t>(bytes,std::max<uint64_t>(65536,uint64_t(capacity)*2))));
        SDL_GPUBufferCreateInfo info{};info.usage=SDL_GPU_BUFFERUSAGE_VERTEX;info.size=size;
        auto* new_vertices=SDL_CreateGPUBuffer(device,&info);require(new_vertices,"Create vertex buffer");
        SDL_GPUTransferBufferCreateInfo transfer{};transfer.usage=SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;transfer.size=size;
        auto* new_upload=SDL_CreateGPUTransferBuffer(device,&transfer);
        if(!new_upload) {SDL_ReleaseGPUBuffer(device,new_vertices);require(false,"Create vertex transfer buffer");}
        if(vertices)SDL_ReleaseGPUBuffer(device,vertices);
        if(upload)SDL_ReleaseGPUTransferBuffer(device,upload);
        vertices=new_vertices;upload=new_upload;capacity=size;
    }
};
Renderer::Renderer(SDL_Window* window,ShaderEncoding encoding,ShaderCode vertex,ShaderCode fragment,bool debug):state_(std::make_unique<State>()) {
    if(!window||!vertex.bytes||!vertex.size||!fragment.bytes||!fragment.size) throw std::runtime_error("Window and both shader blobs are required");
    auto& s=*state_;s.window=window;
    const auto format=encoding==ShaderEncoding::dxbc?SDL_GPU_SHADERFORMAT_DXBC:
        encoding==ShaderEncoding::spirv?SDL_GPU_SHADERFORMAT_SPIRV:SDL_GPU_SHADERFORMAT_MSL;
    s.device=SDL_CreateGPUDevice(format,debug,nullptr);require(s.device,"Create SDL GPU device");
    require(SDL_ClaimWindowForGPUDevice(s.device,window),"Claim GPU window");s.claimed=true;
    SDL_GPUShaderCreateInfo info{};info.format=format;info.stage=SDL_GPU_SHADERSTAGE_VERTEX;
    info.code=static_cast<const Uint8*>(vertex.bytes);info.code_size=vertex.size;info.entrypoint=vertex.entrypoint;info.num_uniform_buffers=1;
    s.vertex=SDL_CreateGPUShader(s.device,&info);require(s.vertex,"Create sprite vertex shader");
    info.stage=SDL_GPU_SHADERSTAGE_FRAGMENT;info.code=static_cast<const Uint8*>(fragment.bytes);info.code_size=fragment.size;
    info.entrypoint=fragment.entrypoint;info.num_uniform_buffers=1;info.num_samplers=1;
    s.fragment=SDL_CreateGPUShader(s.device,&info);require(s.fragment,"Create sprite fragment shader");
    const uint8_t white[]={255,255,255,255};s.white=create_texture(1,1,white,sizeof(white),4);
}
Renderer::~Renderer()=default;
const char* Renderer::driver() const { state_->owner();return SDL_GetGPUDeviceDriver(state_->device); }
TextureId Renderer::create_texture(uint32_t width,uint32_t height,const void* pixels,size_t bytes,size_t pitch,bool target) {
    auto& s=*state_;s.owner();
    const uint64_t row=uint64_t(width)*4,upload_pitch=(row+255)&~uint64_t{255};
    if(!width||!height||upload_pitch>std::numeric_limits<Uint32>::max()||
       height>std::numeric_limits<Uint32>::max()/upload_pitch||
       (!target&&!pixels)|| (pixels&&(pitch<row||bytes<row||uint64_t(height-1)>(bytes-row)/pitch)))
        throw std::runtime_error("Invalid RGBA texture extent or upload span");
    if(s.next==std::numeric_limits<TextureId>::max())throw std::runtime_error("GPU texture IDs exhausted");
    SDL_GPUTextureCreateInfo info{};info.type=SDL_GPU_TEXTURETYPE_2D;info.format=SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    info.usage=SDL_GPU_TEXTUREUSAGE_SAMPLER|(target?SDL_GPU_TEXTUREUSAGE_COLOR_TARGET:0);
    info.width=width;info.height=height;info.layer_count_or_depth=1;info.num_levels=1;info.sample_count=SDL_GPU_SAMPLECOUNT_1;
    auto* texture=SDL_CreateGPUTexture(s.device,&info);require(texture,"Create GPU texture");
    try {
        if(pixels) {
            Transfer transfer(s.device,Uint32(upload_pitch*height));
            auto* mapped=static_cast<Uint8*>(SDL_MapGPUTransferBuffer(s.device,transfer.value,false));require(mapped,"Map texture upload");
            for(uint32_t y=0;y<height;++y)std::memcpy(mapped+size_t(y)*size_t(upload_pitch),static_cast<const Uint8*>(pixels)+size_t(y)*pitch,size_t(row));
            SDL_UnmapGPUTransferBuffer(s.device,transfer.value);
            Command command(s.device);auto* copy=SDL_BeginGPUCopyPass(command.value);require(copy,"Begin upload pass");
            SDL_GPUTextureTransferInfo source{};source.transfer_buffer=transfer.value;source.pixels_per_row=Uint32(upload_pitch/4);source.rows_per_layer=height;
            SDL_GPUTextureRegion region{};region.texture=texture;region.w=width;region.h=height;region.d=1;
            SDL_UploadToGPUTexture(copy,&source,&region,false);SDL_EndGPUCopyPass(copy);command.submit();
        }
        const auto id=s.next++;
        s.textures.emplace(id,State::Texture{texture,width,height,target,pixels!=nullptr});return id;
    }catch(...){SDL_ReleaseGPUTexture(s.device,texture);throw;}
}
void Renderer::destroy_texture(TextureId id) {
    auto& s=*state_;s.owner();if(!id||id==s.white)throw std::runtime_error("Cannot release the built-in white texture");
    auto found=s.textures.find(id);if(found==s.textures.end())throw std::runtime_error("Unknown GPU texture ID");
    SDL_ReleaseGPUTexture(s.device,found->second.value);s.textures.erase(found);
}
bool Renderer::present(const std::vector<Pass>& passes) {
    auto& s=*state_;s.owner();
    if(passes.empty())throw std::runtime_error("A frame needs at least one pass");
    struct Prepared { SDL_GPUGraphicsPipeline* pipeline;SDL_GPUTextureSamplerBinding texture;Uint32 first,count; };
    std::vector<Prepared> prepared;std::vector<Vertex> vertices;
    std::set<TextureId> initialized;
    for(auto& entry:s.textures)if(entry.second.initialized)initialized.insert(entry.first);
    bool screen_initialized=false;
    // Validate the whole frame before issuing any GPU commands.
    for(const auto& pass:passes) {
        if(bool(pass.logical_width)!=bool(pass.logical_height))throw std::runtime_error("Both logical dimensions are required");
        const auto format=pass.target?SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM:SDL_GetGPUSwapchainTextureFormat(s.device,s.window);
        if(pass.target) {
            if(!s.texture(pass.target).target)throw std::runtime_error("Texture is not a render target");
            if(!pass.clear&&!initialized.count(pass.target))throw std::runtime_error("First target use must clear");
        } else {
            if(!pass.clear&&!screen_initialized)throw std::runtime_error("First swapchain pass must clear");
            screen_initialized=true;
        }
        for(const auto& draw:pass.draws) {
            const auto texture_id=draw.texture?draw.texture:s.white;
            if(pass.target==texture_id)throw std::runtime_error("Cannot sample the active render target");
            if(!initialized.count(texture_id))throw std::runtime_error("Sampled texture has no defined contents");
            if(vertices.size()>std::numeric_limits<Uint32>::max()/sizeof(Vertex)-6)throw std::runtime_error("Frame vertex data exceeds SDL buffer limits");
            const auto first=Uint32(vertices.size());
            if(draw.triangles.empty()) {const auto quad=expand_quad(draw.vertices);vertices.insert(vertices.end(),quad.begin(),quad.end());}
            else {
                if(draw.triangles.size()%3 || draw.triangles.size()>std::numeric_limits<Uint32>::max()/sizeof(Vertex)-vertices.size())throw std::runtime_error("Invalid triangle data");
                vertices.insert(vertices.end(),draw.triangles.begin(),draw.triangles.end());
            }
            prepared.push_back({s.pipeline(format,draw.blend,draw.raster),{s.texture(texture_id).value,s.sampler(draw)},first,Uint32(vertices.size()-first)});
        }
        if(pass.target)initialized.insert(pass.target);
    }
    if(!screen_initialized)throw std::runtime_error("Frame must include a swapchain pass");
    const auto size=Uint32(vertices.size()*sizeof(Vertex));s.reserve(size);
    if(size) {
        auto* mapped=SDL_MapGPUTransferBuffer(s.device,s.upload,true);require(mapped,"Map frame upload");
        std::memcpy(mapped,vertices.data(),size);SDL_UnmapGPUTransferBuffer(s.device,s.upload);
    }
    Command command(s.device);SDL_GPUTexture* swapchain=nullptr;Uint32 width=0,height=0;
    require(SDL_WaitAndAcquireGPUSwapchainTexture(command.value,s.window,&swapchain,&width,&height),"Acquire swapchain");
    command.acquired=swapchain!=nullptr;
    if(size) {
        auto* copy=SDL_BeginGPUCopyPass(command.value);require(copy,"Begin upload pass");
        SDL_GPUTransferBufferLocation source{s.upload,0};SDL_GPUBufferRegion destination{s.vertices,0,size};
        SDL_UploadToGPUBuffer(copy,&source,&destination,true);SDL_EndGPUCopyPass(copy);
    }
    size_t index=0;
    for(const auto& pass:passes) {
        if(!swapchain&&!pass.target){index+=pass.draws.size();continue;}
        auto* texture=pass.target?s.texture(pass.target).value:swapchain;
        const auto w=pass.target?s.texture(pass.target).width:width,h=pass.target?s.texture(pass.target).height:height;
        SDL_GPUColorTargetInfo target{};target.texture=texture;
        target.clear_color={pass.clear_color[0],pass.clear_color[1],pass.clear_color[2],pass.clear_color[3]};
        target.load_op=pass.clear?SDL_GPU_LOADOP_CLEAR:SDL_GPU_LOADOP_LOAD;target.store_op=SDL_GPU_STOREOP_STORE;
        SDL_GPUDepthStencilTargetInfo depth{};depth.texture=s.depth(w,h);depth.clear_depth=1;
        depth.load_op=pass.clear_depth?SDL_GPU_LOADOP_CLEAR:SDL_GPU_LOADOP_LOAD;depth.store_op=SDL_GPU_STOREOP_STORE;
        depth.stencil_load_op=pass.clear_stencil?SDL_GPU_LOADOP_CLEAR:SDL_GPU_LOADOP_LOAD;depth.stencil_store_op=SDL_GPU_STOREOP_STORE;
        auto* render=SDL_BeginGPURenderPass(command.value,&target,1,&depth);require(render,"Begin render pass");
        float extent[]={float(pass.logical_width?pass.logical_width:w),float(pass.logical_height?pass.logical_height:h),0,0};
        SDL_PushGPUVertexUniformData(command.value,0,extent,sizeof(extent));
        if(!pass.draws.empty()) {SDL_GPUBufferBinding binding{s.vertices,0};SDL_BindGPUVertexBuffers(render,0,&binding,1);}
        for(size_t i=0;i<pass.draws.size();++i,++index) {
            SDL_BindGPUGraphicsPipeline(render,prepared[index].pipeline);
            SDL_BindGPUFragmentSamplers(render,0,&prepared[index].texture,1);
            extent[2]=pass.draws[i].clip_space?1.f:0.f;
            SDL_PushGPUVertexUniformData(command.value,0,extent,sizeof(extent));
            const auto& raster=pass.draws[i].raster;
            struct Alpha {Uint32 enabled,comparison;float reference,padding;} alpha{raster.alpha_test?1u:0u,Uint32(raster.alpha_compare),float(raster.alpha_reference)/255,0};
            SDL_PushGPUFragmentUniformData(command.value,0,&alpha,sizeof(alpha));
            SDL_DrawGPUPrimitives(render,prepared[index].count,1,prepared[index].first,0);
        }
        SDL_EndGPURenderPass(render);
    }
    command.submit();
    for(auto id:initialized)s.textures.at(id).initialized=true;
    return swapchain!=nullptr;
}
}
