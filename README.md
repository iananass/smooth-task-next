# Smooth Tasks Next

Smooth Tasks Next is a fork of the Smooth Tasks Applet. It aims at the best Tasks Manager
Applet in the KDE desktop environment.

KDE plasma desktop is a great desktop environment. However it doesnot even has a reasonable
task manager applet.

The existing applets has following problems:

- Cannot disable mouse wheeling controls.
- Bad animations.
- Lack of settings.

# Features

1. Fully mouse wheeling controls. (Yes, you can disable it in options!)
2. Good animation.
3. Fully control of itself in settings.

# Requirements

## Build Requirements

I use following packages in build dependency (openSUSE 12.3):

1. kdebase4-workspace-devel
2. libkde4-devel
3. libkdecore4-devel


## Runtime Requirements

KDE4 >= 4.8

# Installation

Firstly please git clone the repository, then execute the following commands:

    mkdir build
    cd build
    cmake .. -DCMAKE_INSTALL_PREFIX=`kde4-config --prefix`
    make
    sudo make install
    kquitapp plasma-desktop && sleep 2 && plasma-desktop

After the plasma desktop restarts, unlock the widgets and add the "Smooth Tasks" widget.

# Contributors

STasks is written by Marcin Baszczewski <marcin.baszczewski@gmail.com>.

Smooth Tasks is a fork of STasks, written by Mathias Panzenböck <grosser.meister.morti@gmx.net>.

Smooth Tasks Next is a fork of Smooth Tasks, written by Xu Zhao <nuklly@gmail.com>.
