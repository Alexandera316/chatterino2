#include "providers/twitch/twitchaccount.hpp"
#include "const.hpp"
#include "util/urlfetch.hpp"

namespace chatterino {
namespace providers {
namespace twitch {
TwitchAccount::TwitchAccount(const QString &_username, const QString &_oauthToken,
                             const QString &_oauthClient)
    : userName(_username)
    , oauthClient(_oauthClient)
    , oauthToken(_oauthToken)
    , _isAnon(_username == ANONYMOUS_USERNAME)
{
}

const QString &TwitchAccount::getUserName() const
{
    return this->userName;
}

const QString &TwitchAccount::getOAuthClient() const
{
    return this->oauthClient;
}

const QString &TwitchAccount::getOAuthToken() const
{
    return this->oauthToken;
}

const QString &TwitchAccount::getUserId() const
{
    return this->userId;
}

void TwitchAccount::setUserId(const QString &id)
{
    this->userId = id;
}

bool TwitchAccount::setOAuthClient(const QString &newClientID)
{
    if (this->oauthClient.compare(newClientID) == 0) {
        return false;
    }

    this->oauthClient = newClientID;

    return true;
}

bool TwitchAccount::setOAuthToken(const QString &newOAuthToken)
{
    if (this->oauthToken.compare(newOAuthToken) == 0) {
        return false;
    }

    this->oauthToken = newOAuthToken;

    return true;
}

bool TwitchAccount::isAnon() const
{
    return this->_isAnon;
}
}  // namespace twitch
}  // namespace providers
}  // namespace chatterino
