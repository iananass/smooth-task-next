/***********************************************************************************
* Smooth Tasks
* Copyright (C) 2009 Marcin Baszczewski <marcin.baszczewski@gmail.com>
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

#include "SmoothTasks/SmoothToolTip.h"
#include "SmoothTasks/CloseIcon.h"

#include <Plasma/PaintUtils>

namespace SmoothTasks {

CloseIcon::CloseIcon(WindowPreview *preview)
	: QWidget(preview),
	  m_preview(preview),
	  m_highlite() {
	connect(
		&m_highlite, SIGNAL(animate(qreal)),
		this, SLOT(repaint()));
}

void CloseIcon::mousePressEvent(QMouseEvent *event) {
	Q_UNUSED(event);

	m_preview->closeTask();
}

void CloseIcon::mouseDoubleClickEvent(QMouseEvent *event) {
	QWidget::mouseDoubleClickEvent(event);
	
	// XXX: debug strange issue with doubled task items:
	m_preview->toolTip()->applet()->dumpItems();
}

void CloseIcon::enterEvent(QEvent *event) {
	Q_UNUSED(event);
	Applet *applet = m_preview->toolTip()->applet();
	m_highlite.startUp(applet->fps(), applet->animationDuration());
}

void CloseIcon::leaveEvent(QEvent *event) {
	Q_UNUSED(event);
	Applet *applet = m_preview->toolTip()->applet();
	m_highlite.startDown(applet->fps(), applet->animationDuration());
}

void CloseIcon::paintEvent(QPaintEvent *event) {
	Q_UNUSED(event);
	
	SmoothToolTip *toolTip = m_preview->toolTip();
	const qreal opacity = m_preview->highlite();

	if (opacity > qreal(0.0)) {
		QPainter painter(this);
		QPixmap  pixmap;
		
		if (m_highlite.atBottom()) {
			pixmap = toolTip->closeIcon();
		}
		else if (m_highlite.atTop()) {
			pixmap = toolTip->hoverCloseIcon();
		}
		else {
			pixmap = Plasma::PaintUtils::transition(
				toolTip->closeIcon(),
				toolTip->hoverCloseIcon(),
				m_highlite.value());
		}
		
		qreal x = qreal(width()  - pixmap.width())  * 0.5;
		qreal y = qreal(height() - pixmap.height()) * 0.5;
		painter.setOpacity(opacity);
		painter.drawPixmap(x, y, pixmap);
	}
}

void CloseIcon::animate() {
	repaint();
}

} // namespace SmoothTasks
