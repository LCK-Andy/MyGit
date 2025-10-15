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

        // create directory structure
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
        // Create a map to store key-value pairs (like "name" -> "Alice", "email" -> "alice@example.com")
        std::map<std::string, std::string> cfg;

        // Open the config file from the .mygit directory
        std::ifstream f(path + "/config");
        if (!f.is_open())
            return cfg;

        std::string line;
        // Read the config file line by line
        while (std::getline(f, line))
        {
            // If the line contains "name", extract the substring after '=' and store it as the author name
            if (line.find("name") != std::string::npos)
                cfg["name"] = line.substr(line.find("=") + 1);

            // If the line contains "email", extract the substring after '=' and store it as the author email
            else if (line.find("email") != std::string::npos)
                cfg["email"] = line.substr(line.find("=") + 1);
        }

        // --- Trim leading whitespace from each value (e.g., " Alice" → "Alice") ---
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
        std::ifstream file(filePath, std::ios::binary); // Open the file in binary mode
        std::ostringstream buffer;
        buffer << file.rdbuf(); // Stream file contents into an in-memory buffer
        file.close();
        std::string content = buffer.str();

        // Compute hash
        // The hash uniquely identifies a file by its content.
        std::string hash = sha1(content);

        // Use the first two characters of the hash as a subdirectory
        // (to distribute objects into multiple folders — just like Git does).
        std::string dir = path + "/objects/" + hash.substr(0, 2);
        std::string fileName = hash.substr(2);

        // store the blob object
        fs::create_directories(dir);

        // Save the raw file content into the object store under ".mygit/objects/"
        std::ofstream blobFile(dir + "/" + fileName, std::ios::binary);
        blobFile << content;
        blobFile.close();

        // Record the blob in the index
        // Append a new entry to ".mygit/index" that maps the filename to its content hash.
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

    void status()
    {
        if (!isInitialized())
        {
            std::cerr << "Error: not a MyGit repository.\n";
            return;
        }

        // --- Read current branch name from HEAD ---
        std::ifstream headFile(path + "/HEAD");
        std::string headRef;
        std::getline(headFile, headRef);
        headFile.close();

        std::string branch = "main";
        if (headRef.find("ref:") != std::string::npos)
            branch = headRef.substr(headRef.find_last_of('/') + 1);

        std::cout << "On branch " << branch << "\n\n";

        // --- Read index (staging area) ---
        std::map<std::string, std::string> indexEntries;
        std::ifstream indexFile(path + "/index");
        if (indexFile.is_open())
        {
            std::string filePath, fileHash;
            while (indexFile >> filePath >> fileHash)
                indexEntries[filePath] = fileHash;
            indexFile.close();
        }

        // --- Read last commit’s tracked files (if any) ---
        std::map<std::string, std::string> committedFiles;
        std::string branchRef = path + "/refs/heads/" + branch;
        if (fs::exists(branchRef))
        {
            std::ifstream refFile(branchRef);
            std::string commitHash;
            refFile >> commitHash;
            refFile.close();

            // Load commit from objects
            std::string commitDir = path + "/objects/" + commitHash.substr(0, 2);
            std::string commitFile = commitHash.substr(2);
            std::ifstream commitObj(commitDir + "/" + commitFile, std::ios::binary);
            if (commitObj.is_open())
            {
                std::string line, treeHash;
                while (std::getline(commitObj, line))
                {
                    if (line.rfind("tree ", 0) == 0)
                    {
                        treeHash = line.substr(5);
                        break;
                    }
                }
                commitObj.close();

                // Read tree object to get committed files
                if (!treeHash.empty())
                {
                    std::string treeDir = path + "/objects/" + treeHash.substr(0, 2);
                    std::string treeName = treeHash.substr(2);
                    std::ifstream treeObj(treeDir + "/" + treeName, std::ios::binary);
                    std::string entry;
                    while (std::getline(treeObj, entry))
                    {
                        std::istringstream iss(entry);
                        std::string mode, name, hash;
                        iss >> mode >> name >> hash;
                        if (!name.empty() && !hash.empty())
                            committedFiles[name] = hash;
                    }
                    treeObj.close();
                }
            }
        }

        // --- Collect file states ---
        std::vector<std::string> staged;
        std::vector<std::string> modified;
        std::vector<std::string> untracked;

        // Check staged files
        for (auto &[filename, hash] : indexEntries)
            staged.push_back(filename);

        // Walk working directory (skip .mygit)
        for (auto &entry : fs::directory_iterator(fs::current_path()))
        {
            if (entry.path().filename() == ".mygit")
                continue;
            if (entry.is_directory())
                continue;

            std::string fname = entry.path().filename();
            std::ifstream f(fname, std::ios::binary);
            std::ostringstream buf;
            buf << f.rdbuf();
            std::string content = buf.str();
            f.close();

            std::string h = sha1(content);

            bool inIndex = indexEntries.count(fname);
            bool inCommit = committedFiles.count(fname);

            if (inCommit && h != committedFiles[fname])
                modified.push_back(fname);
            else if (!inIndex && !inCommit)
                untracked.push_back(fname);
        }

        // --- Print results ---
        if (!staged.empty())
        {
            std::cout << "Staged files:\n";
            for (auto &f : staged)
                std::cout << "    " << f << "\n";
            std::cout << "\n";
        }

        if (!modified.empty())
        {
            std::cout << "Modified (not staged):\n";
            for (auto &f : modified)
                std::cout << "    " << f << "\n";
            std::cout << "\n";
        }

        if (!untracked.empty())
        {
            std::cout << "Untracked files:\n";
            for (auto &f : untracked)
                std::cout << "    " << f << "\n";
            std::cout << "\n";
        }

        if (staged.empty() && modified.empty() && untracked.empty())
            std::cout << "Nothing to commit, working tree clean\n";
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
    else if (cmd == "status")
    {
        repo.status();
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
                 "  status                  Show the working tree status\n"
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