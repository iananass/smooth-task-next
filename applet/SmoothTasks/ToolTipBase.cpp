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
#include "SmoothTasks/ToolTipBase.h"

#include <QEvent>

namespace SmoothTasks {

void ToolTipBase::hide() {
	if (!m_hoverItem.isNull() && !m_hoverItem->isMouseIn()) {
		m_hoverItem->confirmLeave();
		m_hoverItem = NULL;
	}
	m_shown = false;
}

void ToolTipBase::quickShow(TaskItem *item) {
	if (item != m_hoverItem && !m_hoverItem.isNull()) {
		m_hoverItem->confirmLeave();
	}

	m_hoverItem = item;
	m_shown     = true;
}

void ToolTipBase::itemEnter(TaskItem* item) {
	item->confirmEnter();
	quickShow(item);
}

void ToolTipBase::itemLeave(TaskItem* item) {
	if (m_hoverItem == item) {
		hide();
	}
}

void ToolTipBase::itemDelete(TaskItem* item) {
	if (m_hoverItem == item) {
		hide();
	}
}

void ToolTipBase::registerItem(TaskItem *item) {
	item->installEventFilter(this);
}

void ToolTipBase::unregisterItem(TaskItem *item) {
	if (m_hoverItem == item) {
		m_hoverItem = NULL;
		hide();
	}

	item->removeEventFilter(this);
}

bool ToolTipBase::eventFilter(QObject *obj, QEvent *event) {
	TaskItem *item = qobject_cast<TaskItem*>(obj);
	
	if (item) {
		switch (event->type()) {
			case QEvent::GraphicsSceneResize:
			case QEvent::GraphicsSceneMove:
			case QEvent::Resize:
			case QEvent::Move:
				if (item == m_hoverItem) {
					moveBesideTaskItem(false);
				}
				break;
			case QEvent::GraphicsSceneHoverEnter:
			case QEvent::GraphicsSceneDragEnter:
			case QEvent::Enter:
				itemEnter(item);
				break;
			case QEvent::GraphicsSceneHoverLeave:
			case QEvent::GraphicsSceneDragLeave:
			case QEvent::Leave:
				itemLeave(item);
				break;
			case QEvent::Hide:
				if (item == m_hoverItem) {
					hide();
				}
				break;
				/* XXX: why does this not work
			case QEvent::DeferredDelete:
			case QEvent::Destroy:
				itemDelete(item);
				*/
			default:
				break;
		}
	}
	
	return QObject::eventFilter(obj, event);
}

} // namespace SmoothTasks
