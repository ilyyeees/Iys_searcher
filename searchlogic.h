#ifndef SEARCHLOGIC_H
#define SEARCHLOGIC_H

#include <string>
#include <vector>
#include <filesystem>
#include <functional> // For our callback magic ✨
#include <atomic>     // For thread-safe flags that won't get us in trouble
#include <QtGlobal>   // For some Qt goodness (quint64)
#include <QMutex>     // For our pause/resume dance 🕺
#include <QWaitCondition> // The partner for our pause waltz

namespace fs = std::filesystem;

// Hey, this is where we keep all your search preferences in one neat package! 📦
struct SearchConfig {
    std::string searchTerm;
    std::string startPath = "";       // Empty? We'll check all drives!
    std::string extensionFilter = ""; // Looking for .txt or jpg? Pop it here
    std::string outputFile = "";      // Want to save results? Tell me where!
    bool caseInsensitive = false;     // Don't care about CAPS or lowercase?
    bool verboseErrors = false;       // Want to know why I can't peek somewhere?
    bool searchAllRoots = false;      // Flag to signal we're checking ALL the drives
};

// This is our secret handshake with the worker - how we communicate findings
// It'll get called with either (filepath, "") for finds or ("", error_msg) for oopsies
using SearchCallback = std::function<void(const std::string&, const std::string&)>;

// 🔍 The Heart of Our Search Engine 🔍
// This little explorer goes through directories, checking each file it meets
// Reports back immediately when it finds something good!
// We can pause it, cancel it, or let it run wild until it's checked everything
void searchDirectoryRecursive(
    const fs::path& currentPath,
    const SearchConfig& config,
    const SearchCallback& reportResult, // Our messenger pigeon
    unsigned long long& foundCount,     // How many treasures we've found
    std::atomic<bool>& cancellationFlag, // Emergency stop button
    std::atomic<quint64>& filesScannedCount, // How many files we've checked
    std::atomic<bool>& pauseFlag,       // Our "freeze!" command
    QMutex& pauseMutexRef,              // Lock for safe pausing
    QWaitCondition& pauseConditionRef   // Our "wake up!" alarm
    );

// Just a little helper to make text lowercase
// Because sometimes we don't care about SHOUTING or whispering
std::string toLower(std::string s);

// This detective figures out all the places we should look
// Like all drives (C:, D:, etc) or mounted devices
std::vector<fs::path> getRootPaths();


#endif // SEARCHLOGIC_H
