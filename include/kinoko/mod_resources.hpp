#pragma once
#include "kinoko/file_service.h"
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>
namespace kinoko::mods {
struct Info {std::string id,version,content_sha256;};
struct Lookup {bool matched=false;KinokoFile* file=nullptr;};
// Initialize before workers; immutable thereafter. Empty catalog disables mods.
bool load(const std::filesystem::path& catalog,std::string& error);
void clear();
const std::vector<Info>& active();
const std::string& identity(); // SHA256 of canonical catalog bytes; empty when disabled.
Lookup open(const char* resource) noexcept; // Matched but unreadable never falls back to DAT.
std::string sha256(std::string_view bytes);
}
