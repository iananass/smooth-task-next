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

#include <QApplication>
#include <QGraphicsItem>
#include <QDebug>
#include <QTime>

#include <limits>
#include <cmath>

#include <Plasma/Animator>

#include "SmoothTasks/TaskbarLayout.h"
#include "SmoothTasks/TaskItem.h"

namespace SmoothTasks {

const QTime TaskbarLayout::Midnight(0, 0, 0, 0);

TaskbarItem::~TaskbarItem() {
	if (item) {
		item->setParentLayoutItem(NULL);
		if (item->ownedByLayout()) {
			delete item;
		}
	}
}

const qreal TaskbarLayout::PIXELS_PER_SECOND = 500;

TaskbarLayout::TaskbarLayout(Qt::Orientation orientation, QGraphicsLayoutItem *parent)
	: QGraphicsLayout(parent),
	  m_draggedItem(NULL),
	  m_currentIndex(-1),
	  m_currentAnimation(None),
	  m_mouseIn(false),
	  m_orientation(orientation),
	  m_spacing(0.0),
	  m_animationTimer(new QTimer(this)),
	  m_grabPos(),
	  m_fps(35),
	  m_animationsEnabled(true),
	  m_minimumRows(1),
	  m_maximumRows(6),
	  m_expandedWidth(175),
	  m_aspectRatio(1.0),
	  m_expandDuration(160),
	  m_timeStamp(0),
	  m_preferredSize(0.0, 0.0),
	  m_cellHeight(1.0),
	  m_rows(1) {
	m_animationTimer->setInterval(1000 / m_fps);

	connect(m_animationTimer, SIGNAL(timeout()), this, SLOT(animate()));
}

// more or less copied from QGraphicsLinearLayout
TaskbarLayout::~TaskbarLayout() {
	clear();
}

void TaskbarLayout::setOrientation(Qt::Orientation orientation) {
	if (orientation != m_orientation) {
		m_orientation = orientation;
		foreach (TaskbarItem *item, m_items) {
			item->item->setOrientation(orientation);
		}
		stopAnimation();
		invalidate();
	}
}

void TaskbarLayout::setSpacing(qreal spacing) {
	if (spacing < 0) {
		qWarning("TaskbarLayout::setSpacing: invalid spacing %g", spacing);
		return;
	}

	if (spacing != m_spacing) {
		m_spacing = spacing;
		invalidate();
	}
}

void TaskbarLayout::setFps(int fps) {
	if (fps <= 0) {
		qWarning("TaskbarLayout::setFps: invalid fps %d", fps);
		return;
	}

	if (m_fps != fps) {
		m_fps = fps;
		m_animationTimer->setInterval(1000 / m_fps);
	}
}

void TaskbarLayout::setAnimationsEnabled(bool animationsEnabled) {
	m_animationsEnabled = animationsEnabled;

	if (!animationsEnabled) {
		skipAnimation();
	}
}

void TaskbarLayout::setMaximumRows(int maximumRows) {
	if (maximumRows < 1) {
		qWarning("TaskbarLayout::setMaximumRows: invalid maximumRows %d", maximumRows);
		return;
	}

	if (maximumRows != m_maximumRows) {
		m_maximumRows = maximumRows;
		if (m_minimumRows > maximumRows) {
			m_minimumRows = maximumRows;
		}
		if (m_rows > maximumRows) {
			invalidate();
		}
	}
}

void TaskbarLayout::setMinimumRows(int minimumRows) {
	if (minimumRows < 1) {
		qWarning("TaskbarLayout::setMinimumRows: invalid minimumRows %d", minimumRows);
		return;
	}

	if (minimumRows != m_minimumRows) {
		m_minimumRows = minimumRows;
		if (m_maximumRows < minimumRows) {
			m_maximumRows = minimumRows;
		}
		if (m_rows < minimumRows) {
			invalidate();
		}
	}
}

void TaskbarLayout::setRowBounds(int minimumRows, int maximumRows) {
	if (minimumRows < 1) {
		qWarning("TaskbarLayout::setRowBounds: invalid minimumRows %d", minimumRows);
		return;
	}

	if (minimumRows > maximumRows) {
		qWarning(
			"TaskbarLayout::setRowBounds: invalid row bounds: minimumRows: %d, maximumRows: %d",
			minimumRows, maximumRows);
		return;
	}
	
	if (minimumRows != m_minimumRows || maximumRows != m_maximumRows) {
		m_minimumRows = minimumRows;
		m_maximumRows = maximumRows;
		if (m_rows < minimumRows || m_rows > maximumRows) {
			invalidate();
		}
	}
}

void TaskbarLayout::setExpandedWidth(qreal expandedWidth) {
	if (expandedWidth < 0.0) {
		qWarning("TaskbarLayout::setExpandedSize: invalid expandedWidth %g", expandedWidth);
		return;
	}

	if (expandedWidth != m_expandedWidth) {
		m_expandedWidth = expandedWidth;
		invalidate();
	}
}

void TaskbarLayout::setAspectRatio(qreal aspectRatio) {
	if (aspectRatio < 0.0) {
		qWarning("TaskbarLayout::setAspectRatio: invalid aspectRatio %g", aspectRatio);
		return;
	}

	if (aspectRatio != m_aspectRatio) {
		m_aspectRatio = aspectRatio;
		invalidate();
	}
}

void TaskbarLayout::setExpandDuration(int expandDuration) {
	if (expandDuration < 0) {
		qWarning("TaskbarLayout::setExpandDuration: invalid expandDuration %d", expandDuration);
		return;
	}

	if (expandDuration != m_expandDuration) {
		m_expandDuration = expandDuration;
	}
}

QSizeF TaskbarLayout::sizeHint(Qt::SizeHint which, const QSizeF& constraint) const {
	Q_UNUSED(constraint);

	switch (which) {
	case Qt::MinimumSize:
		return QSizeF(0.0, 0.0);
	case Qt::PreferredSize:
		return m_preferredSize;
	case Qt::MaximumSize:
		return QSizeF(std::numeric_limits<qreal>::max(), std::numeric_limits<qreal>::max());
	case Qt::MinimumDescent:
		return QSizeF(0.0, 0.0);
	default:
		break;
	}

	return QSizeF();
}

void TaskbarLayout::setGeometry(const QRectF& rect) {
    QGraphicsLayout::setGeometry(rect);
	doLayout();
}

QRectF TaskbarLayout::effectiveGeometry() const {
	QRectF effectiveRect(geometry());
	qreal left = 0, top = 0, right = 0, bottom = 0;
	getContentsMargins(&left, &top, &right, &bottom);
	
	if (QApplication::isRightToLeft()) {
		if (m_orientation == Qt::Vertical) {
			qSwap(top, bottom);
		}
		else {
			qSwap(left, right);
		}
	}
	effectiveRect.adjust(+left, +top, -right, -bottom);

	return effectiveRect;
}

void TaskbarLayout::buildRows(const int itemsPerRow, const qreal cellWidth, QList<RowInfo>& rowInfos, int& rows, qreal& maxPreferredRowWidth) const {
	const int N = m_items.size();
	const qreal spacing = m_spacing;
	maxPreferredRowWidth = 0;

	// distribution of items, determine:
	//  * the preferred size each single row
	//  * spacings of each row (can be derived of number of elements in row)
	//  * the start and end index of each row
	//  * the maximal preferred row width (as the preferred width for sizeHint)

	int startIndex = 0;
	int endIndex   = 0;

	for (int row = 0; row < rows && endIndex < N; ++ row) {
		startIndex = endIndex;

		if (row + 1 == rows) {
			endIndex = N;
		}
		else {
			endIndex = qMin(startIndex + itemsPerRow, N);
		}

		qreal rowSpacing      = spacing * (endIndex - startIndex - 1);
		qreal thisRowWidth    = 0.0;
		qreal thisRowMinWidth = 0.0;

		for (int index = startIndex; index < endIndex; ++ index) {
			thisRowWidth    += cellWidth + m_items[index]->expansion;
			thisRowMinWidth += cellWidth;
		}

		if (thisRowWidth + rowSpacing > maxPreferredRowWidth) {
			maxPreferredRowWidth = thisRowWidth + rowSpacing;
		}
		rowInfos.append(RowInfo(thisRowWidth, thisRowMinWidth, startIndex, endIndex));
	}

	// if we assumed expanded there still might be empty row
	// therefore just scale up the layout (maybe do only this and not the other way of row removal)
	rows = qMax(m_minimumRows, rowInfos.size());
}

void TaskbarLayout::updateLayout(
		const int rows, const qreal cellWidth, const qreal cellHeight,
		const qreal availableWidth, const qreal maxPreferredRowWidth,
		const QList<RowInfo>& rowInfos, const QRectF& effectiveRect) {
	// before updating the geometries of the items set the properties
	// that might get read by the items in their event handlers:
	const bool  isVertical  = m_orientation == Qt::Vertical;
	const qreal spacing     = m_spacing;
	const bool  animateMove = m_currentAnimation & Move;
	const QSizeF oldPreferredSize(m_preferredSize);

	m_rows = rows;

	if (m_cellHeight != cellHeight) {
		const QSizeF cellSize(cellHeight * m_aspectRatio, cellHeight);
		m_cellHeight = cellHeight;

		foreach (TaskbarItem *item, m_items) {
			item->item->setCellSize(cellSize);
		}
	}

	qreal left = 0, top = 0, right = 0, bottom = 0;
	getContentsMargins(&left, &top, &right, &bottom);

	if (isVertical) {
		m_preferredSize.setWidth(geometry().width());
		m_preferredSize.setHeight(top + maxPreferredRowWidth + bottom);
	}
	else {
		m_preferredSize.setWidth(left + maxPreferredRowWidth + right);
		m_preferredSize.setHeight(geometry().height());
	}

	// now do the actual layouting:
	const bool rtl = QApplication::isRightToLeft();
	qreal rowOffset = isVertical ?
		effectiveRect.left() :
		effectiveRect.top();
	QRectF rect(effectiveRect.left(), effectiveRect.top(), cellHeight, cellHeight);
	const TaskbarItem *draggedItem = m_draggedItem;

	for (int row = 0; row < rowInfos.size(); ++ row) {
		const RowInfo& rowInfo = rowInfos[row];

		qreal pos = isVertical ?
			effectiveRect.top() :
			effectiveRect.left();

		qreal rowSpacings = (rowInfo.endIndex - rowInfo.startIndex) * spacing;
		qreal scale       = 1.0;
		qreal scaleExp    = 0.0;

		// scale item lengths down if necesarry:
		if (rowInfo.preferredWidth + rowSpacings <= availableWidth) {
			scaleExp = 1.0;
		}
		else {
			qreal availableExpWidth = availableWidth - rowSpacings - rowInfo.minimumWidth;
			qreal preferredExpWidth = rowInfo.preferredWidth - rowInfo.minimumWidth;

			if (rowInfo.minimumWidth + rowSpacings <= availableWidth && preferredExpWidth > 0.0 && availableExpWidth > 0.0) {
				scaleExp = availableExpWidth / preferredExpWidth;
			}
			else if (availableWidth > rowSpacings && rowInfo.minimumWidth > 0.0) {
				scale = (availableWidth - rowSpacings) / rowInfo.minimumWidth;
			}
			else {
				scale = 0.0;
			}
		}
		
		for (int index = rowInfo.startIndex; index < rowInfo.endIndex; ++ index) {
			TaskbarItem *item = m_items[index];
			qreal width = (cellWidth + item->expansion * scaleExp) * scale;

			item->row = row;
			if (isVertical) {
				rect.setHeight(width);

				item->destX = rowOffset;
				item->destY = rtl ?
					effectiveRect.bottom() - (pos - effectiveRect.top()) - width :
					pos;
			}
			else {
				rect.setWidth(width);

				item->destX = rtl ?
					effectiveRect.right() - (pos - effectiveRect.left()) - width :
					pos;
				item->destY = rowOffset;
			}

			if ((!animateMove || item->isNew) && item != draggedItem) {
				item->isNew = false;
				rect.moveLeft(item->destX);
				rect.moveTop(item->destY);
			}
			else {
				item->animation |= Move;
				rect.moveTopLeft(item->item->geometry().topLeft());
			}

			item->item->setGeometry(rect);
			pos += width + spacing;
		}

		rowOffset += cellHeight + spacing;
	}

	if (m_currentAnimation != None) {
		startAnimation();
	}

	if (oldPreferredSize != m_preferredSize) {
		emit sizeHintChanged(Qt::PreferredSize);
	}
}

void TaskbarLayout::expandAt(int index, ExpansionDirection direction) {
	if (index < 0 || index >= m_items.size()) {
		qWarning("TaskbarLayout::expandAt: index out of bounds: %d", index);
		return;
	}

	TaskbarItem *item = m_items[index];

	if (item->direction != direction) {
		item->direction = direction;
		int expandAnimation = direction == Collapse ? ResizeCollapse : ResizeExpand;
		item->animation = (item->animation & ~Resize) | expandAnimation;
		m_currentAnimation |= Resize;
		startAnimation();
	}
}

qreal TaskbarLayout::additionalWidth() const {
	// I assume all items to be of the same size. Which size that
	// is depends on if there are more expanded or collapsed items.
	int expandedCount = 0;
	foreach (TaskbarItem *item, m_items) {
		if (item->direction == Expand) {
			++ expandedCount;
		}
	}

	return expandedCount - 2 >= m_items.size() - expandedCount ?
		m_expandedWidth : 0.0;
}

TaskItem *TaskbarLayout::itemAt(int index) const {
	if (index < 0 || index >= m_items.size()) {
		qWarning("TaskbarLayout::itemAt: invalid index %d", index);
		return NULL;
	}
	return m_items[index]->item;
}

int TaskbarLayout::addItem(TaskItem *item, bool expanded) {
	const int index = count();
	insertItem(index, item, expanded);
	return index;
}

void TaskbarLayout::insertItem(int index, TaskItem *item, bool expanded) {
	if (!item) {
		qWarning("TaskbarLayout::insertItem: cannot insert null item");
		return;
	}
	if (indexOf(item) != -1) {
		qWarning("TaskbarLayout::insertItem: cannot instert same item twice");
		return;
	}
	item->setParentLayoutItem(this);
	TaskbarItem *titem = new TaskbarItem(item,
		expanded ? m_expandedWidth : 0.0,
		expanded ? Expand : Collapse);
	m_items.insert(index, titem);

	item->setCellSize(cellSize());
	item->setOrientation(m_orientation);
	connectItem(item);

	invalidate();
}

void TaskbarLayout::connectItem(TaskItem *item) {
	connect(item, SIGNAL(expand(TaskItem*,ExpansionDirection)), this, SLOT(expandItem(TaskItem*,ExpansionDirection)));
}

void TaskbarLayout::move(int fromIndex, int toIndex) {
	if (fromIndex < 0 || fromIndex >= m_items.size()) {
		qWarning("TaskbarLayout::move: invalid fromIndex %d", fromIndex);
		return;
	}

	if (toIndex < 0 || toIndex >= m_items.size()) {
		qWarning("TaskbarLayout::move: invalid toIndex %d", toIndex);
		return;
	}

	m_items.move(fromIndex, toIndex);
	invalidate();
}

void TaskbarLayout::removeAt(int index) {
	if (index < 0 || index >= m_items.size()) {
		qWarning("TaskbarLayout::removeAt: invalid index %d", index);
		return;
	}

	TaskbarItem *item = m_items.takeAt(index);

	if (m_draggedItem == item) {
		m_currentIndex  = -1;
		m_draggedItem   = NULL;
	}

	disconnectItem(item->item);

	delete item;
	if (m_items.isEmpty()) {
		stopAnimation();
	}
	invalidate();
}

void TaskbarLayout::removeItem(TaskItem *item) {
	if (item == NULL) {
		qWarning("TaskbarLayout::removeItem: cannot remove null item");
		return;
	}

	removeAt(indexOf(item));
}

TaskItem *TaskbarLayout::itemAt(const QPointF& pos) const {
	const qreal halfSpacing = m_spacing * 0.5;

	foreach (TaskbarItem *item, m_items) {
		QRectF rect = item->item->geometry();
		qreal y = rect.y();
		qreal x = rect.x();
		if (
				pos.y() >= (y - halfSpacing) && pos.y() < (y + rect.height() + halfSpacing) &&
				pos.x() >= (x - halfSpacing) && pos.x() < (x + rect.width()  + halfSpacing)) {
			return item->item;
		}
	}

	return NULL;
}

int TaskbarLayout::rowOf(TaskItem *item) const {
	if (item == NULL) {
		qWarning("TaskbarLayout::rowOf: item cannot be null");
		return -1;
	}

	foreach (TaskbarItem* titem, m_items) {
		if (titem->item == item) {
			return titem->row;
		}
	}

	qWarning("TaskbarLayout::rowOf: not a child item");
	return -1;
}

int TaskbarLayout::rowOf(int index) const {
	if (index < 0 || index >= m_items.size()) {
		qWarning("TaskbarLayout::rowOf: invalid index %d", index);
		return -1;
	}

	return m_items[index]->row;
}

int TaskbarLayout::rowOf(const QPointF& pos) const {
	QRectF effectiveRect(effectiveGeometry());

	if (m_orientation == Qt::Vertical) {
		qreal x = pos.x();
		if (x <= effectiveRect.left()) {
			return 0;
		}
		else if (x >= effectiveRect.right() || effectiveRect.width() == 0) {
			return m_rows - 1;
		}
		else {
			return (int) ((x - effectiveRect.left()) * m_rows / effectiveRect.width());
		}
	}
	else {
		qreal y = pos.y();
		if (y <= effectiveRect.top()) {
			return 0;
		}
		else if (y >= effectiveRect.bottom() || effectiveRect.height() == 0) {
			return m_rows - 1;
		}
		else {
			return (int) ((y - effectiveRect.top()) * m_rows / effectiveRect.height());
		}
	}
}

int TaskbarLayout::indexOf(const QPointF& pos, int *rowptr) const {
	const QRectF effectiveRect(effectiveGeometry());
	const int    row         = rowOf(pos);
	const qreal  halfSpacing = m_spacing * 0.5;
	const bool   rtl         = QApplication::isRightToLeft();
	const bool   isVertical  = m_orientation == Qt::Vertical;
	const int    N           = m_items.size();

	int rowStart = 0;
	int rowEnd   = N;

	// find first of row:
	for (; rowStart < N; ++ rowStart) {
		if (m_items[rowStart]->row == row) {
			break;
		}
	}

	if (rowptr) {
		*rowptr = row;
	}

	if (rowStart == N) {
		return N;
	}
	else if (rowStart > N) {
		qDebug(
			"TaskbarLayout::indexOf: Bug: rowStart (%d) is bigger than item count (%d)!",
			rowStart, N);
		return N;
	}

	qreal relevantPos;

	if (isVertical) {
		relevantPos = pos.y();

		if (rtl) {
			if (relevantPos > effectiveRect.bottom()) {
				return rowStart;
			}
		}
		else {
			if (relevantPos < effectiveRect.top()) {
				return rowStart;
			}
		}
	}
	else {
		relevantPos = pos.x();

		if (rtl) {
			if (relevantPos > effectiveRect.right()) {
				return rowStart;
			}
		}
		else {
			if (relevantPos < effectiveRect.left()) {
				return rowStart;
			}
		}
	}

	for (int index = rowStart; index < N; ++ index) {
		TaskbarItem* item = m_items[index];
			
		if (item->row != row) {
			rowEnd = index;
			break;
		}

		qreal start = isVertical ?
			item->destY - halfSpacing :
			item->destX - halfSpacing;

		qreal end = isVertical ?
			item->destY + item->item->geometry().height() + halfSpacing :
			item->destX + item->item->geometry().width()  + halfSpacing;
			
		if (relevantPos >= start && relevantPos < end) {
			return index;
		}
	}

	return rowEnd;
}

int TaskbarLayout::indexOf(TaskItem *item) const {
	const int N = m_items.size();
	
	for (int index = 0; index < N; ++ index) {
		if (m_items[index]->item == item) {
			return index;
		}
	}

	return -1;
}

void TaskbarLayout::takeFrom(TaskbarLayout *other) {
	Q_ASSERT(other != NULL);

	if (other == this) {
		return;
	}

	m_currentIndex     = other->m_currentIndex;
	m_draggedItem      = other->m_draggedItem;
	m_currentAnimation = other->m_currentAnimation;
	m_mouseIn          = other->m_mouseIn;
	m_grabPos          = other->m_grabPos;
	m_items.append(other->m_items);

	foreach (TaskbarItem *item, other->m_items) {
		item->item->setParentLayoutItem(this);
		other->disconnectItem(item->item);
		connectItem(item->item);
	}

	other->m_draggedItem  = NULL;
	other->m_currentIndex = -1;
	other->m_mouseIn      = false;
	other->m_items.clear();
	other->stopAnimation();

	if (m_currentAnimation != None) {
		startAnimation();
	}

	invalidate();
}

int TaskbarLayout::dragItem(TaskItem *item, QDrag *drag, const QPointF& pos) {
	if (m_draggedItem != NULL) {
		qWarning("TaskbarLayout::dragItem: already dragging");
		return -1;
	}

	int index = indexOf(item);

	if (index == -1) {
		qWarning("TaskbarLayout::dragItem: invalid item");
		return -1;
	}

	m_mouseIn      = true;
	m_draggedItem  = m_items[index];
	m_currentIndex = index;
	m_grabPos      = pos - m_draggedItem->item->geometry().topLeft();

	bool enabled = m_draggedItem->item->graphicsItem()->isEnabled();

	m_draggedItem->item->graphicsItem()->setZValue(1);
	m_draggedItem->item->graphicsItem()->setEnabled(false);

	int dropIndex = index;

	m_currentAnimation |= Move;
	if (drag->exec(Qt::MoveAction) == Qt::IgnoreAction || drag->target() == drag->source()) {
		dropIndex = currentDragIndex();
	}

	if (m_draggedItem == NULL) {
		qDebug("TaskbarLayout::dragItem: item was deleted during dragging");
	}
	else if (m_draggedItem->item != item) {
		qWarning(
			"TaskbarLayout::dragItem: dragged item changed during dragging!?\n"
			"This _might_ cause a memleak under some circumstances.");
		return -1;
	}
	else {
		m_draggedItem->item->graphicsItem()->setZValue(0);
		m_draggedItem->item->graphicsItem()->setEnabled(enabled);

		if (dropIndex >= 0) {
			// move dropped item animated to dest:
			m_currentAnimation |= Move;
			startAnimation();
		}
	}

	m_currentIndex = -1;
	m_draggedItem  = NULL;

	return dropIndex;
}

void TaskbarLayout::moveDraggedItem(const QPointF& pos) {
	if (m_draggedItem == NULL) {
		return;
	}

	m_mouseIn = true;
	QRectF rect(m_draggedItem->item->geometry());
	QRectF effectiveRect(effectiveGeometry());

	if (m_grabPos.y() > rect.height()) {
		m_grabPos.setY(rect.height() * 0.5);
	}
	
	if (m_grabPos.x() > rect.width()) {
		m_grabPos.setX(rect.width() * 0.5);
	}

	QPointF newPos(pos - m_grabPos);

	if (newPos.y() < effectiveRect.top()) {
		newPos.setY(effectiveRect.top());
	}
	else if (newPos.y() + rect.height() > effectiveRect.bottom()) {
		newPos.setY(effectiveRect.bottom() - rect.height());
	}
	
	if (newPos.x() < effectiveRect.left()) {
		newPos.setX(effectiveRect.left());
	}
	else if (newPos.x() + rect.width() > effectiveRect.right()) {
		newPos.setX(effectiveRect.right() - rect.width());
	}

	rect.moveTopLeft(newPos);

	m_draggedItem->item->setGeometry(rect);

	int row   = 0;
	int index = indexOf(pos, &row);

	if (index == m_items.size()) {
		-- index;
	}
	
	m_items.move(m_currentIndex, index);
	m_currentIndex     = index;
	m_draggedItem->row = row;
	m_currentAnimation |= Move;
	doLayout();
}

void TaskbarLayout::dragLeave() {
	if (m_draggedItem == NULL) {
		return;
	}

	m_mouseIn = false;
	startAnimation();
}

void TaskbarLayout::animate(TaskbarItem *item, qreal move, qreal expand) {
	QRectF rect(item->item->geometry());
	
	if (item != m_draggedItem || !m_mouseIn) {
		qreal x = rect.x();
		qreal y = rect.y();
		
		if (item->animation & MoveY) {
			if (y < item->destY) {
				y += move;
				if (y >= item->destY) {
					y = item->destY;
					item->animation &= ~MoveY;
				}
			}
			else {
				y -= move;
				if (y <= item->destY) {
					y = item->destY;
					item->animation &= ~MoveY;
				}
			}
			
			rect.moveTop(y);
		}
		
		if (item->animation & MoveX) {
			if (x < item->destX) {
				x += PIXELS_PER_SECOND / m_fps;
				if (x >= item->destX) {
					x = item->destX;
					item->animation &= ~MoveX;
				}
			}
			else {
				x -= PIXELS_PER_SECOND / m_fps;
				if (x <= item->destX) {
					x = item->destX;
					item->animation &= ~MoveX;
				}
			}
			
			rect.moveLeft(x);
		}
	}

	if (item->animation & ResizeCollapse) {
		item->expansion -= expand;

		if (item->expansion <= 0.0) {
			item->expansion = 0.0;
			item->animation &= ~ResizeCollapse;
		}
	}
	else if (item->animation & ResizeExpand) {
		item->expansion += expand;

		if (item->expansion >= m_expandedWidth) {
			item->expansion = m_expandedWidth;
			item->animation &= ~ResizeExpand;
		}
	}
	
	item->item->setGeometry(rect);
}

void TaskbarLayout::animate() {
	int   now         = Midnight.msecsTo(QTime::currentTime());
	int   msecs       = now < m_timeStamp ? 1000 / m_fps : now - m_timeStamp;
	int   didAnimate  = None;
	int   willAnimate = None;
	qreal move        = msecs * PIXELS_PER_SECOND / 1000;
	qreal expand      = msecs * m_expandedWidth / m_expandDuration;
	m_timeStamp = now;

	foreach (TaskbarItem *item, m_items) {
		if (item->animation != None) {
			didAnimate |= item->animation;
			animate(item, move, expand);
			willAnimate |= item->animation;
		}
	}

	if (willAnimate == None) {
		stopAnimation();
	}
	
	if (didAnimate & Resize) {
		invalidate();
	}
	
	m_currentAnimation = willAnimate;
}

void TaskbarLayout::disconnectItem(TaskItem *item) {
	disconnect(item, SIGNAL(expand(TaskItem*,ExpansionDirection)), this, SLOT(expandItem(TaskItem*,ExpansionDirection)));
}

void TaskbarLayout::startAnimation() {
	if (m_animationsEnabled && !m_animationTimer->isActive()) {
		m_timeStamp = Midnight.msecsTo(QTime::currentTime());
		m_animationTimer->start();
	}
}

void TaskbarLayout::stopAnimation() {
	m_animationTimer->stop();
	m_currentAnimation = None;
}

void TaskbarLayout::skipAnimation() {
	stopAnimation();

	foreach (TaskbarItem *item, m_items) {
		QRectF rect(item->item->geometry());

		if (item != m_draggedItem || !m_mouseIn) {
			rect.moveTop(item->destY);
			rect.moveLeft(item->destX);
		}

		switch (item->direction) {
		case Collapse:
			item->expansion = 0.0;
			break;
		case Expand:
			item->expansion = m_expandedWidth;
			break;
		}
		
		item->item->setGeometry(rect);
	}

	// TODO: maybe only call invalidate if necesarry
	invalidate();
}

TaskItem *TaskbarLayout::draggedItem() const {
	if (m_draggedItem) {
		return m_draggedItem->item;
	}
	else {
		return NULL;
	}
}

void TaskbarLayout::clear(bool forceDeleteItems) {
	stopAnimation();

	while (!m_items.isEmpty()) {
		TaskbarItem *item = m_items.takeLast();
		TaskItem *titem = item->item;

		if (titem != NULL) {
			disconnectItem(titem);
			if (forceDeleteItems && !titem->ownedByLayout()) {
				delete titem;
				item->item = NULL;
			}
		}
		delete item;
	}

	if (m_draggedItem) {
		m_currentIndex = -1;
		m_draggedItem  = NULL;
	}
}

} // namespace SmoothTasks
