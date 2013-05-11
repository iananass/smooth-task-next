/***********************************************************************************
* Smooth Tasks
* Copyright (C) 2009 Mathias Panzenböck <grosser.meister.morti@gmx.net>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*
***********************************************************************************/
#include <QStyle>
#include <QEvent>
#include <QSizePolicy>
#include <QPainter>

#include <Plasma/PaintUtils>
#include <Plasma/Animator>

#include <cmath>

#include "SmoothTasks/FadedText.h"
#include "SmoothTasks/Global.h"

namespace SmoothTasks {

const int FadedText::SCROLL_DURATION        =   25;
const int FadedText::SCROLL_STOP_DURATION   =    4;
const int FadedText::SCROLL_WAIT_AT_ENDS    = 1000;
const int FadedText::SCROLL_WAIT_AFTER_DRAG = 2000;

void FadedText::init() {
	m_fps             = 35;
	m_scrollAnimation = 0;
	m_scrollState     = NoScroll;
	m_fadeWidth       = 30;
	m_animLeft        = 0;
	m_scrollDistance  = 0;
	m_scrollOffset    = 0;
	m_delayTimer      = NULL;
	m_dragState       = NoDrag;
	m_mouseX          = 0;

	m_textOption.setWrapMode(QTextOption::NoWrap);
	setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
	
	connect(
		Plasma::Animator::self(), SIGNAL(customAnimationFinished(int)),
		this, SLOT(animationFinished(int)));
	
	updateText();
}

FadedText::~FadedText() {
	if (m_scrollAnimation) {
		Plasma::Animator::self()->stopCustomAnimation(m_scrollAnimation);
		m_scrollAnimation = 0;
		m_scrollState     = NoScroll;
	}
}

void FadedText::paintEvent(QPaintEvent* event) {
	QWidget::paintEvent(event);

	if (width() >= 1 && height() >= 1) {
		QPainter painter(this);
		QTextLayout layout;

		QSizeF textSize(layoutText(layout));
		drawTextLayout(painter, layout, textSize);
	}
}

QSizeF FadedText::layoutText(QTextLayout &layout) const {
	layout.setFont(font());
	layout.setText(m_text);
	layout.setTextOption(m_textOption);
	return ::SmoothTasks::layoutText(layout, maximumSize());
}

QPointF FadedText::visualPos(Qt::LayoutDirection direction, const QRect& boundingRect, const QPointF& logicalPos) {
	if (direction == Qt::LeftToRight) {
		return logicalPos;
	}
	return QPointF(-(boundingRect.right() - logicalPos.x()), logicalPos.y());
}

void FadedText::drawTextLayout(QPainter& painter, const QTextLayout& layout, const QSizeF& textSize) {
	const bool rtl = layout.textOption().textDirection() == Qt::RightToLeft;
	QPixmap pixmap(size());
	pixmap.fill(Qt::transparent);
	
	QPainter p(&pixmap);
	p.setPen(painter.pen());

	// Create the alpha gradient for the fade out effect
	QLinearGradient beginAlphaGradient(0, 0, 1, 0);
	QLinearGradient endAlphaGradient(0, 0, 1, 0);
	
	beginAlphaGradient.setCoordinateMode(QGradient::ObjectBoundingMode);
	endAlphaGradient.setCoordinateMode(QGradient::ObjectBoundingMode);

	endAlphaGradient.setColorAt(0, QColor(0, 0, 0, 255));
	endAlphaGradient.setColorAt(1, QColor(0, 0, 0, 0));
		
	beginAlphaGradient.setColorAt(0, QColor(0, 0, 0, 0));
	beginAlphaGradient.setColorAt(1, QColor(0, 0, 0, 255));

	QFontMetrics fm(layout.font());
	QPointF position(m_animLeft, (height() - textSize.height()) * 0.5 +
		(fm.tightBoundingRect(M).height() - fm.xHeight()) * 0.5);

	QList<QRect> fadeBeginRects;
	QList<QRect> fadeEndRects;
	
	// Draw each line in the layout
	for (int i = 0; i < layout.lineCount(); ++ i) {
		QTextLine line(layout.lineAt(i));
		QPointF linePos(position);
		qreal textWidth = line.naturalTextWidth();

		if (rtl) {
			linePos.setX(m_animLeft + textSize.width() - textWidth);
		}

		line.draw(&p, linePos);
		// Add a fade out rect to the list if the line is too long
		int y = int(line.position().y() + linePos.y());
		qreal lineLeft  = linePos.x();
		qreal lineRight = lineLeft + textWidth;
		if (lineRight > width()) {
			int dist = lineRight - width();
			int fadeWidth = dist < m_fadeWidth ? dist : m_fadeWidth;
			int x = pixmap.width() - fadeWidth;
			fadeEndRects.append(QRect(x, y, fadeWidth, int(line.height())));
		}
		if (lineLeft < 0) {
			int dist = -lineLeft;
			int fadeWidth = dist < m_fadeWidth ? dist : m_fadeWidth;
			fadeBeginRects.append(QRect(0, y, fadeWidth, int(line.height())));
		}
	}
	// Reduce the alpha in each fade out rect using the alpha gradient
	if (!fadeBeginRects.isEmpty()) {
		p.setCompositionMode(QPainter::CompositionMode_DestinationIn);
		foreach (const QRect &rect, fadeBeginRects) {
			p.fillRect(rect, beginAlphaGradient);
		}
	}
	if (!fadeEndRects.isEmpty()) {
		if (fadeBeginRects.isEmpty()) {
			p.setCompositionMode(QPainter::CompositionMode_DestinationIn);
		}
		foreach (const QRect &rect, fadeEndRects) {
			p.fillRect(rect, endAlphaGradient);
		}
	}
	p.end();
	QColor shadowColor;
	if (painter.pen().color().value() < 128) {
		shadowColor = Qt::white;
	} 
	else {
		shadowColor = Qt::black;
	}
	
	if (m_shadow) {
		QImage shadow(pixmap.toImage());
		Plasma::PaintUtils::shadowBlur(shadow, 2, shadowColor);
		painter.drawImage(1, 2, shadow);
	}
	
	painter.drawPixmap(0, 0, pixmap);
}

void FadedText::setText(const QString& text) {
	if (m_text != text) {
		m_text = text;
		updateText();
	}
}

void FadedText::resizeEvent(QResizeEvent *event) {
	Q_UNUSED(event);
	updateText();
}

void FadedText::changeEvent(QEvent *event) {
	QWidget::changeEvent(event);
	if (event->type() == QEvent::FontChange) {
		updateText();
	}
	event->ignore();
}

void FadedText::mousePressEvent(QMouseEvent *event) {
	if (m_sizeHint.width() > width()) {
		stopTimer();
		if (m_scrollAnimation) {
			Plasma::Animator::self()->stopCustomAnimation(m_scrollAnimation);
			m_scrollAnimation = 0;
			m_scrollState     = WaitLeftScroll;
		}
		m_mouseX       = event->globalX();
		m_scrollOffset = m_animLeft;
		m_dragState    = BegunDrag;
	}
	event->ignore();
}

void FadedText::mouseMoveEvent(QMouseEvent *event) {
	int dx = event->globalX() - m_mouseX;

	if (m_dragState == BegunDrag && dx == 0) {
		event->ignore();
	}
	else if (m_dragState == BegunDrag || m_dragState == ConfirmedDrag) {
		int x = m_scrollOffset + dx;
		m_dragState = ConfirmedDrag;

		if (x > 0) {
			x = 0;
		}
		else if (x < width() - m_sizeHint.width()) {
			x = width() - m_sizeHint.width();
		}

		if (x != m_animLeft) {
			m_animLeft = x;
			update();
		}
		
		event->accept();
	}
	else {
		event->ignore();
	}
}

void FadedText::mouseReleaseEvent(QMouseEvent *event) {
	if (m_dragState == ConfirmedDrag) {
		if (event->x() < 0 || event->y() < 0 || event->x() >= width() || event->y() >= height()) {
			stopScrollAnimation();
		}
		else {
			singleShot(SCROLL_WAIT_AFTER_DRAG, SLOT(startScrollAnimation()));
		}
		event->accept();
	}
	else {
		event->ignore();
	}

	m_dragState = NoDrag;
}

void FadedText::setTextOption(const QTextOption& textOption) {
	m_textOption = textOption;
	updateText();
}

void FadedText::setWrapMode(QTextOption::WrapMode wrapMode) {
	if (m_textOption.wrapMode() != wrapMode) {
		m_textOption.setWrapMode(wrapMode);
		updateText();
	}
}

void FadedText::setAlignment(Qt::Alignment alignment) {
	if (m_textOption.alignment() != alignment) {
		m_textOption.setAlignment(alignment);
		updateText();
	}
}

void FadedText::setFadeWidth(int fadeWidth) {
	if (fadeWidth < 0) {
		qDebug("FadedText::setFadeWidth: illegal fadeWidth: %d", fadeWidth);
		return;
	}
	
	if (m_fadeWidth != fadeWidth && fadeWidth >= 0) {
		m_fadeWidth = fadeWidth;
		update();
	}
}

void FadedText::setShadow(bool shadow) {
	if (m_shadow != shadow) {
		m_shadow = shadow;
		update();
	}
}

void FadedText::updateText() {
	QTextLayout layout;
	QSizeF realTextSize(layoutText(layout));
	QSize  textSize(std::ceil(realTextSize.width()), std::ceil(realTextSize.height()));
	if (m_sizeHint != textSize) {
		m_sizeHint = textSize;
		updateGeometry();
	}

	if (m_textOption.textDirection() == Qt::RightToLeft) {
		if (m_scrollAnimation) {
			// TODO?
		}
		else {
			m_animLeft = width() - textSize.width();
		}
	}
}

void FadedText::startScrollAnimation() {
	if (m_sizeHint.width() > width()) {
		if (m_scrollAnimation) {
			Plasma::Animator::self()->stopCustomAnimation(m_scrollAnimation);
		}

		switch (m_scrollState) {
		case NoScroll:
		case LeftScroll:
		case WaitLeftScroll: // XXX?
		case StopLeftScroll: // XXX?
			if (m_textOption.textDirection() == Qt::RightToLeft) {
				startScrollAnimation(RightScroll, 0, scrollDistance(), SCROLL_DURATION);
			}
			else {
				startScrollAnimation(LeftScroll, m_animLeft, scrollDistance() + m_animLeft, SCROLL_DURATION);
			}
			break;
		default:
			if (m_textOption.textDirection() == Qt::RightToLeft) {
				startScrollAnimation(LeftScroll, 0, scrollDistance(), SCROLL_DURATION);
			}
			else {
				startScrollAnimation(RightScroll, m_animLeft, scrollDistance() - m_animLeft, SCROLL_DURATION);
			}
		}
	}
}

void FadedText::stopScrollAnimation() {
	const bool rtl = m_textOption.textDirection() == Qt::RightToLeft;

	if (m_scrollAnimation) {
		Plasma::Animator::self()->stopCustomAnimation(m_scrollAnimation);
	}

	if (rtl) {
		startScrollAnimation(StopRightScroll, m_animLeft, scrollDistance() + m_animLeft, SCROLL_STOP_DURATION);
	}
	else {
		startScrollAnimation(StopLeftScroll, 0, -m_animLeft, SCROLL_STOP_DURATION);
	}
}

void FadedText::enterEvent(QEvent *event) {
	Q_UNUSED(event);
	
	startScrollAnimation();
}

void FadedText::leaveEvent(QEvent *event) {
	Q_UNUSED(event);

	stopScrollAnimation();
}

void FadedText::startScrollAnimation(ScrollState scrollState, int offset, int distance, int durationScale) {
	/*
	if (offset + m_sizeHint.width() == width()) {
		scrollState = RightScroll;
		offset      = 0;
		distance    = scrollDistance();
	}
	*/

	const int duration = distance * durationScale;
	const int frames   = m_fps * duration / 1000;

	m_animLeft        = 0;
	m_scrollOffset    = offset;
	m_scrollDistance  = distance;
	m_scrollState     = scrollState;

	stopTimer();
	if (frames <= 0) {
		m_scrollAnimation = 0;
		animateScroll(1.0);
		animationFinished(m_scrollAnimation);
	}
	else {
		m_scrollAnimation = Plasma::Animator::self()->customAnimation(
			frames, duration,
			Plasma::Animator::LinearCurve,
			this, "animateScroll");
	}
}

void FadedText::startLeftScroll() {
	if (m_scrollState == WaitRightScroll) {
		startScrollAnimation(LeftScroll, 0, scrollDistance(), SCROLL_DURATION);
	}
}

void FadedText::startRightScroll() {
	if (m_scrollState == WaitLeftScroll) {
		startScrollAnimation(RightScroll, 0, scrollDistance(), SCROLL_DURATION);
	}
}

void FadedText::animationFinished(int animation) {
	if (animation == m_scrollAnimation) {
		switch (m_scrollState) {
		case LeftScroll:
			m_scrollState = WaitLeftScroll;
			singleShot(SCROLL_WAIT_AT_ENDS, SLOT(startRightScroll()));
			break;
		case RightScroll:
			m_scrollState = WaitRightScroll;
			singleShot(SCROLL_WAIT_AT_ENDS, SLOT(startLeftScroll()));
			break;
		case StopRightScroll:
		case StopLeftScroll:
			m_scrollState = NoScroll;
			delete m_delayTimer;
			m_delayTimer = NULL;
		default:
			break;
		}
		m_scrollAnimation = 0;
	}
}

void FadedText::animateScroll(qreal progress) {
	switch (m_scrollState) {
		case StopRightScroll:
		case LeftScroll:
			m_animLeft = m_scrollOffset - m_scrollDistance * progress;
			update();
			break;
		case StopLeftScroll:
		case RightScroll:
			m_animLeft = m_scrollOffset - m_scrollDistance * (1.0 - progress);
			update();
		default:
			break;
	}
}

void FadedText::singleShot(int ms, const char *slot) {
	if (m_delayTimer) {
		m_delayTimer->stop();
		disconnect(m_delayTimer, SIGNAL(timeout()), this, NULL);
	}
	else {
		m_delayTimer = new QTimer(this);
		m_delayTimer->setSingleShot(true);
	}

	connect(m_delayTimer, SIGNAL(timeout()), this, slot);
	m_delayTimer->start(ms);
}

void FadedText::stopTimer() {
	if (m_delayTimer) {
		m_delayTimer->stop();
		disconnect(m_delayTimer, SIGNAL(timeout()), this, NULL);
	}
}

} // namespace SmoothTasks
