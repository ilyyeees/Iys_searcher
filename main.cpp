#include "mainwindow.h"

#include <QApplication>
#include <QIcon>
#include <QSplashScreen>
#include <QTimer>
#include <QPixmap>
#include <QFile>          // <-- Added
#include <QTextStream>    // <-- Added
#include <QPainter>
#include <QLinearGradient>
#include <QPen>
#include <QFont>
#include <QDebug>         // <-- Added for logging/warning

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // --- Load Stylesheet --- START
    // Assuming stylesheet.qss is added to resources under the prefix "styles"
    // Example .qrc entry: <file alias="stylesheet.qss">styles/stylesheet.qss</file>
    QFile styleFile(":/styles/stylesheet.qss");
    if (styleFile.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream stream(&styleFile);
        QString styleSheet = stream.readAll();
        a.setStyleSheet(styleSheet);
        styleFile.close();
        qDebug() << "Stylesheet loaded successfully from resources.";
    } else {
        qWarning() << "Could not find or open stylesheet ':/styles/stylesheet.qss'. Using default styles.";
        // Optionally, you could try loading from the filesystem as a fallback:
        /*
        styleFile.setFileName("styles/stylesheet.qss"); // Look relative to executable
        if (styleFile.open(QFile::ReadOnly | QFile::Text)) {
             QTextStream stream(&styleFile);
             a.setStyleSheet(stream.readAll());
             styleFile.close();
             qDebug() << "Stylesheet loaded successfully from filesystem.";
        } else {
             qWarning() << "Could not find or open stylesheet from filesystem either.";
        }
        */
    }
    // --- Load Stylesheet --- END


    // --- Application Metadata --- (Keep as is)
    a.setApplicationName("IYS searcher");
    a.setApplicationDisplayName("IYS searcher");
    a.setApplicationVersion("1.0");
    a.setOrganizationName("il.y.s");

    // --- Splash Screen --- (Keep as is - uses programmatic fallback if resource fails)
    QPixmap splashPixmap;
    // Assuming splash screen is in resources under prefix "icons"
    if (QFile::exists(":/icons/splash_screen.png")) {
        splashPixmap = QPixmap(":/icons/splash_screen.png");
    } else {
        qWarning() << "Splash screen resource ':/icons/splash_screen.png' not found. Creating fallback.";
        // Create a programmatic splash screen (keep existing fallback code)
        splashPixmap = QPixmap(480, 320);
        splashPixmap.fill(QColor("#2d283e")); // Colors might be overridden by QSS later

        QPainter painter(&splashPixmap);
        painter.setRenderHint(QPainter::Antialiasing);

        // Draw a gradient background
        QLinearGradient gradient(0, 0, 0, splashPixmap.height());
        gradient.setColorAt(0, QColor("#372f4c"));
        gradient.setColorAt(1, QColor("#201d29"));
        painter.fillRect(splashPixmap.rect(), gradient);

        // Draw border
        painter.setPen(QPen(QColor("#564f6f"), 4));
        painter.drawRect(splashPixmap.rect().adjusted(2, 2, -2, -2));

        // Draw app name
        QFont titleFont("Segoe UI", 28, QFont::Bold);
        painter.setFont(titleFont);
        painter.setPen(QColor("#d4bfff")); // Use theme colors?
        painter.drawText(splashPixmap.rect().adjusted(20, 20, -20, -120),
                         Qt::AlignHCenter | Qt::AlignTop,
                         "IYS searcher");

        // Draw icon (fallback)
        QPixmap iconPixmap(100, 100);
        iconPixmap.fill(Qt::transparent);
        QPainter iconPainter(&iconPixmap);
        iconPainter.setRenderHint(QPainter::Antialiasing);
        iconPainter.setPen(QPen(QColor("#9381ff"), 5)); // Use theme colors?
        iconPainter.setBrush(QColor(79, 93, 117, 100));
        iconPainter.drawEllipse(10, 10, 80, 80);
        iconPainter.setPen(QPen(QColor("#9381ff"), 8));
        iconPainter.drawLine(70, 70, 90, 90);
        painter.drawPixmap(splashPixmap.width()/2 - 50, splashPixmap.height()/2 - 40, iconPixmap);

        // Draw credit text
        QFont creditFont("Segoe UI", 12, QFont::StyleItalic);
        painter.setFont(creditFont);
        painter.setPen(QColor("#9381ff")); // Use theme colors?
        painter.drawText(splashPixmap.rect().adjusted(20, -60, -20, -20),
                         Qt::AlignHCenter | Qt::AlignBottom,
                         "Made By il.y.s");

        // Draw loading text
        QFont loadingFont("Segoe UI", 10);
        painter.setFont(loadingFont);
        painter.setPen(QColor("#d1d7e0")); // Use theme colors?
        painter.drawText(splashPixmap.rect().adjusted(20, -30, -20, -20),
                         Qt::AlignHCenter | Qt::AlignBottom,
                         "Loading...");
    }

    QSplashScreen splash(splashPixmap);
    splash.show();

    // Process events to make sure splash screen is displayed immediately
    a.processEvents();

    // Create main window but don't show it yet
    MainWindow w;

    // Hide splash and show main window after delay (keep as is)
    QTimer::singleShot(2000, &splash, [&splash, &w]() { // Reduced delay slightly
        splash.close();
        w.show();
    });

    return a.exec(); // Start the Qt event loop
}
