project(smooth-tasks)

find_package(Qt4 REQUIRED)
find_package(KDE4 REQUIRED)
find_package(KDE4Workspace REQUIRED)
include(KDE4Defaults)

add_definitions(${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} ${KDE4_INCLUDES})

set(smoothtasks_SRCS
	SmoothTasks/Applet.cpp
	SmoothTasks/Task.cpp
	SmoothTasks/TaskItem.cpp
	SmoothTasks/TaskIcon.cpp
	SmoothTasks/Light.cpp
	SmoothTasks/ToolTipBase.cpp
	SmoothTasks/DelayedToolTip.cpp
	SmoothTasks/SmoothToolTip.cpp
	SmoothTasks/PlasmaToolTip.cpp
	SmoothTasks/ToolTipWidget.cpp
	SmoothTasks/WindowPreview.cpp
	SmoothTasks/FadedText.cpp
	SmoothTasks/Global.cpp
	SmoothTasks/CloseIcon.cpp
	SmoothTasks/ToggleAnimation.cpp
	SmoothTasks/TaskStateAnimation.cpp
	SmoothTasks/TaskbarLayout.cpp
	SmoothTasks/ByShapeTaskbarLayout.cpp
	SmoothTasks/FixedSizeTaskbarLayout.cpp
	SmoothTasks/FixedItemCountTaskbarLayout.cpp
	SmoothTasks/MaxSqueezeTaskbarLayout.cpp
	SmoothTasks/LimitSqueezeTaskbarLayout.cpp)

set(smoothtasks_RCCS resources.qrc)

qt4_add_resources(smoothtasks_SRCS ${smoothtasks_RCCS})

kde4_add_ui_files(smoothtasks_SRCS
	SmoothTasks/Ui/General.ui
	SmoothTasks/Ui/Appearance.ui)

kde4_add_plugin(plasma_applet_smooth-tasks
	${smoothtasks_SRCS})

target_link_libraries(plasma_applet_smooth-tasks
	${KDE4_PLASMA_LIBS}
	${KDE4_KDEUI_LIBS}
	${KDE4_KIO_LIBS}
	taskmanager)

if(X11_FOUND)
	target_link_libraries(plasma_applet_smooth-tasks
		${X11_LIBRARIES})
endif(X11_FOUND)

install(TARGETS plasma_applet_smooth-tasks DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES plasma-applet-smooth-tasks.desktop DESTINATION ${SERVICES_INSTALL_DIR})
