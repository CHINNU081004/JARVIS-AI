#include "core/JarvisController.h"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QIcon>
#include <QDir>
#include <iostream>

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);
    app.setApplicationName("JARVIS");
    app.setApplicationDisplayName("JARVIS - AI Desktop Assistant");
    app.setOrganizationName("JarvisAI");
    app.setWindowIcon(QIcon(":/assets/icon.png"));

    Jarvis::JarvisController controller;
    if (!controller.initialize()) {
        std::cerr << "Warning: Could not fully initialize JARVIS storage/controller." << std::endl;
    }

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("jarvis", &controller);

    const QUrl url(QStringLiteral("qrc:/ui/Main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);

    engine.load(url);

    return app.exec();
}
