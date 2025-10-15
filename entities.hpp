#pragma once

#include <string>
#include <vector>

// Blob is the raw file data, prefixed with a small header, then hashed
struct Blob{
    std::string hash; // SHA-1 hashed
    std::string content;
};

// Tree maps file and folder names to object hashes
struct TreeEntry {
    std::string mode; // file permissions
    std::string name; 
    std::string hash; // hash of the object (blob or tree)
};

struct Tree{
    std::vector<TreeEntry> entries;
};

/**
 * Commits are project snapshots with history
 * 
 * A commit ties everything together:
 * - Points to a tree (the state of the project)
 * - Optionally to one or more parent commits
 * - includes metadata 
 *      - author
 *      - commit message
 */
struct Commit{
    std::string treeHash;
    std::string parentHash;
    std::string author;
    std::string message;
    std::string hash;
};

// Movable pointer to a commit
struct Branch{
    std::string name;
    std::string commitHash;
};