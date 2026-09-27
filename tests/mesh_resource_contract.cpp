#include "kinoko/mesh_resource.hpp"
#include <cstring>
#include <type_traits>
using namespace kinoko::mesh;
static_assert(!std::is_copy_constructible_v<Resource>);
static_assert(!std::is_copy_assignable_v<Resource>);
static_assert(sizeof(decltype(Controller::model)) == sizeof(void*));
struct Probe final : Render {
    std::vector<int>& events;
    int id;
    std::weak_ptr<Node> model;
    Probe(std::vector<int>& log,int number,const std::shared_ptr<Node>& owner)
        : events(log),id(number),model(owner) {}
    ~Probe() override { events.push_back((model.expired()?100:200)+id); }
    std::uint8_t draw() override { events.push_back(id);return id==1?0:1; }
    void replace_texture(const char* name,std::int32_t handle) override {
        if (!std::strcmp(name,"skin") && handle==17) events.push_back(10+id);
    }
};
int main() {
    int methods=0;
    auto* resource=allocate_resource(&methods);
    if (!resource || resource->id!=-1 || resource->methods!=&methods || resource->state || !resource->renders.empty()) return 1;
    std::vector<int> events;
    const auto fill=[&] {
        resource->state=std::make_unique<ResourceState>();
        auto model=std::make_shared<Node>();
        resource->state->model_owners.push_back(model);
        resource->state->root=std::make_unique<Controller>();
        resource->state->root->model=model.get();
        resource->state->registry.push_back(resource->state->root.get());
        resource->state->named["root"]=resource->state->root.get();
        resource->renders.push_back(std::make_unique<Probe>(events,1,model));
        resource->renders.push_back(std::make_unique<Probe>(events,2,model));
    };
    fill();
    resource->draw(); // Failed first renderer must not skip the second.
    resource->replace_texture("",17);resource->replace_texture("skin",0);
    resource->replace_texture("skin",17);
    if(events!=std::vector<int>({1,2,11,12})) return 2;
    const char* name="an independently owned long mesh resource filename";
    kinoko::legacy::StringView(&resource->mesh_name).assign(name,static_cast<std::uint32_t>(std::strlen(name)));
    auto* copy=allocate_resource(&methods);
    if (!copy || !copy->copy_names(*resource)) return 3;
    events.clear();resource->reset_for_load();
    if(events!=std::vector<int>({201,202}) || !resource->state || !resource->renders.empty()) return 4;
    if(std::strcmp(kinoko::legacy::StringView(&resource->mesh_name).data(),name)) return 5;
    fill();events.clear();release_resource(resource);
    if(events!=std::vector<int>({101,102})) return 6;
    if(std::strcmp(kinoko::legacy::StringView(&copy->mesh_name).data(),name)) return 7;
    copy->clear();copy->clear();release_resource(copy);
    return 0;
}
