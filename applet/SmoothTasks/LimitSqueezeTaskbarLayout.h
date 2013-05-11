/***********************************************************************************
* Smooth Tasks
* Copyright (C) 2009 Michal Odstrčil <michalodstrcil@gmail.com>
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

#ifndef SMOOTHTASKS_SQUEEZETASKBARLAYOUT_H
#define SMOOTHTASKS_SQUEEZETASKBARLAYOUT_H

#include "SmoothTasks/TaskbarLayout.h"

namespace SmoothTasks {

class LimitSqueezeTaskbarLayout : public TaskbarLayout {
	Q_OBJECT
	Q_PROPERTY(qreal squeezeRatio READ squeezeRatio WRITE setSqueezeRatio)
	Q_PROPERTY(bool preferGrouping READ preferGrouping WRITE setPreferGrouping)
	
	public:
		LimitSqueezeTaskbarLayout( // add preferGrouping
			qreal squeezeRatio,
			bool preferGrouping,
			Qt::Orientation orientation = Qt::Horizontal,
			QGraphicsLayoutItem *parent = NULL)
				: TaskbarLayout(orientation, parent),
				  m_squeezeRatio(squeezeRatio),
				  m_compresion(1.0),
				  m_preferGrouping(preferGrouping) {}

		qreal  squeezeRatio() const { return m_squeezeRatio; }
		void   setSqueezeRatio(qreal squeezeRatio);

		bool   preferGrouping() const { return m_preferGrouping; }
		void   setPreferGrouping(bool preferGrouping);
		
		virtual TaskbarLayoutType type() const { return LimitSqueeze; }
		virtual int optimumCapacity() const;

	protected:
		virtual void doLayout();

	private:
		qreal m_squeezeRatio;
		qreal m_compresion;
		bool  m_preferGrouping;
		int   m_rows;
};

} // namespace SmoothTasks

#endif
