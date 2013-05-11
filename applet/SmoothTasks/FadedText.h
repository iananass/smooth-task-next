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
#ifndef SMOOTHTASKS_FADETEXT_H
#define SMOOTHTASKS_FADETEXT_H

#include <QWidget>
#include <QTimer>
#include <QTextOption>
#include <QTextLayout>

namespace SmoothTasks {

class FadedText : public QWidget {
	Q_OBJECT
	
	Q_PROPERTY(QString text READ text WRITE setText)
	Q_PROPERTY(QTextOption textOption READ textOption WRITE setTextOption)
	Q_PROPERTY(int fadeWidth READ fadeWidth WRITE setFadeWidth)
	Q_PROPERTY(bool shadow READ shadow WRITE setShadow)
//	Q_PROPERTY(QTextOption::WrapMode wrapMode READ wrapMode WRITE setWrapMode)
	Q_PROPERTY(Qt::Alignment alignment READ alignment WRITE setAlignment)
	Q_PROPERTY(int fps READ fps WRITE setFps)

public:

	const static int SCROLL_DURATION;
	const static int SCROLL_STOP_DURATION;
	const static int SCROLL_WAIT_AT_ENDS;
	const static int SCROLL_WAIT_AFTER_DRAG;

	FadedText(QWidget *parent = NULL, Qt::WindowFlags flags = 0)
		: QWidget(parent, flags),
		  m_text(),
		  m_textOption() { init(); }

	FadedText(const QString& text, QWidget *parent = NULL, Qt::WindowFlags flags = 0)
		: QWidget(parent, flags),
		  m_text(text),
		  m_textOption() { init(); }
	
	~FadedText();
	
	QSize       sizeHint()   const { return m_sizeHint; }
	int         fps()        const { return m_fps; }
	QString     text()       const { return m_text; }
	QTextOption textOption() const { return m_textOption; }
	int         fadeWidth()  const { return m_fadeWidth; }
	bool        shadow()     const { return m_shadow; }
	QTextOption::WrapMode wrapMode()  const { return m_textOption.wrapMode(); }
	Qt::Alignment         alignment() const { return m_textOption.alignment(); }

protected:
	void   paintEvent(QPaintEvent * event);
	QSizeF layoutText(QTextLayout& layout) const;
	void   drawTextLayout(QPainter& painter, const QTextLayout& layout, const QSizeF& textSize);
	void   enterEvent(QEvent *event);
	void   leaveEvent(QEvent *event);
	void   resizeEvent(QResizeEvent *event);
	void   changeEvent(QEvent *event);
	void   mouseMoveEvent(QMouseEvent *event);
	void   mousePressEvent(QMouseEvent *event);
	void   mouseReleaseEvent(QMouseEvent *event);

public slots:
	void startScrollAnimation();
	void stopScrollAnimation();
	void setFps(int fps) { m_fps = fps; }
	void setText(const QString& text);
	void setTextOption(const QTextOption& textOption);
	void setFadeWidth(int fadeWidth);
	void setShadow(bool shadow);
	void setWrapMode(QTextOption::WrapMode wrapMode);
	void setAlignment(Qt::Alignment alignment);

private slots:
	void animateScroll(qreal progress);
	void animationFinished(int animation);
	void startLeftScroll();
	void startRightScroll();

private:

	enum ScrollState {
		NoScroll,
		RightScroll,
		LeftScroll,
		WaitRightScroll,
		WaitLeftScroll,
		StopRightScroll,
		StopLeftScroll
	};

	enum DragState {
		NoDrag,
		BegunDrag,
		ConfirmedDrag
	};

	void init();
	void updateText();
	int  scrollDistance() const { return m_sizeHint.width() - width(); }
	void startScrollAnimation(ScrollState scrollState, int offset, int distance, int durationScale);
	void singleShot(int ms, const char *slot);
	void stopTimer();

	static QPointF visualPos(Qt::LayoutDirection layoutDirection, const QRect& bounds, const QPointF& pos);

	QString     m_text;
	QSize       m_sizeHint;
	QTextOption m_textOption;
	int         m_fadeWidth;
	bool        m_shadow;

	// animation:
	int         m_fps;
	int         m_scrollAnimation;
	qreal       m_animLeft;
	ScrollState m_scrollState;
	int         m_scrollDistance;
	int         m_scrollOffset;
	QTimer     *m_delayTimer;

	// draging text:
	DragState   m_dragState;
	int         m_mouseX;
};

} // namespace SmoothTasks
#endif
