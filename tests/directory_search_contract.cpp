#include "kinoko/directory_search.h"
#include <filesystem>
#include <fstream>
#include <chrono>
#include <memory>
#include <set>
#include <string>
#include <cstdio>
using Search=std::unique_ptr<KinokoDirectorySearch,decltype(&kinoko_directory_close)>;
#define CHECK(x) do { if(!(x)) { std::fprintf(stderr,"failed: %s\n",#x); return 1; } } while(0)
std::set<std::string> names(const std::string& pattern) {
    Search search(kinoko_directory_first(pattern.c_str()),kinoko_directory_close);
    std::set<std::string> result;
    if(search) do { result.insert(kinoko_directory_name(search.get())); } while(kinoko_directory_next(search.get()));
    return result;
}
int main() {
    namespace fs=std::filesystem;
    const auto root=fs::temp_directory_path()/("kinoko-directory-contract-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    CHECK(fs::create_directory(root)); // Retain fixture files for user inspection.
    for(const char* name:{"alpha.dat","beta.dat","plain","literal[1].dat"}) {
        std::ofstream file(root/name); file << "fixture"; CHECK(file.good());
    }
    CHECK(fs::create_directory(root/"subdir"));
    auto pattern=[&](const char* leaf) { return (root/leaf).string(); };
    CHECK(!kinoko_directory_first(nullptr));
    CHECK(!kinoko_directory_first(""));
    CHECK(!kinoko_directory_next(nullptr));
    CHECK(!kinoko_directory_name(nullptr));
    CHECK(!kinoko_directory_close(nullptr));
    CHECK(names(pattern("absent*")).empty());
    CHECK(names((root/"missing"/"*").string()).empty());
    CHECK(names(pattern("*.dat"))==std::set<std::string>({"alpha.dat","beta.dat","literal[1].dat"}));
    CHECK(names(pattern("alpha.da?"))==std::set<std::string>({"alpha.dat"}));
    CHECK(names(pattern("literal[1].dat"))==std::set<std::string>({"literal[1].dat"}));
    const auto all=names(pattern("*.*"));
    CHECK(all.count("plain") && all.count("subdir") && all.count("alpha.dat"));
    CHECK(names(pattern("plain.*")).count("plain"));
    CHECK(names(root.string()+"\\*.dat").count("beta.dat"));
    Search first(kinoko_directory_first(pattern("alpha.dat").c_str()),kinoko_directory_close);
    Search second(kinoko_directory_first(pattern("beta.dat").c_str()),kinoko_directory_close);
    CHECK(first && second);
    CHECK(std::string(kinoko_directory_name(first.get()))=="alpha.dat");
    CHECK(!kinoko_directory_next(first.get()));
    CHECK(std::string(kinoko_directory_name(first.get()))=="alpha.dat");
    CHECK(kinoko_directory_close(first.release()));
    CHECK(std::string(kinoko_directory_name(second.get()))=="beta.dat");
    CHECK(kinoko_directory_close(second.release()));
    std::puts("PASS: directory matching, literal brackets, concurrent search ownership and EOF");
}
