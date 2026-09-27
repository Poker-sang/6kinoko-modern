#include "kinoko/mesh_model.hpp"
#include "kinoko/file_io.h"

namespace kinoko::mesh {
namespace {
bool read_exact(void* context, void* destination, std::uint32_t size) {
    return kinoko_reader_read_exact(static_cast<KinokoArchiveReader*>(context),destination,size) != 0;
}
}
std::unique_ptr<Node> read_model(KinokoArchiveReader* reader) {
    return reader ? decode_model({reader,read_exact}) : nullptr;
}
std::unique_ptr<Material> read_material(KinokoArchiveReader* reader) {
    return reader ? decode_material({reader,read_exact}) : nullptr;
}
}
