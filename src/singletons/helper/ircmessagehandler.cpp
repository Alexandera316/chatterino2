#include "ircmessagehandler.hpp"

#include <memory>

#include "debug/log.hpp"
#include "messages/limitedqueue.hpp"
#include "messages/message.hpp"
#include "singletons/channelmanager.hpp"
#include "singletons/resourcemanager.hpp"
#include "singletons/windowmanager.hpp"
#include "twitch/twitchchannel.hpp"

using namespace chatterino::messages;

namespace chatterino {
namespace singletons {
namespace helper {

IrcMessageHandler::IrcMessageHandler(ChannelManager &_channelManager,
                                     ResourceManager &_resourceManager)
    : channelManager(_channelManager)
    , resourceManager(_resourceManager)
{
}

IrcMessageHandler &IrcMessageHandler::getInstance()
{
    static IrcMessageHandler instance(ChannelManager::getInstance(),
                                      ResourceManager::getInstance());
    return instance;
}

void IrcMessageHandler::handleRoomStateMessage(Communi::IrcMessage *message)
{
    const auto &tags = message->tags();

    auto iterator = tags.find("room-id");

    if (iterator != tags.end()) {
        auto roomID = iterator.value().toString();

        auto channel =
            this->channelManager.getTwitchChannel(QString(message->toData()).split("#").at(1));
        auto twitchChannel = dynamic_cast<twitch::TwitchChannel *>(channel.get());
        if (twitchChannel != nullptr) {
            twitchChannel->setRoomID(roomID);
        }

        this->resourceManager.loadChannelData(roomID);
    }
}

void IrcMessageHandler::handleClearChatMessage(Communi::IrcMessage *message)
{
    assert(message->parameters().length() >= 1);

    auto rawChannelName = message->parameter(0);

    assert(rawChannelName.length() >= 2);

    auto trimmedChannelName = rawChannelName.mid(1);

    auto c = this->channelManager.getTwitchChannel(trimmedChannelName);

    if (!c) {
        debug::Log(
            "[IrcMessageHandler:handleClearChatMessage] Channel {} not found in channel manager",
            trimmedChannelName);
        return;
    }

    // check if the chat has been cleared by a moderator
    if (message->parameters().length() == 1) {
        std::shared_ptr<Message> msg(
            Message::createSystemMessage("Chat has been cleared by a moderator."));

        c->addMessage(msg);

        return;
    }

    assert(message->parameters().length() >= 2);

    // get username, duration and message of the timed out user
    QString username = message->parameter(1);
    QString durationInSeconds, reason;
    QVariant v = message->tag("ban-duration");
    if (v.isValid()) {
        durationInSeconds = v.toString();
    }

    v = message->tag("ban-reason");
    if (v.isValid()) {
        reason = v.toString();
    }

    // add the notice that the user has been timed out
    SharedMessage msg(Message::createTimeoutMessage(username, durationInSeconds, reason));

    c->addMessage(msg);

    // disable the messages from the user
    LimitedQueueSnapshot<SharedMessage> snapshot = c->getMessageSnapshot();
    for (int i = 0; i < snapshot.getLength(); i++) {
        if (snapshot[i]->getTimeoutUser() == username) {
            snapshot[i]->setDisabled(true);
        }
    }

    // refresh all
    WindowManager::getInstance().layoutVisibleChatWidgets(c.get());
}

void IrcMessageHandler::handleUserStateMessage(Communi::IrcMessage *message)
{
    // TODO: Implement
}

void IrcMessageHandler::handleWhisperMessage(Communi::IrcMessage *message)
{
    // TODO: Implement
}

void IrcMessageHandler::handleUserNoticeMessage(Communi::IrcMessage *message)
{
    // do nothing
}

void IrcMessageHandler::handleModeMessage(Communi::IrcMessage *message)
{
    auto channel = channelManager.getTwitchChannel(message->parameter(0).remove(0, 1));
    if (message->parameter(1) == "+o") {
        channel->modList.append(message->parameter(2));
    } else if (message->parameter(1) == "-o") {
        channel->modList.append(message->parameter(2));
    }
}

void IrcMessageHandler::handleNoticeMessage(Communi::IrcNoticeMessage *message)
{
    auto rawChannelName = message->target();

    bool broadcast = rawChannelName.length() < 2;
    std::shared_ptr<Message> msg(Message::createSystemMessage(message->content()));

    if (broadcast) {
        this->channelManager.doOnAll([msg](const auto &c) {
            c->addMessage(msg);  //
        });

        return;
    }

    auto trimmedChannelName = rawChannelName.mid(1);

    auto c = this->channelManager.getTwitchChannel(trimmedChannelName);

    if (!c) {
        debug::Log("[IrcManager:handleNoticeMessage] Channel {} not found in channel manager",
                   trimmedChannelName);
        return;
    }

    c->addMessage(msg);
}

void IrcMessageHandler::handleWriteConnectionNoticeMessage(Communi::IrcNoticeMessage *message)
{
    QVariant v = message->tag("msg-id");
    if (!v.isValid()) {
        return;
    }
    QString msg_id = v.toString();

    static QList<QString> idsToSkip = {"timeout_success", "ban_success"};

    if (idsToSkip.contains(msg_id)) {
        // Already handled in the read-connection
        return;
    }

    this->handleNoticeMessage(message);
}
}
}
}
