#include "kinoko/gpu_legacy_api.hpp"
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
namespace kinoko::gpu_legacy {
namespace {
std::mutex errors_mutex;std::string error;Device* active=nullptr;
HRESULT bad(const char* message){std::lock_guard<std::mutex> lock(errors_mutex);if(error.empty())error=message;return D3DERR_INVALIDCALL;}
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
    UINT width,height;D3DFORMAT format;bool target;
    std::shared_ptr<const Pixels> pixels;
    Image(UINT w,UINT h,D3DFORMAT f,bool t):width(w),height(h),format(f),target(t){}
};
HRESULT Surface::GetDesc(D3DSURFACE_DESC* out){if(!out)return bad("Null surface description");*out={};out->Width=image->width;out->Height=image->height;out->Format=image->format;out->Type=D3DRTYPE_SURFACE;return S_OK;}
HRESULT Texture::LockRect(UINT level,D3DLOCKED_RECT* out,const RECT* rect,DWORD flags){
    std::lock_guard<std::mutex> lock(image->mutex);
    if(level||!out||locked||image->target||flags)return bad("Unsupported texture map request");
    const UINT bpp=image->format==D3DFMT_A1R5G5B5?2:4;
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
    DWORD fvf=D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX1;
    math::Matrix world=math::identity(),view=math::identity(),projection=math::identity();
    std::vector<RecordedPass> recording;std::mutex queue_mutex;std::deque<std::vector<RecordedPass>> queue;
    struct Uploaded {gpu::TextureId id;std::weak_ptr<Image> image;std::weak_ptr<const Pixels> pixels;bool target;};
    std::map<std::pair<uint64_t,uint64_t>,Uploaded> uploaded;
    State(HWND w,UINT x,UINT y):window(w),width(x),height(y) {
        const auto vs=shader("sprite.vert.dxbc"),fs=shader("sprite.frag.dxbc");
        renderer=std::make_unique<gpu::Renderer>(platform::host().window(),gpu::ShaderEncoding::dxbc,gpu::ShaderCode{vs.data(),vs.size()},gpu::ShaderCode{fs.data(),fs.size()});
        auto screen=std::make_shared<Image>(x,y,D3DFMT_A8R8G8B8,true);screen->id=0;
        back=new Surface(screen);depth=new Surface(screen);target=back;target->AddRef();
        states[D3DRS_ZENABLE]=states[D3DRS_ZWRITEENABLE]=TRUE;states[D3DRS_ZFUNC]=D3DCMP_LESSEQUAL;
        states[D3DRS_ALPHAFUNC]=D3DCMP_ALWAYS;states[D3DRS_SRCBLEND]=D3DBLEND_ONE;states[D3DRS_DESTBLEND]=D3DBLEND_ZERO;
        states[D3DRS_BLENDOP]=D3DBLENDOP_ADD;states[D3DRS_CULLMODE]=D3DCULL_CCW;states[D3DRS_LIGHTING]=TRUE;
        for(auto& s:samplers){s[D3DSAMP_ADDRESSU]=s[D3DSAMP_ADDRESSV]=D3DTADDRESS_WRAP;s[D3DSAMP_MAGFILTER]=s[D3DSAMP_MINFILTER]=D3DTEXF_POINT;}
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
        switch(value){case D3DBLEND_ZERO:return render::BlendFactor::zero;case D3DBLEND_ONE:return render::BlendFactor::one;
        case D3DBLEND_SRCCOLOR:return render::BlendFactor::source_color;case D3DBLEND_DESTCOLOR:return render::BlendFactor::destination_color;
        case D3DBLEND_SRCALPHA:return render::BlendFactor::source_alpha;case D3DBLEND_INVSRCALPHA:return render::BlendFactor::inverse_source_alpha;
        default:throw std::runtime_error("Unsupported blend factor");}
    }
    void draw(gpu::Draw draw){
        if(!scene)throw std::runtime_error("Drawing outside a GPU scene");
        draw.blend.enabled=states[D3DRS_ALPHABLENDENABLE]!=0;draw.blend.source=blend(states[D3DRS_SRCBLEND]);draw.blend.destination=blend(states[D3DRS_DESTBLEND]);
        draw.blend.operation=states[D3DRS_BLENDOP]==D3DBLENDOP_ADD?render::BlendOperation::add:render::BlendOperation::reverse_subtract;
        draw.raster.depth_test=states[D3DRS_ZENABLE]!=0;draw.raster.depth_write=states[D3DRS_ZWRITEENABLE]!=0;
        draw.raster.depth_compare=gpu::Compare(states[D3DRS_ZFUNC]-1);draw.raster.alpha_test=states[D3DRS_ALPHATESTENABLE]!=0;
        draw.raster.alpha_compare=gpu::Compare(states[D3DRS_ALPHAFUNC]-1);draw.raster.alpha_reference=uint8_t(states[D3DRS_ALPHAREF]);
        draw.raster.cull=states[D3DRS_CULLMODE]==D3DCULL_NONE?0:states[D3DRS_CULLMODE]==D3DCULL_CW?1:2;
        draw.linear=samplers[0][D3DSAMP_MAGFILTER]==D3DTEXF_LINEAR;
        draw.wrap_u=samplers[0][D3DSAMP_ADDRESSU]==D3DTADDRESS_WRAP;draw.wrap_v=samplers[0][D3DSAMP_ADDRESSV]==D3DTADDRESS_WRAP;
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
                if(image.format==D3DFMT_A1R5G5B5){uint16_t p;std::memcpy(&p,snap.pixels->data()+i*2,2);
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
HRESULT Device::GetDeviceCaps(D3DCAPS9* out){
    std::lock_guard<std::recursive_mutex> cpu_guard(state->cpu_mutex);if(!out)return E_POINTER;*out={};out->MaxTextureWidth=out->MaxTextureHeight=16384;return S_OK;}
HRESULT Device::GetSwapChain(UINT index,SwapChain** out){
    std::lock_guard<std::recursive_mutex> cpu_guard(state->cpu_mutex);if(index||!out)return bad("Unsupported swapchain index");*out=new SwapChain(this);return S_OK;}
HRESULT Device::TestCooperativeLevel(){
    try{state->poll();}catch(const std::exception& e){bad(e.what());}
    std::string message;{std::lock_guard<std::mutex> lock(errors_mutex);message=error;}
    if(!message.empty()){
        if(!state->reported){state->reported=true;MessageBoxA(state->window,message.c_str(),"SDL GPU renderer error",MB_OK|MB_ICONERROR);SDL_Event quit{};quit.type=SDL_EVENT_QUIT;SDL_PushEvent(&quit);}return D3DERR_DEVICELOST;
    }return D3D_OK;
}
HRESULT Device::Reset(D3DPRESENT_PARAMETERS* p){
    std::lock_guard<std::recursive_mutex> cpu_guard(state->cpu_mutex);if(!p)return E_POINTER;return SDL_SetWindowFullscreen(platform::host().window(),p->Windowed==FALSE)?S_OK:bad(SDL_GetError());}
HRESULT Device::BeginScene(){
    std::lock_guard<std::recursive_mutex> cpu_guard(state->cpu_mutex);auto& s=*state;std::lock_guard<std::mutex> lock(s.queue_mutex);if(s.closed||s.queue.size()>=3)return D3DERR_WASSTILLDRAWING;if(s.scene)return bad("Nested GPU scene");s.recording.clear();s.scene=true;return S_OK;}
HRESULT Device::EndScene(){
    std::lock_guard<std::recursive_mutex> cpu_guard(state->cpu_mutex);auto& s=*state;if(!s.scene)return bad("Unbalanced GPU EndScene");s.scene=false;std::lock_guard<std::mutex> lock(s.queue_mutex);if(!s.closed&&!s.recording.empty())s.queue.push_back(std::move(s.recording));return S_OK;}
HRESULT Device::present(){
    std::lock_guard<std::recursive_mutex> cpu_guard(state->cpu_mutex);return S_OK;} // Submission acknowledgement; main-thread poll owns actual presentation.
HRESULT SwapChain::Present(const RECT*,const RECT*,HWND,const RGNDATA*,DWORD){return device->present();}
HRESULT Device::Clear(DWORD count,const D3DRECT*,DWORD flags,D3DCOLOR color,float z,DWORD stencil){
    std::lock_guard<std::recursive_mutex> cpu_guard(state->cpu_mutex);
    if(count||z!=1||stencil!=0)return bad("Unsupported partial or nondefault GPU clear");
    auto& s=*state;if(!s.scene)return S_OK;auto* p=&s.pass();
    if(!p->pass.draws.empty()){RecordedPass next;next.target=p->target;next.pass.logical_width=p->pass.logical_width;next.pass.logical_height=p->pass.logical_height;next.pass.clear=false;next.pass.clear_depth=false;next.pass.clear_stencil=false;s.recording.push_back(std::move(next));p=&s.recording.back();}
    if(flags&D3DCLEAR_TARGET){p->pass.clear=true;p->pass.clear_color={float((color>>16)&255)/255,float((color>>8)&255)/255,float(color&255)/255,float(color>>24)/255};}
    if(flags&D3DCLEAR_ZBUFFER)p->pass.clear_depth=true;
    if(flags&D3DCLEAR_STENCIL)p->pass.clear_stencil=true;return S_OK;
}
HRESULT Device::GetRenderState(D3DRENDERSTATETYPE type,DWORD* out){
    std::lock_guard<std::recursive_mutex> cpu_guard(state->cpu_mutex);if(!out||UINT(type)>=state->states.size())return bad("Invalid render state query");*out=state->states[type];return S_OK;}
HRESULT Device::SetRenderState(D3DRENDERSTATETYPE type,DWORD value){
    std::lock_guard<std::recursive_mutex> cpu_guard(state->cpu_mutex);
    switch(type){
    case D3DRS_ZENABLE:if(value>1)return bad("W-buffering is unsupported");break;
    case D3DRS_ZWRITEENABLE:case D3DRS_ALPHABLENDENABLE:case D3DRS_ALPHATESTENABLE:case D3DRS_LIGHTING:break;
    case D3DRS_ZFUNC:case D3DRS_ALPHAFUNC:if(value<1||value>8)return bad("Unsupported comparison");break;
    case D3DRS_CULLMODE:if(value<1||value>3)return bad("Unsupported winding");break;
    case D3DRS_BLENDOP:if(value!=D3DBLENDOP_ADD&&value!=D3DBLENDOP_REVSUBTRACT)return bad("Unsupported blend operation");break;
    case D3DRS_SRCBLEND:case D3DRS_DESTBLEND:try{State::blend(value);}catch(...){return bad("Unsupported blend factor");}break;
    case D3DRS_ALPHAREF:case D3DRS_STENCILMASK:break;
    case D3DRS_FILLMODE:if(value!=D3DFILL_SOLID)return bad("Only solid mesh fill is implemented");break;
    default:return bad("Unimplemented render state requested");
    }state->states[type]=value;return S_OK;
}
HRESULT Device::GetSamplerState(DWORD stage,D3DSAMPLERSTATETYPE type,DWORD* out){
    std::lock_guard<std::recursive_mutex> cpu_guard(state->cpu_mutex);if(stage>=8||UINT(type)>=16||!out)return bad("Invalid sampler query");*out=state->samplers[stage][type];return S_OK;}
HRESULT Device::SetSamplerState(DWORD stage,D3DSAMPLERSTATETYPE type,DWORD value){
    std::lock_guard<std::recursive_mutex> cpu_guard(state->cpu_mutex);if(stage||UINT(type)>=16)return bad("Unsupported sampler stage");
    if(type==D3DSAMP_ADDRESSU||type==D3DSAMP_ADDRESSV){if(value!=D3DTADDRESS_WRAP&&value!=D3DTADDRESS_CLAMP)return bad("Unsupported addressing");}
    else if(type==D3DSAMP_MINFILTER||type==D3DSAMP_MAGFILTER||type==D3DSAMP_MIPFILTER){if(value!=D3DTEXF_POINT&&value!=D3DTEXF_LINEAR)return bad("Unsupported filtering");}
    else return bad("Unsupported sampler property");state->samplers[stage][type]=value;return S_OK;}
HRESULT Device::SetTextureStageState(DWORD stage,D3DTEXTURESTAGESTATETYPE type,DWORD value){
    std::lock_guard<std::recursive_mutex> cpu_guard(state->cpu_mutex);return !stage&&type==D3DTSS_ALPHAOP&&value==D3DTOP_MODULATE?S_OK:bad("Unsupported texture-stage operation");}
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
HRESULT Device::GetTransform(D3DTRANSFORMSTATETYPE type,D3DMATRIX* out){
    std::lock_guard<std::recursive_mutex> cpu_guard(state->cpu_mutex);if(!out)return E_POINTER;auto& s=*state;const auto* m=type==D3DTS_WORLD?&s.world:type==D3DTS_VIEW?&s.view:type==D3DTS_PROJECTION?&s.projection:nullptr;if(!m)return bad("Unknown transform");std::memcpy(out,m,sizeof(*m));return S_OK;}
HRESULT Device::SetTransform(D3DTRANSFORMSTATETYPE type,const D3DMATRIX* in){
    std::lock_guard<std::recursive_mutex> cpu_guard(state->cpu_mutex);if(!in)return E_POINTER;auto& s=*state;auto* m=type==D3DTS_WORLD?&s.world:type==D3DTS_VIEW?&s.view:type==D3DTS_PROJECTION?&s.projection:nullptr;if(!m)return bad("Unknown transform");std::memcpy(m,in,sizeof(*m));return S_OK;}
HRESULT Device::SetFVF(DWORD value){
    std::lock_guard<std::recursive_mutex> cpu_guard(state->cpu_mutex);if(value!=324&&value!=0x4142&&value!=(D3DFVF_XYZRHW|D3DFVF_DIFFUSE))return bad("Unsupported vertex layout");state->fvf=value;state->mesh_decl=false;return S_OK;}
HRESULT Device::DrawPrimitiveUP(D3DPRIMITIVETYPE type,UINT primitives,const void* data,UINT stride){
    std::lock_guard<std::recursive_mutex> cpu_guard(state->cpu_mutex);
    if(type!=D3DPT_TRIANGLESTRIP||!data||primitives<1||primitives>2||stride<(state->fvf==(D3DFVF_XYZRHW|D3DFVF_DIFFUSE)?20u:28u))return bad("Unsupported immediate primitive");
    try{gpu::Draw draw;draw.clip_space=state->fvf==0x4142;const auto* bytes=static_cast<const uint8_t*>(data);
        const unsigned order[]={0,1,2,2,1,3};
        for(unsigned i=0;i<primitives*3;++i){KinokoSpriteVertex v{};std::memcpy(&v,bytes+order[i]*stride,state->fvf==(D3DFVF_XYZRHW|D3DFVF_DIFFUSE)?20:28);
            draw.triangles.push_back(state->vertex(v.x,v.y,v.z,v.rhw,v.color,v.u,v.v,draw.clip_space));}
        state->draw(std::move(draw));return S_OK;
    }catch(const std::exception& e){return bad(e.what());}
}
HRESULT Device::CreateIndexBuffer(UINT bytes,DWORD usage,D3DFORMAT format,D3DPOOL,Buffer** out,HANDLE*){
    std::lock_guard<std::recursive_mutex> cpu_guard(state->cpu_mutex);if(!out||usage||format!=D3DFMT_INDEX16)return bad("Unsupported index buffer");*out=new Buffer(bytes);return S_OK;}
HRESULT Device::CreateVertexBuffer(UINT bytes,DWORD usage,DWORD,D3DPOOL,Buffer** out,HANDLE*){
    std::lock_guard<std::recursive_mutex> cpu_guard(state->cpu_mutex);if(!out||usage)return bad("Unsupported vertex buffer");*out=new Buffer(bytes);return S_OK;}
HRESULT Device::CreateVertexDeclaration(const D3DVERTEXELEMENT9* elements,Declaration** out){
    std::lock_guard<std::recursive_mutex> cpu_guard(state->cpu_mutex);if(!elements||!out)return E_POINTER;
    const D3DVERTEXELEMENT9 expected[]={
        {0,0,D3DDECLTYPE_FLOAT3,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_POSITION,0},
        {1,0,D3DDECLTYPE_FLOAT3,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_NORMAL,0},
        {2,0,D3DDECLTYPE_FLOAT2,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_TEXCOORD,0},D3DDECL_END()};
    if(std::memcmp(elements,expected,sizeof(expected)))return bad("Unknown mesh vertex declaration");
    *out=new Declaration;return S_OK;}
HRESULT Device::SetVertexDeclaration(Declaration* value){
    std::lock_guard<std::recursive_mutex> cpu_guard(state->cpu_mutex);if(!value)return bad("Missing mesh declaration");state->mesh_decl=true;return S_OK;}
HRESULT Device::SetStreamSource(UINT slot,Buffer* value,UINT offset,UINT stride){
    std::lock_guard<std::recursive_mutex> cpu_guard(state->cpu_mutex);if(slot>=3||!value||offset>value->bytes.size())return bad("Invalid mesh stream");value->AddRef();auto*& old=state->streams[slot];if(old)old->Release();old=value;state->offsets[slot]=offset;state->strides[slot]=stride;return S_OK;}
HRESULT Device::SetIndices(Buffer* value){
    std::lock_guard<std::recursive_mutex> cpu_guard(state->cpu_mutex);if(!value)return bad("Missing mesh indices");value->AddRef();if(state->indices)state->indices->Release();state->indices=value;return S_OK;}
HRESULT Device::DrawIndexedPrimitive(D3DPRIMITIVETYPE type,INT base,UINT minimum,UINT count,UINT start,UINT primitives){
    std::lock_guard<std::recursive_mutex> cpu_guard(state->cpu_mutex);
    auto& s=*state;if(type!=D3DPT_TRIANGLELIST||!s.mesh_decl||!s.indices||!s.streams[0]||!s.streams[2]||uint64_t(start+uint64_t(primitives)*3)*2>s.indices->bytes.size())return bad("Invalid indexed mesh draw");
    try{gpu::Draw draw;draw.clip_space=true;
        for(uint64_t i=0;i<uint64_t(primitives)*3;++i){uint16_t index;std::memcpy(&index,s.indices->bytes.data()+(start+i)*2,2);
            if(index<minimum||uint64_t(index)>=uint64_t(minimum)+count||int64_t(index)+base<0)throw std::runtime_error("Mesh index outside declared range");
            const auto vertex=uint64_t(int64_t(index)+base);float xyz[3],uv[2];
            const auto read=[&](unsigned stream,void* out,size_t n){const auto offset=s.offsets[stream]+vertex*s.strides[stream];if(offset+n>s.streams[stream]->bytes.size())throw std::runtime_error("Mesh stream bounds");std::memcpy(out,s.streams[stream]->bytes.data()+offset,n);};
            read(0,xyz,sizeof(xyz));read(2,uv,sizeof(uv));
            // Default fixed-function lighting has no active lights or ambient.
            const DWORD color=s.states[D3DRS_LIGHTING]?0xff000000u:0xffffffffu;
            draw.triangles.push_back(s.vertex(xyz[0],xyz[1],xyz[2],1,color,uv[0],uv[1],true));
        }s.draw(std::move(draw));return S_OK;
    }catch(const std::exception& e){return bad(e.what());}
}
HRESULT Factory::GetAdapterDisplayMode(UINT adapter,D3DDISPLAYMODE* out){if(adapter||!out)return E_INVALIDARG;*out={640,480,60,D3DFMT_X8R8G8B8};return S_OK;}
HRESULT Factory::CreateDevice(UINT adapter,D3DDEVTYPE,HWND window,DWORD,D3DPRESENT_PARAMETERS* p,Device** out){if(adapter||!p||!out)return E_INVALIDARG;
    try{*out=new Device(window,p->BackBufferWidth,p->BackBufferHeight);return S_OK;}catch(const std::exception& e){return bad(e.what());}}
Factory* create_factory(UINT){std::lock_guard<std::mutex> lock(errors_mutex);error.clear();return new Factory;}
}
extern "C" HRESULT WINAPI kinoko_gpu_create_texture(IDirect3DDevice9*,UINT width,UINT height,UINT levels,DWORD usage,D3DFORMAT format,D3DPOOL,IDirect3DTexture9** out){
    using namespace kinoko::gpu_legacy;
    if(!out||!width||!height||width>16384||height>16384||levels!=1||(format!=D3DFMT_A1R5G5B5&&format!=D3DFMT_A8R8G8B8)||(usage&&usage!=D3DUSAGE_RENDERTARGET))return E_INVALIDARG;
    *out=new Texture(std::make_shared<Image>(width,height,format,usage==D3DUSAGE_RENDERTARGET));return S_OK;
}
