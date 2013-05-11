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
#ifndef SMOOTHTASKS_SMOOTHTOOLTIP_H
#define SMOOTHTASKS_SMOOTHTOOLTIP_H

#include <QPixmap>
#include <QWidget>
#include <QLabel>
#include <QPainter>
#include <QPaintEvent>
#include <QEvent>
#include <QMouseEvent>

#include <Plasma/FrameSvg>

#include <taskmanager/groupmanager.h>
#include <KWindowSystem>
#include <NETRootInfo>

#include "SmoothTasks/Applet.h"
#include "SmoothTasks/DelayedToolTip.h"

namespace SmoothTasks {

class WindowPreview;
class ToolTipWidget;
class Task;

class SmoothToolTip : public DelayedToolTip {
	Q_OBJECT

		friend class ToolTipWidget;
		friend class WindowPreview;

	public:
		SmoothToolTip(Applet *applet);
		~SmoothToolTip();

		Kind           kind() const { return Smooth; }
		void           hide();
		void           moveBesideTaskItem(bool forceAnimated);
		WindowPreview *hoverWindowPreview()      { return m_hoverPreview; }
		const QPixmap& closeIcon()         const { return m_closeIcon; }
		const QPixmap& hoverCloseIcon()    const { return m_hoverCloseIcon; }
		bool           previewsAvailable() const { return m_previewsAvailable; }
		void           popup(const QPoint& point, Task *task);
		void           highlightTask(WId winId);
	
	public slots:
		void         stopEffect();
		virtual void itemUpdate(TaskItem* item);
		virtual void popupMenuAboutToHide();
	
	protected:
		virtual void showAction(bool animate);
		virtual void hideAction();
		
	private slots:
		void updateTheme();
		void previewLayoutChanged(Applet::PreviewLayoutType previewLayout);
		void animateScroll(qreal progress);
		void animationFinished(int animation);
		void previewWindowSizeChanged();
		void enterWindowPreview(WindowPreview *preview);
		void leaveWindowPreview(WindowPreview *preview);
		void highlightDelayTimeout();

	private:
		void updateToolTip(bool forceAnimated);
		void setTasks(TaskManager::ItemList tasks);
		void clear();
		void leave();
		void startScrollAnimation(int dx, int dy, int speed);
		void stopScrollAnimation(bool force = false);
		void moveTo(WindowPreview *preview, const QPoint& mousePos);
		void updatePreviews();
		bool isVertical() const;

		ToolTipWidget         *m_widget;
		QList<WindowPreview*>  m_previews;
		bool                   m_previewsAvailable;
		Plasma::FrameSvg      *m_background;
		bool                   m_hover;
		bool                   m_menuShown;
		bool                   m_previewsUpdated;
		WindowPreview         *m_hoverPreview;
		QTimer                *m_highlightDelay;
		bool                   m_highlighting;

		// animation:
		int   m_scrollAnimation;
		int   m_dx;
		int   m_xStart;
		int   m_dy;
		int   m_yStart;
		bool  m_moveAnimation;
		bool  m_moveAnimationUpdated;
		int   m_position;
		QSize m_size;

		// close icon:
		QPixmap m_closeIcon;
		QPixmap m_hoverCloseIcon;
};

} // namespace SmoothTasks
#endif
