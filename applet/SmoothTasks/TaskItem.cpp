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
#include "SmoothTasks/TaskItem.h"
#include "SmoothTasks/Applet.h"
#include "SmoothTasks/TaskIcon.h"
#include "SmoothTasks/Light.h"
#include "SmoothTasks/Global.h"
#include "SmoothTasks/SmoothToolTip.h"
#include "SmoothTasks/TaskbarLayout.h"

// Qt
#include <QtGlobal>
#include <QApplication>
#include <QPainter>
#include <QFontMetrics>
#include <QSizeF>
#include <QGraphicsLinearLayout>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSceneContextMenuEvent>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsSceneMouseEvent>
#include <QTextLayout>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QMimeData>
#include <QTextDocument>
#include <QTimer>
#include <QString>

// KDE
#include <KIcon>
#include <KIconEffect>
#include <KIconLoader>
#include <KGlobalSettings>
#include <KColorUtils>
#include <KWindowSystem>
#include <KRun>
#include <KServiceTypeTrader>
#include <KStandardDirs>
#include <KUrl>

// Plasma
#include <Plasma/FrameSvg>
#include <Plasma/Theme>
#include <Plasma/PaintUtils>

#include <cmath>
#include <cstring>

namespace SmoothTasks {

const qreal   TaskItem::MINIMIZED_TEXT_ALPHA  = 0.85;
const QString TaskItem::GROUP_EXPANDER_TOP    = QString::fromLatin1("group-expander-top");
const QString TaskItem::GROUP_EXPANDER_RIGHT  = QString::fromLatin1("group-expander-right");
const QString TaskItem::GROUP_EXPANDER_LEFT   = QString::fromLatin1("group-expander-left");
//const QString TaskItem::GROUP_EXPANDER_BOTTOM = QString::fromLatin1("group-expander-bottom");
const QString TaskItem::GROUP_EXPANDER_BOTTOM = QString::fromLatin1("group-expanisGroupItemder-bottom");

TaskItem::TaskItem(AbstractGroupableItem *abstractItem, Applet *applet)
		: QGraphicsWidget(applet),
		  m_applet(applet),
		  m_icon(new TaskIcon(this)),
		  m_task(new Task(abstractItem, this)),
		  m_light(new Light(this)),
		  m_abstractItem(abstractItem),
		  m_activateTimer(NULL),
		  m_updateTimer(new QTimer()),
		  m_mouseIn(false),
		  m_delayedMouseIn(false),
		  m_stateAnimation(),
		  m_orientation(Qt::Horizontal),
		  m_cellSize(0, 0),
		  m_updateScheduled(false) {
	connect(applet, SIGNAL(settingsChanged()), this, SLOT(settingsChanged()));

	m_icon->setIcon(m_task->icon());

	m_updateTimer->setInterval(1000 / m_applet->fps());
	connect(m_updateTimer, SIGNAL(timeout()), this, SLOT(updateTimerTick()));

	setAcceptsHoverEvents(true);
	setAcceptDrops(true);

	// task signals
	connect(m_task, SIGNAL(update()),        this, SLOT(update()));
	connect(m_task, SIGNAL(updateState()),   this, SLOT(updateState()));
	connect(m_task, SIGNAL(updateToolTip()), this, SLOT(updateToolTip()));
	connect(m_task, SIGNAL(gotTask()),       this, SLOT(publishIconGeometry()));

	// icon
	connect(m_icon, SIGNAL(update()),                 this,   SLOT(update()));
	connect(m_task, SIGNAL(updateIcon(const QIcon&)), m_icon, SLOT(setIcon(const QIcon&)));

	updateState();
	
	// light
	connect(m_light, SIGNAL(update()), this, SLOT(update()));
	
	// additional
	connect(
		TaskManager::TaskManager::self(), SIGNAL(desktopChanged(int)),
		this, SLOT(updateState()));
	
	if (m_task->type() == Task::StartupItem) {
		m_icon->startStartupAnimation(500);
		m_light->startAnimation(Light::StartupAnimation, 500, true);
	}
		
        if (abstractItem->itemType() == TaskManager::GroupItemType) {
          TaskManager::TaskGroup* group = static_cast<TaskManager::TaskGroup*>(abstractItem);
		connect(
			group, SIGNAL(itemAdded(AbstractGroupableItem*)),
			this, SLOT(updateToolTip()));
		connect(
			group, SIGNAL(itemRemoved(AbstractGroupableItem*)),
			this, SLOT(updateToolTip()));
	}

	connect(
		&m_stateAnimation, SIGNAL(update()),
		this, SLOT(update()));
}

TaskItem::~TaskItem() {
	m_applet->toolTip()->itemDelete(this);
	m_updateTimer->deleteLater();
	if (m_activateTimer) {
		delete m_activateTimer;
		m_activateTimer = NULL;
	}
}

void TaskItem::setOrientation(Qt::Orientation orientation) {
	m_orientation = orientation;
}

void TaskItem::setCellSize(const QSizeF& cellSize) {
	m_cellSize = cellSize;
}

bool TaskItem::isExpandedByHover() const {
	if (m_applet->expandTasks() && m_delayedMouseIn && m_applet->expandOnHover() && m_task->type() != Task::LauncherItem) {
		if (m_applet->expandOnAttention() && m_task->demandsAttention()) {
			return false;
		}
		switch (m_applet->keepExpanded()) {
			case Applet::ExpandNone:
				return true;
			case Applet::ExpandAll:
				return false;
			case Applet::ExpandCurrentDesktop:
				return !(m_task->isOnCurrentDesktop() || m_task->isOnAllDesktops());
			case Applet::ExpandActive:
				return !m_task->isActive();
		}
	}
	return false;
}


bool TaskItem::isExpanded() const {
	if (m_applet->expandTasks() && m_task->type() != Task::LauncherItem) {
		if (m_delayedMouseIn && m_applet->expandOnHover()) {
			return true;
		}
		if (m_applet->expandOnAttention() && m_task->demandsAttention()) {
			return true;
		}
		switch (m_applet->keepExpanded()) {
		case Applet::ExpandNone:
			return false;
		case Applet::ExpandAll:
			return true;
		case Applet::ExpandCurrentDesktop:
			return m_task->isOnCurrentDesktop() || m_task->isOnAllDesktops();
		case Applet::ExpandActive:
			return m_task->isActive();
		}
	}

	return false;
}

void TaskItem::settingsChanged() {
	m_updateTimer->setInterval(1000 / m_applet->fps());
	updateExpansion();
}

void TaskItem::updateExpansion() {
	if (isExpanded()) {
		expandTask();
	}
	else {
		collapseTask();
	}
}

void TaskItem::updateTimerTick() {
	if (m_updateScheduled) {
		QGraphicsWidget::update();
		m_updateScheduled = false;
	} else {
		m_updateTimer->stop();
	}
}

void TaskItem::update() {
	// limit framerate to configured value
	if (m_updateTimer->isActive()) {
		m_updateScheduled = true;
	}
	else {
		m_updateTimer->start();
		QGraphicsWidget::update();
	}
}

void TaskItem::updateToolTip() {
	m_applet->toolTip()->itemUpdate(this);
}

QPoint TaskItem::popupPosition(const QSize& size, bool center, int *toolTipPosition) {
//	return m_applet->containment()->corona()->popupPosition(this, size);
	const QRect  geometry(iconGeometry());
	const QRectF bounds(boundingRect());
	int x = geometry.left();
	int y = geometry.top();
	int toolTipPos = center ?
		ToolTipBase::Center | ToolTipBase::Middle :
		ToolTipBase::Right  | ToolTipBase::Top;
	
	const QRect currentScreenRect(m_applet->currentScreenGeometry());
	const int screenLeft   = currentScreenRect.left();
	const int screenTop    = currentScreenRect.top();
	const int screenRight  = screenLeft + currentScreenRect.width();
	const int screenBottom = screenTop  + currentScreenRect.height();

#define SET_VERTICAL(POS) \
	toolTipPos = (toolTipPos & ToolTipBase::HorizontalMask) | ToolTipBase::POS;

#define SET_HORIZONTAL(POS) \
	toolTipPos = (toolTipPos & ToolTipBase::VerticalMask)   | ToolTipBase::POS;

	// TODO: maybe don't take location from the applet, like orientation?
	switch (m_applet->location()) {
		case Plasma::BottomEdge:
			if (center) x -= (size.width() - bounds.width()) / 2;
			y -= size.height();
			SET_VERTICAL(Top)
			break;
		case Plasma::TopEdge:
			if (center) x -= (size.width() - bounds.width()) / 2;
			y += bounds.height();
			SET_VERTICAL(Bottom)
			break;
		case Plasma::LeftEdge:
			x += bounds.width();
			if (center) y -= (size.height() - bounds.height()) / 2;
			SET_HORIZONTAL(Right)
			break;
		case Plasma::RightEdge:
			x -= size.width();
			if (center) y -= (size.height() - bounds.height()) / 2;
			SET_HORIZONTAL(Left)
			break;
		default:
			if (m_orientation == Qt::Vertical) {
				if (center) y -= (size.height() - bounds.height()) / 2;

				if (x + size.width() + bounds.width() > screenRight) {
					x -= size.width();
					SET_HORIZONTAL(Right)
				}
				else {
					x += bounds.width();
					SET_HORIZONTAL(Left)
				}
			}
			else {
				if (center) x -= (size.width() - bounds.width()) / 2;

				if (y - size.height() >= screenTop) {
					y -= size.height();
					SET_VERTICAL(Top)
				}
				else {
					y += bounds.height();
					SET_VERTICAL(Bottom)
				}
			}
	}

	if (m_applet->location() != Plasma::LeftEdge && x + size.width() > screenRight) {
		x = screenRight - size.width();
	}
	
	if (m_applet->location() != Plasma::RightEdge && x < screenLeft) {
		x = screenLeft;
	}

	if (m_applet->location() != Plasma::TopEdge && y + size.height() > screenBottom) {
		y = screenBottom - size.height();
	}
	
	if (m_applet->location() != Plasma::BottomEdge && y < screenTop) {
		y = screenTop;
	}

	if (toolTipPosition) {
		*toolTipPosition = toolTipPos;
	}

	return QPoint(x, y);
}

// XXX: for some reason sometimes this cannot find a parent view when
//      TaskItem width > heigth and shown in the plasmoidviewer
QRect TaskItem::iconGeometry() const {
	if (!scene() || !boundingRect().isValid()) {
		return QRect();
	}

	QRectF  sceneBoundingRect(this->sceneBoundingRect());
	QPointF scenePos(this->scenePos());
	QGraphicsView *parentView = NULL;
	QGraphicsView *possibleParentView = NULL;
	// The following was taken from Plasma::Applet,
	// it doesn't make sense to make the item an applet,
	// and this was the easiest way around it.
	
	foreach (QGraphicsView *view, scene()->views()) {
		if (view->sceneRect().intersects(sceneBoundingRect) ||
			view->sceneRect().contains(scenePos)) {
			if (view->isActiveWindow()) {
				parentView = view;
				break;
			} else {
				possibleParentView = view;
			}
		}
	}

	if (!parentView) {
		parentView = possibleParentView;

		if (!parentView) {
			return QRect();
		}
	}

	QRect rect(parentView->mapFromScene(
		mapToScene(boundingRect())).boundingRect().adjusted(0, 0, 1, 1));
	rect.moveTopLeft(parentView->mapToGlobal(rect.topLeft()));
	return rect;
}

void TaskItem::publishIconGeometry() {
	QRect iconRect(iconGeometry());
	TaskManager::Task* task;
	TaskManager::GroupPtr group;
	
	switch (m_task->type()) {
		case Task::TaskItem:
			task = m_task->task();
			
			if (task) {
				task->publishIconGeometry(iconRect);
			}
			break;
		case Task::GroupItem:
			group = m_task->group();
			
			if (group) {
				foreach (TaskManager::AbstractGroupableItem *item, group->members()) {
					TaskManager::TaskItem *task = qobject_cast<TaskManager::TaskItem*>(item);
					if (task) {
						task->task()->publishIconGeometry(iconRect);
					}
				}
			}
		default:
			break;
	}
}

// XXX: this gets called to often! (still?)
void TaskItem::updateState() {
	int newState = m_mouseIn ? TaskStateAnimation::Hover : TaskStateAnimation::Normal;
	//m_applet->sorting(this);
	publishIconGeometry();
	m_icon->stopStartupAnimation();
	m_light->stopAnimation();

	if (m_task->demandsAttention()) {
		newState |= TaskStateAnimation::Attention;
		m_light->startAnimation(Light::AttentionAnimation, 900, true);
	} 
	else if (m_task->type() == Task::LauncherItem) {
		newState |= TaskStateAnimation::Launcher;
	}
	else if (m_task->isMinimized()) {
		newState |= TaskStateAnimation::Minimized;
	}
	else if (m_task->isActive()) {
		emit itemActive(this);
		newState |= TaskStateAnimation::Focus;
	}
	
	updateExpansion();
	
	m_stateAnimation.setState(newState, m_applet->fps(), m_applet->animationDuration());
}

void TaskItem::contextMenuEvent(QGraphicsSceneContextMenuEvent *event) {
	switch (m_task->type()) {
		case Task::TaskItem:
		case Task::GroupItem:
			m_applet->toolTip()->hide();
			m_applet->popup(this);
			event->accept();
			break;
		case Task::LauncherItem:
			m_applet->toolTip()->hide();
			m_applet->popup(this);
			event->accept();
			break;
		default:
			event->ignore();
	}
}

void TaskItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
	update();
	event->accept();
//	event->ignore();
}

void TaskItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
	if (
			QPoint(event->screenPos() - event->buttonDownScreenPos(Qt::LeftButton))
			.manhattanLength() < QApplication::startDragDistance()) {
		return;
	}
	
	m_applet->dragItem(this, event);
}

void TaskItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
	TaskManager::Task* task;
	
	switch (event->button()) {
	case Qt::LeftButton:
		m_applet->toolTip()->hide();
		
		publishIconGeometry();
		
		switch (m_task->type()) {
		case Task::TaskItem:
			task = m_task->task();
			
			if (task) {
				if (event->modifiers() == Qt::ControlModifier) {
					KUrl url = launcherUrl(m_task->abstractItem());
					if (m_applet->groupManager()->launcherExists(url)) {
						new KRun(url, 0);
					}
				} else {
					task->activateRaiseOrIconify();
				}
			}
			break;
		case Task::GroupItem:
			if (event->modifiers() == Qt::ControlModifier) {
				KUrl url = launcherUrl(m_task->abstractItem());
				if (m_applet->groupManager()->launcherExists(url)) {
					new KRun(url, 0);
				}
			} else {
				activateOrIconifyGroup();
			}
//		{
//			TaskManager::GroupPopupMenu *groupMenu = new TaskManager::GroupPopupMenu(
//				NULL,
//				m_task->group(),
//				m_applet->groupManager());
//			groupMenu->exec(QCursor::pos());
//
//			delete groupMenu;
//		}
			break;
		case Task::LauncherItem:
			m_task->launcherItem()->launch();
			break;
 		default:
			break;
		}
		break;
	case Qt::MidButton:
		m_applet->middleClickTask(m_task->abstractItem());
	default:
		break;
	}
}

KUrl TaskItem::launcherUrl(AbstractGroupableItem* item)
{
	if (!item) return KUrl();
	if (item->itemType() == TaskManager::TaskItemType) {
		TaskManager::TaskItem * t = qobject_cast<TaskManager::TaskItem*>(item);
		// Search for applications which are executable and case-insensitively match the windowclass of the task and
		// See http://techbase.kde.org/Development/Tutorials/Services/Traders#The_KTrader_Query_Language
		QString query = QString("exist Exec and ('%1' =~ Name)").arg(t->task()->classClass());
		KService::List services = KServiceTypeTrader::self()->query("Application", query);
		if (!services.empty()) {
			return KUrl::fromPath((services[0]->entryPath()));
		} else {
			// No desktop-file was found, so try to find at least the executable
			// usually it's the lower cased window class class, but if that fails let's trust it
			QString path = KStandardDirs::findExe(t->task()->classClass().toLower());
			if (path.isEmpty()) {
				path = KStandardDirs::findExe(t->task()->classClass());
			}

			if (!path.isEmpty()) {
				return KUrl::fromPath(path);
			}
		}
	} else if (item->itemType() == TaskManager::GroupItemType) {
		TaskManager::TaskGroup * t = qobject_cast<TaskManager::TaskGroup*>(item);
		// Strategy: try to return the first non-group item's launcherUrl,
		// failing that, try to return the  launcherUrl of the first group
		// if any
		foreach (TaskManager::AbstractGroupableItem *i, t->members()) {
			if (i->itemType() != TaskManager::GroupItemType) {
				return launcherUrl(i);
			}
		}

		if (t->members().isEmpty()) {
			return KUrl();
		}

		return launcherUrl(t->members().first());
	} else {
		return KUrl();
	}
	return KUrl();
}

void TaskItem::activateOrIconifyGroup() {
	TaskManager::GroupPtr group = m_task->group();
	
	if (group == NULL) {
		return;
	}
	
	bool includesActive = false;
	TaskManager::ItemList items(group->members());
	int iconified = 0;
	foreach (TaskManager::AbstractGroupableItem *item, items) {
		TaskManager::TaskItem *task = qobject_cast<TaskManager::TaskItem*>(item);
		if (task) {
			if (task->task()->isIconified()) {
				++ iconified;
			}
			
			if (task->task()->isActive()) {
				includesActive = true;
			}
		}
	}
	
	if (includesActive && items.size() - iconified > iconified) {
		// iconify
		foreach (TaskManager::AbstractGroupableItem *item, items) {
			TaskManager::TaskItem *task = qobject_cast<TaskManager::TaskItem*>(item);
			if (task) {
				task->task()->setIconified(true);
			}
		}
	}
	else {
		// activate
		QList<WId> winOrder(KWindowSystem::stackingOrder());
		const int winCount = winOrder.size();
		TaskManager::TaskItem* sortedItems[winCount];
		
		std::memset(sortedItems, 0, sizeof(TaskManager::TaskItem*) * winCount);
		
		foreach (TaskManager::AbstractGroupableItem *item, items) {
			TaskManager::TaskItem *task = qobject_cast<TaskManager::TaskItem*>(item);
			if (task) {
				int index = winOrder.indexOf(task->task()->window());
				if (index != -1) {
					sortedItems[index] = task;
				}
			}
		}
		
		for (int index = 0; index < winCount; ++ index) {
			TaskManager::TaskItem* task = sortedItems[index];
			if (task) {
				task->task()->activate();
			}
		}
	}
}

void TaskItem::activate() {
	TaskManager::Task* task;
	
	switch (m_task->type()) {
	case Task::TaskItem:
		task = m_task->task();
		
		if (task) {
			task->activate();
		}
		break;
	case Task::GroupItem:
		m_applet->toolTip()->quickShow(this);
	default:
		break;
	}
}

void TaskItem::dragEnterEvent(QGraphicsSceneDragDropEvent *event) {
	if (event->mimeData()->hasFormat(TASK_ITEM)) {
		//event->ignore(); //ignore it so the taskbar gets the event
		event->acceptProposedAction();
		return;
	}
	event->accept();
	if (m_task->type() == Task::GroupItem) {
		m_stateAnimation.setState(
			m_stateAnimation.toState() | TaskStateAnimation::Hover,
			m_applet->fps(), m_applet->animationDuration());
	
		if (m_applet->expandTasks() && m_task->type() != Task::LauncherItem) {
			expandTask();
		}
		m_applet->toolTip()->quickShow(this);
	}
	else {
		if (!m_activateTimer) {
			m_activateTimer = new QTimer(this);
			m_activateTimer->setSingleShot(true);
			m_activateTimer->setInterval(DRAG_HOVER_DELAY);
			connect(m_activateTimer, SIGNAL(timeout()), this, SLOT(activate()));
		}
		m_activateTimer->start();
		hoverEnterEvent();
	}
}

void TaskItem::dragMoveEvent(QGraphicsSceneDragDropEvent *event) {
	Q_UNUSED(event);

	if (m_activateTimer) {
		m_activateTimer->start();
	}
	update();

	m_applet->dragMoveEvent(pos() + event->pos());
}

void TaskItem::dragLeaveEvent(QGraphicsSceneDragDropEvent *event) {
	Q_UNUSED(event);
	if (m_activateTimer) {
		delete m_activateTimer;
		m_activateTimer = NULL;
	}
	hoverLeaveEvent();
}

void TaskItem::dropEvent(QGraphicsSceneDragDropEvent *event) {
	if (m_activateTimer) {
		delete m_activateTimer;
		m_activateTimer = NULL;
	}
	m_applet->dropEvent(event);
}

void TaskItem::hoverMoveEvent(QGraphicsSceneHoverEvent* e) {
	update();
	QGraphicsWidget::hoverMoveEvent(e);
}

void TaskItem::hoverEnterEvent(QGraphicsSceneHoverEvent *event) {
	Q_UNUSED(event);
	hoverEnterEvent();
}

void TaskItem::hoverEnterEvent() {
	m_mouseIn = true;
	m_stateAnimation.setState(
		m_stateAnimation.toState() | TaskStateAnimation::Hover,
		m_applet->fps(), m_applet->animationDuration());
}

void TaskItem::confirmEnter() {
	m_delayedMouseIn = true;
	if (m_applet->expandTasks() && m_applet->expandOnHover() && m_task->type() != Task::LauncherItem) {
		expandTask();
	}
}

void TaskItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event) {
	Q_UNUSED(event);
	hoverLeaveEvent();
}

void TaskItem::hoverLeaveEvent() {
	m_mouseIn = false;
	m_stateAnimation.setState(
		m_stateAnimation.toState() & ~TaskStateAnimation::Hover,
		m_applet->fps(), m_applet->animationDuration());

	if (m_applet->toolTip()->hoverItem() != this) {
		collapseTaskOnLeave();
	}
}

void TaskItem::confirmLeave() {
	m_delayedMouseIn = false;
	collapseTaskOnLeave();
}

void TaskItem::collapseTaskOnLeave() {
	if (m_applet->expandTasks()) {
		if (m_applet->expandOnAttention() && m_task->demandsAttention()) {
			return;
		}
		switch (m_applet->keepExpanded()) {
		case Applet::ExpandNone:
			collapseTask();
			break;
		case Applet::ExpandActive:
			if (!m_task->isActive()) {
				collapseTask();
			}
			break;
		case Applet::ExpandCurrentDesktop:
			if (!m_task->isOnCurrentDesktop()) {
				collapseTask();
			}
			break;
		case Applet::ExpandAll:
			break;
		}
	}
}

void TaskItem::expandTask() {
	emit expand(this, Expand);
}

void TaskItem::collapseTask() {
	emit expand(this, Collapse);
}

QPointF TaskItem::mapFromGlobal(const QPoint& point, bool *contained) const {
	QGraphicsScene *scene = this->scene();
	
	if (scene == NULL) {
		if (contained) {
			*contained = false;
		}
		return QPointF(-1, -1);
	}
	
	foreach (QGraphicsView *view, scene->views()) {
		QPointF mapped(mapFromScene(view->mapToScene(view->mapFromGlobal(point))));
		
		if (contains(mapped)) {
			if (contained) {
				*contained = true;
			}
			return mapped;
		}
	}
	
	if (contained) {
		*contained = false;
	}
	return QPointF(-1, -1);
}

void TaskItem::paint(QPainter *p, const QStyleOptionGraphicsItem *option, QWidget *widget) {
	Q_UNUSED(option);
	Q_UNUSED(widget);

	const QRectF bounds(boundingRect());
	const bool isVertical = m_orientation == Qt::Vertical;
	const bool showFrame = m_task->type() != Task::LauncherItem;
	QRectF lightBounds;
	p->setRenderHint(QPainter::Antialiasing);

	// draw frame
	// TODO: cache frame in TaskItem?
	Plasma::FrameSvg *frame = m_applet->frame();
	
	qreal left = 0, top = 0, right = 0, bottom = 0;
	frame->setElementPrefix(NORMAL);
	frame->getMargins(left, top, right, bottom);
	
	if (isVertical) {
		if (m_applet->dontRotateFrame()) {
			frame->resizeFrame(bounds.size());
			if(showFrame) {
				drawFrame(p, frame);
			}
			lightBounds.setHeight(bounds.width() - top - bottom);
			lightBounds.setWidth(bounds.height() - left - right);
			
			lightBounds.moveTop(right);
			lightBounds.moveLeft(top);
			
			p->save();
			p->rotate(-90);
			p->translate(-bounds.height(), 0);
		}
		else {
			p->save();
			p->rotate(-90);
			p->translate(-bounds.height(), 0);
		
			frame->resizeFrame(QSizeF(bounds.height(), bounds.width()));
			if(showFrame) {
				drawFrame(p, frame);
			}
			lightBounds = frame->contentsRect();
		}
	}
	else {
		frame->resizeFrame(bounds.size());
		if(showFrame) {
			drawFrame(p, frame);
		}
		lightBounds = frame->contentsRect();
	}
	
	m_icon->setRect(bounds);
	
	// TODO: It would be nice if I could use p->setClipPath(), but it doesn't
	//       seem to be possible to get a bounding path of a FrameSvg.
	// p->setClipRegion(frame->mask());
	// draw light
	if (m_applet->lights() && m_task->type() != Task::LauncherItem) {
		bool mouseIn = false;
		QPointF pos(mapFromGlobal(QCursor::pos(), &mouseIn));
		
		m_light->paint(p, lightBounds, pos, mouseIn, isVertical);
	}

	// draw text
	if (m_applet->expandTasks()) {
		drawText(p, left, top, right, bottom);
	}
	
	if (isVertical) {
		p->restore();
	}
	
	// draw icon
	m_icon->paint(p, m_stateAnimation.hover(), m_task->type() == Task::GroupItem);
}

void TaskItem::drawFrame(QPainter *p, Plasma::FrameSvg *frame) {
	// draw "layers":
	//    hover
	//    attention
	//    focus
	//    minimized
	//    normal
	int animatedState  = m_stateAnimation.animatedState();
	int reachedUpState = m_stateAnimation.reachedUpState();

	if (animatedState) {
		QPixmap pixmap;
		bool didPaint   = false;
		int  shownState = m_stateAnimation.shownState();
		
		if (!reachedUpState) {
			frame->setElementPrefix(NORMAL);
			pixmap   = frame->framePixmap();
			didPaint = true;
		}

		if (shownState & TaskStateAnimation::Minimized &&
				!(reachedUpState & (
				TaskStateAnimation::Hover |
				TaskStateAnimation::Attention |
				TaskStateAnimation::Focus))) {
			frame->setElementPrefix(MINIMIZED);

			if (didPaint) {
				pixmap = Plasma::PaintUtils::transition(
					pixmap, frame->framePixmap(),
					m_stateAnimation.minimized());
			}
			else {
				pixmap   = frame->framePixmap();
				didPaint = true;
			}
		}

		if (shownState & TaskStateAnimation::Focus &&
				!(reachedUpState & (
				TaskStateAnimation::Hover |
				TaskStateAnimation::Attention))) {
			frame->setElementPrefix(FOCUS);

			if (didPaint) {
				pixmap = Plasma::PaintUtils::transition(
					pixmap, frame->framePixmap(),
					m_stateAnimation.focus());
			}
			else {
				pixmap   = frame->framePixmap();
				didPaint = true;
			}
		}

		if (shownState & TaskStateAnimation::Attention &&
				!(reachedUpState & TaskStateAnimation::Hover)) {
			frame->setElementPrefix(ATTENTION);

			if (didPaint) {
				pixmap = Plasma::PaintUtils::transition(
					pixmap, frame->framePixmap(),
					m_stateAnimation.attention());
			}
			else {
				pixmap   = frame->framePixmap();
				didPaint = true;
			}
		}

		if (shownState & TaskStateAnimation::Hover && !(m_applet->lights() && m_applet->onlyLights())) {
			frame->setElementPrefix(HOVER);

			if (didPaint) {
				pixmap = Plasma::PaintUtils::transition(
					pixmap, frame->framePixmap(),
					m_stateAnimation.hover());
			}
			else {
				pixmap   = frame->framePixmap();
				didPaint = true;
			}
		}

		p->drawPixmap(QPoint(0, Plasma::TopMargin), pixmap);
	}
	else {
		if (reachedUpState & TaskStateAnimation::Hover && !(m_applet->lights() && m_applet->onlyLights())) {
			frame->setElementPrefix(HOVER);
		}
		else if (reachedUpState & TaskStateAnimation::Attention) {
			frame->setElementPrefix(ATTENTION);
		}
		else if (reachedUpState & TaskStateAnimation::Focus) {
			frame->setElementPrefix(FOCUS);
		}
		else if (reachedUpState & TaskStateAnimation::Minimized) {
			frame->setElementPrefix(MINIMIZED);
		}
// 		else if (reachedUpState & TaskStateAnimation::Launcher) {
// 			frame->setElementPrefix(MINIMIZED);
// 		}
		else {
			frame->setElementPrefix(NORMAL);
		}

		frame->paintFrame(p);
	}
}

QColor TaskItem::textColor() const {
	QColor color;
	
	Plasma::Theme *theme = Plasma::Theme::defaultTheme();
	int animatedState  = m_stateAnimation.animatedState();
	int reachedUpState = m_stateAnimation.reachedUpState();

	if (animatedState) {
		// TODO: think about whether this makes sense
		//       I think this can be simplified at least
		bool didPaint   = false;
		int  shownState = m_stateAnimation.shownState();
		
		if (!reachedUpState) {
			color = theme->color(Plasma::Theme::TextColor);
			didPaint = true;
		}

		if (shownState & TaskStateAnimation::Minimized &&
				!(reachedUpState & (
				TaskStateAnimation::Hover |
				TaskStateAnimation::Attention |
				TaskStateAnimation::Focus))) {
			if (didPaint) {
				color.setAlphaF(1.0 -
					(1.0 - MINIMIZED_TEXT_ALPHA) *
					m_stateAnimation.minimized());
			}
			else {
				color = theme->color(Plasma::Theme::TextColor);
				color.setAlphaF(MINIMIZED_TEXT_ALPHA);
				didPaint = true;
			}
		}

		if (shownState & TaskStateAnimation::Focus &&
				!(reachedUpState & (
				TaskStateAnimation::Hover |
				TaskStateAnimation::Attention))) {
			if (didPaint) {
				color = KColorUtils::mix(
					color,
					theme->color(Plasma::Theme::TextColor),
					m_stateAnimation.focus());
			}
			else {
				color    = theme->color(Plasma::Theme::TextColor);
				didPaint = true;
			}
		}

		if (shownState & TaskStateAnimation::Attention &&
				!(reachedUpState & TaskStateAnimation::Hover)) {
			if (didPaint) {
				color = KColorUtils::mix(
					color,
					theme->color(Plasma::Theme::ButtonTextColor),
					m_stateAnimation.focus());
			}
			else {
				color    = theme->color(Plasma::Theme::ButtonTextColor);
				didPaint = true;
			}
		}

		if (shownState & TaskStateAnimation::Hover) {
			if (didPaint) {
				color = KColorUtils::mix(
					color,
					theme->color(Plasma::Theme::TextColor),
					m_stateAnimation.focus());
			}
			else {
				color    = theme->color(Plasma::Theme::TextColor);
				didPaint = true;
			}
		}
	}
	else if (reachedUpState & TaskStateAnimation::Hover) {
		color = theme->color(Plasma::Theme::TextColor);
	}
	else if (reachedUpState & TaskStateAnimation::Attention) {
		color = theme->color(Plasma::Theme::ButtonTextColor);
	}
	else if (reachedUpState & TaskStateAnimation::Focus) {
		color = theme->color(Plasma::Theme::TextColor);
	}
	else if (reachedUpState & TaskStateAnimation::Minimized) {
		color = theme->color(Plasma::Theme::TextColor);
		color.setAlphaF(MINIMIZED_TEXT_ALPHA);
	}
	else {
		color = theme->color(Plasma::Theme::TextColor);
	}
	
	return color;
}

void TaskItem::drawText(QPainter *p, qreal marginLeft, qreal marginTop, qreal marginRight, qreal marginBottom) {
	QColor color(textColor());
	p->setPen(QPen(color, 1.0));

	const bool rtl = QApplication::isRightToLeft();
	QTextLayout layout(m_task->text(), KGlobalSettings::taskbarFont());

	QTextOption textOption(layout.textOption());
	textOption.setTextDirection(QApplication::layoutDirection());
	layout.setTextOption(textOption);

	QRectF bounds(boundingRect());

	qreal x, y, width, height;

	if (m_orientation == Qt::Vertical) {
		x = rtl ? marginTop : m_cellSize.width() + 1;
		y = marginLeft;
		width  = bounds.height() - (m_cellSize.width() + 1) - marginTop;
		height = m_cellSize.height() - marginLeft - marginRight;
	}
	else {
		x = rtl ? marginLeft : m_cellSize.width() + 1;
		y = marginTop;
		width  = bounds.width() - (m_cellSize.width() + 1) - marginRight;
		height = m_cellSize.height() - marginTop - marginBottom;
	}
	QRectF textRect(x, y, width, height);
	QSizeF textSize(::SmoothTasks::layoutText(layout, textRect.size()));

	// Hack for plamsa layouts that define a to large margin:
	// This causes the text to be drawn over the painted button border
	// in case the text does not fit the available space.
	// However, the layouting function still uses the space where the
	// margins are subtracted so this should only make a difference
	// when the button is really thin and not even one line fits the
	// available space.
	if (height <= textSize.height()) {
		qreal hackTextSize = textSize.height() + 1.0;
		textRect.setY(y - hackTextSize * 0.5 + height * 0.5);
		textRect.setHeight(hackTextSize);
	}

	drawTextLayout(p, layout, textRect, textSize, color);
}

void TaskItem::drawTextLayout(QPainter *painter,
		const QTextLayout& layout,
		const QRectF&      rect,
		const QSizeF&      textSize,
		const QColor&      color) const {
	if (rect.width() < 1 || rect.height() < 1) {
		return;
	}
	QPixmap pixmap(std::ceil(rect.width()), std::ceil(rect.height()));
	pixmap.fill(Qt::transparent);

	QPainter p(&pixmap);
	p.setPen(painter->pen());

	// expander measures:
	QRectF expRect(expanderRect(QRectF(QPointF(0, 0), rect.size())));
	QRectF textRect(rect.x(), rect.y(), rect.width() - expRect.width(), rect.height());

	const bool rtl = layout.textOption().textDirection() == Qt::RightToLeft;
	if (rtl) {
		textRect.moveLeft(expRect.right());
	}

	QFontMetrics fm(layout.font());
	QPointF position(0, (textRect.height() - textSize.height()) * 0.5 +
		(fm.tightBoundingRect(M).height() - fm.xHeight()) * 0.5);

	QLinearGradient alphaGradient(0, 0, 1, 0);
	alphaGradient.setCoordinateMode(QGradient::ObjectBoundingMode);
	if (rtl) {
		alphaGradient.setColorAt(0, QColor(0, 0, 0, 0));
		alphaGradient.setColorAt(1, QColor(0, 0, 0, 255));
	}
	else {
		alphaGradient.setColorAt(0, QColor(0, 0, 0, 255));
		alphaGradient.setColorAt(1, QColor(0, 0, 0, 0));
	}

	QList<QRect> fadeRects;
	const int fadeWidth = 30;
	// Draw each line in the layout
	for (int i = 0; i < layout.lineCount(); ++ i) {
		QTextLine line(layout.lineAt(i));
		QPointF linePos(position);
		qreal textWidth = line.naturalTextWidth();

		if (rtl) {
			linePos.setX(textRect.left() + textRect.width() - textWidth);
		}

		line.draw(&p, linePos);
		// Add a fade out rect to the list if the line is too long
		if (textWidth > textRect.width()) {
			int x, y = int(line.position().y() + linePos.y());
			if (rtl) {
				x = int(expRect.left() + qMax(qreal(0.0), textRect.width() - textWidth));
			}
			else {
				x = int(std::ceil(qMin(textWidth, expRect.left()))) - fadeWidth;
			}
			fadeRects.append(QRect(x, y, fadeWidth, (int) std::ceil(line.height())));
		}
	}
	// Reduce the alpha in each fade out rect using the alpha gradient
	if (!fadeRects.isEmpty()) {
		p.setCompositionMode(QPainter::CompositionMode_DestinationIn);
		foreach (const QRect &r, fadeRects) {
			p.fillRect(r, alphaGradient);
		}
		if (!expRect.isEmpty()) {
			if (rtl) {
				p.fillRect(0, 0, (int) expRect.width(), pixmap.height(), Qt::transparent);
			}
			else {
				p.fillRect((int) expRect.left(), 0,
					pixmap.width() - (int) expRect.left(),
					pixmap.height(), Qt::transparent);
			}
		}
		p.setCompositionMode(QPainter::CompositionMode_SourceOver);
	}
	drawExpander(&p, expRect);
	p.end();
	
	if (m_applet->textShadow()) {
		QImage shadow(pixmap.toImage());
		Plasma::PaintUtils::shadowBlur(shadow, 2, (color.value() < 128 ? Qt::white : Qt::black));
		painter->drawImage(rect.topLeft() + QPointF(1, 2), shadow);
	}
	painter->drawPixmap(rect.topLeft(), pixmap);
}

/** initially copied from KDEs Task plasmoid but substantially altered */
void TaskItem::drawExpander(QPainter *painter, const QRectF& expRect) const {
	if (m_task->type() == Task::GroupItem) {
		QFont font(KGlobalSettings::smallestReadableFont());
		QFontMetrics fm(font);
		
		Plasma::FrameSvg *itemBackground = m_applet->frame();
		const QString& expElem(expanderElement());
		QRectF textRect(expRect.left(), expRect.top(), expRect.width(), fm.height());
		
		if (itemBackground->hasElement(expElem)) {
			QSizeF arrowSize(itemBackground->elementSize(expElem));
			QRectF arrowRect(
				expRect.left() + expRect.width() * 0.5 - arrowSize.width() * 0.5,
				expRect.top(),
				arrowSize.width(), arrowSize.height());
			
			switch (m_applet->location()) {
				case Plasma::TopEdge:
				case Plasma::LeftEdge:
					arrowRect.moveTop(arrowRect.top() + fm.height() + fm.leading());
					break;
				case Plasma::BottomEdge:
				case Plasma::RightEdge:
					textRect.moveTop(textRect.top() + arrowSize.height());
					break;
				default:
					if (m_orientation == Qt::Vertical) {
						arrowRect.moveTop(arrowRect.top() + fm.height() + fm.leading());
					}
					else {
						textRect.moveTop(textRect.top() + arrowSize.height());
					}
			}
			itemBackground->paint(painter, arrowRect, expElem);
		}
		else {
			textRect.moveTop(expRect.top() + expRect.height() * 0.5 - textRect.height() * 0.5);
		}
		
		painter->setFont(font);
		painter->drawText(textRect, Qt::AlignHCenter | Qt::AlignTop, QString::number(m_task->taskCount()));
	}
}

/** initially copied from KDEs Task plasmoid but substantially altered */
QRectF TaskItem::expanderRect(const QRectF &bounds) const {
	if (m_task->type() != Task::GroupItem) {
		if (QApplication::isLeftToRight()) {
			return QRectF(bounds.right(), bounds.top(), 0, 0);
		}
		else {
			return QRectF(bounds.left(), bounds.top(), 0, 0);
		}
	}

	QFontMetrics fm(KGlobalSettings::smallestReadableFont());
	Plasma::FrameSvg *itemBackground = m_applet->frame();

	QSize arrowSize(itemBackground->elementSize(expanderElement()));
	QString text(QString::number(m_task->taskCount()));
	
	int   width  = qMax(fm.width(text), arrowSize.width());
	int   height = fm.height() + fm.leading() + arrowSize.height();
	qreal top    = bounds.top() + bounds.height() * 0.5 - height * 0.5;
	
	if (QApplication::isRightToLeft()) {
		return QRectF(bounds.left(), top, width, height);
	}
	else {
		return QRectF(bounds.right() - width, top, width, height);
	}
}

const QString& TaskItem::expanderElement() const {
	// because of the rotation we never need _LEFT or _RIGHT
	switch (m_applet->location()) {
	case Plasma::TopEdge:
	case Plasma::LeftEdge:
		return GROUP_EXPANDER_TOP;
	case Plasma::BottomEdge:
	case Plasma::RightEdge:
		return GROUP_EXPANDER_BOTTOM;
	default:
		if (m_orientation == Qt::Vertical) {
			return GROUP_EXPANDER_TOP;
		}
		else {
			return GROUP_EXPANDER_BOTTOM;
		}
	}
}

} // namespace SmoothTasks
#include "TaskItem.moc"
