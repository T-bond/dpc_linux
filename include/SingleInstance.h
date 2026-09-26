#ifndef SINGLEINSTANCE_H
#define SINGLEINSTANCE_H

#include <QObject>
#include <QString>

class QLocalServer;

// only one instance may use the keyboard: a second one asks the running instance to show its
// window and exits
class SingleInstance : public QObject
{
    Q_OBJECT

public:
    explicit SingleInstance(QObject *parent = nullptr);

    // ask a running instance to come to the front; true if there is one
    bool activateRunningInstance();
    // accept activation requests from later instances
    bool listen();

signals:
    // a later instance was started; token: its Wayland activation token (may be empty)
    void activationRequested(const QString &token);

private:
    QString         m_server_name;
    QLocalServer   *m_server;
};

#endif // SINGLEINSTANCE_H
