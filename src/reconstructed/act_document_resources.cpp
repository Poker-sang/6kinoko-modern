#include "kinoko/act_document.h"
#include "kinoko/act_document_records.hpp"
#include "kinoko/memory_access.hpp"
#include <cstring>

#include "kinoko/act_method_dispatch.hpp"
#include "kinoko/act_texture_resource.hpp"
using namespace kinoko::act;
using kinoko::memory::load;

extern "C" int32_t kinoko_act_document_load_resources(KinokoActDocument *document, const char *prefix) {
    return DocumentMethods(document).load_resources(prefix);
}

// 4289C0: preserve type query order, virtual receivers, non-short-circuit
// accumulation and re-reading the resource end after each resource callback.
extern "C" int32_t __fastcall kinoko_method_load_act_resources(
    KinokoActDocument *document, void *, const char *prefix) {
    const DocumentView view(document);
    const kinoko::legacy::StringView path(view.bytes(&DocumentRecord::resource_path));
    if (prefix) path.assign(prefix, static_cast<uint32_t>(std::strlen(prefix)));
    else prefix = path.data();
    uint8_t result = 1;
    auto *cursor = reinterpret_cast<const unsigned char *>(view.get(&DocumentRecord::resources).begin);
    while (cursor != reinterpret_cast<const unsigned char *>(view.get(&DocumentRecord::resources).end)) {
        auto *resource = load<ResourceRecord *>(cursor);
        const ResourceMethods methods(resource);
        if (auto* converted = methods.query(ResourceKind::texture)) {
            result &= ResourceMethods(converted).load(prefix);
        } else if (auto* converted = methods.query(ResourceKind::render_target)) {
            const auto& target = texture_resource(converted);
            result &= ResourceMethods(converted).create_target(target.width, target.height);
        } else if (auto* converted = methods.query(ResourceKind::mesh)) {
            result &= ResourceMethods(converted).load(prefix);
        } else if (auto* converted = methods.query(ResourceKind::chip)) {
            result &= ResourceMethods(converted).load(prefix);
        } else {
            result = 0;
        }
        cursor += sizeof(ResourceRecord *);
    }
    return result;
}

namespace {
// 428AF0/428BD0: only a real CActResource2D unloads/reloads here. Targets,
// Mesh and Chip are recognized but skipped, not coerced to a texture resource.
uint8_t transition_resources(KinokoActDocument *document,bool suspend) {
    const DocumentView view(document);
    const auto old=view.get(&DocumentRecord::resources_suspended);
    if(suspend ? old==1 : old==0) return 0;
    view.set(&DocumentRecord::resources_suspended,static_cast<uint8_t>(suspend));
    const auto *prefix=kinoko::legacy::StringView(view.bytes(&DocumentRecord::resource_path)).data();
    uint8_t result=1;
    auto *cursor=reinterpret_cast<const unsigned char *>(view.get(&DocumentRecord::resources).begin);
    while(cursor!=reinterpret_cast<const unsigned char *>(view.get(&DocumentRecord::resources).end)) {
        // Re-read the resource slot for each query, as callbacks can mutate it.
        const auto query = [cursor](ResourceKind kind) {
            return ResourceMethods(load<ResourceRecord*>(cursor)).query(kind);
        };
        if (auto* converted = query(ResourceKind::texture)) {
            result &= suspend ? ResourceMethods(converted).unload() : ResourceMethods(converted).load(prefix);
        } else if (!(query(ResourceKind::render_target) || query(ResourceKind::mesh) || query(ResourceKind::chip))) result = 0;
        cursor+=sizeof(ResourceRecord*);
    }
    return result;
}
}
extern "C" int32_t __fastcall kinoko_method_suspend_act_resources(KinokoActDocument *document,void *) {
    return transition_resources(document,true);
}
extern "C" int32_t __fastcall kinoko_method_resume_act_resources(KinokoActDocument *document,void *) {
    return transition_resources(document,false);
}
