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
#include <Plasma/ToolTipManager>

#include <KWindowSystem>

#include <taskmanager/taskmanager.h>
#include <taskmanager/abstractgroupableitem.h>
#include <taskmanager/groupmanager.h>
#include <taskmanager/taskitem.h>
#include <taskmanager/startup.h>

#include "SmoothTasks/Applet.h"
#include "SmoothTasks/Task.h"
#include "SmoothTasks/TaskItem.h"
#include "SmoothTasks/PlasmaToolTip.h"

namespace SmoothTasks {

void PlasmaToolTip::unregisterItem(TaskItem *item) {
	Plasma::ToolTipManager::self()->unregisterWidget(item);
	DelayedToolTip::unregisterItem(item);
}

void PlasmaToolTip::showAction(bool animate) {
	Q_UNUSED(animate);
	
//	qDebug("connect windowPreviewActivated");
	connect(Plasma::ToolTipManager::self(),
		SIGNAL(windowPreviewActivated(WId,Qt::MouseButtons,Qt::KeyboardModifiers,QPoint)),
		this, SLOT(activateWindow(WId,Qt::MouseButtons))); 
		
	updateToolTip();
	Plasma::ToolTipManager::self()->show(m_hoverItem); 
}

void PlasmaToolTip::hideAction() {
	hide();
}

void PlasmaToolTip::hide() {
	// hides itself on leave even when autohide == false!
//	if (!m_hoverItem.isNull()) {
//		Plasma::ToolTipManager::self()->hide(m_hoverItem);
//	}
	/*
	disconnect(Plasma::ToolTipManager::self(),
		SIGNAL(windowPreviewActivated(WId,Qt::MouseButtons,Qt::KeyboardModifiers,QPoint)),
		this, SLOT(activateWindow(WId,Qt::MouseButtons)));
	*/
	DelayedToolTip::hide();
}

// XXX: for some reason this is never called! why?
void PlasmaToolTip::activateWindow(WId window, Qt::MouseButtons buttons) {
	qDebug("activate window: 0x%lx", (unsigned long) window);
	if (buttons & Qt::LeftButton) {
		qDebug("do it!");
		KWindowSystem::activateWindow(window);
	}
}

void PlasmaToolTip::itemUpdate(TaskItem *item) {
	if (m_hoverItem == item && !m_hoverItem.isNull()) {
		updateToolTip();
	}
}

void PlasmaToolTip::updateToolTip() {
	Task *task = m_hoverItem->task();
	
	if (task == NULL || !task->isValid()) {
		return;
	}
	
	Plasma::ToolTipContent data;
	TaskManager::Task* taskPtr(task->task());
	QList<WId> windows;
	int desktop = -1;

	data.setAutohide(false);
	data.setClickable(true);
	data.setHighlightWindows(true);
	
	switch (task->type()) {
	case Task::TaskItem:
		data.setMainText(taskPtr->name());
		data.setSubText(taskPtr->isOnAllDesktops() ?
			i18n("On all desktops") :
			i18nc("Which virtual desktop a window is currently on", "On %1",
				KWindowSystem::desktopName(taskPtr->desktop())));
		data.setImage(taskPtr->icon());
		windows.append(taskPtr->window());
		break;
	case Task::GroupItem:
		data.setMainText(task->group()->name());
		data.setImage(task->group()->icon());
		foreach (TaskManager::AbstractGroupableItem *item, task->group()->members()) {
			TaskManager::TaskItem* taskItem = dynamic_cast<TaskManager::TaskItem*>(item);
			if (taskItem && taskItem->task()) {
				windows.append(taskItem->task()->window());
				if (!taskItem->task()->isOnAllDesktops()) {
					if (desktop == -1) {
						desktop = taskItem->task()->desktop();
					}
					else if (desktop != taskItem->task()->desktop()) {
						desktop = -2;
					}
				}
			}
		}
		data.setSubText(desktop == -1 ?
			i18n("On all desktops") :
			desktop == -2 ?
				i18n("On various desktops") :
				i18nc("Which virtual desktop a window is currently on", "On %1",
					KWindowSystem::desktopName(desktop)));
		break;
	case Task::StartupItem:
		data.setMainText(task->startup()->text());
		data.setImage(task->startup()->icon());
		data.setSubText(i18n("Starting application..."));
		break;
	case Task::LauncherItem:
		data.setMainText(task->launcherItem()->name());
		data.setImage(task->launcherItem()->icon());
		data.setSubText(task->launcherItem()->genericName());
		break;
	default:
		break;
	}
	data.setWindowsToPreview(windows);

	Plasma::ToolTipManager::self()->setContent(m_hoverItem, data);
}

} // namespace SmoothTasks
