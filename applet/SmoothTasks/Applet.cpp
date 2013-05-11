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
#include "SmoothTasks/Applet.h"
#include "SmoothTasks/TaskItem.h"
#include "SmoothTasks/Task.h"
#include "SmoothTasks/ByShapeTaskbarLayout.h"
#include "SmoothTasks/MaxSqueezeTaskbarLayout.h"
#include "SmoothTasks/FixedItemCountTaskbarLayout.h"
#include "SmoothTasks/FixedSizeTaskbarLayout.h"
#include "SmoothTasks/LimitSqueezeTaskbarLayout.h"
#include "SmoothTasks/SmoothToolTip.h"
#include "SmoothTasks/PlasmaToolTip.h"
#include "SmoothTasks/Global.h"

// Plasma
#include <Plasma/Theme>
#include <Plasma/ToolTipManager>
#include <Plasma/Corona>
#include <Plasma/Containment>
#include <Plasma/FrameSvg>

// Qt
#include <QGraphicsView>
#include <QDesktopWidget>
#include <QGraphicsLinearLayout>
#include <QBuffer>
#include <QGraphicsSceneMouseEvent>

// KDE
#include <KLocale>
#include <KRun>
#include <KMenu>
#include <KConfigDialog>
#include <KWindowSystem>

namespace SmoothTasks {

  class GroupManager : public TaskManager::GroupManager
  {
  public:
    GroupManager(Plasma::Applet *applet)
      : TaskManager::GroupManager(applet),
	m_applet(applet)
    {
    }

  protected:
    KConfigGroup config() const
    {
      return m_applet->config();
    }

  private:
    Plasma::Applet *m_applet;
  };

Applet::Applet(QObject *parent, const QVariantList &args)
		: Plasma::Applet(parent, args),
		  m_frame(new Plasma::FrameSvg(this)),
		  m_groupManager(new GroupManager(this)),
		  m_rootGroup(m_groupManager->rootGroup()),
		  m_toolTip(new SmoothToolTip(this)),
		  m_layout(new LimitSqueezeTaskbarLayout(0.6, false, (formFactor() == Plasma::Vertical) ?
			Qt::Vertical : Qt::Horizontal,
			this)),
		  m_tasksHash(),
		  m_configG(),
		  m_configA(),
		  m_groupingStrategy(TaskManager::GroupManager::ProgramGrouping),
		  m_sortingStrategy(TaskManager::GroupManager::AlphaSorting),
		  m_taskSpacing(5),
		  m_iconScale(80),
		  m_lights(true),
		  m_expandTasks(false),
		  m_keepExpanded(ExpandNone),
		  m_expandOnHover(true),
		  m_expandOnAttention(false),
		  m_lightColor(78, 196, 249, 200),
		  m_shape(RectangelIcon),
		  m_activeIconIndex(0),
		  m_previewLayout(NewPreviewLayout),
		  m_middleClickAction(NoAction),
		  m_maxPreviewSize(200),
		  m_tooltipMoveDuration(500),
		  m_highlightDelay(50),
		  m_itemsPerRow(14),
		  m_squeezeRatio(0.6),
		  m_preferGrouping(false),
		  m_itemHeight(40),
		  m_rowAspectRatio(1.5),
		  m_dontRotateFrame(false),
		  m_onlyLights(true),
		  m_textShadow(true),
		  m_lightColorFromIcon(true),
		  m_scrollSwitchTasks(true) {
	KGlobal::locale()->insertCatalog("plasma_applet_smooth-tasks");
	
	setAcceptsHoverEvents(true);
	setAspectRatioMode(Plasma::IgnoreAspectRatio);
	setHasConfigurationInterface(true);
	setAcceptDrops(true);
		
	if (formFactor() == Plasma::Vertical) {
		resize(58, 500);
	}
	else {
		resize(500, 58);
	}
}

Applet::~Applet() {
	// to be sure that nothing gets called on a half deleted applet:
	disconnect(
		m_groupManager, SIGNAL(reload()),
		this, SLOT(reload()));
		
	disconnectRootGroup();

	m_toolTip->hide();
	clear();

	// be VERY carefull with the deletions
	// maybe this is too carefull but better to be too carefull
	ToolTipBase               *toolTip      = m_toolTip;
	Plasma::FrameSvg          *frame        = m_frame;
	TaskManager::GroupManager *groupManager = m_groupManager;

	m_toolTip      = NULL;
	m_frame        = NULL;
	m_groupManager = NULL;

	delete toolTip;
	delete frame;
	delete groupManager;
}

void Applet::init() {
	m_frame->setImagePath("widgets/tasks");
	m_frame->setCacheAllRenderedFrames(false);
	m_frame->setEnabledBorders(Plasma::FrameSvg::AllBorders);
	m_frame->setElementPrefix("normal");

	Plasma::Containment* appletContainment = containment();
	
	if (appletContainment) {
		m_groupManager->setScreen(appletContainment->screen());
	}
	
	connect(
		this, SIGNAL(settingsChanged()),
		this, SLOT(reconnectGroupManager()));

	connectRootGroup();
		
	connect(
		m_groupManager, SIGNAL(reload()),
		this, SLOT(reload()));

	connect(
		this, SIGNAL(settingsChanged()),
		this, SLOT(configuration()));

	connect(
		KWindowSystem::self(), SIGNAL(currentDesktopChanged(int)),
		this, SLOT(currentDesktopChanged()));

	m_layout->setContentsMargins(0, 0, 0, 0);
	m_layout->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	m_layout->setMaximumSize(INT_MAX, INT_MAX);
	setLayout(m_layout);
	connect(
		m_layout, SIGNAL(sizeHintChanged(Qt::SizeHint)),
		this, SIGNAL(sizeHintChanged(Qt::SizeHint)));
	emit settingsChanged();
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	setMaximumSize(INT_MAX, INT_MAX);
}

void Applet::reconnectGroupManager() {
	m_groupManager->reconnect();
	reload();
}

void Applet::connectRootGroup() {
	TaskManager::TaskGroup *group = m_rootGroup.data();
	
	if (group) {
		connect(
			group, SIGNAL(itemAdded(AbstractGroupableItem*)),
			this, SLOT(itemAdded(AbstractGroupableItem*)));
		
		connect(
			group, SIGNAL(itemRemoved(AbstractGroupableItem*)),
			this, SLOT(itemRemoved(AbstractGroupableItem*)));
		
		connect(
			group, SIGNAL(itemPositionChanged(AbstractGroupableItem*)),
                        this, SLOT(itemPositionChanged(AbstractGroupableItem*)));
	}
}

void Applet::disconnectRootGroup() {
	disconnect(m_rootGroup.data(), NULL, this, NULL);
}

void Applet::itemAdded(AbstractGroupableItem* groupableItem) {
//	qDebug("itemAdded: 0x%lx \"%s\"", (unsigned long) groupableItem, qPrintable(groupableItem->name()));
	if (m_tasksHash.value(groupableItem) != NULL) {
		qWarning("Applet::itemAdded: item already exist: %s", qPrintable(groupableItem->name()));
		return;
	}

	TaskItem *item = new TaskItem(groupableItem, this);
	m_toolTip->registerItem(item);
	connect(item, SIGNAL(itemActive(TaskItem*)), this, SLOT(updateActiveIconIndex(TaskItem*)));
	
	if (groupableItem->itemType() == TaskManager::GroupItemType) {
		TaskManager::TaskGroup* group = static_cast<TaskManager::TaskGroup*>(groupableItem);
		connect(
			group, SIGNAL(itemAdded(AbstractGroupableItem*)),
			this, SLOT(updateFullLimit()));
		
		connect(
			group, SIGNAL(itemRemoved(AbstractGroupableItem*)),
			this, SLOT(updateFullLimit()));
	}
	
	int index = m_groupManager->rootGroup()->members().indexOf(groupableItem);

	m_layout->insertItem(index, item, item->isExpanded());
	m_tasksHash[groupableItem] = item;
	updateFullLimit();
	m_layout->activate();
}

void Applet::itemRemoved(AbstractGroupableItem* groupableItem) {
//	qDebug("itemRemoved: 0x%lx \"%s\"", (unsigned long) groupableItem, qPrintable(groupableItem->name()));
	TaskItem* item = m_tasksHash.take(groupableItem);
	if (item == NULL) {
		qWarning("Applet::itemRemoved: trying to remove non-existant task: %s", qPrintable(groupableItem->name()));
		return;
	}
	m_layout->removeItem(item);
	updateFullLimit();
	m_layout->activate();
	delete item;
}

void Applet::launcherAdded(LauncherItem* launcherItem) {	
	KConfigGroup cg = config();
	KConfigGroup launcherCg(&cg, "Launchers");
	
	QVariantList launcherProperties;
	launcherProperties.append(launcherItem->launcherUrl().url());
	launcherProperties.append(launcherItem->icon().name());
	launcherProperties.append(launcherItem->name());
	launcherProperties.append(launcherItem->genericName());
	
	if (launcherItem->icon().name().isEmpty()) {
		QPixmap pixmap = launcherItem->icon().pixmap(QSize(64,64));
		QByteArray bytes;
		QBuffer buffer(&bytes);
		buffer.open(QIODevice::WriteOnly);
		pixmap.save(&buffer, "PNG");
		launcherProperties.append(bytes.toBase64());
	}
	
	launcherCg.writeEntry(launcherItem->name(), launcherProperties);
}

void Applet::launcherRemoved(LauncherItem* launcherItem) {	
	KConfigGroup cg = config();
	KConfigGroup launcherCg(&cg, "Launchers");
	launcherCg.deleteEntry(launcherItem->name());
}

void Applet::currentDesktopChanged() {
	m_layout->skipAnimation();
}

void Applet::itemPositionChanged(AbstractGroupableItem* groupableItem) {
//	qDebug("itemPositionChanged: 0x%lx \"%s\"", (unsigned long) groupableItem, qPrintable(groupableItem->name()));
	TaskItem* item = m_tasksHash[groupableItem];
	int currentIndex = m_layout->indexOf(item);
	
	if (currentIndex == -1) {
		qWarning("Applet::itemPositionChanged: trying to move non-existant task: %s", qPrintable(groupableItem->name()));
		return;
	}
	
	int newIndex = m_groupManager->rootGroup()->members().indexOf(groupableItem);

	// if we changed the sorting manually it is already moved in the layout:
	// (it seems like this slot is not called in this case)
	if (m_layout->itemAt(newIndex) == item) {
		return;
	}

	m_layout->move(currentIndex, newIndex);
}

void Applet::clear() {
	m_tasksHash.clear();
	m_layout->clear(true);
}

void Applet::reload() {
	TaskManager::TaskGroup *group = m_groupManager->rootGroup();
	
	if (group != m_rootGroup.data()) {
		disconnectRootGroup();
		m_rootGroup = group;
		connectRootGroup();
	}

	reloadItems();
}

void Applet::reloadItems() {
	clear();
	
	foreach(AbstractGroupableItem* item, m_groupManager->rootGroup()->members()) {
		itemAdded(item);
	}
	KConfigGroup cg = config();
    
    //load launchers
    KConfigGroup launcherCg(&cg, "Launchers");
	foreach (const QString &key, launcherCg.keyList()) {
		QStringList item = launcherCg.readEntry(key, QStringList());
		if (item.length() >= 4) {
			KUrl url(item[0]);
			KIcon icon;
			if (!item[1].isEmpty()) {
				icon = KIcon(item[1]);
			} else if (item.length() >= 5) {
				QPixmap pixmap;
				QByteArray bytes = QByteArray::fromBase64(item[4].toAscii());
				pixmap.loadFromData(bytes);
				icon.addPixmap(pixmap);
			}
			QString name(item[2]);
			QString genericName(item[3]);
			m_groupManager->addLauncher(url, icon, name, genericName);
		}
	}
    
}

void Applet::updateFullLimit() {
	if (m_groupManager != NULL) {
		m_groupManager->setFullLimit(m_layout->optimumCapacity());
	}
}

void Applet::constraintsEvent(Plasma::Constraints constraints) {
	if (constraints & Plasma::ScreenConstraint) {
		Plasma::Containment* appletContainment = containment();
		if (appletContainment) {
			m_groupManager->setScreen(appletContainment->screen());
		}
	}

	if (constraints & Plasma::SizeConstraint) {
		updateFullLimit();
	}

	if (constraints & Plasma::LocationConstraint) {
		m_layout->setOrientation(formFactor() == Plasma::Vertical ?
			Qt::Vertical : Qt::Horizontal);
	}
	
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void Applet::hoverEnterEvent(QGraphicsSceneHoverEvent *event) {
	Q_UNUSED(event);
	emit mouseEnter();
}

void Applet::dragEnterEvent(QGraphicsSceneDragDropEvent *event) {
	Q_UNUSED(event);
	emit mouseEnter();
}

void Applet::dragMoveEvent(QGraphicsSceneDragDropEvent *event) {
	m_layout->moveDraggedItem(event->pos());
	Plasma::Applet::dragMoveEvent(event);
}

void Applet::dragLeaveEvent(QGraphicsSceneDragDropEvent *event) {
	m_layout->dragLeave();
	Plasma::Applet::dragLeaveEvent(event);
}

void Applet::dragItem(TaskItem *item, QGraphicsSceneMouseEvent *event) {
	bool isGroup = item->task()->type() == Task::GroupItem;
	QByteArray data;
	data.append("dummy");
	
	m_toolTip->hide();
	QMimeData* mimeData = new QMimeData();
	if(item->task()->type() == Task::LauncherItem) {
		QList<QUrl> urls;
		urls.append(item->task()->launcherItem()->launcherUrl());
		mimeData->setUrls(urls);
		mimeData->setData(TASK_ITEM, data);
	} else {
		mimeData->setData(TASK_ITEM, data);
	}
	
	item->task()->addMimeData(mimeData);

	QDrag *drag = new QDrag(event->widget());
	drag->setMimeData(mimeData);
	drag->setPixmap(item->task()->icon().pixmap(20));
//	drag->setDragCursor( set the correct cursor //TODO
	
	if (m_sortingStrategy == TaskManager::GroupManager::ManualSorting) {
		int fromIndex = m_layout->indexOf(item);
		int toIndex   = m_layout->dragItem(item, drag, item->pos() + event->pos());

		if (toIndex != -1) {
			m_groupManager->rootGroup()->moveItem(fromIndex, toIndex);
		}
		else if (isGroup) {
			// XXX: workaround because the pager takes only one task of a group:
			//      maybe allways do that?
			reloadItems();
		}
	}
	else {
		drag->exec(Qt::MoveAction);
	}
}

void Applet::dragTask(TaskManager::AbstractGroupableItem *task, QWidget *source) {
	QMimeData* mimeData = new QMimeData();
	if(task->itemType() == TaskManager::LauncherItemType) {
		TaskManager::LauncherItem *launcher = qobject_cast<TaskManager::LauncherItem*>(task);
		QList<QUrl> urls;
		urls.append(launcher->launcherUrl());
		mimeData->setUrls(urls);
	}
	
	task->addMimeData(mimeData);
	
	QDrag *drag = new QDrag(source);
	drag->setMimeData(mimeData);
	drag->setPixmap(task->icon().pixmap(20));
	//	drag->setDragCursor( set the correct cursor //TODO
	
//	qDebug("drag start");
	drag->exec(Qt::MoveAction);
//	Qt::DropAction action = drag->exec(Qt::MoveAction);
	
//	if (action & Qt::CopyAction) qDebug("CopyAction");
//	if (action & Qt::MoveAction) qDebug("MoveAction");
//	if (action & Qt::LinkAction) qDebug("LinkAction");
//	if (action & Qt::ActionMask) qDebug("ActionMask");
//	if (action & Qt::TargetMoveAction) qDebug("TargetMoveAction");
//	if (action & Qt::IgnoreAction) qDebug("IgnoreAction");
//	qDebug("action: %d", action);
}

void Applet::middleClickTask(TaskManager::AbstractGroupableItem* task) {
	switch (m_middleClickAction) {
	case Applet::CloseTask:
		task->close();
		break;
	case Applet::MoveToCurrentDesktop:
		task->toDesktop(KWindowSystem::currentDesktop());
	case Applet::NoAction:
		break;
	}
}

void Applet::dumpItems() {
	int maxlen = 10;
	int maxitems = m_layout->count();
	TaskManager::ItemList members = m_groupManager->rootGroup()->members();
	for (int i = 0; i < maxitems; ++ i) {
		int len = m_layout->itemAt(i)->task()->text().length();

		if (len > maxlen) {
			maxlen = len;
		}
	}

	if (members.size() > maxitems) {
		maxitems = members.size();
	}

	if (m_layout->count() > maxitems) {
		maxitems = m_layout->count();
	}

	qDebug()
		<< qPrintable(QString("%1 | %2 |")
			.arg(QString("root group").leftJustified(maxlen + 9))
			.arg(QString("m_layout").leftJustified(maxlen + 9)));

	QString hline = QString().leftJustified(maxlen + 9, '-');

	qDebug()
		<< qPrintable(QString("%1-+-%2-+---")
			.arg(hline)
			.arg(hline));

	for (int i = 0; i < maxitems; ++ i) {
		QString sRootGroup;
		QString sLayout;
		WId     winRootGroup = 0;
		WId     winLayout    = 0;

		if (i < members.size()) {
			sRootGroup = members[i]->name();
			TaskManager::TaskItem *item = qobject_cast<TaskManager::TaskItem*>(members[i]);
			
			if (item && item->task()) {
				winRootGroup = item->task()->window();
			}
		}

		if (i < m_layout->count()) {
			TaskItem* titem = dynamic_cast<TaskItem*>(m_layout->itemAt(i));
			if (titem && titem->task()) {
				sLayout = titem->task()->text();
				if (titem->task()->task()) {
					winLayout = titem->task()->task()->window();
				}
			}
		}
		bool allEqual = sRootGroup == sLayout && winRootGroup == winLayout;
		qDebug()
			<< qPrintable(QString("%1 %2 | %3 %4 |")
				.arg(QString::number(winRootGroup).rightJustified(8))
				.arg(sRootGroup.leftJustified(maxlen))
				.arg(QString::number(winLayout).rightJustified(8))
				.arg(sLayout.leftJustified(maxlen)))
			<< (allEqual ? "" : "X");
	}

	qDebug("\n");
}

void Applet::dropEvent(QGraphicsSceneDragDropEvent *event) {
	KUrl::List urls = KUrl::List::fromMimeData(event->mimeData());
	if (urls.count() == 1) {
		//check if url is a *.desktop file
		KUrl u = urls.first();
		if(u.isLocalFile() && u.fileName().endsWith(".desktop")) {
			m_groupManager->addLauncher(u);
		}
		return;
	}

	if (event->mimeData()->hasFormat(TASK_ITEM) &&
		(sortingStrategy() == TaskManager::GroupManager::ManualSorting) &&
		m_layout->isDragging()) {

		event->acceptProposedAction();
	}
}

void Applet::dragMoveEvent(const QPointF& pos) {
	m_layout->moveDraggedItem(pos);
}

int Applet::activeIndex() {
	int index = 0;
	for (int i = 0; i < m_layout->count(); ++ i) {
		TaskItem *item = m_layout->itemAt(i);
		if (item->task()->type() == Task::GroupItem) {
			foreach (AbstractGroupableItem *task, item->task()->group()->members()) {
				if (task->isActive()) {
					return index;
				}
				++ index;
			}
		}
		else if (item->task()->isActive()) {
			return index;
		}
		++ index;
	}
	return index;
}

void Applet::updateActiveIconIndex(TaskItem *item) {
	Q_UNUSED(item);
	//m_activeIconIndex = m_items.indexOf(item);
	m_activeIconIndex = activeIndex();
}

int Applet::totalSubTasks() {
	int count = 0;
	for (int i = 0; i < m_layout->count(); ++ i) {
		TaskItem *item = m_layout->itemAt(i);
		if (item->task()->type() == Task::GroupItem) {
			count += item->task()->group()->members().count();
		}
		else {
			++ count;
		}
	}
	return count;
}

AbstractGroupableItem* Applet::selectSubTask(int index) {
	for (int i = 0; i < m_layout->count(); ++ i) {
		TaskItem *item = m_layout->itemAt(i);
		if (item->task()->type() == Task::GroupItem) {
			TaskManager::ItemList members(item->task()->group()->members());
			const int N = members.count(); 
			if (index < N) {
				return members.at(index);
				//return m_tasksHash[item->task()->group()->members().at(index)];
			} 
			else {
				index -= N;
			}
		} 
		else if (index == 0) {
			return item->task()->abstractItem();
		}
		else {
			-- index;
		}
	}
	return NULL;
}

void Applet::wheelEvent(QGraphicsSceneWheelEvent *event) {
	if (!m_scrollSwitchTasks) {
		return;
	}
	
	int subTasks = totalSubTasks();
	// zero or one tasks don't cycle
	if (subTasks < 1) {
		return;
	}

	// mouse wheel down
	if (event->delta() < 0) {
		++ m_activeIconIndex;
		if (m_activeIconIndex >= subTasks) {
			m_activeIconIndex = 0;
		}
	// mouse wheel up
	} else {
		-- m_activeIconIndex;
		if (m_activeIconIndex < 0) {
			m_activeIconIndex = subTasks - 1; // last item is a spacer
		}
	}

	AbstractGroupableItem *taskItem = selectSubTask(m_activeIconIndex);
	if (taskItem && !taskItem->itemType() == TaskManager::GroupItemType) {
		TaskManager::TaskItem* task = static_cast<TaskManager::TaskItem*>(taskItem);
		if (task->task()) {
			task->task()->activate();
		}
	}
}

void Applet::newNotification(const QString& notif)
{
	qDebug() << "new notification" << notif;
}

void Applet::uiGroupingStrategyChanged(int index) {
	m_configG.onlyGroupWhenFull->setEnabled(
		m_configG.groupingStrategy->itemData(index) != TaskManager::GroupManager::NoGrouping);
}

void Applet::uiMinimumRowsChanged(int minimumRows) {
	if (minimumRows > m_configA.maximumRows->value()) {
		m_configA.maximumRows->setValue(minimumRows);
	}
}

void Applet::uiMaximumRowsChanged(int maximumRows) {
	if (maximumRows < m_configA.minimumRows->value()) {
		m_configA.minimumRows->setValue(maximumRows);
	}
}

void Applet::uiToolTipKindChanged(int index) {
	bool enabled = m_configA.toolTipKind->itemData(index) == ToolTipBase::Smooth;
	m_configA.previewLayout->setEnabled(enabled);
	m_configA.previewLayoutLabel->setEnabled(enabled);
	m_configA.maxPreviewSize->setEnabled(enabled);
	m_configA.maxPreviewSizeLabel->setEnabled(enabled);
	m_configA.tooltipMoveDuration->setEnabled(enabled);
	m_configA.tooltipMoveDurationLabel->setEnabled(enabled);
	m_configA.highlightDelay->setEnabled(enabled);
	m_configA.highlightDelayLabel->setEnabled(enabled);
}

void Applet::uiTaskbarLayoutChanged(int index) {
	int type = m_configA.taskbarLayout->itemData(index).toInt();
	bool enabled = type == TaskbarLayout::LimitSqueeze;
	m_configA.squeezeRatio->setEnabled(enabled);
	m_configA.squeezeRatioLabel->setEnabled(enabled);
	m_configA.preferGrouping->setEnabled(enabled);
	
	enabled = type == TaskbarLayout::FixedSize;
	m_configA.itemHeight->setEnabled(enabled);
	m_configA.itemHeightLabel->setEnabled(enabled);

	enabled = type == TaskbarLayout::FixedItemCount;
	m_configA.itemsPerRow->setEnabled(enabled);
	m_configA.itemsPerRowLabel->setEnabled(enabled);

	enabled = type == TaskbarLayout::ByShape;
	m_configA.rowAspectRatio->setEnabled(enabled);
	m_configA.rowAspectRatioLabel->setEnabled(enabled);
}

void Applet::createConfigurationInterface(KConfigDialog *parent) {
	connect(parent, SIGNAL(applyClicked()),   this, SLOT(configAccepted()));
	connect(parent, SIGNAL(okClicked()),      this, SLOT(configAccepted()));
	connect(parent, SIGNAL(defaultClicked()), this, SLOT(loadDefaults()));

	parent->showButton(KConfigDialog::Help, false);
	parent->enableButton(KConfigDialog::Default, true);

	QWidget *widgetG = new QWidget;
	m_configG.setupUi(widgetG);
	parent->addPage(widgetG, i18n("General"), icon());

	QWidget *widgetA = new QWidget;
	m_configA.setupUi(widgetA);
	parent->addPage(widgetA, i18n("Appearance"), "preferences-desktop-theme");

	// general
	m_configA.expandTasks->setChecked(m_expandTasks);

	// filters
	m_configG.showOnlyCurrentDesktop->setChecked(m_groupManager->showOnlyCurrentDesktop());
	m_configG.showOnlyCurrentScreen->setChecked(m_groupManager->showOnlyCurrentScreen());
	m_configG.showOnlyCurrentActivity->setChecked(m_groupManager->showOnlyCurrentActivity());
	m_configG.showOnlyMinimized->setChecked(m_groupManager->showOnlyMinimized());

	// grouping
	m_configG.groupingStrategy->addItem(i18n("Do Not Group"),    TaskManager::GroupManager::NoGrouping);
	//m_configG.groupingStrategy->addItem(i18n("Manually"),      TaskManager::GroupManager::ManualGrouping);
	m_configG.groupingStrategy->addItem(i18n("By Program Name"), TaskManager::GroupManager::ProgramGrouping);

	m_configG.onlyGroupWhenFull->setChecked(m_groupManager->onlyGroupWhenFull() ? Qt::Checked : Qt::Unchecked);

	connect(m_configG.groupingStrategy, SIGNAL(currentIndexChanged(int)), this, SLOT(uiGroupingStrategyChanged(int)));
	
	m_configG.groupingStrategy->setCurrentIndex(m_configG.groupingStrategy->findData(m_groupManager->groupingStrategy()));
	m_configG.onlyGroupWhenFull->setEnabled(m_groupManager->groupingStrategy() != TaskManager::GroupManager::NoGrouping);
	
	// sorting
	m_configG.sortingStrategy->addItem(i18n("Do Not Sort"),    TaskManager::GroupManager::NoSorting);
	m_configG.sortingStrategy->addItem(i18n("Manually"),       TaskManager::GroupManager::ManualSorting);
	m_configG.sortingStrategy->addItem(i18n("Alphabetically"), TaskManager::GroupManager::AlphaSorting);
	m_configG.sortingStrategy->addItem(i18n("By Desktop"),     TaskManager::GroupManager::DesktopSorting);
	
	m_configG.sortingStrategy->setCurrentIndex(m_configG.sortingStrategy->findData(m_groupManager->sortingStrategy()));
	
	m_configG.middleClickAction->addItem(i18n("No Action"),               NoAction);
	m_configG.middleClickAction->addItem(i18n("Close Task"),              CloseTask);
	m_configG.middleClickAction->addItem(i18n("Move to Current Desktop"), MoveToCurrentDesktop);
	m_configG.middleClickAction->setCurrentIndex(m_configG.middleClickAction->findData(m_middleClickAction));
	
	// apearance
	m_configA.shape->addItem(i18n("Rectangle"), RectangelIcon);
	m_configA.shape->addItem(i18n("Square"),    SquareIcon);
	m_configA.shape->setCurrentIndex(m_configA.shape->findData(m_shape));
	m_configA.dontRotateFrame->setChecked(m_dontRotateFrame);
	m_configA.onlyLights->setChecked(m_onlyLights);
	m_configA.textShadow->setChecked(m_textShadow);
	m_configA.lightColorFromIcon->setChecked(m_lightColorFromIcon);
	m_configA.scrollSwitchTasks->setChecked(m_scrollSwitchTasks);
	
	m_configA.minimumRows->setValue(m_layout->minimumRows());
	m_configA.maximumRows->setValue(m_layout->maximumRows());

	connect(m_configA.minimumRows, SIGNAL(valueChanged(int)), this, SLOT(uiMinimumRowsChanged(int)));
	connect(m_configA.maximumRows, SIGNAL(valueChanged(int)), this, SLOT(uiMaximumRowsChanged(int)));
	
	m_configA.taskSpacing->setValue(m_taskSpacing);
	m_configA.iconScale->setValue(m_iconScale);
	
	m_configA.lights->setChecked(m_lights);
	m_configA.lightColor->setColor(m_lightColor);
	
	m_configA.expandingSize->setValue(m_layout->expandedWidth());
	m_configA.fps->setValue(m_layout->fps());
	m_configA.animationDuration->setValue(m_layout->expandDuration());

	connect(m_configA.toolTipKind, SIGNAL(currentIndexChanged(int)), this, SLOT(uiToolTipKindChanged(int)));
	m_configA.toolTipKind->addItem(i18n("None"),   ToolTipBase::None);
	m_configA.toolTipKind->addItem(i18n("Smooth"), ToolTipBase::Smooth);
	m_configA.toolTipKind->addItem(i18n("Plasma"), ToolTipBase::Plasma);
	m_configA.toolTipKind->setCurrentIndex(m_configA.toolTipKind->findData(m_toolTip->kind()));
	
	connect(m_configA.taskbarLayout, SIGNAL(currentIndexChanged(int)), this, SLOT(uiTaskbarLayoutChanged(int)));
	m_configA.taskbarLayout->addItem(i18n("By Taskbar Shape"),          TaskbarLayout::ByShape);
	m_configA.taskbarLayout->addItem(i18n("Maximum Squeeze"),           TaskbarLayout::MaxSqueeze);
	m_configA.taskbarLayout->addItem(i18n("Fixed Item Number per Row"), TaskbarLayout::FixedItemCount);
	m_configA.taskbarLayout->addItem(i18n("Limited Squeeze"),           TaskbarLayout::LimitSqueeze);
	m_configA.taskbarLayout->addItem(i18n("Fixed Maximum Item Height"), TaskbarLayout::FixedSize);
	m_configA.taskbarLayout->setCurrentIndex(m_configA.taskbarLayout->findData(m_layout->type()));
	
	m_configA.itemsPerRow->setValue(m_itemsPerRow);
	m_configA.squeezeRatio->setValue(m_squeezeRatio);
	m_configA.preferGrouping->setChecked(m_preferGrouping);
	m_configA.itemHeight->setValue(m_itemHeight);
	m_configA.rowAspectRatio->setValue(m_rowAspectRatio);

	m_configA.keepExpanded->addItem(i18n("None"),                     ExpandNone);
	m_configA.keepExpanded->addItem(i18n("Active"),                   ExpandActive);
	m_configA.keepExpanded->addItem(i18n("From the current desktop"), ExpandCurrentDesktop);
	m_configA.keepExpanded->addItem(i18n("All"),                      ExpandAll);
	m_configA.keepExpanded->setCurrentIndex(m_configA.keepExpanded->findData(m_keepExpanded));
	
	m_configA.expandOnHover->setChecked(m_expandOnHover);
	m_configA.expandOnAttention->setChecked(m_expandOnAttention);
	
	m_configA.previewLayout->addItem(i18n("Classic"), ClassicPreviewLayout);
	m_configA.previewLayout->addItem(i18n("New"),     NewPreviewLayout);
	m_configA.previewLayout->setCurrentIndex(m_configA.previewLayout->findData(m_previewLayout));
	
	m_configA.maxPreviewSize->setValue(m_maxPreviewSize);
	m_configA.tooltipMoveDuration->setValue(m_tooltipMoveDuration);
	m_configA.highlightDelay->setValue(m_highlightDelay);

//	connect(m_configA.shape, SIGNAL(currentIndexChanged(int)), this, SLOT(widgetValueChanged()));
}

void Applet::loadDefaults() {
	qDebug("Applet::loadDefaults: TODO");
//	m_configDialog->enableButton(KConfigDialog::Default, true);
}

QSizeF Applet::sizeHint(Qt::SizeHint which, const QSizeF& constraint) const {
	if (m_layout && which == Qt::PreferredSize) {
		return m_layout->preferredSize();
	}
	else {
		return Plasma::Applet::sizeHint(which, constraint);
	}
}

void Applet::configAccepted() {
	KConfigGroup cg(config());
	bool changed = false;

	// general
	if ((bool) m_configA.expandTasks->checkState() != m_expandTasks) {
		cg.writeEntry("expandTasks", m_configA.expandTasks->checkState() == Qt::Checked);
		changed = true;
	}

	// sorting
	if (m_configG.sortingStrategy->currentIndex() == -1) {
		m_sortingStrategy = TaskManager::GroupManager::AlphaSorting;
	}
	else {
		int cfgSortingStrategy =
			m_configG.sortingStrategy->itemData(m_configG.sortingStrategy->currentIndex()).toInt();

		m_sortingStrategy =
				static_cast<TaskManager::GroupManager::TaskSortingStrategy>(cfgSortingStrategy);
	}

	if (m_groupManager->sortingStrategy() != m_sortingStrategy)  {
		m_groupManager->setSortingStrategy(m_sortingStrategy);

		cg.writeEntry("sortingStrategy", static_cast<int>(m_sortingStrategy));
		changed = true;
	}

	// grouping
	if (m_configG.groupingStrategy->currentIndex() == -1) {
		m_groupingStrategy = TaskManager::GroupManager::ProgramGrouping;
	}
	else {
		int cfgGroupingStrategy =
			m_configG.groupingStrategy->itemData(m_configG.groupingStrategy->currentIndex()).toInt();
		
		m_groupingStrategy =
				static_cast<TaskManager::GroupManager::TaskGroupingStrategy>(cfgGroupingStrategy);
	}

	if (m_groupManager->groupingStrategy() != m_groupingStrategy) {
		m_groupManager->setGroupingStrategy(m_groupingStrategy);

		cg.writeEntry("groupingStrategy", static_cast<int>(m_groupingStrategy));
		changed = true;
	}

	bool cfgOnlyGroupWhenFull = m_configG.onlyGroupWhenFull->isChecked();
	if (m_groupManager->onlyGroupWhenFull() != cfgOnlyGroupWhenFull) {
		m_groupManager->setOnlyGroupWhenFull(cfgOnlyGroupWhenFull);
		
		cg.writeEntry("onlyGroupWhenFull", cfgOnlyGroupWhenFull);
		changed = true;
	}
	
	int cfgMiddleClickAction = m_configG.middleClickAction->itemData(m_configG.middleClickAction->currentIndex()).toInt();
	if (cfgMiddleClickAction != m_middleClickAction) {
		cg.writeEntry("middleClickAction", cfgMiddleClickAction);
		changed = true;
	}

	// appearance
	if (m_configA.taskSpacing->value() != m_taskSpacing) {
		cg.writeEntry("taskSpacing", m_configA.taskSpacing->value());
		changed = true;
	}

	int cfgKeepExpanded = m_configA.keepExpanded->itemData(m_configA.keepExpanded->currentIndex()).toInt();
	if (cfgKeepExpanded != m_keepExpanded) {
		cg.writeEntry("keepExpanded", cfgKeepExpanded);
		changed = true;
	}
	
	if (m_configA.expandOnHover->isChecked() != m_expandOnHover) {
		cg.writeEntry("expandOnHover", m_configA.expandOnHover->isChecked());
		changed = true;
	}
	
	if (m_configA.expandOnAttention->isChecked() != m_expandOnAttention) {
		cg.writeEntry("expandOnAttention", m_configA.expandOnAttention->isChecked());
		changed = true;
	}

	int cfgToolTipKind = m_configA.toolTipKind->itemData(m_configA.toolTipKind->currentIndex()).toInt();
	if (cfgToolTipKind != m_toolTip->kind()) {
		cg.writeEntry("toolTipKind", cfgToolTipKind);
		changed = true;
	}

	int cfgTaskbarLayout = m_configA.taskbarLayout->itemData(m_configA.taskbarLayout->currentIndex()).toInt();
	if (cfgTaskbarLayout != m_layout->type()) {
		cg.writeEntry("taskbarLayout", cfgTaskbarLayout);
		changed = true;
	}

	int cfgPreviewLayout = m_configA.previewLayout->itemData(m_configA.previewLayout->currentIndex()).toInt();
	if (cfgPreviewLayout != m_previewLayout) {
		cg.writeEntry("previewLayout", cfgPreviewLayout);
		changed = true;
	}

	if (m_configA.maxPreviewSize->value() != m_maxPreviewSize) {
		cg.writeEntry("maxPreviewSize", m_configA.maxPreviewSize->value());
		changed = true;
	}
	
	if (m_configA.tooltipMoveDuration->value() != m_tooltipMoveDuration) {
		cg.writeEntry("tooltipMoveDuration", m_configA.tooltipMoveDuration->value());
		changed = true;
	}
	
	if (m_configA.highlightDelay->value() != m_highlightDelay) {
		cg.writeEntry("highlightDelay", m_configA.highlightDelay->value());
		changed = true;
	}

	if (m_configA.expandingSize->value() != m_layout->expandedWidth()) {
		cg.writeEntry("expandingSize", m_configA.expandingSize->value());
		changed = true;
	}

	if (m_configA.itemsPerRow->value() != m_itemsPerRow) {
		cg.writeEntry("itemsPerRow", m_configA.itemsPerRow->value());
		changed = true;
	}
	
	if (m_configA.squeezeRatio->value() != m_squeezeRatio) {
		cg.writeEntry("squeezeRatio", m_configA.squeezeRatio->value());
		changed = true;
	}
	
	if (m_configA.preferGrouping->isChecked() != m_preferGrouping) {
		cg.writeEntry("preferGrouping", m_configA.preferGrouping->isChecked());
		changed = true;
	}

	if (m_configA.itemHeight->value() != m_itemHeight) {
		cg.writeEntry("itemHeight", m_configA.itemHeight->value());
		changed = true;
	}

	if (m_configA.rowAspectRatio->value() != m_rowAspectRatio) {
		cg.writeEntry("rowAspectRatio", m_configA.rowAspectRatio->value());
		changed = true;
	}

	if (m_configA.iconScale->value() != m_iconScale) {
		cg.writeEntry("iconScale", m_configA.iconScale->value());
		changed = true;
	}

	if (m_configA.minimumRows->value() != m_layout->minimumRows()) {
		cg.writeEntry("minimumRows", m_configA.minimumRows->value());
		changed = true;
	}

	if (m_configA.maximumRows->value() != m_layout->maximumRows()) {
		cg.writeEntry("maximumRows", m_configA.maximumRows->value());
		changed = true;
	}
	
	if (m_configA.lightColor->color() != m_lightColor) {
		cg.writeEntry("lightColor", m_configA.lightColor->color());
		changed = true;
	}
	
	if (m_configA.fps->value() != m_layout->fps()) {
		cg.writeEntry("fps", m_configA.fps->value());
		changed = true;
	}

	if (m_configA.animationDuration->value() != m_layout->expandDuration()) {
		cg.writeEntry("animationDuration", m_configA.animationDuration->value());
		changed = true;
	}

	if (m_configA.lights->checkState() != m_lights) {
		cg.writeEntry("lights", m_configA.lights->checkState() == Qt::Checked);
		changed = true;
	}

	int cfgShape = m_configA.shape->itemData(m_configA.shape->currentIndex()).toInt();
	if (cfgShape != (int) m_shape) {
		cg.writeEntry("shape", cfgShape);
		changed = true;
	}
	
	bool cfgDontRotateFrame = m_configA.dontRotateFrame->isChecked();
	if (cfgDontRotateFrame != m_dontRotateFrame) {
		cg.writeEntry("dontRotateFrame", cfgDontRotateFrame);
		changed = true;
	}
	
	bool cfgOnlyLights = m_configA.onlyLights->isChecked();
	if (cfgOnlyLights != m_onlyLights) {
		cg.writeEntry("onlyLights", cfgOnlyLights);
		changed = true;
	}
	
	bool cfgTextShadow = m_configA.textShadow->isChecked();
	if (cfgTextShadow != m_textShadow) {
		cg.writeEntry("textShadow", cfgTextShadow);
		changed = true;
	}
	
	bool cfgLightColorFromIcon = m_configA.lightColorFromIcon->isChecked();
	if (cfgLightColorFromIcon != m_lightColorFromIcon) {
		cg.writeEntry("lightColorFromIcon", cfgLightColorFromIcon);
		changed = true;
	}
	
	bool cfgScrollSwitchTasks = m_configA.scrollSwitchTasks->isChecked();
	if (cfgScrollSwitchTasks != m_scrollSwitchTasks) {
		cg.writeEntry("scrollSwitchTasks", cfgScrollSwitchTasks);
		changed = true;
	}
	
	// filters 
	if (m_groupManager->showOnlyCurrentDesktop() != m_configG.showOnlyCurrentDesktop->isChecked()) {
		m_groupManager->setShowOnlyCurrentDesktop(!m_groupManager->showOnlyCurrentDesktop());
		cg.writeEntry("showOnlyCurrentDesktop", m_groupManager->showOnlyCurrentDesktop());
		changed = true;
	}
	if (m_groupManager->showOnlyCurrentActivity() != m_configG.showOnlyCurrentActivity->isChecked()) {
		m_groupManager->setShowOnlyCurrentActivity(!m_groupManager->showOnlyCurrentActivity());
		cg.writeEntry("showOnlyCurrentActivity", m_groupManager->showOnlyCurrentActivity());
		changed = true;
	}
	
	if (m_groupManager->showOnlyCurrentScreen() != m_configG.showOnlyCurrentScreen->isChecked()) {
		m_groupManager->setShowOnlyCurrentScreen(!m_groupManager->showOnlyCurrentScreen());
		cg.writeEntry("showOnlyCurrentScreen", m_groupManager->showOnlyCurrentScreen());
		changed = true;
	}

	if (m_groupManager->showOnlyMinimized() != m_configG.showOnlyMinimized->isChecked()) {
		m_groupManager->setShowOnlyMinimized(!m_groupManager->showOnlyMinimized());
		cg.writeEntry("showOnlyMinimized", m_groupManager->showOnlyMinimized());
		changed = true;
	}

	if (changed) {
		emit settingsChanged();
		emit configNeedsSaving();
	}
}

void Applet::widgetValueChanged() {
//	if (!m_configDialog.isNull()) {
//		// TODO: check whether state is default state
//		m_configDialog->enableButton(KConfigDialog::Default, true);
//	}
}

void Applet::configuration() {	
	KConfigGroup cg = config();

	m_taskSpacing  = cg.readEntry("taskSpacing", 5);
	m_layout->setSpacing(m_taskSpacing);
	m_iconScale    = cg.readEntry("iconScale", 80);
	m_lights       = cg.readEntry("lights", true);
	m_expandTasks  = cg.readEntry("expandTasks", false);

	int cfgMinimumRows = cg.readEntry("minimumRows", 1);
	int cfgMaximumRows = cg.readEntry("maximumRows", 3);

	m_layout->setRowBounds(cfgMinimumRows, cfgMaximumRows);

	int cfgToolTipKind = cg.readEntry("toolTipKind", (int) ToolTipBase::Smooth);

	if (cfgToolTipKind != m_toolTip->kind()) {
		for (int i = 0; i < m_layout->count(); ++ i) {
			m_toolTip->unregisterItem(m_layout->itemAt(i));
		}
		
		delete m_toolTip;

		switch (cfgToolTipKind) {
		case ToolTipBase::None:
			m_toolTip = new ToolTipBase(this);
			break;
		case ToolTipBase::Plasma:
			m_toolTip = new PlasmaToolTip(this);
			break;
		case ToolTipBase::Smooth:
		default:
			m_toolTip = new SmoothToolTip(this);
		}
		
		for (int i = 0; i < m_layout->count(); ++ i) {
			m_toolTip->registerItem(m_layout->itemAt(i));
		}
	}

	int cfgTaskbarLayout = cg.readEntry("taskbarLayout", (int) TaskbarLayout::LimitSqueeze);

	m_itemsPerRow = cg.readEntry("itemsPerRow", 14);

	if (m_itemsPerRow < 1) {
		m_itemsPerRow = 14;
	}

	if (cfgTaskbarLayout == m_layout->type() && cfgTaskbarLayout == TaskbarLayout::FixedItemCount) {
		FixedItemCountTaskbarLayout *layout = static_cast<FixedItemCountTaskbarLayout*>(m_layout);
		layout->setItemsPerRow(m_itemsPerRow);
	}
	
	m_preferGrouping = cg.readEntry("preferGrouping", false);
	
	m_squeezeRatio   = cg.readEntry("squeezeRatio", qreal(0.6));
	
	if (m_squeezeRatio <= 0 || m_squeezeRatio > 1) {
		m_squeezeRatio = 0.6;
	}
	
	if (cfgTaskbarLayout == m_layout->type() && cfgTaskbarLayout == TaskbarLayout::LimitSqueeze) {
		LimitSqueezeTaskbarLayout *layout = static_cast<LimitSqueezeTaskbarLayout*>(m_layout);
		layout->setSqueezeRatio(m_squeezeRatio);
		layout->setPreferGrouping(m_preferGrouping);
	}

	m_itemHeight = cg.readEntry("itemHeight", 40);

	if (m_itemHeight < 1) {
		m_itemHeight = 40;
	}

	if (cfgTaskbarLayout == m_layout->type() && cfgTaskbarLayout == TaskbarLayout::FixedSize) {
		FixedSizeTaskbarLayout *layout = static_cast<FixedSizeTaskbarLayout*>(m_layout);
		layout->setFixedCellHeight(m_itemHeight);
	}

	m_rowAspectRatio = cg.readEntry("rowAspectRatio", qreal(1.5));

	if (m_rowAspectRatio <= 0.0) {
		m_rowAspectRatio = 1.5;
	}

	if (cfgTaskbarLayout == m_layout->type() && cfgTaskbarLayout == TaskbarLayout::ByShape) {
		ByShapeTaskbarLayout *layout = static_cast<ByShapeTaskbarLayout*>(m_layout);
		layout->setRowAspectRatio(m_rowAspectRatio);
	}

	if (cfgTaskbarLayout != m_layout->type()) {
		TaskbarLayout *newLayout;

		disconnect(m_layout, SIGNAL(sizeHintChanged(Qt::SizeHint)), 0, 0);

		switch (cfgTaskbarLayout) {
		case TaskbarLayout::MaxSqueeze:
			newLayout = new MaxSqueezeTaskbarLayout(m_layout->orientation(), NULL);
			break;
		case TaskbarLayout::FixedItemCount:
			newLayout = new FixedItemCountTaskbarLayout(m_itemsPerRow, m_layout->orientation(), NULL);
			break;
		case TaskbarLayout::FixedSize:
			newLayout = new FixedSizeTaskbarLayout(m_itemHeight, m_layout->orientation(), NULL);
			break;
		case TaskbarLayout::ByShape:
			newLayout = new ByShapeTaskbarLayout(m_rowAspectRatio, m_layout->orientation(), NULL);
			break;
		case TaskbarLayout::LimitSqueeze:
		default:
			newLayout = new LimitSqueezeTaskbarLayout(m_squeezeRatio, m_preferGrouping, m_layout->orientation(), NULL);
		}

		newLayout->takeFrom(m_layout);

		newLayout->setSpacing(m_layout->spacing());
		newLayout->setMinimumRows(m_layout->minimumRows());
		newLayout->setMaximumRows(m_layout->maximumRows());
		newLayout->setFps(m_layout->fps());
		newLayout->setExpandDuration(m_layout->expandDuration());
		newLayout->setExpandedWidth(m_layout->expandedWidth());
		newLayout->setAspectRatio(m_layout->aspectRatio());
		newLayout->setAnimationsEnabled(m_layout->animationsEnabled());

		newLayout->setContentsMargins(0, 0, 0, 0);
		newLayout->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
		newLayout->setMaximumSize(INT_MAX, INT_MAX);

		connect(
			newLayout, SIGNAL(sizeHintChanged(Qt::SizeHint)),
			this, SIGNAL(sizeHintChanged(Qt::SizeHint)));

		m_layout = newLayout;
		setLayout(m_layout);
	}

	int cfgKeepExpanded = cg.readEntry("keepExpanded", (int) ExpandNone);

	// input validation:
	switch (cfgKeepExpanded) {
	case ExpandNone:
	case ExpandActive:
	case ExpandCurrentDesktop:
	case ExpandAll:
		m_keepExpanded = (ExpandType) cfgKeepExpanded;
		break;
	default:
		kDebug() << "illegal value for keepExpanded: " << cfgKeepExpanded;
		m_keepExpanded = ExpandNone;
	}
	
	m_expandOnHover     = cg.readEntry("expandOnHover", true);
	m_expandOnAttention = cg.readEntry("expandOnAttention", false);

	int cfgMiddleClickAction = cg.readEntry("middleClickAction", (int) NoAction);
	
	switch (cfgMiddleClickAction) {
	case NoAction:
	case CloseTask:
	case MoveToCurrentDesktop:
		m_middleClickAction = (MiddleClickAction) cfgMiddleClickAction;
		break;
	default:
		kDebug() << "illegal value for middleClickAction: " << cfgMiddleClickAction;
		m_middleClickAction = NoAction;
	}
	
	int cfgPreviewLayout = cg.readEntry("previewLayout", (int) ClassicPreviewLayout);

	// input validation:
	switch (cfgPreviewLayout) {
	case ClassicPreviewLayout:
	case NewPreviewLayout:
		m_previewLayout = (PreviewLayoutType) cfgPreviewLayout;
		emit previewLayoutChanged(m_previewLayout);
		break;
	default:
		kDebug() << "illegal value for previewLayout: " << cfgPreviewLayout;
		m_previewLayout = ClassicPreviewLayout;
		emit previewLayoutChanged(m_previewLayout);
	}

	m_maxPreviewSize = cg.readEntry("maxPreviewSize", 200);

	// input validation
	if (m_maxPreviewSize < 5) {
		m_maxPreviewSize = 5;
	}
	else if (m_maxPreviewSize > 500) {
		m_maxPreviewSize = 500;
	}
	
	m_tooltipMoveDuration = cg.readEntry("tooltipMoveDuration", 500);
	
	// input validation
	if (m_tooltipMoveDuration < 0) {
		m_tooltipMoveDuration = 0;
	}
	else if (m_tooltipMoveDuration > 2000) {
		m_tooltipMoveDuration = 2000;
	}
	
	m_highlightDelay = cg.readEntry("highlightDelay", 50);
	
	// input validation
	if (m_highlightDelay < 50) {
		m_highlightDelay = 50;
	}
	else if (m_highlightDelay > 1000) {
		m_highlightDelay = 1000;
	}

	int cfgShape = cg.readEntry("shape", (int) RectangelIcon);

	// input validation:
	switch (cfgShape) {
	case RectangelIcon:
	case SquareIcon:
		m_shape = (IconShapeType) cfgShape;
		break;
	default:
		kDebug() << "illegal value for shape: " << cfgShape;
		m_shape = RectangelIcon;
	}
	m_layout->setAspectRatio(m_shape == SquareIcon ? 1.0 : 1.2);

	m_dontRotateFrame    = cg.readEntry("dontRotateFrame", false);
	m_onlyLights         = cg.readEntry("onlyLights", true);
	m_textShadow         = cg.readEntry("textShadow", true);
	m_lightColorFromIcon = cg.readEntry("lightColorFromIcon", true);
	m_scrollSwitchTasks  = cg.readEntry("scrollSwitchTasks", true);
	
	m_layout->setExpandedWidth(cg.readEntry("expandingSize", 175));
	m_lightColor = cg.readEntry("lightColor", QColor(78, 196, 249, 200));

	m_layout->setExpandDuration(cg.readEntry("animationDuration", 175));
	m_layout->setFps(cg.readEntry("fps", 25));

	int cfgSortingStrategy = cg.readEntry("sortingStrategy",
		static_cast<int>(TaskManager::GroupManager::AlphaSorting));

	// input validation
	switch (cfgSortingStrategy) {
	case TaskManager::GroupManager::NoSorting:
	case TaskManager::GroupManager::ManualSorting:
	case TaskManager::GroupManager::AlphaSorting:
	case TaskManager::GroupManager::DesktopSorting:
		m_sortingStrategy =
			static_cast<TaskManager::GroupManager::TaskSortingStrategy>(cfgSortingStrategy);
		break;
	default:
		kDebug() << "illegal value for sorting strategy: " << cfgSortingStrategy;
		m_sortingStrategy = TaskManager::GroupManager::AlphaSorting;
	}

	int cfgGroupingStrategy = cg.readEntry("groupingStrategy",
		static_cast<int>(TaskManager::GroupManager::ProgramGrouping));

	switch (cfgGroupingStrategy) {
	case TaskManager::GroupManager::NoGrouping:
	case TaskManager::GroupManager::ProgramGrouping:
		m_groupingStrategy =
			static_cast<TaskManager::GroupManager::TaskGroupingStrategy>(cfgGroupingStrategy);
		break;
	default:
		kDebug() << "illegal value for grouping strategy: " << cfgGroupingStrategy;
		m_groupingStrategy = TaskManager::GroupManager::ProgramGrouping;
	}

	m_groupManager->setOnlyGroupWhenFull(cg.readEntry("onlyGroupWhenFull", false));
	m_groupManager->setSortingStrategy(m_sortingStrategy);
	m_groupManager->setGroupingStrategy(m_groupingStrategy);
	
	m_groupManager->setShowOnlyCurrentDesktop(cg.readEntry("showOnlyCurrentDesktop", false));
	m_groupManager->setShowOnlyCurrentActivity(cg.readEntry("showOnlyCurrentActivity", false));
	m_groupManager->setShowOnlyCurrentScreen(cg.readEntry("showOnlyCurrentScreen", false));
	m_groupManager->setShowOnlyMinimized(cg.readEntry("showOnlyMinimized", false));
	m_groupManager->reconnect();

	for (int i = 0; i < m_layout->count(); ++ i) {
		m_layout->itemAt(i)->settingsChanged();
	}
	
	updateFullLimit();
}

bool Applet::isPopupShowing() const {
	return m_toolTip->isShown();
}

int Applet::animationDuration() const {
	return m_layout->expandDuration();
}

int Applet::fps() const {
	return m_layout->fps();
}

QRect Applet::currentScreenGeometry() const {
	/*
	Plasma::Containment *containment = this->containment();
	if (containment) {
		return containment->corona()->screenGeometry(containment->screen());
	}
	else {
		return containment->corona()->screenGeometry(-1);
	}
	*/
	
	QDesktopWidget* desktop = QApplication::desktop();
	if (desktop == NULL) {
		kDebug() << "currentScreenGeometry(): desktop is NULL\n";
		return QRect();
	}
	QGraphicsView *view = this->view();
	if (view == NULL) {
		kDebug() << "currentScreenGeometry(): view is NULL\n";
		return desktop->screenGeometry(-1);
	}
	QWidget *viewport = view->viewport();
	if (viewport == NULL) {
		kDebug() << "currentScreenGeometry(): viewport is NULL\n";
		return desktop->screenGeometry(-1);
	}
	return desktop->screenGeometry(desktop->screenNumber(viewport));
}

QRect Applet::virtualScreenGeometry() const {
	QDesktopWidget* desktop = QApplication::desktop();
	if (desktop == NULL) {
		kDebug() << "virtualScreenGeometry(): desktop is NULL\n";
		return QRect();
	}
	QWidget *screen = desktop->screen();
	if (screen == NULL) {
		kDebug() << "virtualScreenGeometry(): screen is NULL\n";
		return desktop->screenGeometry(-1);
	}
	return screen->geometry();
}

void Applet::popup(const QPoint& pos, Task *task, QObject *receiver, const char *slot) {
	TaskManager::BasicMenu *taskMenu = popup(task);
	if (taskMenu != NULL) {
		if (receiver != NULL) {
			connect(taskMenu, SIGNAL(aboutToHide()), receiver, slot);
		}
		taskMenu->popup(pos);
	}
}

void Applet::popup(const TaskItem* item) {
	TaskManager::BasicMenu *taskMenu = popup(item->task());
	if (taskMenu != NULL) {
		taskMenu->popup(containment()->corona()->popupPosition(item, taskMenu->sizeHint()));
	}
}

TaskManager::BasicMenu* Applet::popup(Task *task) {
	if (task && task->isValid()) {
		switch (task->type()) {
			case Task::TaskItem:
				return new TaskManager::BasicMenu(
					NULL, task->taskItem(),
					m_groupManager);

			case Task::GroupItem:
				return new TaskManager::BasicMenu(
					NULL, task->group(),
					m_groupManager);
				
			case Task::LauncherItem:
				return new TaskManager::BasicMenu(
					NULL, task->launcherItem(),
					m_groupManager);
				
			default:
				break;
		}
	}
	return NULL;
}

} // namespace SmoothTasks
K_EXPORT_PLASMA_APPLET(smooth-tasks, SmoothTasks::Applet)
#include "Applet.moc"
