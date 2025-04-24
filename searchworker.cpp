#include "searchworker.h"
#include <QDebug> // For my diagnostic chatter
#include <QDir>   // For helping with paths
#include <QMutexLocker> // For safe pausing without any drama
#include <vector>
#include <filesystem> // Modern C++ file stuff - so much nicer!

namespace fs = std::filesystem;

SearchWorker::SearchWorker(QObject *parent)
    : QObject(parent),
    fileCount(0),
    isCancelled(false),
    isPaused(false), // Start ready to roll!
    filesScannedCount(0) // No files checked yet
{
    // Nothing special needed here - we're all initialized and ready to go!
}

SearchWorker::~SearchWorker() {
    // Let's be tidy - close any open files before we leave
    if (outputFileStream.is_open()) {
        outputFileStream.close();
    }
}

// 🛑 Pause Button Handler 🛑
void SearchWorker::pauseSearch() {
    qDebug() << "Whoa there! Pause requested.";
    isPaused.store(true);
    // The worker thread will check this flag and wait when it can.
    // Think of this as raising a yellow flag - everyone will stop when they see it!
}

// ▶️ Resume Button Handler ▶️
void SearchWorker::resumeSearch() {
    qDebug() << "Let's get moving again! Resume requested.";
    if (isPaused.load()) {
        isPaused.store(false); // Lower the yellow flag
        // Tap on the shoulder of any waiting threads
        QMutexLocker locker(&pauseMutex); // Lock for safety
        pauseCondition.wakeAll(); // Ring the bell! "Everyone back to work!"
        qDebug() << "Wake up call sent to the search thread!";
    }
}


// ⛔ Cancel Button Handler ⛔
void SearchWorker::cancelSearch() {
    isCancelled.store(true);
    qDebug() << "Stop everything! User wants to cancel the search.";

    // If we're paused, we need to wake up to see the cancel flag
    if (isPaused.load()) {
        isPaused.store(false); // Don't pause again right away
        QMutexLocker locker(&pauseMutex); // Lock for safety
        pauseCondition.wakeAll();
        qDebug() << "Had to wake the paused thread to tell it we're cancelling.";
    }

    // Let's close up any output file and note that we got cancelled
    if (outputFileStream.is_open()) {
        outputFileStream << "\nSearch was cancelled by user." << std::endl;
        outputFileStream.close();
    }

    // Tell the UI we're on it
    emit progressUpdate(tr("Cancelling search...")); // Use tr() for translation goodness
}

// 🔍 The Big Search Function - This Is Where It All Happens! 🔍
void SearchWorker::doSearch(SearchConfig config) {
    // 🔄 Reset Everything For A Fresh Start
    isCancelled.store(false);
    isPaused.store(false);
    fileCount = 0;
    filesScannedCount.store(0); // Reset our counter
    currentConfig = config;
    currentSearchDir = ""; // No current directory yet
    timer.start(); // Start the stopwatch!

    // 📄 Set Up Output File If Requested
    if (outputFileStream.is_open()) {
        outputFileStream.close(); // Close any previous file
    }
    if (!config.outputFile.empty()) {
        outputFileStream.open(config.outputFile);
        if (!outputFileStream.is_open()) {
            emit errorOccurred(tr("Oops! Can't open the output file: %1").arg(QString::fromStdString(config.outputFile)));
            // We'll continue anyway, just without saving to a file
        } else {
            // Add a nice header to the file
            outputFileStream << "🔍 IYS Searcher Results 🔍" << std::endl;
            outputFileStream << "Search term: '" << config.searchTerm << "'" << std::endl;
            outputFileStream << "Starting from: " << (config.startPath.empty() ? "All Drives" : config.startPath) << std::endl;
            outputFileStream << "------------------------------------------" << std::endl;
        }
    }

    // 🧭 Figure Out Where To Start Looking
    std::vector<fs::path> rootsToSearch;
    if (config.searchAllRoots) {
        emit progressUpdate(tr("Figuring out what drives you have..."));
        rootsToSearch = getRootPaths();
        if (rootsToSearch.empty()) {
            emit errorOccurred(tr("Hmm, couldn't find any drives to search. That's strange!"));
            emit searchFinished(0, timer.elapsed() / 1000.0);
            return;
        }
    } else {
        // The user specified a particular folder
        fs::path userPath = config.startPath;
        // Let's make sure it exists before we try to search it
        if (!fs::exists(userPath)) {
            emit errorOccurred(tr("I couldn't find that folder: %1").arg(QString::fromStdString(config.startPath)));
            emit searchFinished(0, timer.elapsed() / 1000.0);
            return;
        }
        if (!fs::is_directory(userPath)) {
            emit errorOccurred(tr("Hey, that's not a folder: %1").arg(QString::fromStdString(config.startPath)));
            emit searchFinished(0, timer.elapsed() / 1000.0);
            return;
        }
        // Get the absolute path (full path) to avoid confusion
        fs::path absoluteUserPath = fs::absolute(userPath);
        rootsToSearch.push_back(absoluteUserPath);
    }

    // 🚀 Let's Start Searching!
    for (const auto& root : rootsToSearch) {
        if (isCancelled.load()) break; // Bail if cancelled

        // Pause check now happens inside searchDirectoryRecursive
        // This way we can pause even deep in the file tree!

        // Update the UI about where we're looking
        currentSearchDir = QString::fromStdString(root.string());
        emit progressUpdate(tr("Digging through: %1...").arg(currentSearchDir));
        // Also update the detailed counts
        emit progressDetailUpdate(filesScannedCount.load(), currentSearchDir);


        // Set up our callback for handling finds and errors
        auto callback = std::bind(&SearchWorker::handleSearchResult, this,
                                  std::placeholders::_1, std::placeholders::_2);

        // 🔍 Call our recursive explorer with all the tools it needs
        searchDirectoryRecursive(root, config, callback, fileCount, isCancelled, filesScannedCount, 
                                isPaused, pauseMutex, pauseCondition);

        // Update counts after finishing each root
        emit progressDetailUpdate(filesScannedCount.load(), currentSearchDir);

    } // End of loop over roots


    // 🏁 We're Done! Let's Wrap Things Up
    if (outputFileStream.is_open()) {
        outputFileStream << "------------------------------------------" << std::endl;
        if (isCancelled.load()) {
            outputFileStream << "Search was cancelled. Found " << fileCount << " file(s) before stopping." << std::endl;
        } else {
            outputFileStream << "Search complete! Found " << fileCount << " file(s)." << std::endl;
        }
        outputFileStream << "Checked approximately " << filesScannedCount.load() << " items." << std::endl;
        outputFileStream << "Search took " << timer.elapsed() / 1000.0 << " seconds." << std::endl;
        outputFileStream.close();
    }

    // Final update for the UI
    QString finalMessage;
    if (isCancelled.load()) {
        finalMessage = tr("Search cancelled.");
    } else {
        finalMessage = tr("All done! Search finished.");
    }
    emit progressUpdate(finalMessage);
    emit progressDetailUpdate(filesScannedCount.load(), ""); // Final count update


    emit searchFinished(fileCount, timer.elapsed() / 1000.0);
    qDebug() << "Search complete! Signaled the UI we're done.";
}


// 📬 This Is How We Handle Findings & Errors During The Search
// Gets called from inside searchDirectoryRecursive whenever it finds something
void SearchWorker::handleSearchResult(const std::string& foundPath, const std::string& errorMessage) {
    // Don't bother reporting if we're cancelled or paused
    if (isCancelled.load() || isPaused.load()) return;

    if (!foundPath.empty()) {
        // Found a file! 🎉
        if (outputFileStream.is_open()) {
            outputFileStream << foundPath << std::endl;
        }
        // Tell the UI about our find
        emit resultFound(QString::fromStdString(foundPath));
    } else if (!errorMessage.empty() && currentConfig.verboseErrors) {
        // Hit an error 😕
        emit errorOccurred(QString::fromStdString(errorMessage));
    }

    // We could also update progress here for extra responsiveness
    // But that might flood the UI with too many updates
    // So we just let the main loop handle periodic progress reports
}
