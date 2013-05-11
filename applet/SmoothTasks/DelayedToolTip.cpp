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
#include "SmoothTasks/DelayedToolTip.h"
#include "SmoothTasks/Applet.h"

namespace SmoothTasks {

DelayedToolTip::DelayedToolTip(Applet *applet)
	: ToolTipBase(applet),
	  m_delayTimer(new QTimer(this)),
	  m_action(NoAction),
	  m_newHoverItem(NULL) {
	m_delayTimer->setSingleShot(true);
	
	connect(
		m_delayTimer, SIGNAL(timeout()),
		this, SLOT(timeout()));
}
	
void DelayedToolTip::quickShow(TaskItem *item) {
	ToolTipBase::quickShow(item);
	
	m_action       = NoAction;
	m_newHoverItem = NULL;
	
	if (m_delayTimer->isActive()) {
		m_delayTimer->stop();
	}
	
	showAction(false);
}

void DelayedToolTip::hide() {
	m_action = NoAction;
	
	if (m_delayTimer->isActive()) {
		m_delayTimer->stop();
	}
	
	ToolTipBase::hide();
	
	m_newHoverItem = NULL;
}

void DelayedToolTip::itemEnter(TaskItem *item) {
	if (m_newHoverItem == item && ((m_delayTimer->isActive() && m_action == ShowAction) || m_action == NoAction)) {
		// do nothing when already delaying the hover or already hovering of the given item
		return;
	}
	
	if (m_delayTimer->isActive()) {
		m_delayTimer->stop();
	}
	
	int duration;
	if (m_shown) {
		duration = 150;
	}
	else {
		duration = m_applet->animationDuration();
	}
	
	m_action       = ShowAction;
	m_newHoverItem = item;
	m_delayTimer->start(duration);
}

void DelayedToolTip::itemLeave(TaskItem *item) {
	if (m_hoverItem == item || m_newHoverItem == item) {
		if (m_delayTimer->isActive()) {
			m_delayTimer->stop();
			m_action = NoAction;
		}
		
		if (m_shown) {
			m_action = HideAction;
			m_delayTimer->start(m_applet->animationDuration());
		}
		else if (!m_hoverItem.isNull()) {
			m_hoverItem->confirmLeave();
		}
	}
	
	if (m_newHoverItem == item) {
		m_newHoverItem = NULL;
		
		if (m_action == ShowAction) {
			m_action = NoAction;
			m_delayTimer->stop();
		}
	}
}

void DelayedToolTip::itemDelete(TaskItem *item) {
	if (m_hoverItem == item) {
		if (m_delayTimer->isActive()) {
			m_delayTimer->stop();
			m_action = NoAction;
		}
		
		hide();
	}
}

void DelayedToolTip::timeout() {
	switch (m_action) {
		case ShowAction:
			if (!m_newHoverItem.isNull() && !(m_shown && m_hoverItem == m_newHoverItem)) {
				bool wasShown = m_shown;
				if (!m_hoverItem.isNull()) {
					m_hoverItem->confirmLeave();
				}
				m_hoverItem = m_newHoverItem;
				m_shown     = true;
				m_hoverItem->confirmEnter();
				showAction(wasShown);
			}
			break;
		case HideAction:
			hideAction();
		default:
			break;
	}
	m_action = NoAction;
}

} // namespace SmoothTasks

