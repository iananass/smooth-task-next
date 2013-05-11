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
#ifndef SMOOTHTASKS_CLOSEICON_H
#define SMOOTHTASKS_CLOSEICON_H

#include <QWidget>

#include "SmoothTasks/Applet.h"
#include "SmoothTasks/WindowPreview.h"
#include "SmoothTasks/ToggleAnimation.h"

namespace SmoothTasks {

class CloseIcon : public QWidget {
	Q_OBJECT

public:
	CloseIcon(WindowPreview *preview);
	~CloseIcon() {}

private:
	WindowPreview  *m_preview;
	ToggleAnimation m_highlite;

private slots:
	void animate();

protected:
	void paintEvent(QPaintEvent *event);
	void mousePressEvent(QMouseEvent *event);
	void mouseDoubleClickEvent(QMouseEvent *event);
	void enterEvent(QEvent *event);
	void leaveEvent(QEvent *event);
};

} // namespace SmoothTasks
#endif
