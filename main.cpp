#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <ctime>
#include <unordered_map>
#include <openssl/sha.h> // For SHA1 hash
#include <map>

namespace fs = std::filesystem;

// --- Helper: compute SHA-1 hash ---
std::string sha1(const std::string &data)
{
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char *>(data.c_str()), data.size(), hash);
    std::ostringstream os;
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++)
        os << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    return os.str();
}

// ---------- Blob structure ----------
struct Blob
{
    std::string hash;
    std::string content;
};

// ---------- Commit structure ----------
struct Commit
{
    std::string treeHash;
    std::string parentHash;
    std::string author;
    std::string message;
    std::string hash;
};

// ---------- Tree structure ----------
struct TreeEntry
{
    std::string mode; // e.g. "100644"
    std::string name; // "file.cpp"
    std::string hash; // blob or subtree hash
};

struct Tree
{
    std::vector<TreeEntry> entries;
};

// ---------- Repository ----------
struct Repository
{
    std::string path = ".mygit"; // local repo folder

    bool isInitialized() const
    {
        return fs::exists(path) && fs::exists(path + "/objects");
    }
    // --- Initialize a new repository ---
    bool init()
    {
        if (isInitialized())
        {
            std::cout << "Reinitialized MyGit in " << fs::absolute(path) << "\n";
            return false;
        }
        fs::create_directories(path + "/objects");
        fs::create_directories(path + "/refs/heads");
        fs::create_directories(path + "/refs/tags");

        std::ofstream(path + "/HEAD") << "ref: refs/heads/main\n";

        // create basic config file
        std::ofstream cfg(path + "/config");
        cfg << "[core]\n    repositoryformatversion = 0\n    filemode = true\n    bare = false\n"
            << "[user]\n    name = Unknown\n    email = unknown@example.com\n";
        cfg.close();

        std::cout << "Initialized MyGit repository in " << fs::absolute(path) << "\n";
        return true;
    }

    // ----- CONFIG MANAGEMENT -----
    std::map<std::string, std::string> readConfig()
    {
        std::map<std::string, std::string> cfg;
        std::ifstream f(path + "/config");
        if (!f.is_open())
            return cfg;

        std::string line;
        while (std::getline(f, line))
        {
            if (line.find("name") != std::string::npos)
                cfg["name"] = line.substr(line.find("=") + 1);
            else if (line.find("email") != std::string::npos)
                cfg["email"] = line.substr(line.find("=") + 1);
        }
        // trim whitespace
        for (auto &p : cfg)
        {
            if (!p.second.empty() && p.second.front() == ' ')
                p.second.erase(0, p.second.find_first_not_of(' '));
        }
        return cfg;
    }

    void writeConfig(const std::string &name, const std::string &email)
    {
        std::ofstream cfg(path + "/config", std::ios::trunc);
        cfg << "[core]\n"
            << "    repositoryformatversion = 0\n"
            << "    filemode = true\n"
            << "    bare = false\n"
            << "[user]\n"
            << "    name = " << name << "\n"
            << "    email = " << email << "\n";
    }

    // --- Add setter commands ---
    void setAuthorName(const std::string &name)
    {
        if (!isInitialized())
        {
            std::cerr << "Not a MyGit repository.\n";
            return;
        }
        auto cfgmap = readConfig();
        cfgmap["name"] = name;
        writeConfig(cfgmap["name"], cfgmap["email"]);
        std::cout << "Author name set to: " << name << "\n";
    }

    void setAuthorEmail(const std::string &email)
    {
        if (!isInitialized())
        {
            std::cerr << "Not a MyGit repository.\n";
            return;
        }
        auto cfgmap = readConfig();
        cfgmap["email"] = email;
        writeConfig(cfgmap["name"], cfgmap["email"]);
        std::cout << "Author email set to: " << email << "\n";
    }

    // --- Add operation ---
    std::string add(const std::string &filePath)
    {
        if (!isInitialized())
        {
            std::cerr << "Error: not a MyGit repository. \n";
            return "";
        }

        if (!fs::exists(filePath))
        {
            std::cerr << "Error: file not found: " << filePath << "\n";
            return "";
        }

        // Read file content
        std::ifstream file(filePath, std::ios::binary);
        std::ostringstream buffer;
        buffer << file.rdbuf();
        file.close();
        std::string content = buffer.str();

        // Compute hash
        std::string hash = sha1(content);
        std::string dir = path + "/objects/" + hash.substr(0, 2);
        std::string fileName = hash.substr(2);

        fs::create_directories(dir);
        std::ofstream blobFile(dir + "/" + fileName, std::ios::binary);
        blobFile << content;
        blobFile.close();

        // record in index file
        std::ofstream indexFile(path + "/index", std::ios::app);
        indexFile << filePath << " " << hash << "\n";
        indexFile.close();

        std::cout << "Added file " << filePath << " as blob " << hash << "\n";
        return hash;
    }

    // ---------- BUILD TREE FROM INDEX ----------
    Tree buildTree()
    {
        Tree tree;
        std::ifstream index(path + "/index");
        if (!index.is_open())
            return tree;

        std::string filename, blobHash;
        while (index >> filename >> blobHash)
        {
            TreeEntry entry;
            entry.mode = "100644"; // Normal file permission
            entry.name = filename;
            entry.hash = blobHash;
            tree.entries.push_back(entry);
        }
        return tree;
    }

    // ---------- Write Tree Object ----------
    std::string writeTree(const Tree &tree)
    {
        std::ostringstream treeContent;
        for (const auto &entry : tree.entries)
        {
            treeContent << entry.mode << " " << entry.name << "\0" << entry.hash << "\n";
        }
        std::string content = treeContent.str();
        std::string hash = sha1(content);

        std::string dir = path + "/objects/" + hash.substr(0, 2);
        std::string file = hash.substr(2);
        fs::create_directories(dir);
        std::ofstream(dir + "/" + file, std::ios::binary) << content;

        return hash;
    }

    // --- Commit operation ---
    std::string commit(const std::string &message)
    {
        if (!isInitialized())
        {
            std::cerr << "Error: not a MyGit repository.\n";
            return "";
        }

        // Ensure there is an index
        Tree tree = buildTree();
        if (tree.entries.empty())
        {
            std::cerr << "Nothing to commit.\n";
            return "";
        }

        // build a simple tree object
        std::string treeHash = writeTree(tree);

        // Find parent commit
        std::string parentHash;
        std::string refPath = path + "/refs/heads/main";
        if (fs::exists(refPath))
        {
            std::ifstream ref(refPath);
            ref >> parentHash;
        }

        // Create commit object
        std::ostringstream commitBuf;
        commitBuf << "tree " << treeHash << "\n";
        if (!parentHash.empty())
            commitBuf << "parent " << parentHash << "\n";

        // read name/email from config
        auto cfg = readConfig();
        std::string authorName = cfg.count("name") ? cfg["name"] : "Unknown";
        std::string authorEmail = cfg.count("email") ? cfg["email"] : "unknown@example.com";

        commitBuf << "author " << authorName << " <" << authorEmail << "> "
                  << std::time(nullptr) << "\n\n";

        commitBuf << message << "\n";

        std::string commitContent = commitBuf.str();
        std::string commitHash = sha1(commitContent);

        std::string dir = path + "/objects/" + commitHash.substr(0, 2);
        std::string file = commitHash.substr(2);
        fs::create_directories(dir);
        std::ofstream(dir + "/" + file, std::ios::binary) << commitContent;

        std::ofstream(refPath) << commitHash;

        // clear index (as Git does)
        std::ofstream(path + "/index", std::ios::trunc);

        std::cout << "[main " << commitHash.substr(0, 7) << "] " << message << "\n";
        return commitHash;
    }

    // --- log operation ---
    void logCommits() const
    {
        if (!isInitialized())
        {
            std::cerr << "Not a MyGit repository.\n";
            return;
        }

        std::string branchRef = path + "/refs/heads/main";
        if (!fs::exists(branchRef))
        {
            std::cerr << "No commits yet.\n";
            return;
        }

        // Read the latest commit hash
        std::string commitHash;
        std::ifstream(branchRef) >> commitHash;

        while (!commitHash.empty())
        {
            std::string dir = path + "/objects/" + commitHash.substr(0, 2);
            std::string file = commitHash.substr(2);
            std::ifstream commitFile(dir + "/" + file, std::ios::binary);

            if (!commitFile.is_open())
            {
                std::cerr << "Error: cannot open commit " << commitHash << "\n";
                return;
            }

            std::string line;
            std::string treeHash, parentHash, author, message;
            bool msgStarted = false;
            while (std::getline(commitFile, line))
            {
                if (line.rfind("tree ", 0) == 0)
                    treeHash = line.substr(5);
                else if (line.rfind("parent ", 0) == 0)
                    parentHash = line.substr(7);
                else if (line.rfind("author ", 0) == 0)
                    author = line.substr(7);
                else if (line.empty())
                    msgStarted = true;
                else if (msgStarted)
                    message += line + "\n";
            }
            commitFile.close();

            std::cout << "commit " << commitHash << "\n";
            if (!author.empty())
                std::cout << "Author: " << author << "\n";
            std::cout << "\n    " << message << "\n";

            if (parentHash.empty())
                break;
            commitHash = parentHash;
        }
    }
};

// --- Main Command Parser (CLI entry point) ---
int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: mygit <command> [args]\n";
        return 1;
    }

    Repository repo;
    std::string cmd = argv[1];

    if (cmd == "init")
    {
        repo.init();
    }
    else if (cmd == "add")
    {
        if (argc < 3)
        {
            std::cerr << "Usage: mygit add <file>\n";
            return 1;
        }
        repo.add(argv[2]);
    }
    else if (cmd == "commit")
    {
        if (argc < 3)
        {
            std::cerr << "Usage: mygit commit <msg>\n";
            return 1;
        }
        repo.commit(argv[2]);
    }
    else if (cmd == "log")
    {
        repo.logCommits();
    }
    else if (cmd == "set_author")
    {
        if (argc < 3)
        {
            std::cerr << "Usage: mygit set_author <name>\n";
            return 1;
        }
        repo.setAuthorName(argv[2]);
    }
    else if (cmd == "set_email")
    {
        if (argc < 3)
        {
            std::cerr << "Usage: mygit set_email <email>\n";
            return 1;
        }
        repo.setAuthorEmail(argv[2]);
    }
    else if (cmd == "help")
    {
        std::cout << "MyGit - a minimal Git-like version control system\n\n"
                     "Usage:\n"
                     "  mygit <command> [arguments]\n\n"
                     "Commands:\n"
                     "  init                    Initialize a new repository (.mygit directory)\n"
                     "  add <file>              Add file contents to the staging area\n"
                     "  commit <message>        Record staged changes as a new commit\n"
                     "  log                     Display commit history\n"
                     "  set_author <name>       Set the author's name\n"
                     "  set_email <email>       Set the author's email address\n"
                     "  help                    Show this help message\n\n"
                     "Examples:\n"
                     "  ./mygit init\n"
                     "  ./mygit set_author \"John Doe\"\n"
                     "  ./mygit set_email john@example.com\n"
                     "  ./mygit add main.cpp\n"
                     "  ./mygit commit \"Initial commit\"\n"
                     "  ./mygit log\n";
    }
    else
    {
        std::cerr << "Unknown command.\n";
    }

    return 0;
}