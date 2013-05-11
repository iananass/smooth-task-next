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

#include "SmoothTasks/SmoothToolTip.h"
#include "SmoothTasks/ToolTipWidget.h"
#include "SmoothTasks/WindowPreview.h"

#include <QBoxLayout>

#include <Plasma/Theme>
#include <Plasma/WindowEffects>

namespace SmoothTasks {

const int ToolTipWidget::ANIMATION_MARGIN = 25;
const int ToolTipWidget::SCROLL_DURATION  =  5;


ToolTipWidget::ToolTipWidget(SmoothToolTip* toolTip)
	: QWidget(), m_toolTip(toolTip) {
	
	setWindowFlags(Qt::ToolTip | Qt::WindowStaysOnTopHint);
	setWindowModality(Qt::NonModal);
	setAttribute(Qt::WA_TranslucentBackground);
	setMouseTracking(true);
	setAcceptDrops(true);

	Plasma::WindowEffects::overrideShadow(winId(), true);
	
	QBoxLayout *layout = new QBoxLayout(toolTip->isVertical() ?
		QBoxLayout::TopToBottom : QBoxLayout::LeftToRight);
	layout->setContentsMargins(0, 0, 0, 0);
	
	setLayout(layout);
}

void ToolTipWidget::paintEvent(QPaintEvent *event) {
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setClipRect(event->rect());
	painter.setCompositionMode(QPainter::CompositionMode_Source);
	painter.fillRect(rect(), Qt::transparent);

	m_toolTip->m_background->paintFrame(&painter);
	m_toolTip->updatePreviews();
}

void ToolTipWidget::resizeEvent(QResizeEvent *event) {
	QWidget::resizeEvent(event);

	m_toolTip->m_background->resizeFrame(size());

	if (Plasma::Theme::defaultTheme()->windowTranslucencyEnabled()) {
		Plasma::WindowEffects::enableBlurBehind(winId(), true,
			m_toolTip->m_background->mask());
		clearMask();
	}
	else {
		setMask(m_toolTip->m_background->mask());
	}
}

void ToolTipWidget::enterEvent(QEvent *event) {
	m_toolTip->m_hover = true;
	event->accept();
}

void ToolTipWidget::dragEnterEvent(QDragEnterEvent *event) {
	m_toolTip->m_hover = true;
	event->accept();
}

void ToolTipWidget::leaveEvent(QEvent *event) {
	m_toolTip->leave();
	event->accept();
}

void ToolTipWidget::dragLeaveEvent(QDragLeaveEvent *event) {
	m_toolTip->leave();
	event->accept();
}

void ToolTipWidget::mousePressEvent(QMouseEvent *event) {
	m_toolTip->hide();
	event->accept();
}

void ToolTipWidget::mouseMoveEvent(QMouseEvent *event) {
	if (m_toolTip->m_moveAnimation) {
		return;
	}

	const QRect screenGeom = m_toolTip->m_applet->currentScreenGeometry();

	if (m_toolTip->m_applet->formFactor() == Plasma::Vertical) {
		const int height       = this->height();
		const int y            = this->y();
		const int mouseY       = event->globalY();
		const int screenTop    = screenGeom.top();
		const int screenBottom = screenTop + screenGeom.height();

		if (m_toolTip->m_dy == 0) {
			if (y < screenTop && mouseY - ANIMATION_MARGIN <= screenTop) {
				const int dy = screenTop - y;
				m_toolTip->startScrollAnimation(0, dy, qAbs(dy) * SCROLL_DURATION);
			}
			else if (y + height > screenBottom && mouseY + ANIMATION_MARGIN >= screenBottom) {
				const int dy = -(y + height - screenBottom);
				m_toolTip->startScrollAnimation(0, dy, qAbs(dy) * SCROLL_DURATION);
			}
		}
		else if (
				mouseY > screenTop    + ANIMATION_MARGIN &&
				mouseY < screenBottom - ANIMATION_MARGIN) {
			m_toolTip->stopScrollAnimation();
		}
	}
	else {
		const int width       = this->width();
		const int x           = this->x();
		const int mouseX      = event->globalX();
		const int screenLeft  = screenGeom.left();
		const int screenRight = screenLeft + screenGeom.width();

		if (m_toolTip->m_dx == 0) {
			if (x < screenLeft && mouseX - ANIMATION_MARGIN <= screenLeft) {
				const int dx = screenLeft - x;
				m_toolTip->startScrollAnimation(dx, 0, qAbs(dx) * SCROLL_DURATION);
			}
			else if (x + width > screenRight && mouseX + ANIMATION_MARGIN >= screenRight) {
				const int dx = -(x + width - screenRight);
				m_toolTip->startScrollAnimation(dx, 0, qAbs(dx) * SCROLL_DURATION);
			}
		}
		else if (
				mouseX > screenLeft  + ANIMATION_MARGIN &&
				mouseX < screenRight - ANIMATION_MARGIN) {
			m_toolTip->stopScrollAnimation();
		}
	}
}

void ToolTipWidget::hideEvent(QHideEvent *event) {
	Q_UNUSED(event);
	
	m_toolTip->m_previewsUpdated = false;
	m_toolTip->m_hover           = false;
	m_toolTip->stopEffect();
}

void ToolTipWidget::wheelEvent(QWheelEvent *event) {
	int newIndex;

	if (m_toolTip->m_previews.isEmpty()) {
		return;
	}
	else if (m_toolTip->m_hoverPreview == NULL) {
		newIndex = 0;
	}
	else if (m_toolTip->m_previews.size() == 1) {
		return;
	}
	else if (event->delta() < 0) {
		newIndex = m_toolTip->m_hoverPreview->index() + 1;

		if (newIndex >= m_toolTip->m_previews.size()) {
			newIndex = 0;
		}
	}
	else {
		newIndex = m_toolTip->m_hoverPreview->index() - 1;

		if (newIndex < 0) {
			newIndex = m_toolTip->m_previews.size() - 1;
		}
	}

	if (m_toolTip->m_hoverPreview) {
		m_toolTip->m_hoverPreview->hoverLeave();
	}
	WindowPreview *preview = m_toolTip->m_previews[newIndex];
	preview->hoverEnter();
	m_toolTip->moveTo(preview, event->pos());
}

} // namespace SmoothTasks
