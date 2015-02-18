# Installation
## Build and install

    mkdir build
    cd build
    cmake .. -DCMAKE_INSTALL_PREFIX=/usr
    make
    make install

## CMake configure arguments
1. `ENABLE_QT4`:

    Building QtCurve Qt4 theme.

    (Default: `On`)

2. `ENABLE_QT5`:

    Building QtCurve Qt5 theme.

    (Default: `On`)

3. `ENABLE_GTK2`:

    Building QtCurve Gtk2 theme.

    (Default: `On`)

4. `QTC_ENABLE_X11`:

    Enable X11 support. Require `xcb`, `xlib-xcb`. Will cause
    `libqtcurve-utils.so` to link to `libxcb`, `libX11-xcb` and `libX11`.

    NOTE: this options will not affect any modules/libraries other than
    `libqtcurve-utils.so`. `libqtcurve-utils.so` with/without the option
    are API and ABI compatible although some functions will be NO-OP with
    X11 disabled.

    (Default: `On`)

5. `QTC_INSTALL_PO`:

    Install `*.po` translation files.

    (Default: `On`)

6. `QTC_QT4_ENABLE_KDE`:

    Compile QtCurve qt4 style with KDE support and dependencies.

    (Default: `On`)

7. `QTC_QT4_ENABLE_KWIN`:

    Compile QtCurve qt4 kwin style. This will only have effect when KDE support
    is turned on. Also require a X11 build of Qt4 and `libxcb` headers.

    (Default: `On` if kwin header is found and KDE support is on,
    `Off` otherwise)

8. `QTC_KDE4_PREFIX`:

    The install prefix of KDE4.

    (Default: the value returned by `kde4-config` or `${CMAKE_INSTALL_PREFIX}`
    if `kde4-config` is not found.)

9. `QTC_KDE4_ICONS_PREFIX`:

    The path of KDE4 icons.

    (Default: `${QTC_KDE4_PREFIX}/share/icons`)

10. `QTC_QT5_ENABLE_KDE`:

     Compile QtCurve qt5 style with KDE Frameworks support and dependencies.

     (Default: `On`)

## Deprecated and temporary options.
These compile options were used to turn on/off some experimental features or
work around for certain hardware/software/drivers. It is not guaranteed that
any of the non-default value of these options can work and some of these
options may crash certain applications. They may also be removed at any time
(especially the ones to work around old hardware or driver bugs) and are not
recommanded to turn on in distribution packages.

1. `QTC_QT4_STYLE_SUPPORT`:

    Support QtCurve style files. These are stored as
    `<kde prefix>/share/apps/kstyle/themes/qtc_<stylename>.themerc`
    Note that this is not supported in Qt5.

    (Default: `Off`)

# Additional Features
## XBar/MacMenu
The XBar support was copied directly from Bespin. The relevant files are named
`macmenu.*`. These were taken from revision 652.

**Deprecated** in favor of appmenu since KWin has builtin support for that
after `4.10` and `appmenu-qt` does not depend on a certain theme.

## Themes
As of v0.55, you can create QtCurve based themes. These will appear with KDE's
style control panel's combobox of styles. To create a new theme, select
'QtCurve' from within KDE's style panel, then configure QtCurve as required.
After this, in QtCurve's config dialog, select 'Export theme' from the options
menu button. You will then be prompted to give your new 'theme' a name, and a
comment. QtCurve will then create a file named `qtc_<name>.themerc`
(e.g. `qtc_klearlooks.themerc`) - this will have the following format:

    [Misc]
    Name=Klearlooks
    Comment=Clearlooks inspired style
    [KDE]
    WidgetStyle=qtc_klearlooks
    [Settings]
    animatedProgress=false
    appearance=gradient
    ....rest of qtcurve settings...

To use this theme, either copy `qtc_<name>.themerc` to
`$KDEHOME/share/apps/kstyle/themes/`
(usually `~/.kde/share/apps/kstyle/themes/`)
or copy to `<kde install prefix>/share/apps/kstyle/themes/`
(usually `/usr/share/apps/kstyle/themes/`)

When KDE's style panel is restarted, your new theme should appear in the list.

**NOTE**: As of QtCurve 1.0.0 style support has been disabled by default (enable
via `QTC_QT4_STYLE_SUPPORT`) and this is not supported by Qt5.

## Testing
As of v1.2.1, QtCurve can be forced to read its settings from an alternate
config file via the `QTCURVE_CONFIG_FILE` environment variable. This is only
really useful for testing alternate config settings without changing the users
current settings.

Usage:

    QTCURVE_CONFIG_FILE=~/testfile kcalc

# Compiler versions requirement
QtCurve requires the GNU dialect of ISO C99 and ISO C++11 (which means the
compilers have to support `-std=gnu99` and `-std=c++0x` command line option).
`g++>=4.7` and `clang++>=3.3` are fully supported. C compiler for any
reasonable version of gcc and clang should all work well.

For g++ older than 4.7, you need `-Doverride=` in the `CXXFLAGS` since it
does not support the override keyword in c++11. The trick should work down to
g++ 4.4 for the Qt4 style and 4.6 for the Qt5 stle.

clang++ 3.3 is not compatible with the stdc++ header shipped with gcc 4.7 so
libc++ is required when compiling with clang++ 3.3.

# License
QtCurve `1.8.17` and earlier was licensed under GPLv2+. It was relicensed under
LGPLv2.1+ on Nov. 14th 2013 in order to avoid confusion for moving into
KDE-infrastructure. This relicensing have got permissions from all contributors
of QtCurve (`git log --format='%aN <%aE>' | sort -u`):

    Andrey Bondrov <andrey.bondrov@rosalab.ru>
    Boris Pek <tehnick-8@yandex.ru>
    Craig Drummond <craig.p.drummond@gmail.com>
    Sven-Hendrik Haase <sh@lutzhaase.com>
    Yichao Yu <yyc1992@gmail.com>
