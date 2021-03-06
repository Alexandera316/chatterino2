#include "widgets/scrollbar.hpp"
#include "singletons/thememanager.hpp"
#include "widgets/helper/channelview.hpp"

#include <QDebug>
#include <QMouseEvent>
#include <QPainter>
#include <QTimer>
#include <cmath>

#define MIN_THUMB_HEIGHT 10

namespace chatterino {
namespace widgets {

Scrollbar::Scrollbar(ChannelView *parent)
    : BaseWidget(parent)
    , currentValueAnimation(this, "currentValue")
    , smoothScrollingSetting(singletons::SettingManager::getInstance().enableSmoothScrolling)
{
    resize((int)(16 * this->getScale()), 100);
    this->currentValueAnimation.setDuration(150);
    this->currentValueAnimation.setEasingCurve(QEasingCurve(QEasingCurve::OutCubic));

    setMouseTracking(true);

    // don't do this at home kids
    QTimer *timer = new QTimer(this);
    timer->setSingleShot(true);

    connect(timer, &QTimer::timeout, [=]() {
        resize((int)(16 * this->getScale()), 100);
        timer->deleteLater();
    });

    timer->start(10);
}

void Scrollbar::addHighlight(ScrollbarHighlight highlight)
{
    ScrollbarHighlight deleted;
    this->highlights.pushBack(highlight, deleted);
}

void Scrollbar::addHighlightsAtStart(const std::vector<ScrollbarHighlight> &_highlights)
{
    this->highlights.pushFront(_highlights);
}

void Scrollbar::replaceHighlight(size_t index, ScrollbarHighlight replacement)
{
    this->highlights.replaceItem(index, replacement);
}

void Scrollbar::scrollToBottom(bool animate)
{
    this->setDesiredValue(this->maximum - this->getLargeChange(), animate);
}

bool Scrollbar::isAtBottom() const
{
    return this->atBottom;
}

void Scrollbar::setMaximum(qreal value)
{
    this->maximum = value;

    updateScroll();
}

void Scrollbar::setMinimum(qreal value)
{
    this->minimum = value;

    updateScroll();
}

void Scrollbar::setLargeChange(qreal value)
{
    this->largeChange = value;

    updateScroll();
}

void Scrollbar::setSmallChange(qreal value)
{
    this->smallChange = value;

    updateScroll();
}

void Scrollbar::setDesiredValue(qreal value, bool animated)
{
    animated &= this->smoothScrollingSetting.getValue();
    value = std::max(this->minimum, std::min(this->maximum - this->largeChange, value));

    if (this->desiredValue + this->smoothScrollingOffset != value) {
        if (animated) {
            this->currentValueAnimation.stop();
            this->currentValueAnimation.setStartValue(this->currentValue +
                                                      this->smoothScrollingOffset);

            //            if (((this->getMaximum() - this->getLargeChange()) - value) <= 0.01) {
            //                value += 1;
            //            }
            this->currentValueAnimation.setEndValue(value);
            this->smoothScrollingOffset = 0;
            this->atBottom = ((this->getMaximum() - this->getLargeChange()) - value) <= 0.01;
            this->currentValueAnimation.start();
        } else {
            if (this->currentValueAnimation.state() != QPropertyAnimation::Running) {
                this->smoothScrollingOffset = 0;
                this->desiredValue = value;
                this->currentValueAnimation.stop();
                this->atBottom = ((this->getMaximum() - this->getLargeChange()) - value) <= 0.01;
                setCurrentValue(value);
            }
        }
    }

    this->smoothScrollingOffset = 0;
    this->desiredValue = value;
}

qreal Scrollbar::getMaximum() const
{
    return this->maximum;
}

qreal Scrollbar::getMinimum() const
{
    return this->minimum;
}

qreal Scrollbar::getLargeChange() const
{
    return this->largeChange;
}

qreal Scrollbar::getSmallChange() const
{
    return this->smallChange;
}

qreal Scrollbar::getDesiredValue() const
{
    return this->desiredValue + this->smoothScrollingOffset;
}

qreal Scrollbar::getCurrentValue() const
{
    return this->currentValue;
}

void Scrollbar::offset(qreal value)
{
    if (this->currentValueAnimation.state() == QPropertyAnimation::Running) {
        this->smoothScrollingOffset += value;
    } else {
        this->setDesiredValue(this->getDesiredValue() + value);
    }
}

boost::signals2::signal<void()> &Scrollbar::getCurrentValueChanged()
{
    return this->currentValueChanged;
}

void Scrollbar::setCurrentValue(qreal value)
{
    value = std::max(this->minimum, std::min(this->maximum - this->largeChange,
                                             value + this->smoothScrollingOffset));

    if (std::abs(this->currentValue - value) > 0.000001) {
        this->currentValue = value;

        this->updateScroll();
        this->currentValueChanged();

        this->update();
    }
}

void Scrollbar::printCurrentState(const QString &prefix) const
{
    qDebug() << prefix                                         //
             << "Current value: " << this->getCurrentValue()   //
             << ". Maximum: " << this->getMaximum()            //
             << ". Minimum: " << this->getMinimum()            //
             << ". Large change: " << this->getLargeChange();  //
}

void Scrollbar::paintEvent(QPaintEvent *)
{
    bool mouseOver = this->mouseOverIndex != -1;
    int xOffset = mouseOver ? 0 : width() - (int)(4 * this->getScale());

    QPainter painter(this);
    //    painter.fillRect(rect(), this->themeManager.ScrollbarBG);

    //    painter.fillRect(QRect(xOffset, 0, width(), this->buttonHeight),
    //                     this->themeManager.ScrollbarArrow);
    //    painter.fillRect(QRect(xOffset, height() - this->buttonHeight, width(),
    //    this->buttonHeight),
    //                     this->themeManager.ScrollbarArrow);

    this->thumbRect.setX(xOffset);

    // mouse over thumb
    if (this->mouseDownIndex == 2) {
        painter.fillRect(this->thumbRect, this->themeManager.scrollbars.thumbSelected);
    }
    // mouse not over thumb
    else {
        painter.fillRect(this->thumbRect, this->themeManager.scrollbars.thumb);
    }

    // draw highlights
    auto snapshot = this->highlights.getSnapshot();
    int snapshotLength = (int)snapshot.getLength();

    if (snapshotLength == 0) {
        return;
    }

    int w = this->width();
    float y = 0;
    float dY = (float)(this->height()) / (float)snapshotLength;
    int highlightHeight = std::ceil(dY);

    for (int i = 0; i < snapshotLength; i++) {
        ScrollbarHighlight const &highlight = snapshot[i];

        if (!highlight.isNull()) {
            if (highlight.getStyle() == ScrollbarHighlight::Default) {
                painter.fillRect(w / 8 * 3, (int)y, w / 4, highlightHeight,
                                 this->themeManager.tabs.selected.backgrounds.regular.color());
            } else {
                painter.fillRect(0, (int)y, w, 1,
                                 this->themeManager.tabs.selected.backgrounds.regular.color());
            }
        }

        y += dY;
    }
}

void Scrollbar::resizeEvent(QResizeEvent *)
{
    this->resize((int)(16 * this->getScale()), this->height());
}

void Scrollbar::mouseMoveEvent(QMouseEvent *event)
{
    if (this->mouseDownIndex == -1) {
        int y = event->pos().y();

        auto oldIndex = this->mouseOverIndex;

        if (y < this->buttonHeight) {
            this->mouseOverIndex = 0;
        } else if (y < this->thumbRect.y()) {
            this->mouseOverIndex = 1;
        } else if (this->thumbRect.contains(2, y)) {
            this->mouseOverIndex = 2;
        } else if (y < height() - this->buttonHeight) {
            this->mouseOverIndex = 3;
        } else {
            this->mouseOverIndex = 4;
        }

        if (oldIndex != this->mouseOverIndex) {
            update();
        }
    } else if (this->mouseDownIndex == 2) {
        int delta = event->pos().y() - this->lastMousePosition.y();

        setDesiredValue(this->desiredValue + (qreal)delta / this->trackHeight * this->maximum);
    }

    this->lastMousePosition = event->pos();
}

void Scrollbar::mousePressEvent(QMouseEvent *event)
{
    int y = event->pos().y();

    if (y < this->buttonHeight) {
        this->mouseDownIndex = 0;
    } else if (y < this->thumbRect.y()) {
        this->mouseDownIndex = 1;
    } else if (this->thumbRect.contains(2, y)) {
        this->mouseDownIndex = 2;
    } else if (y < height() - this->buttonHeight) {
        this->mouseDownIndex = 3;
    } else {
        this->mouseDownIndex = 4;
    }
}

void Scrollbar::mouseReleaseEvent(QMouseEvent *event)
{
    int y = event->pos().y();

    if (y < this->buttonHeight) {
        if (this->mouseDownIndex == 0) {
            setDesiredValue(this->desiredValue - this->smallChange, true);
        }
    } else if (y < this->thumbRect.y()) {
        if (this->mouseDownIndex == 1) {
            setDesiredValue(this->desiredValue - this->smallChange, true);
        }
    } else if (this->thumbRect.contains(2, y)) {
        // do nothing
    } else if (y < height() - this->buttonHeight) {
        if (this->mouseDownIndex == 3) {
            setDesiredValue(this->desiredValue + this->smallChange, true);
        }
    } else {
        if (this->mouseDownIndex == 4) {
            setDesiredValue(this->desiredValue + this->smallChange, true);
        }
    }

    this->mouseDownIndex = -1;
    update();
}

void Scrollbar::leaveEvent(QEvent *)
{
    this->mouseOverIndex = -1;

    update();
}

void Scrollbar::updateScroll()
{
    this->trackHeight = height() - this->buttonHeight - this->buttonHeight - MIN_THUMB_HEIGHT - 1;

    this->thumbRect = QRect(
        0, (int)(this->currentValue / this->maximum * this->trackHeight) + 1 + this->buttonHeight,
        width(), (int)(this->largeChange / this->maximum * this->trackHeight) + MIN_THUMB_HEIGHT);

    update();
}

}  // namespace widgets
}  // namespace chatterino
