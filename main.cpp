#include <iostream>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

struct Repository {
    std::string path = ".mygit"; // local repo folder

    bool init() const {
        if (fs::exists(path)){
            std::cout << "Reinitialized existing MyGit repository in "
                      << fs::absolute(path) << "\n";
            return false;
        }

    // Create folders (including any necessary parent directories)
    fs::create_directories(path + "/objects");
    fs::create_directories(path + "/refs/heads");
    fs::create_directories(path + "/refs/tags");

        // Create HEAD file
        std::ofstream headFile(path + "/HEAD");
        headFile << "ref: refs/heads/main\n";
        headFile.close();

        // (Optional) Create simple config
        std::ofstream configFile(path + "/config");
        configFile << "[core]\n"
                   << "    repositoryformatversion = 0\n"
                   << "    filemode = true\n"
                   << "    bare = false\n";
        configFile.close();

        std::cout << "Initialized empty MyGit repository in "
                  << fs::absolute(path) << "\n";
        return true;
    }
};

// --- Main Command Parser (CLI entry point) ---
int main(int argc, char* argv[]){
    if (argc < 2){
        std::cerr << "Usage: mygit <command>\n";
        return 1;
    }

    std::string command = argv[1];
    Repository repo;

    if(command == "init"){
        repo.init();
    }else{
        std::cerr << "Unknown command: " << command << "\n";
    }

    return 0;
}