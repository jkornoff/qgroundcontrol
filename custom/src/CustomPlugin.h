#pragma once

#include <QtQml/QQmlAbstractUrlInterceptor>

#include "QGCCorePlugin.h"

class QQmlApplicationEngine;

Q_DECLARE_LOGGING_CATEGORY(CustomLog)

/// Redirects qrc:/qml/<path> loads to qrc:/Custom/qml/<path> when an override resource exists.
class CustomOverrideInterceptor : public QQmlAbstractUrlInterceptor
{
public:
    CustomOverrideInterceptor() = default;

    QUrl intercept(const QUrl& url, QQmlAbstractUrlInterceptor::DataType type) final;
};

/// BSD core plugin. Only installs the resource override interceptor; all stock
/// QGC behaviour (both firmware plugins, advanced UI) is left unchanged.
class CustomPlugin : public QGCCorePlugin
{
    Q_OBJECT

public:
    explicit CustomPlugin(QObject* parent = nullptr);

    static QGCCorePlugin* instance();

    // Overrides from QGCCorePlugin

    QQmlApplicationEngine* createQmlApplicationEngine(QObject* parent) final;
    void destroyQmlApplicationEngine(QQmlApplicationEngine* qmlEngine) final;

private:
    QQmlApplicationEngine* _qmlEngine = nullptr;
    CustomOverrideInterceptor* _urlInterceptor = nullptr;
};
