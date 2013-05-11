#include "SmoothTasks/TaskStateAnimation.h"

#include <Plasma/Animator>

namespace SmoothTasks {

TaskStateAnimation::TaskStateAnimation()
	: QObject(),
	  m_animation(0),
	  m_fromState(Normal),
	  m_toState(Normal),
	  m_hover(0.0),
	  m_minimized(0.0),
	  m_attention(0.0),
	  m_focus(0.0),
	  m_lastProgress(0.0) {
	connect(
		Plasma::Animator::self(), SIGNAL(customAnimationFinished(int)),
		this, SLOT(animationFinished(int)));
}

void TaskStateAnimation::stop() {
	if (m_animation) {
		Plasma::Animator::self()->stopCustomAnimation(m_animation);
		m_animation = 0;
	}
}

void TaskStateAnimation::start(int fps, int duration) {
	int frames = fps * duration / 1000;
	m_lastProgress = 0.0;

	if (frames <= 0) {
		animate(1.0);
		animationFinished(m_animation);
	}
	else {
		m_animation = Plasma::Animator::self()->customAnimation(
			frames, duration,
			Plasma::Animator::LinearCurve,
			this, "animate");
	}
}

void TaskStateAnimation::setState(int newState, int fps, int duration) {
	if (m_toState == newState) {
		return;
	}

	// from  to newTo newFrom
	//   0    0    0    0     
	//   0    0    1    0     
	//   0    1    0    1     (m_fromState ^ m_toState) & ~newState
	//   0    1    1    0     (m_fromState ^ m_toState) & ~newState
	//   1    0    0    1     (m_fromState ^ m_toState) & ~newState
	//   1    0    1    0     (m_fromState ^ m_toState) & ~newState
	//   1    1    0    1     (m_fromState & m_toState)
	//   1    1    1    1     (m_fromState & m_toState)

	stop();
	m_fromState = ((m_fromState ^ m_toState) & ~newState) | (m_fromState & m_toState);
	m_toState   = newState;
	start(fps, duration);
}

void TaskStateAnimation::animate(qreal progress) {
	qreal delta = progress - m_lastProgress;
	m_lastProgress = progress;
	int animatedState = m_fromState ^ m_toState;

	if (animatedState & Hover) {
		if (m_toState & Hover) {
			m_hover += delta;
			if (m_hover >= 1.0) {
				m_hover = 1.0;
				m_fromState |= Hover;
			}
		}
		else {
			m_hover -= delta;
			if (m_hover <= 0.0) {
				m_hover = 0.0;
				m_fromState &= ~Hover;
			}
		}
	}
	
	if (animatedState & Minimized) {
		if (m_toState & Minimized) {
			m_minimized += delta;
			if (m_minimized >= 1.0) {
				m_minimized = 1.0;
				m_fromState |= Minimized;
			}
		}
		else {
			m_minimized -= delta;
			if (m_minimized <= 0.0) {
				m_minimized = 0.0;
				m_fromState &= ~Minimized;
			}
		}
	}
	
	if (animatedState & Attention) {
		if (m_toState & Attention) {
			m_attention += delta;
			if (m_attention >= 1.0) {
				m_attention = 1.0;
				m_fromState |= Attention;
			}
		}
		else {
			m_attention -= delta;
			if (m_attention <= 0.0) {
				m_attention = 0.0;
				m_fromState &= ~Attention;
			}
		}
	}
	
	if (animatedState & Focus) {
		if (m_toState & Focus) {
			m_focus += delta;
			if (m_focus >= 1.0) {
				m_focus = 1.0;
				m_fromState |= Focus;
			}
		}
		else {
			m_focus -= delta;
			if (m_focus <= 0.0) {
				m_focus = 0.0;
				m_fromState &= ~Focus;
			}
		}
	}

	emit update();
}

void TaskStateAnimation::animationFinished(int animation) {
	if (m_animation == animation) {
		m_fromState = m_toState;
		m_animation = 0;
		m_hover     = m_toState & Hover     ? 1.0 : 0.0;
		m_minimized = m_toState & Minimized ? 1.0 : 0.0;
		m_attention = m_toState & Attention ? 1.0 : 0.0;
		m_focus     = m_toState & Focus     ? 1.0 : 0.0;
	}
}

} // namespace SmoothTasks
