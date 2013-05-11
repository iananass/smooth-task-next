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
#ifndef SMOOTHTASKS_GLOBAL_H
#define SMOOTHTASKS_GLOBAL_H

#include <QSizeF>
#include <QTextLayout>

namespace SmoothTasks {
	// global "string table":
	extern const QString TASK_ITEM;
	extern const QString NORMAL;
	extern const QString ACTIVE;
	extern const QString MINIMIZED;
	extern const QString FOCUS;
	extern const QString ATTENTION;
	extern const QString HOVER;
	extern const QString M;
	extern const int     DRAG_HOVER_DELAY;

	QSizeF layoutText(QTextLayout &layout, const QSizeF &constraints);
}

#endif
