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

#include "SmoothTasks/FixedSizeTaskbarLayout.h"

#include <QApplication>

#include <cmath>

namespace SmoothTasks {

void FixedSizeTaskbarLayout::setFixedCellHeight(qreal fixedCellHeight) {
	if (fixedCellHeight < 1) {
		qWarning("FixedSizeTaskbarLayout::setFixedCellHeight: illegal fixedCellHeight: %g", fixedCellHeight);
		return;
	}

	if (m_fixedCellHeight != fixedCellHeight) {
		m_fixedCellHeight = fixedCellHeight;
		invalidate();
	}
}

int FixedSizeTaskbarLayout::optimumCapacity() const {
	const QRectF effectiveRect(effectiveGeometry());

	const bool isVertical = orientation() == Qt::Vertical;

	const qreal availableWidth = isVertical ?
		effectiveRect.height() : effectiveRect.width();

	const qreal spacing = this->spacing();
	const qreal additionalWidth = this->additionalWidth();
	const int itemsPerRow = std::ceil((availableWidth + spacing) / (cellWidth() + additionalWidth + spacing));

	return itemsPerRow * maximumRows();
}

int FixedSizeTaskbarLayout::rowOf(const QPointF& pos) const {
	QRectF effectiveRect(effectiveGeometry());
	qreal  spacing = this->spacing();

	if (orientation() == Qt::Vertical) {
		qreal x     = pos.x();
		qreal width = (m_fixedCellHeight + spacing) * m_rows - spacing;

		if (width > effectiveRect.width()) {
			width = effectiveRect.width();
		}

		if (x <= effectiveRect.left()) {
			return 0;
		}
		else if (x >= effectiveRect.right() || width <= 0) {
			return m_rows - 1;
		}
		else {
			return (int) ((x - effectiveRect.left()) * m_rows / width);
		}
	}
	else {
		qreal y      = pos.y();
		qreal height = (m_fixedCellHeight + spacing) * m_rows - spacing;

		if (height > effectiveRect.height()) {
			height = effectiveRect.height();
		}

		if (y <= effectiveRect.top()) {
			return 0;
		}
		else if (y >= effectiveRect.bottom() || height <= 0) {
			return m_rows - 1;
		}
		else {
			return (int) ((y - effectiveRect.top()) * m_rows / height);
		}
	}
}

void FixedSizeTaskbarLayout::doLayout() {
	// I think this way the loops can be optimized by the compiler.
	// (lifting out the comparison and making two loops; TODO: find out whether this is true):
	const bool isVertical = orientation() == Qt::Vertical;

	const QList<TaskbarItem*>& items = this->items();
	const int N = items.size();

	qreal left = 0, top = 0, right = 0, bottom = 0;
	getContentsMargins(&left, &top, &right, &bottom);

	// if there is nothing to layout fill in some dummy data and leave
	if (N == 0) {
		stopAnimation();

		QRectF rect(geometry());
		QSizeF newPreferredSize;

		m_rows = 1;

		if (isVertical) {
			newPreferredSize.setWidth(top + m_fixedCellHeight + bottom);
			newPreferredSize.setHeight(qMin(10.0, rect.height()));
		}
		else {
			newPreferredSize.setWidth(qMin(10.0, rect.width()));
			newPreferredSize.setHeight(top + m_fixedCellHeight + bottom);
		}

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

#define CELL_HEIGHT(ROWS) (((availableHeight + spacing) / ((qreal) (ROWS))) - spacing)

	const qreal additionalWidth = this->additionalWidth();

	// reduce the number of rows until there are no empty rows
	// (or rows == m_minimumRows)
	int itemsPerRow = N;
	int rows = maximumRows();

	qreal cellWidth  = m_fixedCellHeight * aspectRatio();

	itemsPerRow = std::ceil((availableWidth + spacing) / (cellWidth + additionalWidth + spacing));

	if (itemsPerRow * rows < N) {
		itemsPerRow = std::ceil(((qreal) N) / rows);
	}

	QList<RowInfo> rowInfos;
	qreal maxPreferredRowWidth = 0;

	buildRows(itemsPerRow, cellWidth, rowInfos, rows, maxPreferredRowWidth);
	qreal cellHeight = qMin(m_fixedCellHeight, CELL_HEIGHT(rows));

	updateLayout(rows, cellWidth, cellHeight, availableWidth, maxPreferredRowWidth, rowInfos, effectiveRect);

#undef CELL_HEIGHT
}

} // namespace SmoothTasks
