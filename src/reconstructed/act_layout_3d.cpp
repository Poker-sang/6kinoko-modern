#include "kinoko/mesh_resource.hpp"
#include "kinoko/act_method_dispatch.hpp"
#include "kinoko/act_layout_3d.hpp"
#include "kinoko/map_layout_records.hpp"
#include "kinoko/graphics_device.h"
#include "kinoko/legacy_memory.hpp"
#include <cstdlib>
#include <cstring>

#include "kinoko/matrix_math.hpp"
namespace {
using kinoko::math::Matrix;
kinoko::graphics::Matrix* matrix_result(kinoko::graphics::Matrix* out,const Matrix& value){std::memcpy(out,&value,sizeof(value));return out;}
kinoko::graphics::Matrix* matrix_rotation(kinoko::graphics::Matrix* out,float yaw,float pitch,float roll){return matrix_result(out,kinoko::math::rotation(yaw,pitch,roll));}
kinoko::graphics::Matrix* matrix_translation(kinoko::graphics::Matrix* out,float x,float y,float z){return matrix_result(out,kinoko::math::translation(x,y,z));}
kinoko::graphics::Matrix* matrix_scaling(kinoko::graphics::Matrix* out,float x,float y,float z){return matrix_result(out,kinoko::math::scaling(x,y,z));}
kinoko::graphics::Matrix* matrix_multiply(kinoko::graphics::Matrix* out,const kinoko::graphics::Matrix* a,const kinoko::graphics::Matrix* b){Matrix x,y;std::memcpy(&x,a,sizeof(x));std::memcpy(&y,b,sizeof(y));return matrix_result(out,kinoko::math::multiply(x,y));}
}

namespace kinoko::act {
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
    kinoko::graphics::Matrix world{}, operation{};
    world.m[0][0]=world.m[1][1]=world.m[2][2]=world.m[3][3]=1.0f;
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
    kinoko::graphics::Matrix previous{},world{};
    device->GetRenderState(kinoko::graphics::state_zenable,&depth);
    device->GetRenderState(kinoko::graphics::state_zwriteenable,&write_depth);
    device->GetRenderState(kinoko::graphics::state_alphablendenable,&alpha);
    device->SetRenderState(kinoko::graphics::state_zenable,1);
    device->SetRenderState(kinoko::graphics::state_zwriteenable,1);
    device->SetRenderState(kinoko::graphics::state_alphablendenable,1);
    device->GetTransform(kinoko::graphics::transform_world,&previous);
    std::memcpy(&world,layout->world,sizeof(world));
    device->SetTransform(kinoko::graphics::transform_world,&world);
    void* converted = nullptr;
    // 43CB5D deliberately returns before restoring the state on failed Query.
    if (!ResourceMethods(resource).query_type(&resource_type_descriptor(ResourceKind::mesh), &converted)) return E_FAIL;
    static_cast<kinoko::mesh::Resource*>(converted)->draw();
    device->SetTransform(kinoko::graphics::transform_world,&previous);
    device->SetRenderState(kinoko::graphics::state_zenable,depth);
    device->SetRenderState(kinoko::graphics::state_zwriteenable,write_depth);
    device->SetRenderState(kinoko::graphics::state_alphablendenable,alpha);
    return S_OK;
}

// 43C220 copies all nine scalar fields, the borrowed layer, and the matrix.
Layout3DRecord *clone_layout_3d(const Layout3DRecord *layout) {
    auto *copy=static_cast<Layout3DRecord *>(std::malloc(sizeof(Layout3DRecord)));
    if (copy) std::memcpy(copy,layout,sizeof(*copy));
    return copy;
}
}
