#include "kinoko/base_utilities.h"
#include <cerrno>
#include <cstring>
#include <string_view>
extern "C" int32_t kinoko_path_split(const char* path, char* directory, char* file) {
    if (!path || !directory) return EINVAL;
    const std::string_view value(path);
    const auto slash=value.find_last_of("/\\");
    const auto drive=value.size()>=2 && value[1]==':' ? 2u : 0u;
    const auto prefix=slash==value.npos ? drive : slash+1;
    const auto name=value.substr(prefix);
    if(prefix>=260 || (file && name.size()>=260)) {
        directory[0]=0; if(file)file[0]=0; return ERANGE;
    }
    std::memcpy(directory,path,prefix);directory[prefix]=0;
    if(file){std::memcpy(file,name.data(),name.size());file[name.size()]=0;}
    return 0;
}
