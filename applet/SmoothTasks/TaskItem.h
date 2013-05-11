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
#ifndef SMOOTHTASKS_TASKITEM_H
#define SMOOTHTASKS_TASKITEM_H

// Smooth Tasks
#include "SmoothTasks/Task.h"
#include "SmoothTasks/TaskStateAnimation.h"
#include "SmoothTasks/ExpansionDirection.h"

// Qt
#include <QGraphicsWidget>
#include <QPixmap>
#include <QColor>
#include <QTime>
#include <QTimer>

// Plasma
#include <Plasma/IconWidget>

// Taskmanager
#include <taskmanager/taskmanager.h>
#include <taskmanager/abstractgroupableitem.h>
#include <taskmanager/groupmanager.h>
#include <taskmanager/taskitem.h>
#include <taskmanager/startup.h>
#include <taskmanager/taskactions.h>

class QPainter;
class QStyleOptionGraphicsItem;
class PeachyApplet;
class QRectF;
class QSizeF;
class QGraphicsSceneHoverEvent;
class QTextOption;
class QTextLayout;

namespace Plasma {
    class FrameSvg;
}

namespace SmoothTasks {

class TaskbarLayout;
class Light;
class TaskIcon;
class Applet;

class TaskItem : public QGraphicsWidget {
    Q_OBJECT

public:
	TaskItem(TaskManager::AbstractGroupableItem *abstractItem, Applet *parent);
	~TaskItem();

	Task *task() const { return m_task; }
	bool  isExpanded() const;

	Applet   *applet() const { return m_applet; }
	TaskIcon *icon()   const { return m_icon; }

	Qt::Orientation orientation() const { return m_orientation; }
	const QSizeF&   cellSize() const { return m_cellSize; }

	bool isMouseIn() const { return m_mouseIn; }
	bool isExpandedByHover() const;
	
	QPoint popupPosition(const QSize& size, bool center, int *toolTipPosition);
	
	QPointF mapFromGlobal(const QPoint& point, bool *contained = NULL) const;
	
public slots:
	void setOrientation(Qt::Orientation orientation);
	void setCellSize(const QSizeF& cellSize);
	void activate();
	void settingsChanged();
	void update();
	void updateState();
	void confirmLeave();
	void confirmEnter();

private slots:
	void updateTimerTick();
	void updateToolTip();
	void publishIconGeometry();

private:
	static const qreal MINIMIZED_TEXT_ALPHA;

	// "string table"
	static const QString GROUP_EXPANDER_TOP;
	static const QString GROUP_EXPANDER_RIGHT;
	static const QString GROUP_EXPANDER_LEFT;
	static const QString GROUP_EXPANDER_BOTTOM;

	void           updateExpansion();
	void           activateOrIconifyGroup();
	void           drawText(QPainter *p, qreal marginLeft, qreal marginTop, qreal marginRight, qreal marginBottom);
	void           drawFrame(QPainter *p, Plasma::FrameSvg *frame);
	void           expandTask();
	void           collapseTask();
	void           collapseTaskOnLeave();
	QRect          iconGeometry() const;
	QRectF         expanderRect(const QRectF &bounds) const;
	const QString& expanderElement() const;
	void           drawExpander(QPainter *painter, const QRectF& expRect) const;
	
	KUrl		   launcherUrl(TaskManager::AbstractGroupableItem* item);

	Applet   *m_applet;
	TaskIcon *m_icon;
	Task     *m_task;
	Light    *m_light;
	TaskManager::AbstractGroupableItem *m_abstractItem;

	QTimer            *m_activateTimer;
	QTimer            *m_updateTimer;
	bool               m_mouseIn;
	bool               m_delayedMouseIn;
	TaskStateAnimation m_stateAnimation;

	Qt::Orientation m_orientation;
	QSizeF          m_cellSize;
	
	bool m_updateScheduled;

protected:
	void dropEvent(QGraphicsSceneDragDropEvent *event);
	void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);
	void paint(QPainter *p, const QStyleOptionGraphicsItem *option, QWidget *widget);
	void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
	void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);
	void hoverMoveEvent(QGraphicsSceneHoverEvent* event);
	void mousePressEvent(QGraphicsSceneMouseEvent *event);
	void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
	void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
	void dragEnterEvent(QGraphicsSceneDragDropEvent *event);
	void dragMoveEvent(QGraphicsSceneDragDropEvent *event);
	void dragLeaveEvent(QGraphicsSceneDragDropEvent *event);
	// text
	QColor textColor() const;

private:
	void hoverEnterEvent();
	void hoverLeaveEvent();
	void drawTextLayout(
		QPainter *painter, const QTextLayout& layout,
		const QRectF& rect, const QSizeF& textSize, const QColor& color) const;

signals:
	void itemActive(TaskItem* item);
	void expand(TaskItem* item, ExpansionDirection direction);
};

} // namespace SmoothTasks
#endif
