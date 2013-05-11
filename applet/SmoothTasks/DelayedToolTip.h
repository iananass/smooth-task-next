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
#ifndef SMOOTHTASKS_DELAYEDTOOLTIP_H
#define SMOOTHTASKS_DELAYEDTOOLTIP_H

#include "SmoothTasks/ToolTipBase.h"

namespace SmoothTasks {

class DelayedToolTip : public ToolTipBase {
	Q_OBJECT
	
	public:
		DelayedToolTip(Applet *applet);
		
		virtual Kind kind() const = 0;
		virtual void hide();
		virtual void quickShow(TaskItem *item);
		
	protected slots:
		virtual void itemDelete(TaskItem *item);
		virtual void itemEnter(TaskItem *item);
		virtual void itemLeave(TaskItem *item);
		
	protected:
		virtual void showAction(bool animate) = 0;
		virtual void hideAction() = 0;
		
	private slots:
		void timeout();
	
	private:
		enum ActionType {
			NoAction,
			ShowAction,
			HideAction
		};
		
		QTimer            *m_delayTimer;
		ActionType         m_action;
		QPointer<TaskItem> m_newHoverItem;
};

} // namespace SmoothTasks

#endif
