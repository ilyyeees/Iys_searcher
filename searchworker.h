#ifndef SEARCHWORKER_H
#define SEARCHWORKER_H

#include <QObject>
#include <QString>
#include <QElapsedTimer> // For timing
#include <QMutex>         // <-- Added for pausing
#include <QWaitCondition> // <-- Added for pausing
#include <fstream>        // For file output handling
#include <atomic>         // For atomic flags
#include <QtGlobal>       // <-- Added for quint64

#include "searchlogic.h" // Include the logic definitions

class SearchWorker : public QObject
{
    Q_OBJECT // Macro required for signals/slots

public:
    explicit SearchWorker(QObject *parent = nullptr);
    ~SearchWorker(); // Destructor to close the file if open

signals:
    // --- Existing Signals ---
    // Signal emitted when a matching file is found
    void resultFound(const QString& path);

    // Signal emitted when a verbose error/warning occurs
    void errorOccurred(const QString& message); // Keep this for errors

    // Signal emitted when the search is complete
    // Params: count, time_in_seconds
    void searchFinished(unsigned long long count, double duration);

    // Signal to update general progress text (e.g., "Searching C:\...")
    void progressUpdate(const QString& message); // Keep for general status

    // --- New Signals ---
    // Signal for more detailed progress update
    void progressDetailUpdate(quint64 filesScanned, const QString& currentDir); // <-- New


public slots:
    // Slot to start the search process
    void doSearch(SearchConfig config);

    // Slot to allow cancelling the search
    void cancelSearch();

    // --- New Slots ---
    // Slot to pause the search
    void pauseSearch(); // <-- New

    // Slot to resume the search
    void resumeSearch(); // <-- New


private:
    // Callback function wrapper to emit signals (no signature change needed yet)
    void handleSearchResult(const std::string& foundPath, const std::string& errorMessage);

    // --- Member Variables ---
    SearchConfig currentConfig;
    QElapsedTimer timer;
    unsigned long long fileCount;       // Counter for *found* files
    std::ofstream outputFileStream;     // Keep file stream here

    std::atomic<bool> isCancelled;      // Flag for cancellation
    std::atomic<bool> isPaused;         // <-- New: Flag for pausing
    std::atomic<quint64> filesScannedCount; // <-- New: Counter for *scanned* files

    QMutex pauseMutex;                  // <-- New: Mutex for pause condition
    QWaitCondition pauseCondition;      // <-- New: Wait condition for pausing

    QString currentSearchDir;           // <-- New: Store current dir for detailed progress signal
};

#endif // SEARCHWORKER_H
