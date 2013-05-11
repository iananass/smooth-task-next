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

#ifndef SMOOTHTASKS_WINDOWPREVIEW_H
#define SMOOTHTASKS_WINDOWPREVIEW_H

#include "SmoothTasks/Applet.h"
#include "SmoothTasks/SmoothToolTip.h"
#include "SmoothTasks/FadedText.h"
#include "SmoothTasks/ToggleAnimation.h"

#include <QWidget>
#include <QSize>
#include <QMouseEvent>
#include <QSpacerItem>

#include <Plasma/FrameSvg>

namespace SmoothTasks {

class WindowPreview : public QWidget {
	Q_OBJECT

	public:
		WindowPreview(
			TaskManager::TaskItem *task,
			int index,
			SmoothToolTip *toolTip);
		~WindowPreview();

		QRect previewRect(QPoint point) const { return previewRect(point.x(), point.y()); }
		QRect previewRect(int x, int y) const;
		SmoothToolTip *toolTip() const { return m_toolTip; }
		Task* task()             const { return m_task; }
		qreal highlite()         const { return m_highlite.value(); }
		int   index()            const { return m_index; }
		void  hoverEnter();
		void  hoverLeave();

	protected:
		void paintEvent(QPaintEvent *event);
		void enterEvent(QEvent *event);
		void leaveEvent(QEvent *event);
		void mousePressEvent(QMouseEvent *event);
		// void mouseMoveEvent(QMouseEvent *event);
		void mouseReleaseEvent(QMouseEvent *event);
		void dragEnterEvent(QDragEnterEvent *event);
		void dragLeaveEvent(QDragLeaveEvent *event);
		void dragMoveEvent(QDragMoveEvent *event);
		void dropEvent(QDropEvent *event);
	
	signals:
		void sizeChanged();
		void enter(WindowPreview *windowPreview);
		void leave(WindowPreview *windowPreview);

	public slots:
		void highlightTask();
		void activateTask();
		void closeTask();
		void updateTheme();
	
	private slots:
		void activateForDrop();
		void updateTask(::TaskManager::TaskChanges changes);

	private:
		static const QSize BIG_ICON_SIZE;
		static const QSize SMALL_ICON_SIZE;

		QPixmap hoverIcon() const;
		void    setPreviewSize();
		void    setClassicLayout();
		void    setNewLayout();

		Plasma::FrameSvg      *m_background;
		FadedText             *m_taskNameLabel;
		QSpacerItem           *m_iconSpace;
		QSpacerItem           *m_previewSpace;
		ToggleAnimation        m_highlite;
		Task                  *m_task;
		SmoothToolTip         *m_toolTip;
		QSize                  m_previewSize;
		QPixmap                m_icon;
		bool                   m_hover;
		int                    m_index;
		QTimer                *m_activateTimer;
		bool                   m_didPress;
		QPoint                 m_dragStartPosition;
};

} // namespace SmoothTasks
#endif
