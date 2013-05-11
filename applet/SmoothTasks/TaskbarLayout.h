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
#ifndef SMOOTHTASKS_TASKBARLAYOUT_H
#define SMOOTHTASKS_TASKBARLAYOUT_H

#include <QGraphicsLayout>
#include <QList>
#include <QPointer>
#include <QObject>
#include <QTime>

#include "SmoothTasks/ExpansionDirection.h"
#include "SmoothTasks/TaskItem.h"

namespace SmoothTasks {

class TaskItem;
class TaskbarLayout;
class TaskbarItem;
class RowInfo;

class TaskbarLayout : public QObject, public QGraphicsLayout {
	Q_OBJECT
	Q_PROPERTY(Qt::Orientation orientation READ orientation WRITE setOrientation)
	Q_PROPERTY(qreal spacing READ spacing WRITE setSpacing)
	Q_PROPERTY(int fps READ fps WRITE setFps)
	Q_PROPERTY(bool animationsEnabled READ animationsEnabled WRITE setAnimationsEnabled)
	Q_PROPERTY(QGraphicsLayoutItem* draggedItem READ draggedItem)
	Q_PROPERTY(int minimumRows READ minimumRows WRITE setMinimumRows)
	Q_PROPERTY(int maximumRows READ maximumRows WRITE setMaximumRows)
	Q_PROPERTY(int rows READ rows)
	Q_PROPERTY(qreal cellWidth READ cellWidth)
	Q_PROPERTY(qreal cellHeight READ cellHeight)
	Q_PROPERTY(qreal expandedWidth READ expandedWidth WRITE setExpandedWidth)
	Q_PROPERTY(qreal aspectRatio READ aspectRatio WRITE setAspectRatio)
	Q_PROPERTY(qreal expandDuration READ expandDuration WRITE setExpandDuration)
	
	friend class TaskbarItem;
	
	public:
		enum TaskbarLayoutType {
			ByShape        = 0,
			MaxSqueeze     = 1,
			FixedItemCount = 2,
			FixedSize      = 3,
			LimitSqueeze   = 4
		};

		TaskbarLayout(
			Qt::Orientation orientation = Qt::Horizontal,
			QGraphicsLayoutItem *parent = NULL);
		~TaskbarLayout();

		void takeFrom(TaskbarLayout *other);
		int  dragItem(TaskItem *item, QDrag *drag, const QPointF& pos);
		void moveDraggedItem(const QPointF& pos);
		void dragLeave();

		QSizeF sizeHint(Qt::SizeHint which, const QSizeF& constraint = QSizeF()) const;
		void   setGeometry(const QRectF& rect);

		void      clear(bool forceDeleteItems = false);
		void      startAnimation();
		void      stopAnimation();
		void      skipAnimation();
		int       count() const { return m_items.size(); }
		void      expandAt(int index, ExpansionDirection direction);
		TaskItem *itemAt(int index) const;
		TaskItem *itemAt(const QPointF& pos) const;
		int       addItem(TaskItem *item, bool expanded);
		void      insertItem(int index, TaskItem *item, bool expanded);
		void      move(int fromIndex, int toIndex);
		void      removeAt(int index);
		void      removeItem(TaskItem *item);
		int       indexOf(TaskItem *item) const;
		int       rowOf(TaskItem *item) const;
		int       rowOf(int index) const;
		int       currentDragIndex() const { return m_currentIndex; }
		bool      isDragging() const { return m_draggedItem != NULL; }

		virtual int optimumCapacity() const = 0;

		virtual TaskbarLayoutType type() const = 0;

		TaskItem *draggedItem() const;

		Qt::Orientation orientation() const { return m_orientation; }
		void            setOrientation(Qt::Orientation orientation);

		qreal spacing() const { return m_spacing; }
		void  setSpacing(qreal spacing);

		int  fps() const { return m_fps; }
		void setFps(int fps);

		bool animationsEnabled() const { return m_animationsEnabled; }
		void setAnimationsEnabled(bool animationsEnabled);

		int  maximumRows() const { return m_maximumRows; }
		void setMaximumRows(int maximumRows);

		int  minimumRows() const { return m_minimumRows; }
		void setMinimumRows(int minimumRows);

		void setRowBounds(int minimumRows, int maximumRows);

		int rows() const { return m_rows; }

		qreal  cellWidth()  const { return m_cellHeight * m_aspectRatio; }
		qreal  cellHeight() const { return m_cellHeight; }
		QSizeF cellSize()   const { return QSizeF(m_cellHeight * m_aspectRatio, m_cellHeight); }

		qreal expandedWidth() const { return m_expandedWidth; }
		void  setExpandedWidth(qreal expandedWidth);

		qreal aspectRatio() const { return m_aspectRatio; }
		void  setAspectRatio(qreal aspectRatio);

		int  expandDuration() const { return m_expandDuration; }
		void setExpandDuration(int expandDuration);

		QSizeF preferredSize() const { return m_preferredSize; }
	
	public slots:
		void expandItem(TaskItem *item, ExpansionDirection direction) {
			expandAt(indexOf(item), direction);
		}

	protected:

		enum {
			None           =  0,
			MoveX          =  1,
			MoveY          =  2,
			Move           = MoveX | MoveY,
			ResizeCollapse =  4,
			ResizeExpand   =  8,
			Resize         = ResizeCollapse | ResizeExpand,
			FadeIn         = 16, // TODO
			FadeOut        = 32, // TODO
			Fade           = FadeIn | FadeOut
		};

		const QList<TaskbarItem*>& items() const { return m_items; }
		QRectF       effectiveGeometry()   const;
		qreal        additionalWidth()     const;
		int          currentAnimation()    const { return m_currentAnimation; }
		TaskbarItem *draggedTaskbarItem()  const { return m_draggedItem; }

		virtual int  rowOf(const QPointF& pos) const;
		virtual void doLayout() = 0;

		void buildRows(
			const int itemsPerRow, const qreal cellWidth,
			QList<RowInfo>& rowInfos, int& rows,
			qreal& maxPreferredRowWidth) const;

		void updateLayout(
			const int rows, const qreal cellWidth, const qreal cellHeight,
			const qreal availableWidth, const qreal maxPreferredRowWidth,
			const QList<RowInfo>& rowInfos, const QRectF& effectiveRect);

	signals:
		void sizeHintChanged(Qt::SizeHint which);

	private slots:
		void animate();

	private:
		static const qreal PIXELS_PER_SECOND;

		int indexOf(const QPointF& pos, int *row = NULL) const;
		void animate(TaskbarItem *item, qreal move, qreal expand);
		void connectItem(TaskItem *item);
		void disconnectItem(TaskItem *item);

		TaskbarItem         *m_draggedItem;
		int                  m_currentIndex;
		int                  m_currentAnimation;
		bool                 m_mouseIn;
		QList<TaskbarItem*>  m_items;
		Qt::Orientation      m_orientation;
		qreal                m_spacing;
		QTimer              *m_animationTimer;
		QPointF              m_grabPos;
		int                  m_fps;
		bool                 m_animationsEnabled;
		int                  m_minimumRows;
		int                  m_maximumRows; // use INT_MAX for "no" maximum
		qreal                m_expandedWidth;
		qreal                m_aspectRatio;
		int                  m_expandDuration;
		int                  m_timeStamp;

		const static QTime   Midnight;

	protected:
		QSizeF               m_preferredSize;
		qreal                m_cellHeight;
		int                  m_rows;
};

class TaskbarItem {
	
	public:
		
		TaskbarItem(
			TaskItem *item,
			qreal expansion,
			ExpansionDirection direction,
			int animation = TaskbarLayout::None)
			: item(item), destX(0.0), destY(0.0),
			  isNew(true), expansion(expansion),
			  row(0), direction(direction),
			  animation(animation) {}
		~TaskbarItem();

		TaskItem          *item;
		qreal              destX;
		qreal              destY;
		bool               isNew;
		qreal              expansion;
		int                row;
		ExpansionDirection direction;
		int                animation;
};

class RowInfo {

	public:
		RowInfo(qreal preferredWidth, qreal minimumWidth, int startIndex, int endIndex)
			: preferredWidth(preferredWidth),
			  minimumWidth(minimumWidth),
			  startIndex(startIndex),
			  endIndex(endIndex) {}

		qreal preferredWidth;
		qreal minimumWidth;
		int   startIndex;
		int   endIndex;
};

} // namespace SmoothTasks
#endif
