#include "kinoko/file_service.hpp"
#include <chrono>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr,"file service line %d: %s\n",__LINE__,#x); return 1; } } while(0)
int main() {
    namespace fs=std::filesystem;
    const auto directory=fs::temp_directory_path()/(
        "kinoko-files-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    CHECK(fs::create_directory(directory));
    const auto path=(directory/"fixture.dat").string();
    std::printf("Retained file fixtures: %s\n",directory.string().c_str());
    CHECK(!kinoko_file_open((directory/"missing.dat").string().c_str(),KINOKO_FILE_READ));
    uint32_t count=99;
    CHECK(!kinoko_file_read(nullptr,nullptr,0,&count) && count==0);
    CHECK(kinoko_file_seek(nullptr,0,KINOKO_FILE_BEGIN)==-1);
    {
        kinoko::io::File writer(path.c_str(),KINOKO_FILE_WRITE);
        CHECK(writer && writer.write("ABCDE",5));
        CHECK(kinoko_file_seek(writer.get(),2,KINOKO_FILE_BEGIN)==2);
        CHECK(writer.write("x",1));
        CHECK(kinoko_file_size(writer.get())==5);
        CHECK(kinoko_file_seek(writer.get(),0,KINOKO_FILE_CURRENT)==3);
        // No write at this offset: exercise >4 GiB seeks without a huge file.
        CHECK(kinoko_file_seek(writer.get(),INT64_C(0x100000003),KINOKO_FILE_BEGIN)==INT64_C(0x100000003));
        CHECK(kinoko_file_size(writer.get())==5);
    }
    {
        kinoko::io::File reader(path.c_str(),KINOKO_FILE_READ_SHARED);
        char bytes[8]{};
        CHECK(reader && kinoko_file_read(reader.get(),bytes,8,&count) && count==5);
        CHECK(std::memcmp(bytes,"ABxDE",5)==0);
        CHECK(kinoko_file_read(reader.get(),bytes,1,&count) && count==0);
        CHECK(kinoko_file_seek(reader.get(),-2,KINOKO_FILE_END)==3);
        CHECK(reader.read(bytes,2) && std::memcmp(bytes,"DE",2)==0);
        CHECK(kinoko_file_seek(reader.get(),-1,KINOKO_FILE_BEGIN)==-1);
        CHECK(kinoko_file_seek(reader.get(),0,99)==-1);
        CHECK(kinoko_file_read(reader.get(),nullptr,0,&count) && count==0);
        auto* owned=reader.detach(); CHECK(!reader); kinoko_file_close(owned);
    }
    {
        kinoko::io::File writer(path.c_str(),KINOKO_FILE_WRITE);
        CHECK(writer && kinoko_file_size(writer.get())==0);
    }
    std::puts("PASS: file bytes, EOF, seek/size, >4 GiB positions and ownership");
}
