#pragma once
#include "kinoko/mesh_model.hpp"
#include "kinoko/legacy_string.hpp"
#include "kinoko/resource_allocation.hpp"
#include <map>

namespace kinoko::mesh {
// Controller nodes borrow models retained by ResourceState, and own children
// and material references. Registry and name lookup entries remain borrowed.
struct Controller {
    const Node* model = nullptr;
    std::int32_t registry_index = 0, reference_group = 0;
    std::vector<std::unique_ptr<Controller>> children;
    std::vector<std::shared_ptr<Material>> material_owners;
};
struct ResourceState {
    std::unique_ptr<Controller> root;
    std::vector<std::shared_ptr<Node>> model_owners;
    std::vector<Controller*> registry;
    std::map<std::string,Controller*> named;
};
// Actual native virtual interface, independent of GPU and original byte tables.
class Render {
public:
    virtual ~Render() = default;
    virtual std::uint8_t draw() = 0;
    virtual void replace_texture(const char* name, std::int32_t handle) = 0;
};
struct Resource final {
    const void* methods = nullptr;
    std::int32_t id = -1;
    legacy::StringRecord name{{},0,15};
    legacy::StringRecord mesh_name{{},0,15};
    legacy::StringRecord prefix{{},0,15};
    std::unique_ptr<ResourceState> state;
    std::vector<std::unique_ptr<Render>> renders;

    Resource() noexcept = default;
    Resource(const Resource&) = delete;
    Resource& operator=(const Resource&) = delete;
    ~Resource() { clear(); }
    void clear() noexcept {
        // Original destruction releases the controller before render buffers.
        state.reset();
        clear_renders();
        legacy::StringView(&prefix).destroy();
        legacy::StringView(&mesh_name).destroy();
        legacy::StringView(&name).destroy();
    }
    void clear_renders() noexcept {
        // The old list deleted from its front. Do not rely on vector's
        // implementation-specific element destruction order.
        for (auto& render : renders) render.reset();
        renders.clear();
    }
    void reset_for_load() {
        // Reload has the opposite order: discard renderers before old models.
        clear_renders();
        state.reset();
        state = std::make_unique<ResourceState>();
    }
    bool copy_names(const Resource& source) {
        for (auto member : {&Resource::name,&Resource::mesh_name,&Resource::prefix}) {
            legacy::StringView input(const_cast<legacy::StringRecord*>(&(source.*member)));
            legacy::StringView(&(this->*member)).assign(input.data(),input.length());
            if ((this->*member).length != input.length()) return false;
        }
        return true;
    }
    void draw() {
        // Each renderer is visited in collection order even after a failure.
        for (auto& render : renders) render->draw();
    }
    void replace_texture(const char* name, std::int32_t handle) {
        if (name && *name && handle)
            for (auto& render : renders) render->replace_texture(name,handle);
    }
};
#if INTPTR_MAX == INT32_MAX
static_assert(offsetof(Resource,id)==4 && offsetof(Resource,name)==8);
#endif
inline Resource* allocate_resource(const void* methods) noexcept {
    return act::allocate_resources<Resource>(methods);
}
inline void* release_resource(Resource* resource, unsigned char flags = 1) {
    return act::destroy_resources(resource,flags,[](Resource&) {});
}
} // namespace kinoko::mesh
