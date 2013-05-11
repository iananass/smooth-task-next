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
#ifndef SMOOTHTASKS_DEFAULTTASKBARLAYOUT_H
#define SMOOTHTASKS_DEFAULTTASKBARLAYOUT_H

#include "SmoothTasks/TaskbarLayout.h"

namespace SmoothTasks {

class ByShapeTaskbarLayout : public TaskbarLayout {
	Q_OBJECT
	Q_PROPERTY(qreal rowAspectRatio READ rowAspectRatio WRITE setRowAspectRatio)

	public:
		ByShapeTaskbarLayout(
			qreal rowAspectRatio,
			Qt::Orientation orientation = Qt::Horizontal,
			QGraphicsLayoutItem *parent = NULL)
				: TaskbarLayout(orientation, parent),
				  m_rowAspectRatio(rowAspectRatio) {}

		virtual TaskbarLayoutType type() const { return ByShape; }
		virtual int optimumCapacity() const;

		qreal rowAspectRatio() const { return m_rowAspectRatio; }
		void  setRowAspectRatio(qreal rowAspectRatio);

	protected:
		virtual void doLayout();

	private:
		qreal m_rowAspectRatio;
};

} // namespace SmoothTasks

#endif
