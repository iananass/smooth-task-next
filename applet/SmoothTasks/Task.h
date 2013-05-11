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
#ifndef SMOOTHTASKS_TASK_H
#define SMOOTHTASKS_TASK_H

// Qt
#include <QGraphicsWidget>
#include <QPixmap>
#include <QTime>
#include <QTimer>

// Plasma
#include <Plasma/IconWidget>

// KDE
#include <KIcon>

// Taskmanager
#include <taskmanager/taskmanager.h>
#include <taskmanager/abstractgroupableitem.h>
#include <taskmanager/taskitem.h>
#include <taskmanager/launcheritem.h>
#include <taskmanager/groupmanager.h>
#include <taskmanager/startup.h>
#include <taskmanager/taskactions.h>

namespace SmoothTasks {

enum TaskFlag {
	TaskWantsAttention = 1,
	TaskHasFocus       = 2,
	TaskIsMinimized    = 4
};
Q_DECLARE_FLAGS(TaskFlags, TaskFlag)

class Task : public QObject {
	Q_OBJECT

public:
	Task(TaskManager::AbstractGroupableItem *abstractItem, QObject *parent = NULL);
	~Task() {}

	bool isActive() const;
	bool isOnCurrentDesktop() const;
	bool isOnAllDesktops() const;
	bool isMinimized() const;
	bool demandsAttention() const;

	enum ItemType {
		OtherItem = 0,
		StartupItem,
		TaskItem,
		GroupItem,
		LauncherItem
	};

	void                    setIcon(const QIcon icon);
	const QIcon&            icon() const { return m_icon; }
	void                    setText(const QString &text);
	QString                 text() const;
	QString                 description() const;
	int                     desktop() const;
	TaskManager::Task*    task() const;
	TaskManager::AbstractGroupableItem *abstractItem() { return m_abstractItem; }
	TaskManager::GroupPtr   group() const { return m_group; }
	TaskManager::TaskItem  *taskItem() const { return m_task; }
	TaskManager::LauncherItem  *launcherItem() const { return m_launcher; }
	TaskManager::Startup* startup() const;
	TaskFlags               flags() const { return m_flags; }
	ItemType                type()  const { return m_type; }
	void                    addMimeData(QMimeData* mimeData);
	int                     taskCount() const;
	bool                    isValid() const { return m_abstractItem != NULL; }

private:
	void setWindowTask(TaskManager::TaskItem* taskItem);

	TaskManager::TaskItem              *m_task;
	TaskManager::TaskGroup             *m_group;
	TaskManager::LauncherItem		   *m_launcher;
	TaskManager::AbstractGroupableItem *m_abstractItem;

	TaskFlags m_flags;
	ItemType  m_type;
	KIcon     m_icon;

private slots:
	void updateTask(::TaskManager::TaskChanges changes);
	void gotTaskPointer();
	void itemDestroyed();

signals:
	void updateToolTip();
	void updateState();
	void updateIcon(const QIcon& icon);
	void update();
	void gotTask();
};

} // namespace SmoothTasks
#endif
