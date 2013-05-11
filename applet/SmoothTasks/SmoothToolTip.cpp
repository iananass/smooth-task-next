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

#include "SmoothTasks/SmoothToolTip.h"
#include "SmoothTasks/ToolTipWidget.h"
#include "SmoothTasks/WindowPreview.h"
#include "SmoothTasks/TaskItem.h"
#include "SmoothTasks/Task.h"

#include <Plasma/Theme>
#include <Plasma/IconWidget>
#include <Plasma/WindowEffects>
#include <KIcon>
#include <KIconLoader>
#include <KIconEffect>

#include <QTimer>
#include <QPushButton>
#include <QBoxLayout>
#include <QGraphicsLinearLayout>
#include <QToolButton>
#include <QGraphicsView>
#include <QDesktopWidget>

namespace SmoothTasks {

SmoothToolTip::SmoothToolTip(Applet *applet)
	: DelayedToolTip(applet),
	  m_widget(new ToolTipWidget(this)),
	  m_previews(),
	  m_previewsAvailable(false),
	  m_background(new Plasma::FrameSvg(this)),
	  m_hover(false),
	  m_menuShown(false),
	  m_previewsUpdated(false),
	  m_hoverPreview(NULL),
	  m_highlightDelay(new QTimer(this)),
	  m_highlighting(false),
	  m_scrollAnimation(0),
	  m_dx(0),
	  m_xStart(0),
	  m_dy(0),
	  m_yStart(0),
	  m_moveAnimation(false),
	  m_moveAnimationUpdated(false),
	  m_position(0),
	  m_size(0, 0),
	  m_closeIcon(),
	  m_hoverCloseIcon() {
	
	connect(
		Plasma::Animator::self(), SIGNAL(customAnimationFinished(int)),
		this, SLOT(animationFinished(int)));

	connect(
		applet, SIGNAL(mouseEnter()),
		this, SLOT(stopEffect()));

	previewLayoutChanged(m_applet->previewLayout());

	m_background->setImagePath("widgets/tooltip");
	m_background->setEnabledBorders(Plasma::FrameSvg::AllBorders);

	updateTheme();

	m_highlightDelay->setInterval(m_applet->highlightDelay());
	m_highlightDelay->setSingleShot(true);
	
	connect(m_highlightDelay, SIGNAL(timeout()), this, SLOT(highlightDelayTimeout()));
	connect(m_background, SIGNAL(repaintNeeded()), this, SLOT(updateTheme()));
	connect(
		m_applet, SIGNAL(previewLayoutChanged(Applet::PreviewLayoutType)),
		this, SLOT(previewLayoutChanged(Applet::PreviewLayoutType)));
}

SmoothToolTip::~SmoothToolTip() {
	stopScrollAnimation(true);
	m_widget->hide();
	delete m_widget;
	m_widget = NULL;
}

void SmoothToolTip::previewLayoutChanged(Applet::PreviewLayoutType previewLayout) {
	QLayout *layout = m_widget->layout();
	switch (previewLayout) {
		case Applet::NewPreviewLayout:
			layout->setSpacing(0);
			break;
		case Applet::ClassicPreviewLayout:
		default:
			layout->setSpacing(3);
	}
	layout->activate();
}

void SmoothToolTip::showAction(bool animate) {
	updateToolTip(animate);
	m_widget->show();
}

void SmoothToolTip::itemUpdate(TaskItem *item) {
	if (item == m_hoverItem && m_shown) {
		updateToolTip(true);
	}
}

void SmoothToolTip::updateToolTip(bool forceAnimated) {
	m_previewsAvailable = Plasma::WindowEffects::isEffectAvailable(
		Plasma::WindowEffects::WindowPreview);

	m_widget->hide();

	m_previewsUpdated = false;
//	m_hover           = false; // XXX really?

	Task *task = m_hoverItem->task();

	m_widget->setUpdatesEnabled(false);
	clear();
	switch (task->type()) {
	case Task::TaskItem:
	case Task::StartupItem:
		setTasks(TaskManager::ItemList() << task->taskItem());
		break;
	case Task::GroupItem:
		setTasks(task->group()->members());
		break;
	case Task::LauncherItem:
		m_previewsAvailable = false;
		setTasks(TaskManager::ItemList() << task->launcherItem());
		break;
	default:
		setTasks(TaskManager::ItemList());
	}
	m_widget->setUpdatesEnabled(true);

	moveBesideTaskItem(forceAnimated);
}

void SmoothToolTip::hideAction() {
	if (!m_hover && !m_menuShown) {
		hide();
	}
}

void SmoothToolTip::moveBesideTaskItem(bool forceAnimated) {
	if (m_hoverItem.isNull()) {
		return;
	}
	int newPosition = 0;
	m_widget->adjustSize();
	QSize newSize(m_widget->frameSize());
	QPoint pos(m_hoverItem->popupPosition(newSize, true, &newPosition));
	
	if (pos == m_widget->pos() && m_scrollAnimation == 0) {
		if (forceAnimated) {
			m_shown = true;
			m_widget->show();
		}
		return;
	}

	int xStart = m_widget->x();
	int yStart = m_widget->y();

	if (m_scrollAnimation && m_moveAnimation) {
		m_moveAnimationUpdated = true;
		if (forceAnimated) {
			m_shown = true;
			m_widget->show();
		}
		return;
	}
	
	if (forceAnimated) {
		m_moveAnimation = true;

		if (m_position & ToolTipBase::Top) {
			yStart -= newSize.height() - m_size.height();
		}
		else if (m_position & ToolTipBase::Middle) {
			yStart -= (newSize.height() - m_size.height()) / 2;
		}
		int dy = pos.y() - yStart;

		if (m_position & ToolTipBase::Left) {
			xStart -= newSize.width() - m_size.width();
		}
		else if (m_position & ToolTipBase::Center) {
			xStart -= (newSize.width() - m_size.width()) / 2;
		}
		int dx = pos.x() - xStart;

		m_position = newPosition;
		m_size     = newSize;
		m_widget->move(xStart, yStart);

		m_shown = true;
		m_widget->show();

		startScrollAnimation(dx, dy, m_applet->toolTipMoveDuraton());
	}
	else {
		m_position = newPosition;
		m_size     = newSize;
		m_widget->move(pos);
	}
}

void SmoothToolTip::moveTo(WindowPreview *preview, const QPoint& mousePos) {
	const QRect screenGeom(m_applet->currentScreenGeometry());
	const QPoint offset(preview->geometry().center() - mousePos);

	if (qobject_cast<QBoxLayout*>(m_widget->layout())->direction() == QBoxLayout::TopToBottom) {
		const int screenTop    = screenGeom.top();
		const int screenBottom = screenTop + screenGeom.height();
		const int top          = m_widget->y() + preview->y() - offset.y();
		const int bottom       = top + preview->height();
		
		if (top < screenTop) {
			startScrollAnimation(0, screenTop - top - offset.y(), m_applet->toolTipMoveDuraton());
		}
		else if (bottom > screenBottom) {
			startScrollAnimation(0, screenBottom - bottom - offset.y(), m_applet->toolTipMoveDuraton());
		}
		else {
			startScrollAnimation(0, -offset.y(), m_applet->toolTipMoveDuraton());
		}
	}
	else {
		const int screenLeft  = screenGeom.left();
		const int screenRight = screenLeft + screenGeom.width();
		const int left        = m_widget->x() + preview->x() - offset.x();
		const int right       = left + preview->width();
		
		if (left < screenLeft) {
			startScrollAnimation(screenLeft - left - offset.x(), 0, m_applet->toolTipMoveDuraton());
		}
		else if (right > screenRight) {
			startScrollAnimation(screenRight - right - offset.x(), 0, m_applet->toolTipMoveDuraton());
		}
		else {
			startScrollAnimation(-offset.x(), 0, m_applet->toolTipMoveDuraton());
		}
	}
}

void SmoothToolTip::startScrollAnimation(int dx, int dy, int duration) {
	if (m_scrollAnimation) {
		Plasma::Animator::self()->stopCustomAnimation(m_scrollAnimation);
		m_scrollAnimation = 0;
	}

	int frames = m_applet->fps() * duration / 1000; 
	m_dx     = dx;
	m_xStart = m_widget->x();
	m_dy     = dy;
	m_yStart = m_widget->y();

	if (frames <= 0) {
		m_scrollAnimation = 0;
		animateScroll(1.0);
		animationFinished(m_scrollAnimation);
	}
	else {
		m_scrollAnimation = Plasma::Animator::self()->customAnimation(
			frames, duration,
			Plasma::Animator::LinearCurve,
			this, "animateScroll");
	}
}

void SmoothToolTip::animationFinished(int animation) {
	if (animation == m_scrollAnimation) {
		m_scrollAnimation = 0;
		m_dx              = 0;
		m_xStart          = 0;
		m_dy              = 0;
		m_yStart          = 0;
		m_moveAnimation   = false;

		if (m_moveAnimationUpdated) {
			m_moveAnimationUpdated = false;
			moveBesideTaskItem(m_shown);
		}
	}
}

void SmoothToolTip::stopScrollAnimation(bool force) {
	if (force || !m_moveAnimation) {
		if (m_scrollAnimation) {
			Plasma::Animator::self()->stopCustomAnimation(m_scrollAnimation);
		}
		m_scrollAnimation      = 0;
		m_dx                   = 0;
		m_xStart               = 0;
		m_dy                   = 0;
		m_yStart               = 0;
		m_moveAnimation        = false;
		m_moveAnimationUpdated = false;
	}
}

void SmoothToolTip::animateScroll(qreal progress) {
	int x = m_xStart + (int)(progress * m_dx);
	int y = m_yStart + (int)(progress * m_dy);
	m_widget->move(x, y);
	
	if (m_moveAnimationUpdated && !m_hoverItem.isNull()) {
		QPoint pos = m_hoverItem->popupPosition(m_widget->size(), true, NULL);
	
		int dx = pos.x() - x;
		int dy = pos.y() - y;

		if ((dx <= 0.0 && m_dx >= 0.0) || (dx >= 0.0 && m_dx <= 0.0) ||
			(dy <= 0.0 && m_dy >= 0.0) || (dy >= 0.0 && m_dy <= 0.0)) {
			stopScrollAnimation(true);
			moveBesideTaskItem(m_shown);
		}
	}
}

void SmoothToolTip::leave() {
	m_hover        = false;
	m_hoverPreview = NULL;

	stopScrollAnimation();

	itemLeave(m_hoverItem);
}

void SmoothToolTip::enterWindowPreview(WindowPreview *preview) {
	if (m_hoverPreview != NULL) {
		m_hoverPreview->hoverLeave();
	}
	if (m_hoverPreview != preview) {
		m_hoverPreview = preview;
		
		if (m_highlighting) {
			highlightDelayTimeout();
		}
		else {
			m_highlightDelay->start(m_applet->highlightDelay());
		}
	}
}

void SmoothToolTip::leaveWindowPreview(WindowPreview *preview) {
	if (m_hoverPreview == preview) {
		m_hoverPreview = NULL;
		m_highlightDelay->stop();
	}
}

void SmoothToolTip::highlightDelayTimeout() {
	if (m_hoverPreview != NULL) {
		m_hoverPreview->highlightTask();
	}
}

void SmoothToolTip::stopEffect() {
	Plasma::WindowEffects::highlightWindows(
		m_widget->winId(), QList<WId>());
	m_highlighting = false;
}

void SmoothToolTip::clear() {
	stopScrollAnimation(true);

	Plasma::WindowEffects::showWindowThumbnails(m_widget->winId());

	m_hoverPreview = NULL;
	QBoxLayout *layout = qobject_cast<QBoxLayout*>(m_widget->layout());

	foreach (WindowPreview* preview, m_previews) {
		preview->hide();
		layout->removeWidget(preview);
		delete preview;
	}
	m_previews.clear();
}

bool SmoothToolTip::isVertical() const {
	return m_applet->formFactor() == Plasma::Vertical || !KWindowSystem::compositingActive();
}

void SmoothToolTip::setTasks(TaskManager::ItemList tasks) {
	QBoxLayout *layout = qobject_cast<QBoxLayout*>(m_widget->layout());
	const int N = tasks.count();
	int actualWidth  = 0;
	int actualHeight = 0;

	layout->setDirection(isVertical() ?
		QBoxLayout::TopToBottom : QBoxLayout::LeftToRight);

	for (int i = 0; i < N; ++ i) {
		TaskManager::TaskItem *task = static_cast<TaskManager::TaskItem*>(tasks.at(i));

		if (task == NULL) {
			continue;
		}

		WindowPreview *preview = new WindowPreview(task, i, this);
		
		actualWidth += preview->width();
		if (actualHeight < preview->height()) {
			actualHeight = preview->height();
		}

		layout->addWidget(preview);
		
		connect(
			preview, SIGNAL(sizeChanged()),
			this, SLOT(previewWindowSizeChanged()));

		connect(
			preview, SIGNAL(enter(WindowPreview*)),
			this, SLOT(enterWindowPreview(WindowPreview*)));
		
		connect(
			preview, SIGNAL(leave(WindowPreview*)),
			this, SLOT(leaveWindowPreview(WindowPreview*)));

		m_previews.append(preview);
	}

//	layout->update();
	layout->activate();
	m_widget->adjustSize();
	m_previewsUpdated = false;
}

void SmoothToolTip::previewWindowSizeChanged() {
	if (m_hoverItem.isNull()) {
		kDebug() << "previewWindowSizeChanged() but no m_hoverItem";
	}
	else {
		moveBesideTaskItem(m_shown);
	}
	m_previewsUpdated = false;
	updatePreviews();
}

void SmoothToolTip::updatePreviews() {
	if (!m_previewsAvailable || m_previewsUpdated) {
		return;
	}

	m_previewsUpdated = true;

	m_widget->layout()->activate();

	QList<WId> winIds;
	QList<QRect> rects;

	foreach (WindowPreview *preview, m_previews) {
		preview->show();

		TaskManager::Task* task = preview->task()->task();
		
		if (task && preview->task()->type() != Task::StartupItem && preview->task()->type() != Task::LauncherItem) {
			winIds.append(task->window());
			rects.append(preview->previewRect(preview->pos()));
		}
	}

	Plasma::WindowEffects::showWindowThumbnails(m_widget->winId(), winIds, rects);
}

void SmoothToolTip::hide() {
	m_widget->hide();
	m_previewsUpdated = false;
	m_hover           = false;
	
	clear();
	
	DelayedToolTip::hide();
}

void SmoothToolTip::updateTheme() {
	m_background->clearCache();

	m_widget->layout()->setContentsMargins(
		m_background->marginSize(Plasma::LeftMargin),
		m_background->marginSize(Plasma::TopMargin),
		m_background->marginSize(Plasma::RightMargin),
		m_background->marginSize(Plasma::BottomMargin));

	QPalette plasmaPalette = QPalette();
	plasmaPalette.setColor(
		QPalette::Window,
		Plasma::Theme::defaultTheme()->color(Plasma::Theme::BackgroundColor));
	plasmaPalette.setColor(
		QPalette::WindowText,
		Plasma::Theme::defaultTheme()->color(Plasma::Theme::TextColor));

	m_widget->setAutoFillBackground(true);
	m_widget->setPalette(plasmaPalette);

	m_closeIcon = KIcon("dialog-close").pixmap(16, 16);
	KIconEffect *effect = KIconLoader::global()->iconEffect();
	if (effect->hasEffect(KIconLoader::Desktop, KIconLoader::ActiveState)) {
		m_hoverCloseIcon = effect->apply(
			m_closeIcon,
			KIconLoader::Desktop,
			KIconLoader::ActiveState);
	}

	m_widget->update();
}

void SmoothToolTip::popup(const QPoint& point, Task *task) {
	m_menuShown = true;
	m_applet->popup(point, task, this, SLOT(popupMenuAboutToHide()));
}

void SmoothToolTip::popupMenuAboutToHide() {
	m_menuShown = false;
	m_hover     = m_widget->geometry().contains(QCursor::pos());

	if (!m_hover) {
		itemLeave(m_hoverItem);
	}
}

void SmoothToolTip::highlightTask(WId winId) {
	QList<WId> winIds;
	winIds << m_applet->view()->winId() << m_widget->winId() << winId;
	Plasma::WindowEffects::highlightWindows(
		m_widget->winId(), winIds);
	m_highlighting = true;
}

} // namespace SmoothTasks
