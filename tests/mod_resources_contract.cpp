#include "kinoko/mod_resources.hpp"
#include "retained_fixture.hpp"
#include <fstream>
#include <cstdio>
#include <cstring>
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"Mod contract line %d\n",__LINE__);return 1;}}while(0)
int main() {
 using namespace kinoko::mods;
 CHECK(sha256("")=="e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
 CHECK(sha256("abc")=="ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
 CHECK(sha256(std::string(1000000,'a'))=="cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0");
 auto root=retained_fixture("mods");std::filesystem::create_directories(root/"assets/data");
 std::ofstream(root/"assets/data/probe.bin",std::ios::binary)<<"abc";
 const std::string catalog="KINOKOMODS1\nM\tsample\t1.0\t"+sha256("manifest")+"\nF\tdata/probe.bin\t"+sha256("abc")+"\n";
 std::ofstream(root/"catalog.tsv",std::ios::binary)<<catalog;std::string error;
 CHECK(load(root/"catalog.tsv",error));CHECK(active().size()==1 && identity()==sha256(catalog));
 auto value=open("./DATA\\PROBE.BIN");CHECK(value.matched && value.file);
 char out[3];uint32_t count;CHECK(kinoko_file_read(value.file,out,3,&count) && count==3 && std::memcmp(out,"abc",3)==0);kinoko_file_close(value.file);
 CHECK(!open("data/missing.bin").matched);CHECK(!open("data/../probe.bin").matched);
 std::ofstream(root/"assets/data/probe.bin",std::ios::binary)<<"bad";
 value=open("data/probe.bin");CHECK(value.matched && !value.file);
 CHECK(!load(root/"catalog.tsv",error) && active().empty() && identity().empty());
 CHECK(load({},error) && !open("data/probe.bin").matched);
 const std::string invalid="KINOKOMODS2\nM\tsample\t1\t"+sha256("sample")+"\nE\tsample\tdata/custom/sample/missing.nut\nF\tdata/probe.bin\t"+sha256("bad")+"\n";
 std::ofstream(root/"catalog.tsv",std::ios::binary)<<invalid;
 CHECK(!load(root/"catalog.tsv",error) && entrypoints().empty());
 return 0;
}
