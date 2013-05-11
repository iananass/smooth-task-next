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
#ifndef SMOOTHTASKS_TOOLTIPBASE_H
#define SMOOTHTASKS_TOOLTIPBASE_H

#include <QObject>

#include "SmoothTasks/TaskItem.h"

namespace SmoothTasks {

class ToolTipBase : public QObject {
	Q_OBJECT
	
	public:
		enum Kind {
			None   = 0,
			Smooth = 1,
			Plasma = 2
		};

		enum Position {
			Left   =  1,
			Center =  2,
			Right  =  4,
			Top    =  8,
			Middle = 16,
			Bottom = 32,
			HorizontalMask = Left | Center | Right,
			VerticalMask   = Top | Middle | Bottom
		};

		ToolTipBase(Applet *applet)
			: QObject(),
			  m_applet(applet),
			  m_shown(false),
			  m_hoverItem(NULL) {}
		virtual ~ToolTipBase() {}

		virtual Kind  kind() const { return None; }
		virtual void  hide();
		virtual void  quickShow(TaskItem *item);
		Applet       *applet()    const { return m_applet; }
		TaskItem     *hoverItem() const { return m_hoverItem; }
		bool          isShown()   const { return m_shown; }
		virtual void  registerItem(TaskItem *item);
		virtual void  unregisterItem(TaskItem *item);
		virtual void  moveBesideTaskItem(bool forceAnimated) {
			Q_UNUSED(forceAnimated)
		}
		bool shows(const TaskItem *item) const {
			return m_shown && m_hoverItem == item;
		}
		
	public slots:
		// TODO: maybe use eventFilter on the items?
		virtual void itemUpdate(TaskItem *item) { Q_UNUSED(item) }
		virtual void itemDelete(TaskItem *item);
		virtual void popupMenuAboutToHide() { hide(); }
	
	protected slots:
		virtual void itemEnter(TaskItem *item);
		virtual void itemLeave(TaskItem *item);
	
	protected:
		bool eventFilter(QObject *obj, QEvent *event);
	
		Applet            *m_applet;
		bool               m_shown;
		QPointer<TaskItem> m_hoverItem;
};

} // namespace SmoothTasks

#endif
