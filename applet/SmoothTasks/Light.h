/***********************************************************************************
* Smooth Tasks
* Copyright (C) 2009 Marcin Baszczewski <marcin.baszczewski@gmail.com>
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
#ifndef SMOOTHTASKSLIGHT_H
#define SMOOTHTASKSLIGHT_H

// Qt
#include <QObject>
#include <QPixmap>
#include <QIcon>

class QRadialGradient;
class QTimer;
class QStyleOptionGraphicsItem;

namespace SmoothTasks {

class TaskItem;

class Light : public QObject {
	Q_OBJECT

public:
	enum AnimationType {
		NoAnimation,
		StartupAnimation,
		AttentionAnimation
	};

	Light(TaskItem *item);
	~Light();

	void paint(QPainter *p, const QRectF& geometry, const QPointF& mousePos, bool mouseIn, const bool isRotated);

public slots:
	void startAnimation(AnimationType animation, int duration = 300, bool repeater = true);
	void stopAnimation();
	void repeatAnimation();

private slots:
	void animation(qreal progress);

private:
	TaskItem     *m_item;
	int           m_count;
	int           m_currentAnimationDuration;
	int           m_animation;
	qreal         m_progress;
	bool          m_move;
	AnimationType m_currentAnimation;
	QTimer       *m_animationRepeater;
	bool          m_repeater;

signals:
	void update();
};

} // namespace SmoothTasks
#endif
