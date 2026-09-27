#include "kinoko/graphics_api.hpp"
#include "kinoko/gpu_renderer.hpp"
#include "kinoko/platform.hpp"
#include "kinoko/matrix_math.hpp"
#include "kinoko/com_owner.hpp"
#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <cstring>
#include <deque>
#include <fstream>
#include <iterator>
#include <map>
#include <mutex>
#include <stdexcept>
#include <string>
namespace kinoko::graphics {
namespace {
std::mutex errors_mutex;std::string error;Device* active=nullptr;
HRESULT bad(const char* message){std::lock_guard<std::mutex> lock(errors_mutex);if(error.empty())error=message;return kinoko::graphics::error_invalidcall;}
std::vector<char> shader(const char* filename){
    const auto* base=SDL_GetBasePath();if(!base)throw std::runtime_error(SDL_GetError());
    std::ifstream file(std::string(base)+"shaders/"+filename,std::ios::binary);
    if(!file)throw std::runtime_error(std::string("Missing staged GPU shader: ")+filename);
    return {std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>()};
}
std::atomic<uint64_t> sequence{1};
using Pixels=std::vector<uint8_t>;
}
struct Image {
    std::mutex mutex;
    uint64_t id=sequence++,generation=0;
    UINT width,height;kinoko::graphics::Format format;bool target;
    std::shared_ptr<const Pixels> pixels;
    Image(UINT w,UINT h,kinoko::graphics::Format f,bool t):width(w),height(h),format(f),target(t){}
};
HRESULT Surface::GetDesc(kinoko::graphics::SurfaceDescription* out){if(!out)return bad("Null surface description");*out={};out->Width=image->width;out->Height=image->height;out->Format=image->format;out->Type=kinoko::graphics::resource_surface;return S_OK;}
HRESULT Texture::LockRect(UINT level,kinoko::graphics::MappedPixels* out,const RECT* rect,DWORD flags){
    std::lock_guard<std::mutex> lock(image->mutex);
    if(level||!out||locked||image->target||flags)return bad("Unsupported texture map request");
    const UINT bpp=image->format==kinoko::graphics::format_a1r5g5b5?2:4;
    RECT r=rect?*rect:RECT{0,0,LONG(image->width),LONG(image->height)};
    if(r.left<0||r.top<0||r.right>LONG(image->width)||r.bottom>LONG(image->height)||r.left>=r.right||r.top>=r.bottom)return bad("Invalid texture map region");
    staging=image->pixels?*image->pixels:Pixels(size_t(image->width)*image->height*bpp);
    out->Pitch=INT(image->width*bpp);out->pBits=staging.data()+(size_t(r.top)*image->width+r.left)*bpp;locked=true;return S_OK;
}
HRESULT Texture::UnlockRect(UINT level){std::lock_guard<std::mutex> lock(image->mutex);if(level||!locked)return bad("Unbalanced texture unmap");image->pixels=std::make_shared<const Pixels>(std::move(staging));++image->generation;locked=false;return S_OK;}
HRESULT Texture::GetSurfaceLevel(UINT level,Surface** out){if(level||!out)return bad("Unsupported texture surface level");*out=new Surface(image);return S_OK;}
HRESULT Buffer::Lock(UINT offset,UINT size,void** out,DWORD flags){if(!out||flags||offset>bytes.size()||(size&&size>bytes.size()-offset))return bad("Invalid mesh buffer map");*out=bytes.data()+offset;return S_OK;}
struct Snapshot {std::shared_ptr<Image> image;std::shared_ptr<const Pixels> pixels;uint64_t generation=0;};
struct RecordedPass {std::shared_ptr<Image> target;gpu::Pass pass;std::vector<Snapshot> textures;};
struct Device::State {
    std::recursive_mutex cpu_mutex;
    HWND window;UINT width,height;bool scene=false,closed=false,reported=false,mesh_decl=false;
    std::unique_ptr<gpu::Renderer> renderer;
    Surface *back=nullptr,*depth=nullptr,*target=nullptr;
    std::array<DWORD,256> states{};std::array<std::array<DWORD,16>,8> samplers{};
    std::array<Texture*,8> textures{};
    std::array<Buffer*,3> streams{};std::array<UINT,3> offsets{},strides{};Buffer* indices=nullptr;
    DWORD fvf=kinoko::graphics::vertex_xyzrhw|kinoko::graphics::vertex_diffuse|kinoko::graphics::vertex_tex1;
    math::Matrix world=math::identity(),view=math::identity(),projection=math::identity();
    std::vector<RecordedPass> recording;std::mutex queue_mutex;std::deque<std::vector<RecordedPass>> queue;
    struct Uploaded {gpu::TextureId id;std::weak_ptr<Image> image;std::weak_ptr<const Pixels> pixels;bool target;};
    std::map<std::pair<uint64_t,uint64_t>,Uploaded> uploaded;
    State(HWND w,UINT x,UINT y):window(w),width(x),height(y) {
        const auto vs=shader("sprite.vert.dxbc"),fs=shader("sprite.frag.dxbc");
        renderer=std::make_unique<gpu::Renderer>(platform::host().window(),gpu::ShaderEncoding::dxbc,gpu::ShaderCode{vs.data(),vs.size()},gpu::ShaderCode{fs.data(),fs.size()});
        auto screen=std::make_shared<Image>(x,y,kinoko::graphics::format_a8r8g8b8,true);screen->id=0;
        back=new Surface(screen);depth=new Surface(screen);target=back;target->AddRef();
        states[kinoko::graphics::state_zenable]=states[kinoko::graphics::state_zwriteenable]=TRUE;states[kinoko::graphics::state_zfunc]=kinoko::graphics::compare_lessequal;
        states[kinoko::graphics::state_alphafunc]=kinoko::graphics::compare_always;states[kinoko::graphics::state_srcblend]=kinoko::graphics::blend_one;states[kinoko::graphics::state_destblend]=kinoko::graphics::blend_zero;
        states[kinoko::graphics::state_blendop]=kinoko::graphics::blend_operation_add;states[kinoko::graphics::state_cullmode]=kinoko::graphics::cull_ccw;states[kinoko::graphics::state_lighting]=TRUE;
        for(auto& s:samplers){s[kinoko::graphics::sampler_addressu]=s[kinoko::graphics::sampler_addressv]=kinoko::graphics::address_wrap;s[kinoko::graphics::sampler_magfilter]=s[kinoko::graphics::sampler_minfilter]=kinoko::graphics::filter_point;}
    }
    ~State(){for(auto* t:textures)if(t)t->Release();for(auto* b:streams)if(b)b->Release();if(indices)indices->Release();if(target)target->Release();if(back)back->Release();if(depth)depth->Release();}
    RecordedPass& pass(){
        const auto image=target->image;
        if(recording.empty()||recording.back().target!=image){
            RecordedPass p;p.target=image;p.pass.clear=false;p.pass.clear_depth=false;p.pass.clear_stencil=false;
            p.pass.logical_width=image->width;p.pass.logical_height=image->height;
            // A new frame backbuffer has undefined contents until cleared.
            if(image->id==0&&std::none_of(recording.begin(),recording.end(),[](auto& v){return v.target->id==0;}))p.pass.clear=p.pass.clear_depth=true;
            recording.push_back(std::move(p));
        }
        return recording.back();
    }
    static render::BlendFactor blend(DWORD value){
        switch(value){case kinoko::graphics::blend_zero:return render::BlendFactor::zero;case kinoko::graphics::blend_one:return render::BlendFactor::one;
        case kinoko::graphics::blend_srccolor:return render::BlendFactor::source_color;case kinoko::graphics::blend_destcolor:return render::BlendFactor::destination_color;
        case kinoko::graphics::blend_srcalpha:return render::BlendFactor::source_alpha;case kinoko::graphics::blend_invsrcalpha:return render::BlendFactor::inverse_source_alpha;
        default:throw std::runtime_error("Unsupported blend factor");}
    }
    void draw(gpu::Draw draw){
        if(!scene)throw std::runtime_error("Drawing outside a GPU scene");
        draw.blend.enabled=states[kinoko::graphics::state_alphablendenable]!=0;draw.blend.source=blend(states[kinoko::graphics::state_srcblend]);draw.blend.destination=blend(states[kinoko::graphics::state_destblend]);
        draw.blend.operation=states[kinoko::graphics::state_blendop]==kinoko::graphics::blend_operation_add?render::BlendOperation::add:render::BlendOperation::reverse_subtract;
        draw.raster.depth_test=states[kinoko::graphics::state_zenable]!=0;draw.raster.depth_write=states[kinoko::graphics::state_zwriteenable]!=0;
        draw.raster.depth_compare=gpu::Compare(states[kinoko::graphics::state_zfunc]-1);draw.raster.alpha_test=states[kinoko::graphics::state_alphatestenable]!=0;
        draw.raster.alpha_compare=gpu::Compare(states[kinoko::graphics::state_alphafunc]-1);draw.raster.alpha_reference=uint8_t(states[kinoko::graphics::state_alpharef]);
        draw.raster.cull=states[kinoko::graphics::state_cullmode]==kinoko::graphics::cull_none?0:states[kinoko::graphics::state_cullmode]==kinoko::graphics::cull_cw?1:2;
        draw.linear=samplers[0][kinoko::graphics::sampler_magfilter]==kinoko::graphics::filter_linear;
        draw.wrap_u=samplers[0][kinoko::graphics::sampler_addressu]==kinoko::graphics::address_wrap;draw.wrap_v=samplers[0][kinoko::graphics::sampler_addressv]==kinoko::graphics::address_wrap;
        auto* t=textures[0];Snapshot snap;if(t){std::lock_guard<std::mutex> lock(t->image->mutex);snap={t->image,t->image->pixels,t->image->generation};}
        auto& p=pass();p.pass.draws.push_back(std::move(draw));p.textures.push_back(std::move(snap));
    }
    gpu::Vertex vertex(float x,float y,float z,float w,DWORD color,float u,float v,bool transform){
        gpu::Vertex out{{x,y,z,w},{float((color>>16)&255)/255,float((color>>8)&255)/255,float(color&255)/255,float(color>>24)/255},{u,v}};
        if(transform){const auto m=math::multiply(math::multiply(world,view),projection);float input[4]={x,y,z,w};
            for(int j=0;j<4;++j){out.position[j]=0;for(int i=0;i<4;++i)out.position[j]+=input[i]*m.m[i][j];}}
        return out;
    }
    gpu::TextureId upload(const Snapshot& snap){
        if(!snap.image||snap.image->id==0)return 0;
        auto& image=*snap.image;const auto key=std::make_pair(image.id,image.target?0:snap.generation);
        auto found=uploaded.find(key);if(found!=uploaded.end())return found->second.id;
        Pixels rgba;
        if(!image.target){
            if(!snap.pixels)throw std::runtime_error("Sampling an unmapped texture");
            rgba.resize(size_t(image.width)*image.height*4);
            for(size_t i=0;i<rgba.size()/4;++i){uint32_t color;
                if(image.format==kinoko::graphics::format_a1r5g5b5){uint16_t p;std::memcpy(&p,snap.pixels->data()+i*2,2);
                    auto expand=[](uint32_t x){return (x<<3)|(x>>2);};
                    color=(p&0x8000?0xff000000u:0)|(expand((p>>10)&31)<<16)|(expand((p>>5)&31)<<8)|expand(p&31);
                }else std::memcpy(&color,snap.pixels->data()+i*4,4);
                rgba[i*4]=uint8_t(color>>16);rgba[i*4+1]=uint8_t(color>>8);rgba[i*4+2]=uint8_t(color);rgba[i*4+3]=uint8_t(color>>24);
            }
        }
        const auto id=renderer->create_texture(image.width,image.height,rgba.empty()?nullptr:rgba.data(),rgba.size(),size_t(image.width)*4,image.target);
        try {uploaded.emplace(key,Uploaded{id,snap.image,snap.pixels,image.target});}catch(...){renderer->destroy_texture(id);throw;}return id;
    }
    void poll(){
        std::vector<RecordedPass> frame;
        {std::lock_guard<std::mutex> lock(queue_mutex);if(!queue.empty()){frame=std::move(queue.front());queue.pop_front();}}
        if(!frame.empty()){
            std::vector<gpu::Pass> passes;
            for(auto& p:frame){p.pass.target=upload({p.target,nullptr,0});
                for(size_t i=0;i<p.textures.size();++i)p.pass.draws[i].texture=upload(p.textures[i]);
                passes.push_back(std::move(p.pass));}
            renderer->present(passes);
        }
        for(auto i=uploaded.begin();i!=uploaded.end();){
            if(i->second.image.expired()||(!i->second.target&&i->second.pixels.expired())){renderer->destroy_texture(i->second.id);i=uploaded.erase(i);}else ++i;
        }
    }
};
Device::Device(HWND w,UINT x,UINT y):state(std::make_unique<State>(w,x,y)){active=this;}
Device::~Device(){if(active==this)active=nullptr;}
void stop(){if(active){std::lock_guard<std::mutex> lock(active->state->queue_mutex);active->state->closed=true;}}
HRESULT Device::GetDeviceCaps(kinoko::graphics::Capabilities* out){
    std::lock_guard<std::recursive_mutex> cpu_guard(state->cpu_mutex);if(!out)return E_POINTER;*out={};out->MaxTextureWidth=out->MaxTextureHeight=16384;return S_OK;}
HRESULT Device::GetSwapChain(UINT index,SwapChain** out){
    std::lock_guard<std::recursive_mutex> cpu_guard(state->cpu_mutex);if(index||!out)return bad("Unsupported swapchain index");*out=new SwapChain(this);return S_OK;}
HRESULT Device::TestCooperativeLevel(){
    try{state->poll();}catch(const std::exception& e){bad(e.what());}
    std::string message;{std::lock_guard<std::mutex> lock(errors_mutex);message=error;}
    if(!message.empty()){
        if(!state->reported){state->reported=true;MessageBoxA(state->window,message.c_str(),"SDL GPU renderer error",MB_OK|MB_ICONERROR);SDL_Event quit{};quit.type=SDL_EVENT_QUIT;SDL_PushEvent(&quit);}return kinoko::graphics::error_devicelost;
    }return kinoko::graphics::ok;
}
HRESULT Device::Reset(kinoko::graphics::Presentation* p){
    std::lock_guard<std::recursive_mutex> cpu_guard(state->cpu_mutex);if(!p)return E_POINTER;return SDL_SetWindowFullscreen(platform::host().window(),p->Windowed==FALSE)?S_OK:bad(SDL_GetError());}
HRESULT Device::BeginScene(){
    std::lock_guard<std::recursive_mutex> cpu_guard(state->cpu_mutex);auto& s=*state;std::lock_guard<std::mutex> lock(s.queue_mutex);if(s.closed||s.queue.size()>=3)return kinoko::graphics::error_wasstilldrawing;if(s.scene)return bad("Nested GPU scene");s.recording.clear();s.scene=true;return S_OK;}
HRESULT Device::EndScene(){
    std::lock_guard<std::recursive_mutex> cpu_guard(state->cpu_mutex);auto& s=*state;if(!s.scene)return bad("Unbalanced GPU EndScene");s.scene=false;std::lock_guard<std::mutex> lock(s.queue_mutex);if(!s.closed&&!s.recording.empty())s.queue.push_back(std::move(s.recording));return S_OK;}
HRESULT Device::present(){
    std::lock_guard<std::recursive_mutex> cpu_guard(state->cpu_mutex);return S_OK;} // Submission acknowledgement; main-thread poll owns actual presentation.
HRESULT SwapChain::Present(const RECT*,const RECT*,HWND,const RGNDATA*,DWORD){return device->present();}
HRESULT Device::Clear(DWORD count,const kinoko::graphics::ClearRect*,DWORD flags,kinoko::graphics::Color color,float z,DWORD stencil){
    std::lock_guard<std::recursive_mutex> cpu_guard(state->cpu_mutex);
    if(count||z!=1||stencil!=0)return bad("Unsupported partial or nondefault GPU clear");
    auto& s=*state;if(!s.scene)return S_OK;auto* p=&s.pass();
    if(!p->pass.draws.empty()){RecordedPass next;next.target=p->target;next.pass.logical_width=p->pass.logical_width;next.pass.logical_height=p->pass.logical_height;next.pass.clear=false;next.pass.clear_depth=false;next.pass.clear_stencil=false;s.recording.push_back(std::move(next));p=&s.recording.back();}
    if(flags&kinoko::graphics::clear_target){p->pass.clear=true;p->pass.clear_color={float((color>>16)&255)/255,float((color>>8)&255)/255,float(color&255)/255,float(color>>24)/255};}
    if(flags&kinoko::graphics::clear_zbuffer)p->pass.clear_depth=true;
    if(flags&kinoko::graphics::clear_stencil)p->pass.clear_stencil=true;return S_OK;
}
HRESULT Device::GetRenderState(kinoko::graphics::RenderState type,DWORD* out){
    std::lock_guard<std::recursive_mutex> cpu_guard(state->cpu_mutex);if(!out||UINT(type)>=state->states.size())return bad("Invalid render state query");*out=state->states[type];return S_OK;}
HRESULT Device::SetRenderState(kinoko::graphics::RenderState type,DWORD value){
    std::lock_guard<std::recursive_mutex> cpu_guard(state->cpu_mutex);
    switch(type){
    case kinoko::graphics::state_zenable:if(value>1)return bad("W-buffering is unsupported");break;
    case kinoko::graphics::state_zwriteenable:case kinoko::graphics::state_alphablendenable:case kinoko::graphics::state_alphatestenable:case kinoko::graphics::state_lighting:break;
    case kinoko::graphics::state_zfunc:case kinoko::graphics::state_alphafunc:if(value<1||value>8)return bad("Unsupported comparison");break;
    case kinoko::graphics::state_cullmode:if(value<1||value>3)return bad("Unsupported winding");break;
    case kinoko::graphics::state_blendop:if(value!=kinoko::graphics::blend_operation_add&&value!=kinoko::graphics::blend_operation_revsubtract)return bad("Unsupported blend operation");break;
    case kinoko::graphics::state_srcblend:case kinoko::graphics::state_destblend:try{State::blend(value);}catch(...){return bad("Unsupported blend factor");}break;
    case kinoko::graphics::state_alpharef:case kinoko::graphics::state_stencilmask:break;
    case kinoko::graphics::state_fillmode:if(value!=kinoko::graphics::fill_solid)return bad("Only solid mesh fill is implemented");break;
    default:return bad("Unimplemented render state requested");
    }state->states[type]=value;return S_OK;
}
HRESULT Device::GetSamplerState(DWORD stage,kinoko::graphics::SamplerState type,DWORD* out){
    std::lock_guard<std::recursive_mutex> cpu_guard(state->cpu_mutex);if(stage>=8||UINT(type)>=16||!out)return bad("Invalid sampler query");*out=state->samplers[stage][type];return S_OK;}
HRESULT Device::SetSamplerState(DWORD stage,kinoko::graphics::SamplerState type,DWORD value){
    std::lock_guard<std::recursive_mutex> cpu_guard(state->cpu_mutex);if(stage||UINT(type)>=16)return bad("Unsupported sampler stage");
    if(type==kinoko::graphics::sampler_addressu||type==kinoko::graphics::sampler_addressv){if(value!=kinoko::graphics::address_wrap&&value!=kinoko::graphics::address_clamp)return bad("Unsupported addressing");}
    else if(type==kinoko::graphics::sampler_minfilter||type==kinoko::graphics::sampler_magfilter||type==kinoko::graphics::sampler_mipfilter){if(value!=kinoko::graphics::filter_point&&value!=kinoko::graphics::filter_linear)return bad("Unsupported filtering");}
    else return bad("Unsupported sampler property");state->samplers[stage][type]=value;return S_OK;}
HRESULT Device::SetTextureStageState(DWORD stage,kinoko::graphics::TextureStageState type,DWORD value){
    std::lock_guard<std::recursive_mutex> cpu_guard(state->cpu_mutex);return !stage&&type==kinoko::graphics::texture_stage_alphaop&&value==kinoko::graphics::texture_operation_modulate?S_OK:bad("Unsupported texture-stage operation");}
HRESULT Device::GetTexture(DWORD stage,Texture** out){
    std::lock_guard<std::recursive_mutex> cpu_guard(state->cpu_mutex);if(stage>=8||!out)return E_INVALIDARG;*out=state->textures[stage];if(*out)(*out)->AddRef();return S_OK;}
HRESULT Device::SetTexture(DWORD stage,Texture* value){
    std::lock_guard<std::recursive_mutex> cpu_guard(state->cpu_mutex);if(stage>=8||(stage&&value))return bad("Unsupported texture stage");if(value)value->AddRef();auto*& current=state->textures[stage];if(current)current->Release();current=value;return S_OK;}
HRESULT Device::GetRenderTarget(DWORD index,Surface** out){
    std::lock_guard<std::recursive_mutex> cpu_guard(state->cpu_mutex);if(index||!out)return E_INVALIDARG;*out=state->target;(*out)->AddRef();return S_OK;}
HRESULT Device::SetRenderTarget(DWORD index,Surface* value){
    std::lock_guard<std::recursive_mutex> cpu_guard(state->cpu_mutex);if(index||!value||!value->image->target)return bad("Invalid render target");value->AddRef();state->target->Release();state->target=value;return S_OK;}
HRESULT Device::GetDepthStencilSurface(Surface** out){
    std::lock_guard<std::recursive_mutex> cpu_guard(state->cpu_mutex);if(!out)return E_POINTER;*out=state->depth;(*out)->AddRef();return S_OK;}
HRESULT Device::GetTransform(kinoko::graphics::Transform type,kinoko::graphics::Matrix* out){
    std::lock_guard<std::recursive_mutex> cpu_guard(state->cpu_mutex);if(!out)return E_POINTER;auto& s=*state;const auto* m=type==kinoko::graphics::transform_world?&s.world:type==kinoko::graphics::transform_view?&s.view:type==kinoko::graphics::transform_projection?&s.projection:nullptr;if(!m)return bad("Unknown transform");std::memcpy(out,m,sizeof(*m));return S_OK;}
HRESULT Device::SetTransform(kinoko::graphics::Transform type,const kinoko::graphics::Matrix* in){
    std::lock_guard<std::recursive_mutex> cpu_guard(state->cpu_mutex);if(!in)return E_POINTER;auto& s=*state;auto* m=type==kinoko::graphics::transform_world?&s.world:type==kinoko::graphics::transform_view?&s.view:type==kinoko::graphics::transform_projection?&s.projection:nullptr;if(!m)return bad("Unknown transform");std::memcpy(m,in,sizeof(*m));return S_OK;}
HRESULT Device::SetFVF(DWORD value){
    std::lock_guard<std::recursive_mutex> cpu_guard(state->cpu_mutex);if(value!=324&&value!=0x4142&&value!=(kinoko::graphics::vertex_xyzrhw|kinoko::graphics::vertex_diffuse))return bad("Unsupported vertex layout");state->fvf=value;state->mesh_decl=false;return S_OK;}
HRESULT Device::DrawPrimitiveUP(kinoko::graphics::Primitive type,UINT primitives,const void* data,UINT stride){
    std::lock_guard<std::recursive_mutex> cpu_guard(state->cpu_mutex);
    if(type!=kinoko::graphics::primitive_trianglestrip||!data||primitives<1||primitives>2||stride<(state->fvf==(kinoko::graphics::vertex_xyzrhw|kinoko::graphics::vertex_diffuse)?20u:28u))return bad("Unsupported immediate primitive");
    try{gpu::Draw draw;draw.clip_space=state->fvf==0x4142;const auto* bytes=static_cast<const uint8_t*>(data);
        const unsigned order[]={0,1,2,2,1,3};
        for(unsigned i=0;i<primitives*3;++i){KinokoSpriteVertex v{};std::memcpy(&v,bytes+order[i]*stride,state->fvf==(kinoko::graphics::vertex_xyzrhw|kinoko::graphics::vertex_diffuse)?20:28);
            draw.triangles.push_back(state->vertex(v.x,v.y,v.z,v.rhw,v.color,v.u,v.v,draw.clip_space));}
        state->draw(std::move(draw));return S_OK;
    }catch(const std::exception& e){return bad(e.what());}
}
HRESULT Device::CreateIndexBuffer(UINT bytes,DWORD usage,kinoko::graphics::Format format,kinoko::graphics::Pool,Buffer** out,HANDLE*){
    std::lock_guard<std::recursive_mutex> cpu_guard(state->cpu_mutex);if(!out||usage||format!=kinoko::graphics::format_index16)return bad("Unsupported index buffer");*out=new Buffer(bytes);return S_OK;}
HRESULT Device::CreateVertexBuffer(UINT bytes,DWORD usage,DWORD,kinoko::graphics::Pool,Buffer** out,HANDLE*){
    std::lock_guard<std::recursive_mutex> cpu_guard(state->cpu_mutex);if(!out||usage)return bad("Unsupported vertex buffer");*out=new Buffer(bytes);return S_OK;}
HRESULT Device::CreateVertexDeclaration(const kinoko::graphics::VertexElement* elements,Declaration** out){
    std::lock_guard<std::recursive_mutex> cpu_guard(state->cpu_mutex);if(!elements||!out)return E_POINTER;
    const kinoko::graphics::VertexElement expected[]={
        {0,0,kinoko::graphics::element_float3,kinoko::graphics::declaration_method_default,kinoko::graphics::semantic_position,0},
        {1,0,kinoko::graphics::element_float3,kinoko::graphics::declaration_method_default,kinoko::graphics::semantic_normal,0},
        {2,0,kinoko::graphics::element_float2,kinoko::graphics::declaration_method_default,kinoko::graphics::semantic_texcoord,0},kinoko::graphics::declaration_end()};
    if(std::memcmp(elements,expected,sizeof(expected)))return bad("Unknown mesh vertex declaration");
    *out=new Declaration;return S_OK;}
HRESULT Device::SetVertexDeclaration(Declaration* value){
    std::lock_guard<std::recursive_mutex> cpu_guard(state->cpu_mutex);if(!value)return bad("Missing mesh declaration");state->mesh_decl=true;return S_OK;}
HRESULT Device::SetStreamSource(UINT slot,Buffer* value,UINT offset,UINT stride){
    std::lock_guard<std::recursive_mutex> cpu_guard(state->cpu_mutex);if(slot>=3||!value||offset>value->bytes.size())return bad("Invalid mesh stream");value->AddRef();auto*& old=state->streams[slot];if(old)old->Release();old=value;state->offsets[slot]=offset;state->strides[slot]=stride;return S_OK;}
HRESULT Device::SetIndices(Buffer* value){
    std::lock_guard<std::recursive_mutex> cpu_guard(state->cpu_mutex);if(!value)return bad("Missing mesh indices");value->AddRef();if(state->indices)state->indices->Release();state->indices=value;return S_OK;}
HRESULT Device::DrawIndexedPrimitive(kinoko::graphics::Primitive type,INT base,UINT minimum,UINT count,UINT start,UINT primitives){
    std::lock_guard<std::recursive_mutex> cpu_guard(state->cpu_mutex);
    auto& s=*state;if(type!=kinoko::graphics::primitive_trianglelist||!s.mesh_decl||!s.indices||!s.streams[0]||!s.streams[2]||uint64_t(start+uint64_t(primitives)*3)*2>s.indices->bytes.size())return bad("Invalid indexed mesh draw");
    try{gpu::Draw draw;draw.clip_space=true;
        for(uint64_t i=0;i<uint64_t(primitives)*3;++i){uint16_t index;std::memcpy(&index,s.indices->bytes.data()+(start+i)*2,2);
            if(index<minimum||uint64_t(index)>=uint64_t(minimum)+count||int64_t(index)+base<0)throw std::runtime_error("Mesh index outside declared range");
            const auto vertex=uint64_t(int64_t(index)+base);float xyz[3],uv[2];
            const auto read=[&](unsigned stream,void* out,size_t n){const auto offset=s.offsets[stream]+vertex*s.strides[stream];if(offset+n>s.streams[stream]->bytes.size())throw std::runtime_error("Mesh stream bounds");std::memcpy(out,s.streams[stream]->bytes.data()+offset,n);};
            read(0,xyz,sizeof(xyz));read(2,uv,sizeof(uv));
            // Default fixed-function lighting has no active lights or ambient.
            const DWORD color=s.states[kinoko::graphics::state_lighting]?0xff000000u:0xffffffffu;
            draw.triangles.push_back(s.vertex(xyz[0],xyz[1],xyz[2],1,color,uv[0],uv[1],true));
        }s.draw(std::move(draw));return S_OK;
    }catch(const std::exception& e){return bad(e.what());}
}
HRESULT Factory::GetAdapterDisplayMode(UINT adapter,kinoko::graphics::DisplayMode* out){if(adapter||!out)return E_INVALIDARG;*out={640,480,60,kinoko::graphics::format_x8r8g8b8};return S_OK;}
HRESULT Factory::CreateDevice(UINT adapter,kinoko::graphics::DeviceKind,HWND window,DWORD,kinoko::graphics::Presentation* p,Device** out){if(adapter||!p||!out)return E_INVALIDARG;
    try{*out=new Device(window,p->BackBufferWidth,p->BackBufferHeight);return S_OK;}catch(const std::exception& e){return bad(e.what());}}
Factory* create_factory(UINT){std::lock_guard<std::mutex> lock(errors_mutex);error.clear();return new Factory;}
}
HRESULT kinoko::graphics::create_texture(kinoko::graphics::Device*,UINT width,UINT height,UINT levels,DWORD usage,kinoko::graphics::Format format,kinoko::graphics::Pool,kinoko::graphics::Texture** out){
    using namespace kinoko::graphics;
    if(!out||!width||!height||width>16384||height>16384||levels!=1||(format!=kinoko::graphics::format_a1r5g5b5&&format!=kinoko::graphics::format_a8r8g8b8)||(usage&&usage!=kinoko::graphics::usage_rendertarget))return E_INVALIDARG;
    *out=new Texture(std::make_shared<Image>(width,height,format,usage==kinoko::graphics::usage_rendertarget));return S_OK;
}
