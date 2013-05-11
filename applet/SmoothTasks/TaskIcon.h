/***********************************************************************************
* Smooth Tasks
* Copyright (C) 2009 Marcin Baszczewski <marcin.baszczewski@gmail.com>
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
#ifndef SMOOTHTASKS_TASKICON_H
#define SMOOTHTASKS_TASKICON_H

// Qt
#include <QObject>
#include <QPixmap>
#include <QIcon>

class QTimer;
class QStyleOptionGraphicsItem;

namespace SmoothTasks {

class TaskItem;

class TaskIcon : public QObject {
	Q_OBJECT

public:
	TaskIcon(TaskItem *item);
	~TaskIcon();

	void paint(QPainter *p, qreal hover, bool isGroup);
	void setRect(const QRectF& geometry);
	QRgb highlightColor() const;
	qreal size() const;
	QPointF pos() const { return m_pos; }

public slots:
	void setIcon(const QIcon& icon);
	void startStartupAnimation(int duration = 300);
	void stopStartupAnimation();

private slots:
	void animation(qreal progress);
	void repeatAnimation();

private:
	QRgb averageColor() const;
	QRgb meanColor() const;
	QRgb dominantColor() const;
	
	TaskItem *m_item;
	QIcon     m_icon;
	QRgb      m_highlightColor;
	QRectF    m_rect;
	QPixmap   m_pixmap;
	QTimer   *m_animationRepeater;
	int       m_currentAnimationDuration;
	int       m_animation;
	qreal     m_progress;
	QPointF   m_pos;

	void updatePos();
	void animationHover(qreal hover);
	void animationStartup(qreal progress);

signals:
	void update();
};

} // namespace SmoothTasks
#endif
