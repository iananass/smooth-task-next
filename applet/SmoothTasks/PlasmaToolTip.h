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
#ifndef SMOOTHTASKS_PLASMATOOLTIP_H
#define SMOOTHTASKS_PLASMATOOLTIP_H

#include "SmoothTasks/DelayedToolTip.h"

namespace SmoothTasks {

class PlasmaToolTip : public DelayedToolTip {
	Q_OBJECT
	
	public:
		PlasmaToolTip(Applet *applet)
			: DelayedToolTip(applet) {}

		virtual Kind kind() const { return Plasma; }
		virtual void hide();
		virtual void unregisterItem(TaskItem *item);

	protected:
		virtual void showAction(bool animated);
		virtual void hideAction();
		
	public slots:;
		void itemUpdate(TaskItem *item);
	
	private:
		void updateToolTip();
	
	private slots:
		void activateWindow(WId window, Qt::MouseButtons buttons);
};

} // namespace SmoothTasks
#endif
