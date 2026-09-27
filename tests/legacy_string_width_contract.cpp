#include "kinoko/legacy_string.hpp"
#include <cstddef>

using kinoko::legacy::StringRecord;
static_assert(sizeof(StringRecord) == 24);
static_assert(offsetof(StringRecord, length) == 16);
static_assert(sizeof(char*) * 2 <= sizeof(StringRecord::characters));

int main() { return 0; }
