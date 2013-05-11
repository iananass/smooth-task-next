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

#ifndef SMOOTHTASKS_FIXEDSIZETASKBARLAYOUT_H
#define SMOOTHTASKS_FIXEDSIZETASKBARLAYOUT_H

#include "SmoothTasks/TaskbarLayout.h"

namespace SmoothTasks {

class FixedSizeTaskbarLayout : public TaskbarLayout {
	Q_OBJECT
	Q_PROPERTY(qreal fixedCellHeight READ fixedCellHeight WRITE setFixedCellHeight)

	public:
		FixedSizeTaskbarLayout(
			qreal fixedCellHeight,
			Qt::Orientation orientation = Qt::Horizontal,
			QGraphicsLayoutItem *parent = NULL)
				: TaskbarLayout(orientation, parent),
				  m_fixedCellHeight(fixedCellHeight) {}

		virtual TaskbarLayoutType type() const { return FixedSize; }

		qreal fixedCellHeight() const { return m_fixedCellHeight; }
		void  setFixedCellHeight(qreal fixedCellHeight);

		virtual int optimumCapacity() const;

	protected:
		virtual int  rowOf(const QPointF& pos) const;
		virtual void doLayout();

	private:
		qreal m_fixedCellHeight;
};

} // namespace SmoothTasks

#endif
