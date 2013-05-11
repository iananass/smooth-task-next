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
#ifndef SMOOTHTASKS_TOOLTIPWIDGET_H
#define SMOOTHTASKS_TOOLTIPWIDGET_H

#include <QWidget>

namespace SmoothTasks {

class SmoothToolTip;

class ToolTipWidget : public QWidget {
	Q_OBJECT

		friend class SmoothToolTip;

	protected:
		ToolTipWidget(SmoothToolTip* toolTip);

	private:
		SmoothToolTip *m_toolTip;
		
		static const int ANIMATION_MARGIN;
		static const int SCROLL_DURATION;
			
	protected:
		void paintEvent(QPaintEvent *event);
		void resizeEvent(QResizeEvent *event);
		void enterEvent(QEvent *event);
		void leaveEvent(QEvent *event);
		void mousePressEvent(QMouseEvent *event);
		void mouseMoveEvent(QMouseEvent *event);
		void hideEvent(QHideEvent *event);
		void wheelEvent(QWheelEvent *event);
		void dragEnterEvent(QDragEnterEvent *event);
		void dragLeaveEvent(QDragLeaveEvent *event);
};

} // namespace SmoothTasks

#endif
