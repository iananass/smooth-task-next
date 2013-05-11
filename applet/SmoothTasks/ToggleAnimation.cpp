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

#include "SmoothTasks/ToggleAnimation.h"

#include <Plasma/Animator>

namespace SmoothTasks {

void ToggleAnimation::init() {
	connect(
		Plasma::Animator::self(), SIGNAL(customAnimationFinished(int)),
		this, SLOT(finished(int)));
}

void ToggleAnimation::stop() {
	if (m_animation) {
		Plasma::Animator::self()->stopCustomAnimation(m_animation);
		m_animation = 0;
	}
}

void ToggleAnimation::start(Direction direction, int fps, int duration) {
	if (direction == Up) {
		startUp(fps, duration);
	}
	else {
		startDown(fps, duration);
	}
}

void ToggleAnimation::startUp(int fps, int duration) {
	if (m_direction == Up && m_animation) {
		return;
	}
	m_direction = Up;
	start(fps, duration * (qreal(1.0) - m_value), "animateUp");
}

void ToggleAnimation::startDown(int fps, int duration) {
	if (m_direction == Down && m_animation) {
		return;
	}
	m_direction = Down;
	start(fps, duration * m_value, "animateDown");
}

void ToggleAnimation::start(int fps, int duration, const char *animationSlot) {
	int frames = fps * duration / 1000;
	stop();

	m_finished  = false;
	m_oldValue  = m_value;

	if (frames <= 0) {
		m_animation = 0;
		m_value     = m_direction == Up ? 1.0 : 0.0;
		emit animate(m_value);
		m_finished  = true;

		emit finished(m_value);
	}
	else {
		m_animation = Plasma::Animator::self()->customAnimation(
			frames, duration,
			Plasma::Animator::LinearCurve,
			this, animationSlot);
	}
}

void ToggleAnimation::animateUp(qreal progress) {
	m_value = m_oldValue + (qreal(1.0) - m_oldValue) * progress;
	emit animate(m_value);
}

void ToggleAnimation::animateDown(qreal progress) {
	m_value = m_oldValue - m_oldValue * progress;
	emit animate(m_value);
}

void ToggleAnimation::finished(int animation) {
	if (m_animation == animation) {
		m_animation = 0;
		m_value     = m_direction == Up ? 1.0 : 0.0;
		m_finished  = true;

		emit finished(m_value);
	}
}

} // namespace SmoothTasks
