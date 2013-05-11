/***********************************************************************************
* Smooth Tasks
* Copyright (C) 2009 Michal Odstrčil <michalodstrcil@gmail.com>
* Copyright (C) 2009 Mathias Panzenböck <grosser.meister.morti@gmx.net> (minor contributions)
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

#include "SmoothTasks/LimitSqueezeTaskbarLayout.h"

namespace SmoothTasks {

void LimitSqueezeTaskbarLayout::setSqueezeRatio(qreal squeezeRatio) {
	if (m_squeezeRatio != squeezeRatio) {
		m_squeezeRatio = squeezeRatio;
		invalidate();
	}
}

void   LimitSqueezeTaskbarLayout::setPreferGrouping(bool preferGrouping) {
	if (m_preferGrouping != preferGrouping) {
		m_preferGrouping = preferGrouping;
		invalidate();
	}
}

int LimitSqueezeTaskbarLayout::optimumCapacity() const {
	const QRectF effectiveRect(effectiveGeometry());
	const QList<TaskbarItem*> & items = this->items();
	const int N                 = items.size();
	const bool isVertical       = orientation() == Qt::Vertical;
	const qreal availableHeight = isVertical ? effectiveRect.width()  : effectiveRect.height();
	const qreal availableWidth  = isVertical ? effectiveRect.height() : effectiveRect.width();
	const qreal spacing         = this->spacing();
	qreal thisRowWidth          = 0;
	int numberGrouped           = 0;
	
#define CELL_HEIGHT(ROWS) (((availableHeight + spacing) / ((qreal) (ROWS))) - spacing)

	int cellHeight = CELL_HEIGHT(m_rows);
	int cellWidth  = cellHeight * aspectRatio();
	
	for (int i =0; i < items.size(); i++) {
		thisRowWidth += this->spacing() + itemAt(i)->task()->taskCount() * (cellWidth +
			(itemAt(i)->isExpanded()) * expandedWidth());
		
		if (itemAt(i)->task()->type() == Task::GroupItem) {
			++ numberGrouped;
		}
	}

	qreal compression = (m_rows * availableWidth) / thisRowWidth;
	// group when there is not enough place
	if ((compression < (m_squeezeRatio + (m_preferGrouping ? 0.1 : 0.0)) && (m_rows == maximumRows() || m_preferGrouping))
		// stay grouped when number of rows grows
		|| ( m_rows > minimumRows() && m_preferGrouping )
		// should prevent ungrouping when nubmer of rows is changed (not perfeect)
		|| (m_rows == maximumRows() - 1 && numberGrouped > 0 && m_preferGrouping == 0 && compression < m_squeezeRatio)
		) {
		return N - 1; //now it is the right size
	}
	else {
		return N + 10;
	}
}

void LimitSqueezeTaskbarLayout::doLayout() {
	// I think this way the loops can be optimized by the compiler.
	// (lifting out the comparison and making two loops; TODO: find out whether this is true):
	const bool isVertical = orientation() == Qt::Vertical;

	const QList<TaskbarItem*>& items = this->items();
	const int N = items.size();

	// if there is nothing to layout fill in some dummy data and leave
	if (N == 0) {
		stopAnimation();

		QRectF rect(geometry());
		m_rows = 1;

		if (isVertical) {
			m_cellHeight = rect.width();
		}
		else {
			m_cellHeight = rect.height();
		}

		QSizeF newPreferredSize(qMin(10.0, rect.width()), qMin(10.0, rect.height()));

		if (newPreferredSize != m_preferredSize) {
			m_preferredSize = newPreferredSize;
			emit sizeHintChanged(Qt::PreferredSize);
		}

		return;
	}

	const QRectF effectiveRect(effectiveGeometry());
	const qreal availableWidth  = isVertical ? effectiveRect.height() : effectiveRect.width();
	const qreal availableHeight = isVertical ? effectiveRect.width()  : effectiveRect.height();
	const qreal spacing         = this->spacing();

	int rows = minimumRows() - 1;

	qreal cellHeight = 0.0;
	qreal cellWidth  = 0.0;

	QList<RowInfo> rowInfos;
	
	qreal maxPreferredRowWidth   = 0.0;
	qreal compression            = 1.0;
	qreal thisRowWidth           = 0.0;
	int   expandedHoverItemCount = 0;   // if there is some icon expanded by mouse
	int   expandedItemCount      = 0;
	
	for (int index = 0; index < N; ++ index) {
		// icon is hovered by mouse
		if (itemAt(index)->isExpandedByHover()) {
			++ expandedHoverItemCount;
		}

		if (itemAt(index)->isExpanded()) {
			++ expandedItemCount;
		}
	}

	// determine the number of rows
	while (rows < maximumRows()) {
		++ rows;
		cellHeight = CELL_HEIGHT(rows);
		cellWidth  = cellHeight * aspectRatio();
		thisRowWidth = N * (cellWidth + spacing) +
			(expandedItemCount - expandedHoverItemCount) * expandedWidth(); // suppress the expansion by hover
		compression = qMin((rows * availableWidth) / thisRowWidth, 1.0);
		
		if (rows * availableWidth > (thisRowWidth - N * cellWidth) * m_squeezeRatio + N * cellWidth / (rows + 1)) {
			break;
		}
	}

	// distribution of items, determine:
	//  * the preferred size each single row
	//  * spacings of each row (can be derived of number of elements in row)
	//  * the start and end index of each row
	//  * the maximal preferred row width (as the preferred width for sizeHint)

	int startIndex = 0;
	int endIndex   = 0;
	int index      = 0;
	for (int row = 0; row < rows && endIndex < N; ++ row) {
		startIndex = endIndex;
		
		expandedHoverItemCount = 0;
		expandedItemCount      = 0;
		// determine number of items in one row
		for (index = startIndex; index < N; ++ index) {
			if (itemAt(index)->isExpandedByHover()) {
				++ expandedHoverItemCount;
			}

			if (itemAt(index)->isExpanded()) {
				++ expandedItemCount;
			}

			thisRowWidth = (index - startIndex) * (cellWidth + spacing) +
				(expandedItemCount - expandedHoverItemCount) * expandedWidth();
			// the 0.9 is here to prefer higher rows
			if (availableWidth < compression * thisRowWidth * 0.9) {
				break;
			}
		}

		if (row + 1 == rows) {
			endIndex = N;
		}
		else {
			if (startIndex == index) { // prevents empty rows
				++ index;
			}
			
			endIndex = qMin(N, index);
		}
		
		qreal rowSpacing = spacing * (endIndex - startIndex);
		thisRowWidth = 0.0;
		qreal thisRowMinWidth = 0.0;
		
		for (int index = startIndex; index < endIndex; ++ index) {
			thisRowWidth    += cellWidth + items[index]->expansion;
			thisRowMinWidth += cellWidth;
		}

		if (thisRowWidth + rowSpacing > maxPreferredRowWidth) {
			maxPreferredRowWidth = thisRowWidth + rowSpacing;
		}

		if (startIndex != endIndex) {
			rowInfos.append(RowInfo(thisRowWidth, thisRowMinWidth, startIndex, endIndex));
		}
	}

	// if we assumed expanded there still might be empty row
	// therefore just scale up the layout (maybe do only this and not the other way of row removal)
	rows = qMax(minimumRows(), rowInfos.size());
	
	cellHeight = CELL_HEIGHT(rows);
	updateLayout(rows, cellWidth, cellHeight, availableWidth, maxPreferredRowWidth, rowInfos, effectiveRect);
	
	m_rows = rows;
	m_compresion = compression;
#undef CELL_HEIGHT
}

} // namespace SmoothTasks
