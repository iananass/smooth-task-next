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
#include "SmoothTasks/Global.h"

// Qt
#include <QFontMetrics>

// STD C++
#include <limits>

namespace SmoothTasks {

const QString TASK_ITEM        = QString::fromLatin1("SmoothTasks::TaskItem");
const QString NORMAL           = QString::fromLatin1("normal");
const QString ACTIVE           = QString::fromLatin1("active");
const QString MINIMIZED        = QString::fromLatin1("minimized");
const QString FOCUS            = QString::fromLatin1("focus");
const QString ATTENTION        = QString::fromLatin1("attention");
const QString HOVER            = QString::fromLatin1("hover");
const QString M                = QString::fromLatin1("M");
const int     DRAG_HOVER_DELAY = 500;

QSizeF layoutText(QTextLayout &layout, const QSizeF &constraints) {
	QFontMetrics metrics(layout.font());
	const qreal maxWidth  = constraints.width();
	const qreal maxHeight = constraints.height();
	int   leading     = metrics.leading();
	qreal height      = 0;
	qreal widthUsed   = 0;
	int   lineSpacing = metrics.lineSpacing();

	layout.beginLayout();
	forever {
		QTextLine line(layout.createLine());

		if (!line.isValid()) {
			break;
		}

		height += leading;

		// Make the last line that will fit infinitely long.
		// drawTextLayout() will handle this by fading the line out
		// if it won't fit in the constraints.
		if (height + 2 * lineSpacing > maxHeight) {
			line.setLineWidth(std::numeric_limits<qreal>::infinity());
			line.setPosition(QPointF(0, height));
			height += line.height();
			widthUsed = qMax(widthUsed, line.naturalTextWidth());
			break;
		}

		line.setLineWidth(maxWidth);
		line.setPosition(QPointF(0, height));

		height += line.height();
		widthUsed = qMax(widthUsed, line.naturalTextWidth());
	}
	layout.endLayout();

	return QSizeF(widthUsed, height);
}

} // namespace SmoothTasks
