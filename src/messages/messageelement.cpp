#include "messages/messageelement.hpp"
#include "messages/layouts/messagelayoutcontainer.hpp"
#include "messages/layouts/messagelayoutelement.hpp"
#include "singletons/settingsmanager.hpp"
#include "util/benchmark.hpp"
#include "util/emotemap.hpp"

namespace chatterino {
namespace messages {

MessageElement::MessageElement(Flags _flags)
    : flags(_flags)
{
}

MessageElement *MessageElement::setLink(const Link &_link)
{
    this->link = _link;
    return this;
}

MessageElement *MessageElement::setTooltip(const QString &_tooltip)
{
    this->tooltip = _tooltip;
    return this;
}

MessageElement *MessageElement::setTrailingSpace(bool value)
{
    this->trailingSpace = value;
    return this;
}

const QString &MessageElement::getTooltip() const
{
    return this->tooltip;
}

const Link &MessageElement::getLink() const
{
    return this->link;
}

bool MessageElement::hasTrailingSpace() const
{
    return this->trailingSpace;
}

MessageElement::Flags MessageElement::getFlags() const
{
    return this->flags;
}

// IMAGE
ImageElement::ImageElement(Image *_image, MessageElement::Flags flags)
    : MessageElement(flags)
    , image(_image)
{
    this->setTooltip(_image->getTooltip());
}

void ImageElement::addToContainer(MessageLayoutContainer &container, MessageElement::Flags _flags)
{
    if (_flags & this->getFlags()) {
        QSize size(this->image->getWidth() * this->image->getScale() * container.getScale(),
                   this->image->getHeight() * this->image->getScale() * container.getScale());

        container.addElement(
            (new ImageLayoutElement(*this, this->image, size))->setLink(this->getLink()));
    }
}

// EMOTE
EmoteElement::EmoteElement(const util::EmoteData &_data, MessageElement::Flags flags)
    : MessageElement(flags)
    , data(_data)
    , textElement(nullptr)
{
    if (_data.isValid()) {
        this->setTooltip(data.image1x->getTooltip());
        this->textElement = new TextElement(_data.image1x->getName(), MessageElement::Misc);
    }
}

EmoteElement::~EmoteElement()
{
    if (this->textElement != nullptr) {
        delete this->textElement;
    }
}

void EmoteElement::addToContainer(MessageLayoutContainer &container, MessageElement::Flags _flags)
{
    if (_flags & this->getFlags()) {
        if (_flags & MessageElement::EmoteImages) {
            if (!this->data.isValid()) {
                return;
            }

            int quality = singletons::SettingManager::getInstance().preferredEmoteQuality;

            Image *_image;
            if (quality == 3 && this->data.image3x != nullptr) {
                _image = this->data.image3x;
            } else if (quality >= 2 && this->data.image2x != nullptr) {
                _image = this->data.image2x;
            } else {
                _image = this->data.image1x;
            }

            QSize size((int)(container.getScale() * _image->getScaledWidth()),
                       (int)(container.getScale() * _image->getScaledHeight()));

            container.addElement(
                (new ImageLayoutElement(*this, _image, size))->setLink(this->getLink()));
        } else {
            if (this->textElement != nullptr) {
                this->textElement->addToContainer(container, MessageElement::Misc);
            }
        }
    }
}

// TEXT
TextElement::TextElement(const QString &text, MessageElement::Flags flags,
                         const MessageColor &_color, FontStyle _style)
    : MessageElement(flags)
    , color(_color)
    , style(_style)
{
    for (QString word : text.split(' ')) {
        this->words.push_back({word, -1});
        // fourtf: add logic to store mutliple spaces after message
    }
}

void TextElement::addToContainer(MessageLayoutContainer &container, MessageElement::Flags _flags)
{
    if (_flags & this->getFlags()) {
        QFontMetrics &metrics = singletons::FontManager::getInstance().getFontMetrics(
            this->style, container.getScale());
        singletons::ThemeManager &themeManager =
            singletons::ThemeManager::ThemeManager::getInstance();

        for (Word &word : this->words) {
            auto getTextLayoutElement = [&](QString text, int width, bool trailingSpace) {
                QColor color = this->color.getColor(themeManager);
                themeManager.normalizeColor(color);

                auto e = (new TextLayoutElement(*this, text, QSize(width, metrics.height()), color,
                                                this->style, container.getScale()))
                             ->setLink(this->getLink());
                e->setTrailingSpace(trailingSpace);
                return e;
            };

            // fourtf: add again
            //            if (word.width == -1) {
            word.width = metrics.width(word.text);
            //            }

            // see if the text fits in the current line
            if (container.fitsInLine(word.width)) {
                container.addElementNoLineBreak(
                    getTextLayoutElement(word.text, word.width, this->hasTrailingSpace()));
                continue;
            }

            // see if the text fits in the next line
            if (!container.atStartOfLine()) {
                container.breakLine();

                if (container.fitsInLine(word.width)) {
                    container.addElementNoLineBreak(
                        getTextLayoutElement(word.text, word.width, this->hasTrailingSpace()));
                    continue;
                }
            }

            // we done goofed, we need to wrap the text
            QString text = word.text;
            int textLength = text.length();
            int wordStart = 0;
            int width = metrics.width(text[0]);
            int lastWidth = 0;

            for (int i = 1; i < textLength; i++) {
                int charWidth = metrics.width(text[i]);

                if (!container.fitsInLine(width + charWidth)) {
                    container.addElementNoLineBreak(getTextLayoutElement(
                        text.mid(wordStart, i - wordStart), width - lastWidth, false));
                    container.breakLine();

                    wordStart = i;
                    lastWidth = width;
                    width = 0;
                    if (textLength > i + 2) {
                        width += metrics.width(text[i]);
                        width += metrics.width(text[i + 1]);
                        i += 1;
                    }
                    continue;
                }
                width += charWidth;
            }

            container.addElement(getTextLayoutElement(text.mid(wordStart), word.width - lastWidth,
                                                      this->hasTrailingSpace()));
            container.breakLine();
        }
    }
}

// TIMESTAMP
TimestampElement::TimestampElement()
    : TimestampElement(QTime::currentTime())
{
}

TimestampElement::TimestampElement(QTime _time)
    : MessageElement(MessageElement::Timestamp)
    , time(_time)
    , element(formatTime(_time))
{
    assert(this->element != nullptr);
}

TimestampElement::~TimestampElement()
{
    delete this->element;
}

void TimestampElement::addToContainer(MessageLayoutContainer &container,
                                      MessageElement::Flags _flags)
{
    if (_flags & this->getFlags()) {
        if (singletons::SettingManager::getInstance().timestampFormat != this->format) {
            this->format = singletons::SettingManager::getInstance().timestampFormat.getValue();
            delete this->element;
            this->element = TimestampElement::formatTime(this->time);
        }

        this->element->addToContainer(container, _flags);
    }
}

TextElement *TimestampElement::formatTime(const QTime &time)
{
    static QLocale locale("en_US");

    QString format =
        locale.toString(time, singletons::SettingManager::getInstance().timestampFormat);

    return new TextElement(format, Flags::Timestamp, MessageColor::System, FontStyle::Medium);
}

// TWITCH MODERATION
TwitchModerationElement::TwitchModerationElement()
    : MessageElement(MessageElement::ModeratorTools)
{
}

void TwitchModerationElement::addToContainer(MessageLayoutContainer &container,
                                             MessageElement::Flags _flags)
{
    if (_flags & MessageElement::ModeratorTools) {
        QSize size((int)(container.getScale() * 16), (int)(container.getScale() * 16));

        for (const singletons::ModerationAction &m :
             singletons::SettingManager::getInstance().getModerationActions()) {
            if (m.isImage()) {
                container.addElement((new ImageLayoutElement(*this, m.getImage(), size))
                                         ->setLink(Link(Link::UserAction, m.getAction())));
            } else {
                container.addElement((new TextIconLayoutElement(*this, m.getLine1(), m.getLine2(),
                                                                container.getScale(), size))
                                         ->setLink(Link(Link::UserAction, m.getAction())));
            }
        }
    }
}

}  // namespace messages
}  // namespace chatterino
