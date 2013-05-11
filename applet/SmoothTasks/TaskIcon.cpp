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

// Smooth Tasks
#include "SmoothTasks/TaskIcon.h"
#include "SmoothTasks/TaskItem.h"
#include "SmoothTasks/Applet.h"

// Qt
#include <QTimer>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QFont>
#include <QApplication>

// Plasma
#include <Plasma/Applet>
#include <Plasma/PaintUtils>

// KDE
#include <KIcon>
#include <KIconEffect>
#include <KIconLoader>

// Std C++
#include <iterator>

namespace SmoothTasks {

TaskIcon::TaskIcon(TaskItem *item)
	: QObject(item),
	 m_item(item),
	 m_icon(),
	 m_highlightColor(0),
	 m_rect(),
	 m_pixmap(),
	 m_animationRepeater(new QTimer(this)),
	 m_currentAnimationDuration(0),
	 m_animation(0),
	 m_progress(0.0) {
}

TaskIcon::~TaskIcon() {
	if (m_animation) {
		Plasma::Animator::self()->stopCustomAnimation(m_animation);
	}
}

QRgb TaskIcon::highlightColor() const {
	Applet *applet = m_item->applet();
	if (m_highlightColor != 0 && applet->lightColorFromIcon()) {
		return m_highlightColor;
	}
	else {
		return applet->lightColor().rgb();
	}
}

void TaskIcon::updatePos() {
	qreal size     = this->size();
	QSize iconSize = m_icon.actualSize(QSize(size, size));
	QRectF boundingRect;
	const QSizeF& cellSize(m_item->cellSize());
	const bool isVertical = m_item->orientation() == Qt::Vertical;

	if (isVertical) {
		boundingRect = QRectF(
			m_rect.left(),
			qMax(m_rect.top(), m_rect.bottom() - cellSize.width()),
			qMin(cellSize.height(), m_rect.width()),
			qMin(cellSize.width(),  m_rect.height()));
	}
	else {
		boundingRect = QRectF(m_rect.left(), m_rect.top(),
			qMin(cellSize.width(),  m_rect.width()),
			qMin(cellSize.height(), m_rect.height()));
	}

	QPointF center(boundingRect.center());
	m_pos.setX(center.x() - (iconSize.width()  * 0.5));
	m_pos.setY(center.y() - (iconSize.height() * 0.5));

	if (QApplication::isRightToLeft()) {
		if (isVertical) {
			m_pos.setY(m_rect.height() - m_pos.y() - iconSize.height());
		}
		else {
			m_pos.setX(m_rect.width() - m_pos.x() - iconSize.width());
		}
	}
}

void TaskIcon::paint(QPainter *p, qreal hover, bool isGroup) {
	m_pixmap = m_icon.pixmap(size());

	if (m_pixmap.isNull()) {
		kDebug() << "TaskIcon pixmap is null";
		return;
	}

	if (m_animation) {
		animationStartup(m_progress);
	}

	if (hover > 0.0) {
		animationHover(hover);
	}

	if (isGroup) {
		QPainter iconPainter(&m_pixmap);
		QPixmap  icon(KIcon("document-multiple").pixmap(
			m_pixmap.width()  * 0.45,
			m_pixmap.height() * 0.45));

		iconPainter.drawPixmap(
			m_pixmap.width()  - icon.width(),
			m_pixmap.height() - icon.height(),
			icon);

		iconPainter.end();
	}
	p->drawPixmap(m_pos, m_pixmap);
}

void TaskIcon::setRect(const QRectF& rect) {
	if (rect != m_rect) {
		m_rect = rect;
		updatePos();
	}
}

void TaskIcon::setIcon(const QIcon& icon) {
	m_icon = icon;
	m_highlightColor = dominantColor();
	updatePos();
}

qreal TaskIcon::size() const {
	qreal size = qMin(m_rect.width(), m_rect.height()) * m_item->applet()->iconScale();

	if (size < 1.0) {
		size = 1.0;
	}

	return size;
}

void TaskIcon::startStartupAnimation(int duration) {
	m_currentAnimationDuration = duration;

	m_animationRepeater->setInterval(duration);
	m_animationRepeater->start();

	repeatAnimation();

	connect(m_animationRepeater, SIGNAL(timeout()), this, SLOT(repeatAnimation()));
}

void TaskIcon::stopStartupAnimation() {
	m_animationRepeater->stop();

	if (m_animation) {
		Plasma::Animator::self()->stopCustomAnimation(m_animation);
		m_animation = 0;
	}
}

void TaskIcon::repeatAnimation() {
	if (m_animation) {
		Plasma::Animator::self()->stopCustomAnimation(m_animation);
	}

	m_animation = Plasma::Animator::self()->customAnimation(
		m_item->applet()->fps() * m_currentAnimationDuration / 1000,
		m_currentAnimationDuration,
		Plasma::Animator::LinearCurve,
		this, "animation");
}

void TaskIcon::animation(qreal progress) {
	m_progress = progress;
	emit update();
}

void TaskIcon::animationHover(qreal hover) {
	KIconEffect *effect = KIconLoader::global()->iconEffect();

	if (effect->hasEffect(KIconLoader::Desktop, KIconLoader::ActiveState)) {
		if (qFuzzyCompare(qreal(1.0), hover)) {
			m_pixmap = effect->apply(
				m_pixmap,
				KIconLoader::Desktop,
				KIconLoader::ActiveState);
		}
		else if (hover != 0.0) {
			m_pixmap = Plasma::PaintUtils::transition(
				m_pixmap,
				effect->apply(
					m_pixmap,
					KIconLoader::Desktop,
					KIconLoader::ActiveState),
				hover);
		}
	}
}

void TaskIcon::animationStartup(qreal progress) {
	QPixmap pixmap = QPixmap(m_pixmap.width(), m_pixmap.height());
	pixmap.fill(Qt::transparent);
	int width;
	int height;

	if (progress < 0.5) {
		width  = m_pixmap.width()  * (0.5 + progress * 0.5);
		height = m_pixmap.height() * (1.0 - progress * 0.5);
	}
	else {
		width  = m_pixmap.width()  * (1.0 - progress * 0.5);
		height = m_pixmap.height() * (0.5 + progress * 0.5);
	}

	QPixmap scaled = m_pixmap.scaled(
		width, height,
		Qt::IgnoreAspectRatio,
		Qt::SmoothTransformation);

	if (!scaled.isNull()) {
		QPainter pixmapPainter(&pixmap);
		pixmapPainter.drawPixmap(
			(m_pixmap.width()  - width)  / 2,
			(m_pixmap.height() - height) / 2,
			scaled);


		pixmapPainter.end();
	}
	m_pixmap = pixmap;

	QPixmap transparentPixmap = QPixmap(m_pixmap.width(), m_pixmap.height());
	transparentPixmap.fill(Qt::transparent);
	m_pixmap = Plasma::PaintUtils::transition(transparentPixmap, m_pixmap, 0.85);
}

QRgb TaskIcon::averageColor() const {
	// Computes, and returns average color of the icon image.
	// Added by harsh@harshj.com for color hot-tracking support.
	QImage image(m_icon.pixmap(size()).toImage());
	unsigned int r(0), g(0), b(0);
	unsigned int count = 0;

	for(int x = 0; x < image.width(); ++ x) {
		for(int y = 0; y < image.height(); ++ y) {
			QRgb color = image.pixel(x, y);
			
			if (qAlpha(color) != 0) {
				r += qRed(color);
				g += qGreen(color);
				b += qBlue(color);
				++ count;
			}
		}
	}
	r /= count;
	g /= count;
	b /= count;

	return qRgb(r, g, b);
}

static bool hsvLess(const QColor& c1, const QColor& c2) {
	int h1, s1, v1, h2, s2, v2;
	c1.getHsv(&h1, &s1, &v1);
	c2.getHsv(&h2, &s2, &v2);
	
	return
		(h1 << 16 | s1 << 8 | v1) <
		(h2 << 16 | s2 << 8 | v2);
	/*
	 * should be equivalent to this, but with less branches:
	int cmp = h1 - h2;
	
	if (cmp == 0) {
		cmp = s1 - s2;
		if (cmp == 0) {
			cmp = v1 - v2;
		}
	}
	return cmp < 0;
	*/
}

QRgb TaskIcon::meanColor() const {
	QImage image(m_icon.pixmap(size()).toImage());
	QVector<QColor> colors(image.width() * image.height());
	
	int count = 0;
	
	for(int x = 0; x < image.width(); ++ x) {
		for(int y = 0; y < image.height(); ++ y) {
			QRgb color = image.pixel(x, y);
			
			// only use non-(total-)transparent colors
			if (qAlpha(color) != 0) {
				colors[count] = color;
				++ count;
			}
		}
	}
	
	if (count == 0) {
		return 0;
	}

	colors.resize(count);
	qSort(colors.begin(), colors.end(), hsvLess);
	
	int mid = count / 2;
	if (count & 1) { // odd case
		return colors[mid].rgb();
	}
	else { // even case
		QColor color1(colors[mid]);
		QColor color2(colors[mid + 1]);
		
		int r = (color1.red()   + color2.red())   / 2;
		int g = (color1.green() + color2.green()) / 2;
		int b = (color1.blue()  + color2.blue())  / 2;
		
		return qRgb(r, g, b);
	}
}

static bool isNear(const QColor& c1, const QColor& c2) {
	int h1, s1, v1, h2, s2, v2;
	c1.getHsv(&h1, &s1, &v1);
	c2.getHsv(&h2, &s2, &v2);
	
	return
		qAbs(h1 - h2) <=  8 &&
		qAbs(s1 - s2) <= 16 &&
		qAbs(v1 - v2) <= 32;
}

QRgb TaskIcon::dominantColor() const {
	QImage image(m_icon.pixmap(size()).toImage());
	QVector<QColor> colors(image.width() * image.height());
	
	int count = 0;
	
	// find the mean color
	for(int x = 0; x < image.width(); ++ x) {
		for(int y = 0; y < image.height(); ++ y) {
			QRgb rgb = image.pixel(x, y);
			
			// only use non-(total-)transparent colors
			if (qAlpha(rgb) != 0) {
				QColor color(rgb);
				
				// only use colors that aren't too grey
				if (color.saturation() > 24) {
					colors[count] = color;
				
					++ count;
				}
			}
		}
	}
	
	if (count == 0) {
		return 0;
	}
	
	colors.resize(count);
	qSort(colors.begin(), colors.end(), hsvLess);
	
	int mid = count / 2;
	QColor midColor(colors[mid]);
	QColor* begin = colors.begin() + mid;
	
	// find similar colors before the mean:
	if (mid != 0) {
		-- begin;
	
		while (begin != colors.begin()) {
			if (isNear(*(begin - 1), midColor)) {
				-- begin;
			}
			else {
				break;
			}
		}
	}
	
	QColor* end = colors.begin() + mid;
	
	// find similar colors after the mean:
	while (end != colors.end()) {
		if (isNear(*end, midColor)) {
			++ end;
		}
		else {
			break;
		}
	}
	
	// average of similar colors:
	unsigned int r = 0, g = 0, b = 0;
	for (QColor* it = begin; it != end; ++ it) {
		r += it->red();
		g += it->green();
		b += it->blue();
	}
	
	int similarCount = std::distance(begin, end);
	QColor color(r / similarCount, g / similarCount, b / similarCount);
	int h, s, v;
	color.getHsv(&h, &s, &v);
	
	if (v < 196) {
		v = 196;
	}
	
	if (s < 128) {
		s = 128;
	}
	
	color.setHsv(h, s, v);
	
	return color.rgb();
}

} // namespace SmoothTasks
#include "TaskIcon.moc"
