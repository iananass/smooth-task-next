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

// Smooth Tasks
#include "SmoothTasks/Task.h"
#include "SmoothTasks/Applet.h"

// Qt
#include <QApplication>

// KDE
#include <KIcon>

namespace SmoothTasks {

Task::Task(TaskManager::AbstractGroupableItem *abstractItem, QObject *parent)
	: QObject(parent),
	  m_task(NULL),
	  m_group(NULL),
	  m_abstractItem(abstractItem),
	  m_flags(0),
	  m_type(OtherItem),
	  m_icon() {
	
	connect(
		abstractItem, SIGNAL(destroyed(QObject*)),
		this, SLOT(itemDestroyed()));
	
        if (abstractItem->itemType() == TaskManager::GroupItemType) {
                m_type  = GroupItem;
		m_group = static_cast<TaskManager::TaskGroup*>(abstractItem);
		connect(
			m_group, SIGNAL(changed(::TaskManager::TaskChanges)),
			this, SLOT(updateTask(::TaskManager::TaskChanges)));
		updateTask(::TaskManager::EverythingChanged);
	}
	else if (abstractItem->itemType() == TaskManager::LauncherItemType) {
		m_type = LauncherItem;
		m_launcher = static_cast<TaskManager::LauncherItem*>(abstractItem);
		connect(
			m_launcher, SIGNAL(changed(::TaskManager::TaskChanges)),
			this, SLOT(updateTask(::TaskManager::TaskChanges)));
		updateTask(::TaskManager::EverythingChanged);
	}
	else {
		m_task = static_cast<TaskManager::TaskItem*>(abstractItem);
		if (task()) {
			m_type = TaskItem;
			connect(
				m_task, SIGNAL(changed(::TaskManager::TaskChanges)),
				this, SLOT(updateTask(::TaskManager::TaskChanges)));
			updateTask(::TaskManager::EverythingChanged);
			emit gotTask();
		}
		else if (startup()) {
			m_type = StartupItem;
			connect(m_task, SIGNAL(gotTaskPointer()), this, SLOT(gotTaskPointer()));
			connect(
				m_task, SIGNAL(changed(::TaskManager::TaskChanges)),
				this, SLOT(updateTask(::TaskManager::TaskChanges)));
			updateTask(::TaskManager::EverythingChanged);
		}
	}
}

void Task::itemDestroyed() {
	m_abstractItem = NULL;
	m_task         = NULL;
	m_group        = NULL;
	m_launcher	   = NULL;
}

bool Task::isActive() const {
	return m_abstractItem ? m_abstractItem->isActive() : false;
}

bool Task::isOnCurrentDesktop() const {
	return m_abstractItem ? m_abstractItem->isOnCurrentDesktop() : false;
}

bool Task::isOnAllDesktops() const {
	return m_abstractItem ? m_abstractItem->isOnAllDesktops() : false;
}

bool Task::isMinimized() const {
	return m_abstractItem ? m_abstractItem->isMinimized() : false;
}

bool Task::demandsAttention() const {
	return m_abstractItem ? m_abstractItem->demandsAttention() : false;
}

void Task::addMimeData(QMimeData* mimeData) {
	if (m_group) {
		m_group->addMimeData(mimeData);
	}
	else if (m_task) {
		m_task->addMimeData(mimeData);
	}
}

QString Task::text() const {
	TaskManager::Task*    task;
	TaskManager::Startup* startup;
	
	switch (type()) {
	case StartupItem:
		startup = this->startup();
		
		if (startup) {
			return startup->text();
		}
		break;
		
	case TaskItem:
		task = this->task();
		
		if (task) {
			return task->visibleName();
		}
		break;
		
	case GroupItem:
		if (m_group != NULL) {
			return m_group->name();
		}
		break;
		
	case LauncherItem:
		if (m_launcher != NULL) {
			return m_launcher->name();
		}
		break;
	
	default:
		if (m_abstractItem != NULL) {
			return m_abstractItem->name();
		}
		break;
	}
	
	return QString::null;
}

QString Task::description() const {
	QString temp;
	switch (type()) {
	case StartupItem:
		temp = i18n("Starting application...");
		break;
	case TaskItem:
	case GroupItem:
		temp = isOnAllDesktops() ?
			i18n("On all desktops") :
			i18nc("Which virtual desktop a window is currently on", "On %1",
				KWindowSystem::desktopName(m_abstractItem->desktop()));
		break;
	case LauncherItem:
		temp = launcherItem()->genericName();
		break;
	case OtherItem:
		break;
	}
	return temp;
}

TaskManager::Task* Task::task() const {
    if (m_task) {
		return m_task->task();
	} else {
        return NULL;
    }
}

TaskManager::Startup* Task::startup() const {
	if (m_task) {
          return m_task->startup();
	} else {
          return NULL;
	}
}

int Task::taskCount() const {
	switch (type()) {
	case GroupItem:
		if (m_group == NULL) {
			return 0;
		}
		
		return m_group->members().size();
	default:
		return 1;
	}
}

void Task::updateTask(::TaskManager::TaskChanges changes) {
//	 if (m_type != TaskItem && m_type != GroupItem)
//	 return;

	if (!isValid()) {
		return;
	}

	bool needsUpdate = false;
	bool needsUpdateState = false;
	TaskFlags t_flags = m_flags;

	if (isActive()) {
		t_flags |= TaskHasFocus;
	} 
	else {
		t_flags &= ~TaskHasFocus;
	}

	if (demandsAttention()) {
		t_flags |= TaskWantsAttention;
	}
	else {
		t_flags &= ~TaskWantsAttention;
	}

	if (isMinimized()) {
		t_flags |= TaskIsMinimized;
	}
	else {
		t_flags &= ~TaskIsMinimized;
	}

	if (m_flags != t_flags) {
		needsUpdate = true;
		m_flags = t_flags;
		needsUpdateState = true;
	}

	if (changes & TaskManager::IconChanged) {
		switch (type()) {
		case StartupItem:
			if (!KIcon(startup()->icon()).isNull()) {
				m_icon = KIcon(startup()->icon());
			}
			break;
		case TaskItem:
			if (!KIcon(task()->icon()).isNull()) {
				m_icon = KIcon(task()->icon());
			}
			break;
		case GroupItem:
			if (!KIcon(group()->icon()).isNull()) {
				m_icon = KIcon(group()->icon());
			}
			break;
		case LauncherItem:
			if (!KIcon(launcherItem()->icon()).isNull()) {
				m_icon = KIcon(launcherItem()->icon());
			}
			break;
		case OtherItem:
			break;
		}
		emit updateIcon(m_icon);
		needsUpdate = true;
	}

	if (changes & (TaskManager::NameChanged | TaskManager::StateChanged)) {
		needsUpdate = true;
	}

	if (changes & (TaskManager::StateChanged | TaskManager::DesktopChanged)) {
		emit updateToolTip();
		needsUpdateState = true;
	}
	
	if (changes & (TaskManager::IconChanged | TaskManager::NameChanged)) {
		needsUpdate = true;
	}

	if (needsUpdateState) {
		emit updateState();
	}

	if (needsUpdate) {
		emit update();
	}
}

void Task::gotTaskPointer() {
	TaskManager::TaskItem *item = qobject_cast<TaskManager::TaskItem*>(sender());

	if (item) {
		setWindowTask(item);
	}
}

// XXX: do I have to tell the applet about this?
void Task::setWindowTask(TaskManager::TaskItem* taskItem) {
	m_type = TaskItem;
	if (m_task && m_task->task()) {
		disconnect(m_task->task(), 0, this, 0);
	}
	
	m_task = taskItem;
	m_abstractItem = qobject_cast<TaskManager::AbstractGroupableItem *>(taskItem);
	
	if (m_abstractItem) {
		connect(m_abstractItem, SIGNAL(destroyed(QObject*)), this, SLOT(itemDestroyed()));
	}
	
	connect(
		m_task, SIGNAL(changed(::TaskManager::TaskChanges)),
		this, SLOT(updateTask(::TaskManager::TaskChanges)));
	
	updateTask(::TaskManager::EverythingChanged);
	
	// for publishing the item geometry:
	emit gotTask();
}

} // namespace SmoothTasks
#include "Task.moc"
