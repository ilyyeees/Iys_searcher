#include "mainwindow.h"
#include "ui_mainwindow.h" // Include the UI definition generated from mainwindow.ui

#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>
#include <QFont>
#include <QIcon>
#include <QColor>
#include <QTextCursor> // Keep for now, might be removed if not used elsewhere
#include <QFile>
#include <QPainter>
#include <QPen>
#include <QRadialGradient>
#include <QTimer>
#include <QStyleOptionButton> // Keep if customizeCheckbox uses it implicitly

// <-- New Includes -->
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <QTableView>        // Explicit include
#include <QHeaderView>       // For table header customization
#include <QMenu>
#include <QAction>
#include <QDesktopServices>  // For opening file location
#include <QUrl>              // For converting path to URL
#include <QClipboard>        // For copying path
#include <QApplication>      // For clipboard access
#include <QTabWidget>        // Explicit include
#include <QPlainTextEdit>    // Explicit include

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , searchThread(nullptr)
    , worker(nullptr)
    , resultsModel(nullptr)      // Initialize new pointers
    , resultsProxyModel(nullptr)
    , currentFoundCount(0)
    , currentScannedCount(0)   // Initialize new counter
    , isSearchPaused(false)    // Initialize new state
    , statusLabel(nullptr)
    , countLabel(nullptr)
    , scannedLabel(nullptr)    // Initialize new label pointer
    , progressBar(nullptr)
    , resultsContextMenu(nullptr) // Initialize new menu pointer
    , openLocationAction(nullptr)
    , copyPathAction(nullptr)
{
    ui->setupUi(this);

    // --- Setup Core UI Elements ---
    setupStatusBar();        // Setup status bar including new label
    setupResultsView();      // Setup the results table view and models
    createContextMenu();     // Create the context menu actions

    // --- Initial State ---
    setGuiEnabled(true);     // Initially enable GUI (disables cancel/pause)
    ui->tabWidget->setCurrentIndex(0); // Start on Results tab

    // --- Styling & Icons ---
    customizeCheckbox(ui->caseInsensitiveCheckBox);
    customizeCheckbox(ui->verboseErrorsCheckBox);

    // Set window icon (using programmatic fallback as before)
    QIcon appIcon;
    if (QFile::exists(":/icons/search_icon.png")) { // Make sure this path is correct in your .qrc
        appIcon = QIcon(":/icons/search_icon.png");
    } else {
        // Programmatic icon generation (keep as fallback)
        QPixmap pixmap(64, 64);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        QRadialGradient gradient(32, 32, 30);
        gradient.setColorAt(0, QColor("#372f4c")); // Consider using QSS colors
        gradient.setColorAt(1, QColor("#201d29"));
        painter.setPen(QPen(QColor("#9381ff"), 2)); // Consider using QSS colors
        painter.setBrush(gradient);
        painter.drawEllipse(4, 4, 56, 56);
        painter.setPen(QPen(QColor("#9381ff"), 3));
        painter.setBrush(QColor(79, 93, 117, 100));
        painter.drawEllipse(12, 12, 30, 30);
        painter.setPen(QPen(QColor("#9381ff"), 5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawLine(40, 40, 52, 52);
        appIcon = QIcon(pixmap);
    }
    setWindowIcon(appIcon);

    // --- Other Setup ---
    QFont defaultFont("Segoe UI", 9); // Can be overridden by QSS
    setFont(defaultFont);
    ui->creditLabel->setFont(QFont("Segoe UI", 8, QFont::StyleItalic)); // Keep specific styles

    // Connect filter input
    connect(ui->resultsFilterLineEdit, &QLineEdit::textChanged, this, &MainWindow::on_resultsFilterLineEdit_textChanged);
}

MainWindow::~MainWindow()
{
    // Graceful thread shutdown (existing code is okay)
    if (searchThread && searchThread->isRunning()) {
        qDebug() << "Main window closing, requesting worker cancel...";
        if(worker) {
            // Use invokeMethod for thread safety
            QMetaObject::invokeMethod(worker, "cancelSearch", Qt::QueuedConnection);
        }
        searchThread->quit();
        if (!searchThread->wait(5000)) {
            qWarning() << "Search thread did not finish gracefully, terminating.";
            searchThread->terminate();
            searchThread->wait();
        }
    }
    // Worker and thread are deleted via deleteLater in onSearchThreadFinished

    // Models are parented to 'this', Qt handles deletion.
    // delete resultsProxyModel; // No need if parented
    // delete resultsModel;      // No need if parented

    delete ui;
}

// --- Setup Methods ---

void MainWindow::setupStatusBar() {
    statusLabel = new QLabel(tr("Ready"), this);
    countLabel = new QLabel(this); // Text set later
    scannedLabel = new QLabel(this); // <-- New label for scanned count
    progressBar = new QProgressBar(this);

    progressBar->setRange(0, 0); // Indeterminate
    progressBar->setVisible(false);
    progressBar->setMaximumHeight(16);
    progressBar->setMaximumWidth(150);

    QFont statusFont("Segoe UI", 9);
    statusFont.setItalic(true);
    statusLabel->setFont(statusFont); // QSS can override

    QFont countFont("Segoe UI", 9);
    countFont.setBold(true);
    countLabel->setFont(countFont);   // QSS can override
    scannedLabel->setFont(countFont); // Use same font for scanned count

    // Initialize text
    countLabel->setText(tr("Found: 0"));
    scannedLabel->setText(tr("Scanned: 0"));

    // Apply stylesheet for padding (better than hardcoding)
    // ui->statusbar->setStyleSheet("QStatusBar { padding: 4px; }"); // Moved to QSS

    ui->statusbar->addWidget(statusLabel, 1); // Stretch status label
    ui->statusbar->addPermanentWidget(scannedLabel);
    ui->statusbar->addPermanentWidget(countLabel);
    ui->statusbar->addPermanentWidget(progressBar);
}

void MainWindow::setupResultsView() {
    // Create models
    resultsModel = new QStandardItemModel(0, 2, this); // 0 rows, 2 columns (Name, Path), parented
    resultsModel->setHorizontalHeaderLabels({tr("Name"), tr("Path")});

    resultsProxyModel = new QSortFilterProxyModel(this); // Parented
    resultsProxyModel->setSourceModel(resultsModel);
    resultsProxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive); // Default filter behavior
    resultsProxyModel->setFilterKeyColumn(-1); // Filter across all columns by default

    // Setup TableView
    ui->resultsTableView->setModel(resultsProxyModel);
    ui->resultsTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->resultsTableView->setSelectionMode(QAbstractItemView::SingleSelection); // Allow single selection for context menu
    ui->resultsTableView->setEditTriggers(QAbstractItemView::NoEditTriggers); // Read-only
    ui->resultsTableView->setSortingEnabled(true);
    ui->resultsTableView->sortByColumn(0, Qt::AscendingOrder); // Initial sort by name

    // Adjust column widths
    ui->resultsTableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive); // Allow resizing Name
    ui->resultsTableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);     // Stretch Path
    ui->resultsTableView->setAlternatingRowColors(true); // Nice visual separation (QSS can enhance)

    // Enable context menu
    ui->resultsTableView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->resultsTableView, &QTableView::customContextMenuRequested,
            this, &MainWindow::showResultsContextMenu);
}

void MainWindow::createContextMenu() {
    resultsContextMenu = new QMenu(this);

    // Use icons from resources if available
    openLocationAction = new QAction(QIcon::fromTheme("folder-open", QIcon(":/icons/folder-open.svg")), tr("Open File Location"), this);
    copyPathAction = new QAction(QIcon::fromTheme("edit-copy", QIcon(":/icons/copy.svg")), tr("Copy Full Path"), this);

    connect(openLocationAction, &QAction::triggered, this, &MainWindow::openFileLocation);
    connect(copyPathAction, &QAction::triggered, this, &MainWindow::copyFilePath);

    resultsContextMenu->addAction(openLocationAction);
    resultsContextMenu->addAction(copyPathAction);
}


void MainWindow::setGuiEnabled(bool enabled) {
    ui->groupBox->setEnabled(enabled); // Search options group
    ui->startButton->setEnabled(enabled);
    ui->cancelButton->setEnabled(!enabled);
    ui->pauseButton->setEnabled(!enabled); // Pause enabled only when search is running
    ui->resultsFilterLineEdit->setEnabled(true); // Keep filter enabled always? Or disable during search? Let's keep enabled.

    if (enabled) {
        progressBar->setVisible(false);
        ui->pauseButton->setText(tr("Pause")); // Reset pause button text
        ui->pauseButton->setProperty("paused", false); // Reset custom property for styling
        style()->polish(ui->pauseButton); // Re-apply style based on property
        isSearchPaused = false; // Reset internal state too
    } else {
        progressBar->setVisible(true);
        // Pause button text/state will be handled by on_pauseButton_clicked
    }
}

// customizeCheckbox remains the same as before
void MainWindow::customizeCheckbox(QCheckBox* checkbox) {
    // Keep previous styling or integrate fully with QSS
    QString style = "QCheckBox::indicator {"
                    "   width: 18px;"
                    "   height: 18px;"
                    /* Let QSS handle border/background */
                    "}"
                    "QCheckBox::indicator:checked {"
                    /* Let QSS handle checked state */
                    "   image: none;"
                    "}";
    // Only apply minimal style if QSS doesn't cover it fully
    // checkbox->setStyleSheet(style); // Consider removing if QSS handles it

    // Palette approach (might conflict with QSS)
    // QPalette pal = checkbox->palette();
    // pal.setColor(QPalette::WindowText, QColor("#d1d7e0")); // Use QSS color
    // checkbox->setPalette(pal);

    // Force update on state change might still be needed for some styles
    connect(checkbox, &QCheckBox::stateChanged, [checkbox](int) {
        checkbox->style()->polish(checkbox);
        checkbox->update();
    });
}

// --- Action Slots ---

void MainWindow::on_browseStartPathButton_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select Start Directory"), // Use tr()
                                                    QDir::homePath(),
                                                    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!dir.isEmpty()) {
        ui->startPathLineEdit->setText(QDir::toNativeSeparators(dir));
    }
}

void MainWindow::on_browseOutputFileButton_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Results As"), // Use tr()
                                                    QDir::homePath() + "/search_results.txt",
                                                    tr("Text Files (*.txt);;All Files (*)")); // Use tr()
    if (!fileName.isEmpty()) {
        ui->outputFileLineEdit->setText(QDir::toNativeSeparators(fileName));
    }
}

void MainWindow::on_startButton_clicked()
{
    QString searchTerm = ui->searchTermLineEdit->text().trimmed();
    if (searchTerm.isEmpty()) {
        QMessageBox::warning(this, tr("Input Required"), tr("Please enter a search term.")); // Use tr()
        return;
    }

    // --- Prepare Configuration --- (Same as before)
    SearchConfig config;
    config.searchTerm = searchTerm.toStdString();
    config.startPath = ui->startPathLineEdit->text().trimmed().toStdString();
    config.extensionFilter = ui->extensionLineEdit->text().trimmed().toStdString();
    config.outputFile = ui->outputFileLineEdit->text().trimmed().toStdString();
    config.caseInsensitive = ui->caseInsensitiveCheckBox->isChecked();
    config.verboseErrors = ui->verboseErrorsCheckBox->isChecked();
    config.searchAllRoots = config.startPath.empty();

    // --- Validate Start Path --- (Improved slightly)
    if (!config.searchAllRoots) {
        fs::path startFsPath = config.startPath;
        std::error_code ec;
        bool exists = fs::exists(startFsPath, ec);
        if (ec || !exists) {
            QMessageBox::warning(this, tr("Invalid Path"), tr("The specified start path does not exist or cannot be accessed."));
            return;
        }
        bool isDir = fs::is_directory(startFsPath, ec);
        if (ec || !isDir) {
            QMessageBox::warning(this, tr("Invalid Path"), tr("The specified start path is not a valid directory."));
            return;
        }
    }

    // --- Setup Thread and Worker ---
    if (searchThread && searchThread->isRunning()) {
        QMessageBox::information(this, tr("Busy"), tr("A search is already in progress.")); // Use tr()
        return;
    }

    // Clean up previous thread/worker if they exist but aren't running
    // onSearchThreadFinished now uses deleteLater, so manual deletion here isn't strictly needed
    // delete worker; worker = nullptr;
    // delete searchThread; searchThread = nullptr;

    searchThread = new QThread(this); // Parented to main window
    worker = new SearchWorker();      // No parent, will be moved

    worker->moveToThread(searchThread);

    // --- Clear Previous Results & Reset State ---
    resultsModel->removeRows(0, resultsModel->rowCount()); // Clear table model
    ui->errorLogTextEdit->clear();                       // Clear error log
    ui->tabWidget->setCurrentIndex(0);                   // Switch to results tab
    currentFoundCount = 0;
    currentScannedCount = 0;
    isSearchPaused = false;
    statusLabel->setText(tr("Starting search..."));
    countLabel->setText(tr("Found: 0"));
    scannedLabel->setText(tr("Scanned: 0"));


    // --- Connect Signals and Slots ---
    // Worker -> MainWindow
    connect(worker, &SearchWorker::resultFound, this, &MainWindow::handleResultFound); // Use default connection (Auto/Queued)
    connect(worker, &SearchWorker::errorOccurred, this, &MainWindow::handleErrorOccurred);
    connect(worker, &SearchWorker::searchFinished, this, &MainWindow::handleSearchFinished);
    connect(worker, &SearchWorker::progressUpdate, this, &MainWindow::handleProgressUpdate);
    connect(worker, &SearchWorker::progressDetailUpdate, this, &MainWindow::handleProgressDetailUpdate); // <-- New connection

    // Thread control
    connect(searchThread, &QThread::started, worker, [this, config](){ worker->doSearch(config); }); // Pass config via lambda
    connect(searchThread, &QThread::finished, this, &MainWindow::onSearchThreadFinished);

    // UI -> Worker (Must use QueuedConnection for thread safety)
    // Cancel button connection now established *during* search start
    connect(ui->cancelButton, &QPushButton::clicked, worker, &SearchWorker::cancelSearch, Qt::QueuedConnection);
    // Pause/Resume connection also established here
    // We connect our pause button handler to the worker's slots
    // connect(ui->pauseButton, &QPushButton::clicked, this, &MainWindow::on_pauseButton_clicked); // Already done by setupUi


    // --- Start Search ---
    setGuiEnabled(false); // Disable controls, enable cancel/pause
    searchThread->start();
    qDebug() << "Search thread started.";
}

void MainWindow::on_cancelButton_clicked()
{
    // This button click primarily signals the worker.
    // The worker's cancelSearch slot handles the core logic.
    statusLabel->setText(tr("Cancelling search..."));
    ui->cancelButton->setEnabled(false); // Prevent multiple clicks
    ui->pauseButton->setEnabled(false);  // Disable pause when cancelling

    // Worker connection is made in startButton_clicked, signal will be queued.
    qDebug() << "Cancel button clicked, signal sent to worker.";
    // Force termination logic removed, relying on graceful shutdown via worker flag check
}

void MainWindow::on_pauseButton_clicked() {
    if (!searchThread || !searchThread->isRunning()) return; // Should not happen if button is enabled correctly

    isSearchPaused = !isSearchPaused; // Toggle state

    if (isSearchPaused) {
        QMetaObject::invokeMethod(worker, "pauseSearch", Qt::QueuedConnection);
        ui->pauseButton->setText(tr("Resume"));
        ui->pauseButton->setProperty("paused", true); // For styling
        statusLabel->setText(tr("Search paused."));
        qDebug() << "Pause requested via invokeMethod.";
    } else {
        QMetaObject::invokeMethod(worker, "resumeSearch", Qt::QueuedConnection);
        ui->pauseButton->setText(tr("Pause"));
        ui->pauseButton->setProperty("paused", false); // For styling
        statusLabel->setText(tr("Resuming search..."));
        qDebug() << "Resume requested via invokeMethod.";
    }
    // Re-apply style based on property change
    style()->polish(ui->pauseButton);
}


void MainWindow::on_resultsFilterLineEdit_textChanged(const QString &text) {
    if (resultsProxyModel) {
        resultsProxyModel->setFilterFixedString(text);
        // Or use: resultsProxyModel->setFilterRegularExpression(QRegularExpression(text, QRegularExpression::CaseInsensitiveOption));
    }
}

void MainWindow::showResultsContextMenu(const QPoint &pos) {
    QModelIndex index = ui->resultsTableView->indexAt(pos);
    bool itemSelected = index.isValid();

    // Enable/disable actions based on whether a valid item is under the cursor
    openLocationAction->setEnabled(itemSelected);
    copyPathAction->setEnabled(itemSelected);

    // Show menu at global position
    resultsContextMenu->popup(ui->resultsTableView->viewport()->mapToGlobal(pos));
}

// --- Worker Signal Handlers ---

void MainWindow::handleResultFound(const QString& path)
{
    // Add row to the model
    QList<QStandardItem*> newRow;
    QFileInfo fileInfo(path);
    QStandardItem* nameItem = new QStandardItem(fileInfo.fileName());
    QStandardItem* pathItem = new QStandardItem(path);

    // Optional: Set tooltip to full path for name column
    nameItem->setToolTip(path);

    // Items are not editable
    nameItem->setEditable(false);
    pathItem->setEditable(false);

    newRow.append(nameItem);
    newRow.append(pathItem);
    resultsModel->appendRow(newRow); // Add to base model

    currentFoundCount++;
    countLabel->setText(tr("Found: %1").arg(currentFoundCount));
}

void MainWindow::handleErrorOccurred(const QString& message)
{
    // Append error to the error log tab
    ui->errorLogTextEdit->appendPlainText(QString("[%1] ERROR: %2")
                                              .arg(QDateTime::currentDateTime().toString(Qt::ISODate))
                                              .arg(message));
    // Optionally switch to the error tab
    // ui->tabWidget->setCurrentWidget(ui->errorLogTextEdit); // Might be annoying, maybe just indicate error?
    // Maybe change the Error tab title color?
    // ui->tabWidget->tabBar()->setTabTextColor(1, Qt::red); // Index 1 assumes error tab is second
}

void MainWindow::handleProgressUpdate(const QString& message)
{
    // Update general status label, unless paused
    if (!isSearchPaused) {
        statusLabel->setText(message);
    }
}

void MainWindow::handleProgressDetailUpdate(quint64 filesScanned, const QString& currentDir) {
    // Update scanned count label
    currentScannedCount = filesScanned; // Update member variable
    scannedLabel->setText(tr("Scanned: %1").arg(currentScannedCount));

    // Optionally update general status if currentDir is provided and we're not paused
    // if (!currentDir.isEmpty() && !isSearchPaused) {
    //    statusLabel->setText(tr("Scanning: %1...").arg(currentDir));
    // }
}


void MainWindow::handleSearchFinished(unsigned long long count, double duration)
{
    qDebug() << "MainWindow received searchFinished signal.";

    // Update GUI - ensure final state is correct
    setGuiEnabled(true); // Re-enable controls, disable cancel/pause, hide progress bar
    statusLabel->setText(tr("Search completed in %1 seconds").arg(duration, 0, 'f', 2));
    countLabel->setText(tr("Found: %1").arg(count)); // Ensure final count is correct
    scannedLabel->setText(tr("Scanned: %1").arg(currentScannedCount)); // Ensure final scanned count

    // Tell the thread to quit its event loop if not already finished
    if (searchThread && !searchThread->isFinished()) {
        searchThread->quit();
    }

    // Add summary to Error Log tab maybe? Or keep in status bar?
    // ui->errorLogTextEdit->appendPlainText(QString("\n--- SEARCH COMPLETE ---"));
    // ui->errorLogTextEdit->appendPlainText(QString("Found %1 files in %2 seconds.").arg(count).arg(duration, 0, 'f', 2));
    // ui->errorLogTextEdit->appendPlainText(QString("Scanned approximately %1 items.").arg(currentScannedCount));
    // ui->errorLogTextEdit->appendPlainText(QString("-----------------------\n"));


    // Automatically disconnect signals connected dynamically? Qt does this when objects are deleted.
    // If we didn't use deleteLater, we'd need to disconnect manually here.
    // disconnect(ui->cancelButton, &QPushButton::clicked, worker, &SearchWorker::cancelSearch);

    qDebug() << "Search finished, GUI updated.";
}

void MainWindow::onSearchThreadFinished() {
    qDebug() << "Search thread finished signal received.";

    // Ensure GUI is in the correct final state
    // setGuiEnabled(true); // Already called by handleSearchFinished normally
    progressBar->setVisible(false);

    // Disconnect signals to worker manually just in case, although deleteLater should handle it.
    // Check if worker pointer is valid before disconnecting
    if(worker) {
        disconnect(ui->cancelButton, &QPushButton::clicked, worker, &SearchWorker::cancelSearch);
        // We didn't directly connect pause button to worker slots, used invokeMethod
    }

    // Schedule worker and thread for deletion safely
    if(worker) {
        worker->deleteLater();
        worker = nullptr;
    }
    if(searchThread) {
        searchThread->deleteLater();
        searchThread = nullptr;
    }

    qDebug() << "Worker and Thread scheduled for deletion.";

    // Final status update if search didn't finish cleanly
    if (statusLabel->text().startsWith(tr("Cancelling"))) {
        statusLabel->setText(tr("Search cancelled."));
    } else if (statusLabel->text().startsWith(tr("Starting")) || statusLabel->text().contains(tr("Searching")) || statusLabel->text().contains(tr("Scanning"))) {
        statusLabel->setText(tr("Search stopped."));
    }
}

// --- Context Menu Action Implementations ---

QString MainWindow::getSelectedPathFromView() const {
    QModelIndexList selectedRows = ui->resultsTableView->selectionModel()->selectedRows();
    if (selectedRows.isEmpty()) {
        return QString(); // No selection
    }

    // Get the index for the first selected row, second column (Path)
    // We need to map the proxy index back to the source model index
    QModelIndex proxyIndex = selectedRows.first();
    QModelIndex sourceIndex = resultsProxyModel->mapToSource(proxyIndex);
    // Assuming path is in the second column (index 1) of the source model
    QModelIndex pathIndex = resultsModel->index(sourceIndex.row(), 1);

    if (pathIndex.isValid()) {
        return resultsModel->data(pathIndex).toString();
    }

    return QString(); // Index was invalid for some reason
}


void MainWindow::openFileLocation() {
    QString path = getSelectedPathFromView();
    if (path.isEmpty()) return;

    QFileInfo fileInfo(path);
    // Open the directory containing the file
    QString directory = fileInfo.absolutePath();
    bool success = QDesktopServices::openUrl(QUrl::fromLocalFile(directory));

    if (!success) {
        QMessageBox::warning(this, tr("Error"), tr("Could not open file location:\n%1").arg(directory));
    }
}

void MainWindow::copyFilePath() {
    QString path = getSelectedPathFromView();
    if (path.isEmpty()) return;

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(path);

    // Optional: Provide feedback in status bar
    // statusLabel->setText(tr("Path copied to clipboard."));
    // QTimer::singleShot(2000, this, [this](){ if(statusLabel->text() == tr("Path copied to clipboard.")) statusLabel->setText(""); }); // Clear after 2s
}
