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

#include "SmoothTasks/WindowPreview.h"
#include "SmoothTasks/SmoothToolTip.h"
#include "SmoothTasks/CloseIcon.h"
#include "SmoothTasks/Task.h"
#include "SmoothTasks/Global.h"

// Qt
#include <QFontInfo>
#include <QPainter>
#include <QTextLayout>
#include <QGraphicsView>

// KDE
#include <KIcon>
#include <KIconEffect>
#include <KWindowSystem>
#include <Plasma/PaintUtils>

// taskmanager
#include <taskmanager/taskmanager.h>
#include <taskmanager/abstractgroupableitem.h>
#include <taskmanager/groupmanager.h>
#include <taskmanager/taskitem.h>
#include <taskmanager/startup.h>
#include <taskmanager/taskactions.h>

namespace SmoothTasks {

const QSize WindowPreview::BIG_ICON_SIZE(48, 48);
const QSize WindowPreview::SMALL_ICON_SIZE(16, 16);

WindowPreview::WindowPreview(
		TaskManager::TaskItem *task,
		int index,
		SmoothToolTip *toolTip)
	: QWidget(),
	  m_background(new Plasma::FrameSvg(this)),
	  m_iconSpace(NULL),
	  m_previewSpace(NULL),
	  m_highlite(),
	  m_task(new Task(task, this)),
	  m_toolTip(toolTip),
	  m_previewSize(0, 0),
	  m_hover(false),
	  m_index(index),
	  m_activateTimer(NULL),
	  m_didPress(false),
	  m_dragStartPosition() {
	setMouseTracking(true);
	setAcceptDrops(true);

//	m_background->setImagePath("widgets/button");
	m_background->setImagePath("widgets/tasks");
	m_background->setElementPrefix(NORMAL);
	m_background->setCacheAllRenderedFrames(true);

	setPreviewSize();

	// placeholder for preview:
	if (toolTip->previewsAvailable()) {
		m_previewSpace = new QSpacerItem(
			0, 0,
			QSizePolicy::Minimum,
			QSizePolicy::Minimum);
	}

	switch (m_toolTip->applet()->previewLayout()) {
	case Applet::NewPreviewLayout:
		setNewLayout();
		break;
	case Applet::ClassicPreviewLayout:
	default:
		setClassicLayout();
	}
	
	setMaximumWidth(260);
	updateTheme();
	
	connect(m_background, SIGNAL(repaintNeeded()), this, SLOT(updateTheme()));
	connect(
		task, SIGNAL(changed(::TaskManager::TaskChanges)),
		this, SLOT(updateTask(::TaskManager::TaskChanges)));

	connect(&m_highlite, SIGNAL(animate(qreal)), this, SLOT(repaint()));
}

WindowPreview::~WindowPreview() {
	if (m_activateTimer) {
		m_activateTimer->stop();
		delete m_activateTimer;
		m_activateTimer = NULL;
	}
}

void WindowPreview::setPreviewSize() {
	if (m_toolTip->previewsAvailable()) {
		// determine preview size:
		WId wid = 0;
		TaskManager::Task* task = m_task->task();
		
		if (task) {
			wid = task->window();
		}

	if (wid && m_task->type() != Task::StartupItem && m_task->type() != Task::LauncherItem) {
			m_previewSize = KWindowSystem::windowInfo(wid,
				NET::WMGeometry | NET::WMFrameExtents).frameGeometry().size();
		}
		else {
			m_previewSize = m_task->icon().pixmap(BIG_ICON_SIZE).size();
		}
	}
	else {
		m_previewSize = QSize(0, 0);
	}
	
	if (m_previewSize.isValid()) {
		int maxSize = m_toolTip->applet()->maximumPreviewSize();
		if (m_previewSize.width() > maxSize || m_previewSize.height() > maxSize) {
			m_previewSize.scale(maxSize, maxSize, Qt::KeepAspectRatio);
		}
	}
}

void WindowPreview::updateTask(::TaskManager::TaskChanges changes) {
	bool doUpdate = false;
	QSize oldSize = size();

	if (changes & TaskManager::IconChanged) {
		const KIcon icon(m_task->icon());

		if (m_toolTip->previewsAvailable() && (m_task->type() == Task::StartupItem || m_task->type() == Task::LauncherItem)) {
			m_previewSize = icon.pixmap(BIG_ICON_SIZE).size();
		}
	
		switch (m_toolTip->applet()->previewLayout()) {
		case Applet::NewPreviewLayout:
			m_icon = icon.pixmap(BIG_ICON_SIZE);
			break;
		case Applet::ClassicPreviewLayout:
		default:
			m_icon = icon.pixmap(SMALL_ICON_SIZE);
		}
		
		doUpdate = true;
	}

	if (changes & TaskManager::NameChanged) {
		m_taskNameLabel->setText(m_task->text());
		doUpdate = true;
	}

	if (changes & TaskManager::GeometryChanged) {
		setPreviewSize();
		doUpdate = true;
	}

	if (changes & TaskManager::NameChanged) {
		m_taskNameLabel->setText(m_task->text());
		doUpdate = true;
	}
	
	if (doUpdate) {
		updateTheme();

		if (size() != oldSize) {
			emit sizeChanged();
		}
	}
}

void WindowPreview::setClassicLayout() {
	QGridLayout *layout = new QGridLayout;
	layout->setSpacing(3);
	layout->setContentsMargins(0, 0, 0, 0);
	setLayout(layout);

	// task icon:
	m_iconSpace = new QSpacerItem(
		20, 20,
		QSizePolicy::Fixed,
		QSizePolicy::Fixed);
	m_icon = m_task->icon().pixmap(16, 16);
	layout->addItem(m_iconSpace, 0, 0, 1, 1);
	
	// task name:
	m_taskNameLabel = new FadedText(m_task->text(), this);
	m_taskNameLabel->setShadow(m_toolTip->applet()->textShadow());
	m_taskNameLabel->setFps(m_toolTip->applet()->fps());
	m_taskNameLabel->setMouseTracking(true);
	m_taskNameLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum);
	layout->addWidget(m_taskNameLabel, 0, 1, 1, 1);

	if(m_task->type() != Task::LauncherItem) { // no window preview or close icon for launcher item
		// close icon:
		CloseIcon *iconClose = new CloseIcon(this);
		iconClose->setMouseTracking(true);
		iconClose->setFixedSize(20, 20);
		layout->addWidget(iconClose, 0, 2, 1, 1);
	}

	// placeholder for preview:
	if (m_previewSpace) {
		layout->addItem(m_previewSpace, 1, 0, 1, 3);
		layout->setRowStretch(1, 100);
	}
}

void WindowPreview::setNewLayout() {
	int fps = m_toolTip->applet()->fps();
	QGridLayout *layout = new QGridLayout;
	layout->setSpacing(3);
	layout->setContentsMargins(8, 8, 8, 8);
	setLayout(layout);
	
	layout->setColumnStretch(1, 100);
	// placeholder for preview:
	if (m_previewSpace) {
		layout->addItem(m_previewSpace, 0, 0, 1, 2);
		layout->setRowStretch(0, 100);
	}
		
	if(m_task->type() != Task::LauncherItem) { // no window preview or close icon for launcher item
		// close icon:
		CloseIcon *iconClose = new CloseIcon(this);
		iconClose->setMouseTracking(true);
		iconClose->setFixedSize(20, 20);
		if(m_previewSpace) {
			layout->addWidget(iconClose, 0, 2, 1, 1, Qt::AlignTop | Qt::AlignRight);
		} else {
			layout->addWidget(iconClose, 0, 3, 1, 1, Qt::AlignTop | Qt::AlignRight);
		}
	}
	
	// task icon:
	m_iconSpace = new QSpacerItem(
		52, 52,
		QSizePolicy::Fixed,
		QSizePolicy::Fixed);
	m_icon = m_task->icon().pixmap(BIG_ICON_SIZE);
	if(m_previewSpace) {
		layout->addItem(m_iconSpace, 1, 0, 2, 1, Qt::AlignCenter);
	} else {
		layout->addItem(m_iconSpace, 0, 0, 2, 1, Qt::AlignCenter);
	}
	
	// task name:
	m_taskNameLabel = new FadedText(m_task->text(), this);
	m_taskNameLabel->setShadow(m_toolTip->applet()->textShadow());
	m_taskNameLabel->setFps(fps);
	QFont font(m_taskNameLabel->font());
	font.setBold(true);
	m_taskNameLabel->setMouseTracking(true);
	m_taskNameLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum);
	m_taskNameLabel->setFont(font);
	if(m_previewSpace) {
		layout->addWidget(m_taskNameLabel, 1, 1, 1, 2);
	} else {
		layout->addWidget(m_taskNameLabel, 0, 1, 1, 2);
	}

	// task desktop:
	FadedText *desktopLabel = new FadedText(m_task->description());

	desktopLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum);
	desktopLabel->setShadow(m_toolTip->applet()->textShadow());
	desktopLabel->setFps(fps);
	desktopLabel->setWrapMode(QTextOption::WordWrap);
	desktopLabel->setMouseTracking(true);
	desktopLabel->setMaximumWidth(160);
	if(m_previewSpace) {
		layout->addWidget(desktopLabel, 2, 1, 1, 2, Qt::AlignTop);
	} else {
		layout->addWidget(desktopLabel, 1, 1, 1, 2, Qt::AlignTop);
	}
}

void WindowPreview::highlightTask() {
	TaskManager::Task* task = m_task->task();
	
	if (task) {
		m_toolTip->highlightTask(task->window());
	}
}

QRect WindowPreview::previewRect(int x, int y) const {
	if (m_previewSpace) {
		QRect spaceGeom(m_previewSpace->geometry());
		QRect rect(
			x + spaceGeom.left() + (spaceGeom.width()  - m_previewSize.width())  / 2,
			y + spaceGeom.top()  + (spaceGeom.height() - m_previewSize.height()) / 2,
			m_previewSize.width(),
			m_previewSize.height());

		return rect;
	}
	else {
		return QRect();
	}
}

void WindowPreview::activateTask() {
	m_highlite.stop();
	m_toolTip->hide();
	
        if(m_task->type() != Task::LauncherItem) {
                TaskManager::Task* task = m_task->task();
                if (task) {
                        task->activate();
                }
                else {
                        qWarning("WindowPreview::activateTask: Bug: the task is gone but the task item is still here!");
                }
        }
}

void WindowPreview::activateForDrop() {
	TaskManager::Task* task = m_task->task();
	if (task) {
		if (task->isMinimized()) {
			task->restore();
		}
		task->raise();
		m_toolTip->hide();
	}
}

void WindowPreview::closeTask() {
	TaskManager::Task* task = m_task->task();
	if (task) {
		task->close();
	}
	else {
		qWarning("WindowPreview::closeTask: Bug: the task is gone but the task item is still here!");
		m_toolTip->applet()->reload();
	}
}

QPixmap WindowPreview::hoverIcon() const {
	KIconEffect *effect = KIconLoader::global()->iconEffect();
	if (effect->hasEffect(KIconLoader::Desktop, KIconLoader::ActiveState)) {
		return effect->apply(
			m_icon,
			KIconLoader::Desktop,
			KIconLoader::ActiveState);
	}
	else {
		return m_icon;
	}
}

void WindowPreview::paintEvent(QPaintEvent *event) {
	Q_UNUSED(event)
	QPainter painter(this);

	// draw background (only when previews are used)
	if (m_previewSpace) {
		QPixmap  backgroundPixmap;
		qreal    normalLeft = 0, normalTop = 0, normalRight = 0, normalBottom = 0;

		// caching of the pixmap is done by the class FrameSvg
		m_background->setElementPrefix(NORMAL);
		m_background->getMargins(normalLeft, normalTop, normalRight, normalBottom);

		if (m_highlite.atBottom()) {
			backgroundPixmap = m_background->framePixmap();
		}
		else if (m_highlite.atTop()) {
			m_background->setElementPrefix(HOVER);
			backgroundPixmap = m_background->framePixmap();
		}
		else {
			QPixmap normal(m_background->framePixmap());
	
			m_background->setElementPrefix(HOVER);
			QPixmap hover(m_background->framePixmap());
				
			backgroundPixmap = Plasma::PaintUtils::transition(normal, hover, m_highlite.value());
		}
		QRect  spaceGeom(m_previewSpace->geometry());
		QPoint backgroundPos(
			spaceGeom.left() + (spaceGeom.width()  - m_previewSize.width())  / 2 - normalLeft,
			spaceGeom.top()  + (spaceGeom.height() - m_previewSize.height()) / 2 - normalTop);

		painter.drawPixmap(backgroundPos, backgroundPixmap);
		
		// draw icon as fake preview for startup items
		if (m_task->type() == Task::StartupItem) {
			painter.drawPixmap(previewRect(0, 0), m_task->icon().pixmap(BIG_ICON_SIZE));
		}
	}
	
	// draw icon
	QPixmap  iconPixmap;
	QRect    iconGeom(m_iconSpace->geometry());
	QPointF  iconPos(
		iconGeom.left() + (iconGeom.width()  - m_icon.width())  * 0.5,
		iconGeom.top()  + (iconGeom.height() - m_icon.height()) * 0.5);

	if (m_highlite.atBottom()) {
		iconPixmap = m_icon;
	}
	else if (m_highlite.atTop()) {
		iconPixmap = hoverIcon();
	}
	else {
		iconPixmap = Plasma::PaintUtils::transition(m_icon, hoverIcon(), m_highlite.value());
	}
	painter.drawPixmap(iconPos, iconPixmap);
}

void WindowPreview::enterEvent(QEvent *event) {
	Q_UNUSED(event)
	hoverEnter();
}

void WindowPreview::leaveEvent(QEvent *event) {
	Q_UNUSED(event)
	hoverLeave();
}

void WindowPreview::hoverEnter() {
	Applet *applet = m_toolTip->applet();
	m_highlite.startUp(applet->fps(), applet->animationDuration());
	m_hover = true;
	emit enter(this);

	update();
}

void WindowPreview::hoverLeave() {
	Applet *applet = m_toolTip->applet();
	m_highlite.startDown(applet->fps(), applet->animationDuration());
	m_hover = false;
	if (m_activateTimer) {
		delete m_activateTimer;
		m_activateTimer = NULL;
	}
	emit leave(this);

	update();
}

void WindowPreview::mousePressEvent(QMouseEvent *event) {
	m_didPress = true;
	m_dragStartPosition = event->pos();
	event->accept();
}

/* FIXME: why doesn't this work?
void WindowPreview::mouseMoveEvent(QMouseEvent *event) {
	if (!(event->buttons() & Qt::LeftButton)) {
		return;
	}
	
	if ((event->pos() - m_dragStartPosition).manhattanLength()
		< QApplication::startDragDistance()) {
		return;
	}
	
	//m_didPress = false;
	event->ignore();
	m_toolTip->applet()->dragTask(m_task, this);
}
*/

void WindowPreview::mouseReleaseEvent(QMouseEvent *event) {
	if (event->x() < 0 || event->y() < 0 || event->x() >= width() || event->y() >= height()) {
		event->ignore();
	}
	else if (m_didPress) {
		switch (event->button()) {
		case Qt::LeftButton:
			activateTask();
			break;
		case Qt::RightButton:
			m_toolTip->popup(QCursor::pos(), m_task);
			break;
		case Qt::MidButton:
			if (m_task->isValid()) {
				m_toolTip->applet()->middleClickTask(m_task->abstractItem());
			}
		default:
			break;
		}
		event->accept();
	}
	else {
		event->ignore();
	}
	m_didPress = false;
}

void WindowPreview::dragEnterEvent(QDragEnterEvent *event) {
	hoverEnter();
	if (m_activateTimer == NULL) {
		m_activateTimer = new QTimer(this);
		m_activateTimer->setSingleShot(true);
		m_activateTimer->setInterval(DRAG_HOVER_DELAY);
		connect(m_activateTimer, SIGNAL(timeout()), this, SLOT(activateForDrop()));
	}
	m_activateTimer->start();
	event->ignore();
}

void WindowPreview::dragMoveEvent(QDragMoveEvent *event) {
	if (m_activateTimer) {
		m_activateTimer->start();
	}
	event->accept();
}

void WindowPreview::dragLeaveEvent(QDragLeaveEvent *event) {
	hoverLeave();
	event->accept();
}

void WindowPreview::dropEvent(QDropEvent *event) {
	if (m_activateTimer) {
		delete m_activateTimer;
		m_activateTimer = NULL;
	}
	event->accept();
}

void WindowPreview::updateTheme() {
	QLayout *layout = this->layout();
	m_background->clearCache();

	// frame around preview:
	qreal hoverLeft  = 0, hoverTop  = 0, hoverRight  = 0, hoverBottom  = 0;
	qreal normalLeft = 0, normalTop = 0, normalRight = 0, normalBottom = 0;

	m_background->setElementPrefix(HOVER);
	m_background->getMargins(hoverLeft, hoverTop, hoverRight, hoverBottom);
	
	m_background->setElementPrefix(NORMAL);
	m_background->getMargins(normalLeft, normalTop, normalRight, normalBottom);

	qreal left   = qMax(normalLeft,   hoverLeft);
	qreal top    = qMax(normalTop,    hoverTop);
	qreal right  = qMax(normalRight,  hoverRight);
	qreal bottom = qMax(normalBottom, hoverBottom);

	QSizeF normalSize(
		normalLeft + m_previewSize.width()  + normalRight,
		normalTop  + m_previewSize.height() + normalBottom);

	m_background->setElementPrefix(HOVER);
//	m_background->resizeFrame(QSizeF(
//		hoverLeft + m_previewSize.width()  + hoverRight,
//		hoverTop  + m_previewSize.height() + hoverBottom));
	m_background->resizeFrame(normalSize);

	m_background->setElementPrefix(NORMAL);
	m_background->resizeFrame(normalSize);

	// placeholder for preview:
	if (m_previewSpace) {
		m_previewSpace->changeSize(
			left + m_previewSize.width()  + right,
			top  + m_previewSize.height() + bottom,
			QSizePolicy::Minimum,
			QSizePolicy::Minimum);
		
		m_previewSpace->invalidate();
	}
	
	
	layout->invalidate();
	layout->activate();
	update();
	
	if (KWindowSystem::compositingActive()) {
		if (m_toolTip->applet()->previewLayout() == Applet::NewPreviewLayout) {
			m_taskNameLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum);
		}
		else {
			m_taskNameLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
		}
	}
	else {
		m_taskNameLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
	}
	adjustSize();
}

} // namespace SmoothTasks
