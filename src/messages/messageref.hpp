#pragma once

#include "messages/message.hpp"
#include "messages/wordpart.hpp"

#include <QPixmap>

#include <memory>

namespace chatterino {
namespace messages {

class MessageRef;

typedef std::shared_ptr<MessageRef> SharedMessageRef;

class MessageRef
{
public:
    MessageRef(SharedMessage message);

    Message *getMessage();
    int getHeight() const;

    bool layout(int width, float scale);

    const std::vector<WordPart> &getWordParts() const;

    std::shared_ptr<QPixmap> buffer = nullptr;
    bool updateBuffer = false;

    const Word *tryGetWordPart(QPoint point);
    int getLastCharacterIndex() const;
    int getSelectionIndex(QPoint position);

    bool isCollapsed() const;
    void setCollapsed(bool value);
    int getCollapsedHeight() const;
    int getCollapsedLineCount() const;

    bool isDisabled() const;

private:
    // variables
    SharedMessage message;
    std::vector<WordPart> wordParts;

    int height = 0;

    int currentLayoutWidth = -1;
    int fontGeneration = -1;
    int emoteGeneration = -1;
    float scale = -1;

    Word::Flags currentWordTypes = Word::None;

    bool collapsed;
    int collapsedHeight = 32;

    // methods
    void rebuild();
    void actuallyLayout(int width);
    void _alignWordParts(int lineStart, int lineHeight, int width, int &firstLineHeight);
    int _updateLineHeight(int currentLineHeight, Word &word, bool overlapEmotes);
    void updateTextSizes();
    void updateImageSizes();
};

}  // namespace messages
}  // namespace chatterino
