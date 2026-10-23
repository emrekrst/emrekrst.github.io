#include <QApplication>
#include <QFileInfo>
#include <QTimer>

#include <gst/gst.h>

#include "config/CameraConfig.hpp"
#include "ui/MainWindow.hpp"

int main(int argc, char* argv[])
{
    gst_init(&argc, &argv);

    QApplication application(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("Camera Tracker"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.1.0"));
    QCoreApplication::setOrganizationName(QStringLiteral("Camera Tracker"));

    const QStringList arguments = application.arguments();
    QString configPath = QStringLiteral("config/camera-tracker.ini");
    const int configIndex = arguments.indexOf(QStringLiteral("--config"));
    if (configIndex >= 0 && configIndex + 1 < arguments.size()) {
        configPath = arguments.at(configIndex + 1);
    }
    const CameraConfigResult cameraConfig =
        loadCameraConfig(QFileInfo(configPath).absoluteFilePath());

    MainWindow window(cameraConfig);
    window.show();

    // --open <path> skips the file dialog so playback can be exercised
    // repeatably from the command line.
    const int openIndex = arguments.indexOf(QStringLiteral("--open"));
    if (openIndex >= 0 && openIndex + 1 < arguments.size()) {
        const QString path = arguments.at(openIndex + 1);
        QTimer::singleShot(300, &window, [&window, path] {
            window.openVideoFile(path);
        });
    }

    if (arguments.contains(QStringLiteral("--camera"))) {
        QTimer::singleShot(300, &window, [&window] {
            QMetaObject::invokeMethod(&window, "startCamera");
        });
    }

    const int selectIndex = arguments.indexOf(QStringLiteral("--select"));
    if (selectIndex >= 0 && selectIndex + 2 < arguments.size()) {
        bool xOk = false;
        bool yOk = false;
        const double x = arguments.at(selectIndex + 1).toDouble(&xOk);
        const double y = arguments.at(selectIndex + 2).toDouble(&yOk);
        if (xOk && yOk) {
            QTimer::singleShot(1500, &window, [&window, x, y] {
                window.selectTarget({x, y});
            });
        }
    }

    if (application.arguments().contains(QStringLiteral("--smoke-test"))) {
        QTimer::singleShot(750, &application, &QCoreApplication::quit);
    }

    return application.exec();
}
