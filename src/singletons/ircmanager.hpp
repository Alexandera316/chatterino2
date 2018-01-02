#pragma once

#define TWITCH_MAX_MESSAGELENGTH 500

#include "messages/message.hpp"
#include "twitch/twitchuser.hpp"

#include <ircconnection.h>
#include <IrcMessage>
#include <QMap>
#include <QMutex>
#include <QNetworkAccessManager>
#include <QString>
#include <pajlada/signals/signal.hpp>

#include <memory>
#include <mutex>

namespace chatterino {
namespace singletons {

class ChannelManager;
class ResourceManager;
class AccountManager;
class WindowManager;

class IrcManager : public QObject
{
    IrcManager(ChannelManager &channelManager, ResourceManager &resources,
               AccountManager &accountManager);

public:
    static IrcManager &getInstance();

    void connect();
    void disconnect();

    bool isTwitchUserBlocked(QString const &username);
    bool tryAddIgnoredUser(QString const &username, QString &errorMessage);
    void addIgnoredUser(QString const &username);
    bool tryRemoveIgnoredUser(QString const &username, QString &errorMessage);
    void removeIgnoredUser(QString const &username);

    void sendMessage(const QString &channelName, QString message);

    void joinChannel(const QString &channelName);
    void partChannel(const QString &channelName);

    void setUser(std::shared_ptr<twitch::TwitchUser> newAccount);

    pajlada::Signals::Signal<Communi::IrcPrivateMessage *> onPrivateMessage;
    void privateMessageReceived(Communi::IrcPrivateMessage *message);

    Communi::IrcConnection *getReadConnection();

private:
    ChannelManager &channelManager;
    ResourceManager &resources;
    AccountManager &accountManager;

    // variables
    std::shared_ptr<twitch::TwitchUser> account = nullptr;

    std::unique_ptr<Communi::IrcConnection> writeConnection = nullptr;
    std::unique_ptr<Communi::IrcConnection> readConnection = nullptr;

    std::mutex connectionMutex;

    QMap<QString, bool> twitchBlockedUsers;
    QMutex twitchBlockedUsersMutex;

    QNetworkAccessManager networkAccessManager;

    void initializeConnection(const std::unique_ptr<Communi::IrcConnection> &connection,
                              bool isReadConnection);

    void refreshIgnoredUsers(const QString &username, const QString &oauthClient,
                             const QString &oauthToken);

    void beginConnecting();

    void messageReceived(Communi::IrcMessage *message);

    void writeConnectionMessageReceived(Communi::IrcMessage *message);

    void onConnected();
    void onDisconnected();

private:
    QByteArray messageSuffix;
};

}  // namespace chatterino
}
