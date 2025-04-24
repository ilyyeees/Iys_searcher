#include "searchlogic.h"
#include <iostream>   // Just in case we need to chat with the console
#include <algorithm>
#include <cctype>
#include <fstream> // In case we want to save our findings
#include <stdexcept>
#include <QDebug> // For keeping track of what's happening during our adventures

#ifdef _WIN32
#include <windows.h> // Windows-specific goodies if QStorage isn't enough
#include <QStorageInfo> // Qt's awesome cross-platform drive detector!
#else
#include <QStorageInfo> // Works for Mac and Linux too! 🐧
#endif


// This just turns any text to lowercase - super handy for case-insensitive searches!
std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return s;
}

// 🔍 Here's where the real search magic happens! 🔍
void searchDirectoryRecursive(
    const fs::path& currentPath,
    const SearchConfig& config,
    const SearchCallback& reportResult, // Our messenger
    unsigned long long& foundCount,
    std::atomic<bool>& cancellationFlag, // Our emergency exit
    std::atomic<quint64>& filesScannedCount, // Keeping count of our journey
    std::atomic<bool>& pauseFlag,        // Time freeze button
    QMutex& pauseMutexRef,               // Safety lock for pausing
    QWaitCondition& pauseConditionRef    // Alarm clock to wake us up
)
{
    // ⛔ Quick check - did someone hit the cancel button?
    if (cancellationFlag.load()) {
        return; // Time to bail out!
    }

    // ⏸️ Check if someone hit pause - we'll wait here if they did
    if (pauseFlag.load()) {
        QMutexLocker locker(&pauseMutexRef); // Lock the door while we nap
        while (pauseFlag.load() && !cancellationFlag.load()) {
            pauseConditionRef.wait(&pauseMutexRef); // Snooze until someone wakes us
        }
        // Oops, were we cancelled during our nap?
        if (cancellationFlag.load()) {
            return; // Let's head home
        }
    }

    // Let's prep our search terms based on case sensitivity
    std::string searchTermEffective = config.caseInsensitive ? toLower(config.searchTerm) : config.searchTerm;
    std::string extensionFilterEffective = config.caseInsensitive ? toLower(config.extensionFilter) : config.extensionFilter;

    // Add a dot to our extension if needed - just a little housekeeping
    if (!extensionFilterEffective.empty() && extensionFilterEffective[0] != '.') {
        extensionFilterEffective = "." + extensionFilterEffective;
    }


    try {
        // If the path doesn't exist or isn't a directory, nothing to do here! 🤷‍♂️
        if (!fs::exists(currentPath) || !fs::is_directory(currentPath)) {
            return;
        }

        fs::directory_iterator dir_iter;
        try {
            // Let's peek into this directory, but skip any "no entry" signs
            dir_iter = fs::directory_iterator(currentPath, fs::directory_options::skip_permission_denied);
        } catch (const fs::filesystem_error& e) {
            if (config.verboseErrors) {
                reportResult("", "Warning: Oops! Can't look into " + currentPath.string() + " - " + e.what());
            }
            return; // No point trying further here
        } catch (const std::exception& e) { // Catch other surprise errors
            if (config.verboseErrors) {
                reportResult("", "Warning: Something went wrong with " + currentPath.string() + " - " + e.what());
            }
            return;
        }


        // 🚶‍♂️ Let's stroll through all the items in this directory
        for (const auto& entry : dir_iter) {
            // ⛔ Check if we need to abort the mission
            if (cancellationFlag.load()) {
                return; // Mission aborted!
            }

            // ⏸️ Or maybe take a quick break?
            if (pauseFlag.load()) {
                QMutexLocker locker(&pauseMutexRef);
                while (pauseFlag.load() && !cancellationFlag.load()) {
                    pauseConditionRef.wait(&pauseMutexRef);
                }
                // Check if someone cancelled us while we were paused
                if (cancellationFlag.load()) {
                    return;
                }
            }

            // 🔢 Count everything we look at - helps the user know we're working!
            filesScannedCount++; // One more file checked!


            try {
                fs::path entryPath = entry.path(); // Get the full path to this item

                // 📁 Is it a directory? Let's dive deeper!
                if (entry.is_directory()) {
                    // Recursion time! Let's explore this subfolder too
                    searchDirectoryRecursive(entryPath, config, reportResult, foundCount, cancellationFlag, filesScannedCount, 
                                            pauseFlag, pauseMutexRef, pauseConditionRef); // Passing all our gear down
                }
                // 📄 Or maybe it's a file? Let's check if it matches what we're looking for
                else if (entry.is_regular_file()) {
                    std::string filename = entryPath.filename().string();
                    std::string filenameEffective = config.caseInsensitive ? toLower(filename) : filename;

                    // 🔍 Test 1: Does the filename contain our search term?
                    if (filenameEffective.find(searchTermEffective) == std::string::npos) {
                        continue; // Nope, not a match - next!
                    }

                    // 🔍 Test 2: If we're filtering by extension, does it match?
                    if (!config.extensionFilter.empty()) {
                        std::string extension = entryPath.extension().string();
                        std::string extensionEffective = config.caseInsensitive ? toLower(extension) : extension;

                        if (extensionEffective != extensionFilterEffective) {
                            continue; // Extension doesn't match - next!
                        }
                    }

                    // 🎉 Success! We found a matching file!
                    foundCount++;
                    // Let's tell the worker right away about our find
                    reportResult(entryPath.string(), ""); // "Hey! Found something!"

                }
                // Ignore other file-system objects (symlinks, etc.) - we're just after regular files

            } catch (const fs::filesystem_error& e) {
                // Oops! Something went wrong with this particular item
                if (config.verboseErrors) {
                    // Try to get the name of the troublemaker
                    std::string entryPathStr = "unknown entry";
                    try { entryPathStr = entry.path().string(); } catch(...) {}
                    reportResult("", "Warning: Can't check " + entryPathStr + " - " + e.what());
                }
                continue; // Let's try the next item
            } catch (const std::exception& e) { // Catch other surprises
                if (config.verboseErrors) {
                    std::string entryPathStr = "unknown entry";
                    try { entryPathStr = entry.path().string(); } catch(...) {}
                    reportResult("", "Warning: Something weird happened with " + entryPathStr + " - " + e.what());
                }
                continue; // On to the next one!
            }
        } // End of our walk through this directory

    } catch (const fs::filesystem_error& e) {
        // Trouble with the directory itself
        if (config.verboseErrors) {
            reportResult("", "Warning: Had trouble with folder " + currentPath.string() + " - " + e.what());
        }
    } catch (const std::exception& e) {
        // Some other kind of trouble
        if (config.verboseErrors) {
            reportResult("", "Warning: Unexpected issue with folder " + currentPath.string() + " - " + e.what());
        }
    }
} // End of our recursive explorer

// 🔎 Let's find all the drives/roots we can search! 🔎
std::vector<fs::path> getRootPaths() {
    std::vector<fs::path> roots;
    // Qt makes this so much easier than raw C++!
    for (const QStorageInfo &storage : QStorageInfo::mountedVolumes()) {
        if (storage.isValid() && storage.isReady() && !storage.isReadOnly()) {
            // Found an accessible drive! Let's add it to our list
            roots.push_back(fs::path(storage.rootPath().toStdString()));
        }
    }

    // Just in case Qt's method doesn't find anything, we have a backup plan
    if (roots.empty()) {
#ifdef _WIN32
        // On Windows, we'll check all drive letters A-Z
        DWORD drives = GetLogicalDrives();
        for (char driveLetter = 'A'; driveLetter <= 'Z'; ++driveLetter) {
            if ((drives >> (driveLetter - 'A')) & 1) {
                std::string driveStr = ""; driveStr += driveLetter; driveStr += ":\\";
                try {
                    // Make sure it exists and is a directory before adding
                    fs::path p(driveStr);
                    if (fs::exists(p) && fs::is_directory(p)) {
                        roots.push_back(p);
                    }
                } catch (...) { /* Ignore troublemaker drives */ }
            }
        }
#else
        // On Unix/Linux/Mac, at least check root
        if (fs::exists("/") && fs::is_directory("/")) {
            roots.push_back(fs::path("/"));
        }
#endif
    }

    return roots; // Here are all the places we can look!
}
