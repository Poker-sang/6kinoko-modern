#include "kinoko/act_layout_3d.hpp"
#include "kinoko/map_layout_records.hpp"
#include "kinoko/graphics_device.h"
#include "kinoko/legacy_memory.hpp"
#include <cstdlib>
#include <cstring>

#include "kinoko/matrix_math.hpp"
namespace {
using kinoko::math::Matrix;
D3DMATRIX* matrix_result(D3DMATRIX* out,const Matrix& value){std::memcpy(out,&value,sizeof(value));return out;}
D3DMATRIX* matrix_rotation(D3DMATRIX* out,float yaw,float pitch,float roll){return matrix_result(out,kinoko::math::rotation(yaw,pitch,roll));}
D3DMATRIX* matrix_translation(D3DMATRIX* out,float x,float y,float z){return matrix_result(out,kinoko::math::translation(x,y,z));}
D3DMATRIX* matrix_scaling(D3DMATRIX* out,float x,float y,float z){return matrix_result(out,kinoko::math::scaling(x,y,z));}
D3DMATRIX* matrix_multiply(D3DMATRIX* out,const D3DMATRIX* a,const D3DMATRIX* b){Matrix x,y;std::memcpy(&x,a,sizeof(x));std::memcpy(&y,b,sizeof(y));return matrix_result(out,kinoko::math::multiply(x,y));}
}

namespace kinoko::act {
namespace {
using kinoko::legacy::load;
using Query = uint8_t (__thiscall *)(KinokoActResource *,const void *,void **);
struct ResourceMethods { void *write,*read; Query query; };
struct ResourcePrefix { const ResourceMethods *methods; };
struct RenderNode;
using Draw = uint8_t (__thiscall *)(void *);
struct RenderMethods { void *destroy,*set_controller,*set_mesh; Draw draw; };
struct RenderPrefix { const RenderMethods *methods; };
struct RenderNode { RenderNode *next,*previous; RenderPrefix render; };
struct MeshListPrefix { unsigned char preceding[236]; RenderNode *head; };
static_assert(offsetof(RenderNode,render)==8);
static_assert(offsetof(MeshListPrefix,head)==236);
}

// 43C690 does not install the 2D layer's transform-property aliases.
int32_t bind_layout_3d(Layout3DRecord *layout, KinokoActLayer *layer) {
    if (!layer) return E_FAIL;
    layout->layer=layer;
    return S_OK;
}

int32_t update_layout_3d(Layout3DRecord *layout) {
    if (!layout->layer) return E_FAIL;
    const map::LayerView owner(layout->layer);
    if (!owner.get(&map::LayerRecord::visible)) return S_OK;
    if (!owner.get(&map::LayerRecord::resource)) return E_FAIL;
    D3DMATRIX world{}, operation{};
    world._11=world._22=world._33=world._44=1.0f;
    // 43C920: yaw/pitch/roll in radians, then translation, then scaling.
    // Do not substitute the usual S*R*T or the owning layer's world position.
    matrix_rotation(&operation,layout->rotation.x,layout->rotation.y,layout->rotation.z);
    matrix_multiply(&world,&world,&operation);
    matrix_translation(&operation,layout->translation.x,layout->translation.y,layout->translation.z);
    matrix_multiply(&world,&world,&operation);
    matrix_scaling(&operation,layout->scale.x,layout->scale.y,layout->scale.z);
    matrix_multiply(&world,&world,&operation);
    std::memcpy(layout->world,&world,sizeof(world));
    return S_OK;
}

int32_t draw_layout_3d(Layout3DRecord *layout) {
    if (!layout->layer) return E_FAIL;
    const map::LayerView owner(layout->layer);
    if (!owner.get(&map::LayerRecord::visible)) return S_OK;
    auto *resource=owner.get(&map::LayerRecord::resource);
    if (!resource) return E_FAIL;
    auto *device=kinoko_graphics.device;
    DWORD depth{},write_depth{},alpha{};
    D3DMATRIX previous{},world{};
    device->GetRenderState(D3DRS_ZENABLE,&depth);
    device->GetRenderState(D3DRS_ZWRITEENABLE,&write_depth);
    device->GetRenderState(D3DRS_ALPHABLENDENABLE,&alpha);
    device->SetRenderState(D3DRS_ZENABLE,1);
    device->SetRenderState(D3DRS_ZWRITEENABLE,1);
    device->SetRenderState(D3DRS_ALPHABLENDENABLE,1);
    device->GetTransform(D3DTS_WORLD,&previous);
    std::memcpy(&world,layout->world,sizeof(world));
    device->SetTransform(D3DTS_WORLD,&world);
    struct Descriptor { void *methods,*cache; char name[sizeof(".?AVCActResourceMesh@@")]; };
    static const Descriptor type{nullptr,nullptr,".?AVCActResourceMesh@@"};
    void *converted=nullptr;
    const auto methods=load<ResourcePrefix>(resource).methods;
    // 43CB5D deliberately returns before restoring the state on failed Query.
    if (!methods->query(resource,&type,&converted)) return E_FAIL;
    auto *head=load<MeshListPrefix>(converted).head;
    for (auto *node=head->next;node!=head;node=node->next)
        node->render.methods->draw(&node->render);
    device->SetTransform(D3DTS_WORLD,&previous);
    device->SetRenderState(D3DRS_ZENABLE,depth);
    device->SetRenderState(D3DRS_ZWRITEENABLE,write_depth);
    device->SetRenderState(D3DRS_ALPHABLENDENABLE,alpha);
    return S_OK;
}

// 43C220 copies all nine scalar fields, the borrowed layer, and the matrix.
Layout3DRecord *clone_layout_3d(const Layout3DRecord *layout) {
    auto *copy=static_cast<Layout3DRecord *>(std::malloc(sizeof(Layout3DRecord)));
    if (copy) std::memcpy(copy,layout,sizeof(*copy));
    return copy;
}
}
