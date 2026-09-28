#include "appsettings.h"
#include "clipmodel.h"
#include "clipstorage.h"
#include "devicestatus.h"
#include "recordingengine.h"
#include "subtitlelog.h"
#include "telemetry.h"

#include <sailfishapp.h>

#include <QCoreApplication>
#include <QGuiApplication>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickView>
#include <QQuickWindow>
#include <QScopedPointer>

int main(int argc, char *argv[])
{
    // Qt Multimedia owns GStreamer for Camera + VideoOutput. Do not gst_init()
    // here or the HAL viewfinder sink never attaches (Jolla Camera does not).
    QQuickWindow::setDefaultAlphaBuffer(true);

    QCoreApplication::setOrganizationName(QStringLiteral("org.asphaltcam"));
    QCoreApplication::setApplicationName(QStringLiteral("harbour-asphaltcam"));

    QScopedPointer<QGuiApplication> app(SailfishApp::application(argc, argv));
    app->setOrganizationName(QStringLiteral("org.asphaltcam"));
    app->setApplicationName(QStringLiteral("harbour-asphaltcam"));

    qRegisterMetaType<QStringList>("QStringList");

    AppSettings settings;
    Telemetry telemetry(&settings);
    DeviceStatus deviceStatus(&settings);
    SubtitleLog subtitles(&telemetry);
    ClipStorage storage(&settings);
    ClipModel clips(&storage);
    RecordingEngine recorder(&settings, &storage, &subtitles);

    telemetry.start();

    QScopedPointer<QQuickView> view(SailfishApp::createView());
    QQmlContext *ctx = view->rootContext();
    ctx->setContextProperty(QStringLiteral("appSettings"), &settings);
    ctx->setContextProperty(QStringLiteral("telemetry"), &telemetry);
    ctx->setContextProperty(QStringLiteral("deviceStatus"), &deviceStatus);
    ctx->setContextProperty(QStringLiteral("clipStorage"), &storage);
    ctx->setContextProperty(QStringLiteral("clipModel"), &clips);
    ctx->setContextProperty(QStringLiteral("recorder"), &recorder);
    ctx->setContextProperty(QStringLiteral("subtitleLog"), &subtitles);

    view->engine()->addImportPath(SailfishApp::pathTo(QStringLiteral("qml")).toLocalFile());
    view->setSource(SailfishApp::pathToMainQml());
    view->show();
    return app->exec();
}
