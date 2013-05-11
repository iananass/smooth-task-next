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

#ifndef SMOOTHTASKS_APPLET_H
#define SMOOTHTASKS_APPLET_H

// Ui
#include "ui_General.h"
#include "ui_Appearance.h"

// Qt
#include <QList>
#include <QPointer>
#include <QWeakPointer>

// Plasma
#include <Plasma/Applet>
#include <Plasma/IconWidget>
#include <Plasma/DataEngine>

// KDE
#include <KDialog>

// Taskmanager
#include <taskmanager/startup.h>
#include <taskmanager/taskitem.h>
#include <taskmanager/launcheritem.h>
#include <taskmanager/taskmanager.h>
#include <taskmanager/groupmanager.h>
#include <taskmanager/abstractgroupableitem.h>
#include <taskmanager/taskactions.h>

class QGraphicsLinearLayout;

using TaskManager::AbstractGroupableItem;
using TaskManager::LauncherItem;

namespace Plasma {
	class FrameSvg;
}

namespace TaskManager {
  class GroupManager;
}

namespace SmoothTasks {

class Task;
class TaskItem;
class ToolTipBase;
class TaskbarLayout;
class GroupManager;

class Applet : public Plasma::Applet {
	Q_OBJECT

public:
	enum ExpandType {
		ExpandNone           = 0,
		ExpandActive         = 1,
		ExpandCurrentDesktop = 2,
		ExpandAll            = 3
	};
	
	enum PreviewLayoutType {
		ClassicPreviewLayout = 0,
		NewPreviewLayout     = 1
	};

	enum IconShapeType {
		RectangelIcon = 0,
		SquareIcon    = 1
	};
	
	enum MiddleClickAction {
		NoAction             = 0,
		CloseTask            = 1,
		MoveToCurrentDesktop = 2
	};

	Applet(QObject *parent, const QVariantList &args);
	~Applet();
	void init();
	void dropEvent(QGraphicsSceneDragDropEvent *event);
	void dragMoveEvent(const QPointF& pos);
	
	//config
	QColor lightColor() const { return m_lightColor; } 
	TaskManager::GroupManager::TaskGroupingStrategy groupingStrategy() {
		return m_groupingStrategy;
	}
	TaskManager::GroupManager::TaskSortingStrategy sortingStrategy() {
		return m_sortingStrategy;
	}
	bool              isPopupShowing()        const;
	bool              lights()                const { return m_lights; }
	bool              expandTasks()           const { return m_expandTasks; }
	ExpandType        keepExpanded()          const { return m_keepExpanded; }
	bool              expandOnHover()         const { return m_expandOnHover; }
	bool              expandOnAttention()     const { return m_expandOnAttention; }
	double            iconScale()             const { return (double) m_iconScale / 100; }
	int               animationDuration()     const;
	IconShapeType     iconShape()             const { return m_shape; }
	int               fps()                   const;
	ToolTipBase      *toolTip()                     { return m_toolTip; }
	TaskManager::GroupManager *groupManager()       { return m_groupManager; }
	Plasma::FrameSvg *frame()                       { return m_frame; }
	QRect             currentScreenGeometry() const;
	QRect             virtualScreenGeometry() const;
	PreviewLayoutType previewLayout()         const { return m_previewLayout; }
	int               maximumPreviewSize()    const { return m_maxPreviewSize; }
	int               toolTipMoveDuraton()    const { return m_tooltipMoveDuration; }
	int               highlightDelay()        const { return m_highlightDelay; }
	bool              dontRotateFrame()       const { return m_dontRotateFrame; }
	bool              onlyLights()            const { return m_onlyLights; }
	bool              textShadow()            const { return m_textShadow; }
	bool              lightColorFromIcon()    const { return m_lightColorFromIcon; }
	MiddleClickAction middleClickAction()     const { return m_middleClickAction; }
	void              dragItem(TaskItem *item, QGraphicsSceneMouseEvent *event);
	void              dragTask(TaskManager::AbstractGroupableItem* task, QWidget *source);
	void              middleClickTask(TaskManager::AbstractGroupableItem* task);
	
	void popup(const QPoint& pos, Task *task, QObject *receiver, const char *slot);
	void popup(const TaskItem* item);
	
	void dumpItems();
	
private:
	void reloadItems();
	void connectRootGroup();
	void disconnectRootGroup();
	TaskManager::BasicMenu *popup(Task *task);
	
	// other
	Plasma::FrameSvg                    *m_frame;
	TaskManager::GroupManager           *m_groupManager;
	QWeakPointer<TaskManager::TaskGroup> m_rootGroup; 
	ToolTipBase                         *m_toolTip;

	TaskbarLayout *m_layout;
	QHash<TaskManager::AbstractGroupableItem*, TaskItem*> m_tasksHash;
	Ui::General    m_configG;
	Ui::Appearance m_configA;

	void clear();
	void constraintsEvent(Plasma::Constraints constraints);
	int  totalSubTasks();
	TaskManager::AbstractGroupableItem *selectSubTask(int index);
	int  activeIndex();
	
	// task
	TaskManager::GroupManager::TaskGroupingStrategy m_groupingStrategy;
	TaskManager::GroupManager::TaskSortingStrategy  m_sortingStrategy;
	
	// config
	int               m_taskSpacing;
	int               m_iconScale;
	bool              m_lights;
	int               m_expandTasks;
	ExpandType        m_keepExpanded;
	bool              m_expandOnHover;
	bool              m_expandOnAttention;
	QColor            m_lightColor;
	IconShapeType     m_shape;
	int               m_activeIconIndex;
	PreviewLayoutType m_previewLayout;
	MiddleClickAction m_middleClickAction;
	int               m_maxPreviewSize;
	int               m_tooltipMoveDuration;
	int               m_highlightDelay;
	int               m_itemsPerRow;
	qreal             m_squeezeRatio;
	bool              m_preferGrouping;
	int               m_itemHeight;
	qreal             m_rowAspectRatio;
	bool              m_dontRotateFrame;
	bool              m_onlyLights;
	bool              m_textShadow;
	bool              m_lightColorFromIcon;
	bool              m_scrollSwitchTasks;
	
protected:
	void   createConfigurationInterface(KConfigDialog *parent);
	QSizeF sizeHint(Qt::SizeHint which, const QSizeF& constraint = QSizeF()) const;
	void   wheelEvent(QGraphicsSceneWheelEvent *event);
	void   hoverEnterEvent(QGraphicsSceneHoverEvent *event);
	void   dragEnterEvent(QGraphicsSceneDragDropEvent *event);
	void   dragMoveEvent(QGraphicsSceneDragDropEvent *event);
	void   dragLeaveEvent(QGraphicsSceneDragDropEvent *event);

public slots:
	void updateActiveIconIndex(TaskItem *item);
	void reload();

private slots:
	void reconnectGroupManager();
	void updateFullLimit();
	void widgetValueChanged();
	void configuration();
	void loadDefaults();
	void itemAdded(AbstractGroupableItem *groupableItem);
	void itemRemoved(AbstractGroupableItem *groupableItem);
	void itemPositionChanged(AbstractGroupableItem *groupableItem);
	void launcherAdded(LauncherItem* launcherItem);
	void launcherRemoved(LauncherItem* launcherItem);
	void currentDesktopChanged();
	void uiToolTipKindChanged(int index);
	void uiTaskbarLayoutChanged(int index);
	void uiMinimumRowsChanged(int minimumRows);
	void uiMaximumRowsChanged(int maximumRows);
	void uiGroupingStrategyChanged(int index);
	void newNotification(const QString& notif);

protected slots:
	void configAccepted();

signals:
	void settingsChanged();
	void previewLayoutChanged(Applet::PreviewLayoutType previewLayout);
	void mouseEnter();
};

} // namespace SmoothTasks
#endif
