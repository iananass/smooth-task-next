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

#ifndef SMOOTHTASKS_FIXEDITEMCOUNTTASKBARLAYOUT_H
#define SMOOTHTASKS_FIXEDITEMCOUNTTASKBARLAYOUT_H

#include "SmoothTasks/TaskbarLayout.h"

namespace SmoothTasks {

class FixedItemCountTaskbarLayout : public TaskbarLayout {
	Q_OBJECT
	Q_PROPERTY(int itemsPerRow READ itemsPerRow WRITE setItemsPerRow)

	public:
		FixedItemCountTaskbarLayout(
			int itemsPerRow,
			Qt::Orientation orientation = Qt::Horizontal,
			QGraphicsLayoutItem *parent = NULL)
				: TaskbarLayout(orientation, parent), m_itemsPerRow(itemsPerRow) {}

		int  itemsPerRow() const { return m_itemsPerRow; }
		void setItemsPerRow(int itemsPerRow);

		virtual TaskbarLayoutType type() const { return FixedItemCount; }
		virtual int optimumCapacity() const;

	protected:
		virtual void doLayout();

	private:
		int m_itemsPerRow;
};

} // namespace SmoothTasks

#endif
