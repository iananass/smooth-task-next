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

#include "SmoothTasks/MaxSqueezeTaskbarLayout.h"

#include <QApplication>

#include <cmath>

namespace SmoothTasks {

int MaxSqueezeTaskbarLayout::optimumCapacity() const {
	const QRectF effectiveRect(effectiveGeometry());

	const bool isVertical = orientation() == Qt::Vertical;
	const qreal availableHeight = isVertical ?
		effectiveRect.width() : effectiveRect.height();

	const qreal availableWidth = isVertical ?
		effectiveRect.height() : effectiveRect.width();

	const qreal spacing    = this->spacing();
	const qreal cellHeight = ((availableHeight + spacing) / ((qreal) maximumRows())) - spacing;
	const qreal cellWidth  = cellHeight * aspectRatio();
	const qreal additionalWidth = this->additionalWidth();
	const int itemsPerRow = std::ceil((availableWidth + spacing) / (cellWidth + additionalWidth + spacing));

	return itemsPerRow * maximumRows();
}

void MaxSqueezeTaskbarLayout::doLayout() {
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

	QRectF effectiveRect(effectiveGeometry());
	const qreal availableWidth  = isVertical ? effectiveRect.height() : effectiveRect.width();
	const qreal availableHeight = isVertical ? effectiveRect.width()  : effectiveRect.height();
	const qreal spacing         = this->spacing();

#define CELL_HEIGHT(ROWS) (((availableHeight + spacing) / ((qreal) (ROWS))) - spacing)

	const qreal additionalWidth = this->additionalWidth();

	// reduce the number of rows until there are no empty rows
	// (or rows == m_minimumRows)
	int itemsPerRow = N;
	int rows = maximumRows();

	qreal cellHeight = CELL_HEIGHT(rows);
	qreal cellWidth  = cellHeight * aspectRatio();

	itemsPerRow = std::ceil((availableWidth + spacing) / (cellWidth + additionalWidth + spacing));

	if (itemsPerRow * rows < N) {
		itemsPerRow = std::ceil(((qreal) N) / rows);
	}

	QList<RowInfo> rowInfos;
	qreal maxPreferredRowWidth = 0;

	buildRows(itemsPerRow, cellWidth, rowInfos, rows, maxPreferredRowWidth);
	cellHeight = CELL_HEIGHT(rows);

	updateLayout(rows, cellWidth, cellHeight, availableWidth, maxPreferredRowWidth, rowInfos, effectiveRect);

#undef CELL_HEIGHT
}

} // namespace SmoothTasks
