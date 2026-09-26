#include "SingleInstance.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QLocalServer>
#include <QLocalSocket>
#include <QStandardPaths>

#include <unistd.h>

namespace
{

const int kTimeoutMs = 1000;
// longest socket path that fits into sockaddr_un
const int kMaxSocketPath = 100;
const QByteArray kActivateRequest = "activate";

} // namespace

SingleInstance::SingleInstance(QObject *parent)
    : QObject(parent)
{
    m_server = nullptr;

    // per user socket, e.g. /run/user/1000/DrevoPowerConsole.socket
    QString dir = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    if (dir.isEmpty())
        dir = QDir::tempPath();
    m_server_name = dir + "/" + QCoreApplication::applicationName() + ".socket";
    if (QFile::encodeName(m_server_name).size() > kMaxSocketPath)
    {
        m_server_name = QDir::tempPath() + QString("/%1-%2.socket")
                                               .arg(QCoreApplication::applicationName()).arg(getuid());
    }
}

// ask a running instance to come to the front; true if there is one
bool SingleInstance::activateRunningInstance()
{
    QLocalSocket socket;
    socket.connectToServer(m_server_name);
    if (!socket.waitForConnected(kTimeoutMs))
        return false;

    // "activate <token>": the token lets the running instance take the focus on Wayland
    QByteArray request = kActivateRequest;
    const QByteArray token = qgetenv("XDG_ACTIVATION_TOKEN");
    if (!token.isEmpty())
        request += " " + token;
    socket.write(request + "\n");
    socket.waitForBytesWritten(kTimeoutMs);
    socket.disconnectFromServer();
    return true;
}

// accept activation requests from later instances
bool SingleInstance::listen()
{
    m_server = new QLocalServer(this);
    m_server->setSocketOptions(QLocalServer::UserAccessOption);

    // a socket left behind by a crashed instance does not answer: remove it
    if (!m_server->listen(m_server_name))
    {
        QLocalServer::removeServer(m_server_name);
        if (!m_server->listen(m_server_name))
            return false;
    }

    connect(m_server, &QLocalServer::newConnection, this, [this]() {
        while (QLocalSocket *socket = m_server->nextPendingConnection())
        {
            connect(socket, &QLocalSocket::readyRead, this, [this, socket]() {
                if (!socket->canReadLine())
                    return;
                const QByteArray line = socket->readLine().trimmed();
                if (line == kActivateRequest || line.startsWith(kActivateRequest + " "))
                    emit activationRequested(QString::fromUtf8(line.mid(kActivateRequest.size()).trimmed()));
                socket->disconnectFromServer();
            });
            connect(socket, &QLocalSocket::disconnected, socket, &QObject::deleteLater);
        }
    });
    return true;
}
