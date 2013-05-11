/***********************************************************************************
* Smooth Tasks
* Copyright (C) 2009 Marcin Baszczewski <marcin.baszczewski@gmail.com>
*                    Mathias panzenböck <grosser.meister.morti@gmx.net>
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

// Smooth Tasks
#include "SmoothTasks/Light.h"
#include "SmoothTasks/Applet.h"
#include "SmoothTasks/TaskItem.h"
#include "SmoothTasks/TaskIcon.h"

// Qt
#include <QTimer>
#include <QPainter>
#include <QStyleOptionGraphicsItem>

// Plasma
#include <Plasma/Applet>
#include <Plasma/PaintUtils>

// KDE
#include <KIcon>
#include <KIconEffect>
#include <KIconLoader>

// Other
#include <cmath>

namespace SmoothTasks {

Light::Light(TaskItem *item) : QObject(item),
	m_item(item),
	m_count(0),
	m_currentAnimationDuration(0),
	m_animation(0),
	m_progress(0.0),
	m_move(true),
	m_currentAnimation(NoAnimation),
	m_animationRepeater(new QTimer()),
	m_repeater(true) {
}

Light::~Light() {
	if (m_animation) {
		Plasma::Animator::self()->stopCustomAnimation(m_animation);
	}
	m_animationRepeater->deleteLater();
}

void Light::paint(QPainter *p, const QRectF& boundingRect, const QPointF& mousePos, bool mouseIn, const bool isRotated) {
	if (!mouseIn && !m_animation) {
		return;
	}

	QRectF drawRect(boundingRect);

	// XXX: ugly hack because I don't know the real contents area of the FrameSvg
	drawRect.adjust(-4, -4, +4, +4);
	
	qreal  width  = drawRect.width();
	qreal  height = drawRect.height();
	qreal  size   = 0.5;
	qreal  x;
	qreal  y;
	QColor lightColor(m_item->icon()->highlightColor());

	switch (m_currentAnimation) {
	case StartupAnimation:
	case AttentionAnimation:
		x = drawRect.left() + width  * 0.5;
		y = drawRect.top()  + height * 0.5;
		size = size * 2.0 *
			(m_progress < 0.5 ? (m_progress * 0.5 + 0.5) : (1 - m_progress / 2));
		break;
	case NoAnimation:
		if (isRotated) {
			x = m_item->size().height() - mousePos.y();
			y = mousePos.x();
		}
		else {
			x = mousePos.x();
			y = mousePos.y();
		}
		// y = drawRect.top() + height * 1.10;
		width  *= 2.0;
		height *= 2.0;
		break;
	default:
		return;
	}
	
	QRadialGradient gradient(0.5, 0.5, size);
	gradient.setCoordinateMode(QRadialGradient::ObjectBoundingMode);
	
	lightColor.setAlpha(200);
	gradient.setColorAt(0.0, lightColor);
	
	lightColor.setAlpha(60);
	gradient.setColorAt(0.6, lightColor);
	
	lightColor.setAlpha(0);
	gradient.setColorAt(1.0, lightColor);
	
	p->setClipRect(drawRect);
	p->fillRect(
		x - width  * 0.5,
		y - height * 0.5,
		width, height,
		gradient);
	p->setClipping(false);
}

void Light::startAnimation(AnimationType animation, int duration, bool repeater) {
	m_count = 0;
	m_currentAnimationDuration = duration;
	m_currentAnimation = animation;

	m_animationRepeater->setInterval(duration);
	m_animationRepeater->start();

	repeatAnimation();

	m_repeater = repeater;
	connect(m_animationRepeater, SIGNAL(timeout()), this, SLOT(repeatAnimation()));
}

void Light::stopAnimation() {
	m_animationRepeater->stop();
	m_currentAnimation = NoAnimation;
	Plasma::Animator::self()->stopCustomAnimation(m_animation);
	m_animation = 0;
}

void Light::repeatAnimation() {
	if (!m_repeater && m_count != 0) {
		return;
	}

	if (m_animation) {
		Plasma::Animator::self()->stopCustomAnimation(m_animation);
		m_animation = 0;
	}
	int frames = m_item->applet()->fps() * m_currentAnimationDuration / 1000;

	if (frames <= 0) {
		animation(1.0);
	}
	else {
		m_animation = Plasma::Animator::self()->customAnimation(
			frames, m_currentAnimationDuration,
			Plasma::Animator::LinearCurve,
			this, "animation");
	}
	++ m_count;
}

void Light::animation(qreal progress) {
	m_progress = progress;
	emit update();
}

} // namespace SmoothTasks
#include "Light.moc"
