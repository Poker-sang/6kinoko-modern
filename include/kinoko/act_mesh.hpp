#include "kinoko/compiler_compat.h"
#include "kinoko/file_io.h"
struct KinokoArchiveReader;
struct SQVM;
#pragma once
#include "kinoko/act_layout_records.hpp"
#include "kinoko/mesh_resource.hpp"
#include <cstdint>
namespace kinoko::mesh {
Resource *create_resource();
int32_t read_resource_properties(Resource *resource,KinokoArchiveReader** reader_holder,int32_t version);
void clear_resource(Resource *resource);
uint8_t load_resource(Resource *resource,const char *prefix);
int32_t replace_texture(Resource *resource,const char *name,KinokoActResource *texture);
const void *resource_methods();
const void *layout_methods();
act::Layout3DRecord *create_layout();
uint32_t resource_type();
uint32_t layout_type();
}
extern "C" {
int32_t kinoko_publish_mesh_resource_class(struct SQVM*,void*,int32_t *);
int32_t __fastcall kinoko_method_read_mesh_resource(KinokoActResource*,void *,KinokoArchiveReader** ,int32_t);
int32_t __fastcall kinoko_method_write_mesh_resource(KinokoActResource*,void *,KinokoArchiveReader* );
int32_t __fastcall kinoko_method_write_layout_3d(KinokoActLayout*,void *,KinokoArchiveReader* );
int32_t __fastcall kinoko_method_register_mesh_resource(void*,void *,struct SQVM*);
int32_t __fastcall kinoko_method_bind_mesh_object(KinokoActResource*,void *,void*,const char *);
int32_t __fastcall kinoko_method_bind_mesh_table(KinokoActResource*,void *,void*,const char *);
int32_t __fastcall kinoko_method_register_layout_3d(KinokoActLayout*,void *);
}
