## (WIP)
1. Fix shadow color
   [QtCurve-Bug](https://github.com/QtCurve/qtcurve/issues/54)
2. Revert toolbar background
   [QtCurve-Bug](https://github.com/QtCurve/qtcurve/issues/52)
3. Workaround background image drawing.
   [QtCurve-Bug](https://github.com/QtCurve/qtcurve/issues/49)
4. Fix text emboldening in gtk2.
   [QtCurve-Bug](https://github.com/QtCurve/qtcurve/issues/58)
5. Become a KDE project
6. KF5 configure interface.
   Thanks to Eike Hein
7. Fix OSX build.
   Thanks to René J.V. Bertin.

## 1.8.18
1. Gtk2: Remove mozilla version detection.
2. Gtk2: Remove `QTC_GTK2_OLD_MOZILLA`.
3. Remove `xcb-image` dependency. It is never used.
4. Gtk2: Fix old configure file loading in gtk2 version.
   Thanks to Lars Wendler for noticing this.
5. Qt4: Fix compile without Qt
6. Relicense under LGPL. With permission from all contributors in the git log.
7. Qt: Treat `Qt::Sheet` as `Qt::Dialog`. This fixes translucent background for
   close confirmation dialog in QtDesigner.
8. Remove backward compatible options.
9. Qt5: Introduce `prePolish()` to Workaround a bug in Qt5 which make it
   impossible for QMainWindow and QDialog to be transparent.
   [Qt-Bug](https://bugreports.qt-project.org/browse/QTBUG-34064)
10. Qt4: Fix compilation without X11 enabled.
11. Qt4: Use `prePolish()` in qt4, this fixes most of application crashes.
12. Qt: Fix Qt menubar background when translucency is enabled.
13. Gtk2: Fix flash plugin in non-chromium/firefox browsers in linux when
    translucent background is enabled for gtk.
14. Qt4: Fix (workaround) kaffeine with translucent background.
15. Remove parentless dialog fix
16. Qt4: Fix kscreenlocker. Second version of prepolishing
17. Qt: Improve shadow of QBalloonTip
18. Draw shadow at runtime. Prepare for more advanced shadow feature and
    configuration.
19. Gtk2: Remove `QTC_GTK2_USE_CAIRO_FOR_ARROWS`. I cannot reproduce any
    problems on my Intel card. This is also necessary for the Gtk3 port. If
    it was really a bug of the Intel driver, it should rather be fixed there.
20. Qt: Remove `QTC_QT4_OLD_NVIDIA_ARROW_FIX` and
    `QTC_QT5_OLD_NVIDIA_ARROW_FIX`. They honestly only make things worse. Again,
    if it was really a bug of the driver, it should rather be fixed there.
21. Reorganize directories.
22. Gtk2: Add `libqtcurve-cairo` for common drawing routines (Shared between
    Gtk2 and Gtk3).
23. Qt4: Re-enable translucent background in KWin.
24. Gtk2: Fix translucent background in inkscape.
25. Gtk2: Enable translucent background in flash plugin.
26. Improve shadow gradient.
27. Gtk2: Fix chromium background color when non-custom color is used.
28. Only link to X11 libraries in `libqtcurve-utils.so`.
29. `QTC_ENABLE_X11` have not effect on any modules other than
    `libqtcurve-utils.so` also making `libqtcurve-utils.so` API and ABI
    compatible with and without X11 enabled.
30. Gtk2: Workaround buggy signal handler in applications.
    [QtCurve-Bug](https://github.com/QtCurve/qtcurve/issues/41)
    [EasyTag-Bug](https://bugzilla.gnome.org/show_bug.cgi?id=721943)
31. Qt5: Disable QtQuickControl support by default.

## 1.8.17
1. **Add Qt5 support!!**.
2. Gtk2: Workaround bug in glib >= 2.36.1 (`g_spawn_command_line_sync()`)

   [Debian-Bug](http://bugs.debian.org/707946)
   [QtCurve-Bug](https://github.com/QtCurve/qtcurve-gtk2/pull/1)

3. Require gnu99 and c++0x.
4. Porting to xcb.
5. Qt5: Remove XBar support from Qt5 (should be replaced by appmenu).
6. Gtk2: Remove KDE3 support from Gtk2.
7. Qt4: Fix compile without X Server. Thanks to Sven-Hendrik Haase.
8. Add `libqtcurve-utils.so`.
9. Move colorutils to `libqtcurve-utils`.
10. Gtk2: Fix some warnings.
11. Merge Gtk2 version and Qt{4,5} versions.
12. Move xcb handling to `libqtcurve-utils`.
13. Move KWin X11 shadow helper to `libqtcurve-utils`.
14. Qt4: Disable transparent background from XEmbed window (e.g. in kpartplugins).
    Changing the depth of the window (which require recreating the window)
    breaks the XEmbed protocol.
15. Gtk2: Generate all GdkPixbuf inline csourse at compile time.
16. Qt4: Require 4.6.0
17. Qt4, Qt5: Longer scrollbar in order to be more friendly to applications
    (e.g. choqok) that use this size hint.

    [KDE-Bug](https://bugs.kde.org/show_bug.cgi?id=317690)
    [QtCurve-Bug](https://github.com/QtCurve/qtcurve-qt4/issues/7)

18. Gtk2: Generate gdkpixbuf headers at compile time.
19. Fix compiling with `clang++` and `libc++`.
20. Remove configure file support for pre-1.0 releases.
    (More than 3 years from now)
21. Qt: Fix/Workaround QMdiSubWindow not drawn correctly with translucent
    background.
22. Move all Xlib calls to `libqtcurve-utils`.
23. Make it possible to disable X11 dependency when compiling
    `libqtcurve-utils` and `Qt4` and `Qt5` styles.
24. Qt4: Workaround amarok crash (also affect Oxygen-Transparent).
25. Qt4: Figured out the real reason for the window positioning problem
    when setting `Qt::WA_TranslucentBackground` and a better workaround/fix.

    [Qt-Bug](https://bugreports.qt-project.org/browse/QTBUG-34108)

26. Qt: Remove konsole background and konqueror menubar hack since they doesn't
    seems useful and produce problems.
27. Qt4: Remove Kwin maximum button hack.
28. Qt: Always assume QMdiSubWindow has alpha channel.

## 1.8.15-KDE4
1. Add Russian localization. Thanks to Juliette Tux.
2. Remove KDE3 support
3. Workaround rendering bug in Qt >= 4.8.5

   [Qt-Bug](https://bugreports.qt-project.org/browse/QTBUG-33512)
   [QtCurve-Bug](https://github.com/QtCurve/qtcurve-qt4/issues/3)

4. CMake option to disable kwin support (`-DQTC_QT4_ENABLE_KWIN=Off`)
5. Rename come CMake options. Old options will still work.
6. KWin 4.10 appmenu button support.
7. Remove packging script.
8. Generate `shadow.h` and `*pixmap.h` at compile time. Remove the need of
   Qt3 tools.

## 1.8.16-gtk2
1. fix memleak with newer versions of cairo.

## 1.8.15-gtk2
1. Fix tab activation (via mouse) when configured to drag windows from all
   empty areas and either highlight is set to 0% or coloured mousr over is set
   to none.
2. Check widget is realized before attempting to find menubar/statusbar in
   GhbCompositor class. Fixes startup crash wih Handbrake when menu/statusbar
   hiding is enabled.

## 1.8.14-KDE4
1. Fix loading of kwin decoration/config under KDE4.9

## 1.8.14-gtk2
1. Fix reading of KDE settings.
2. Fix button on editable combos with Gtk 2.24

## 1.8.13-KDE4
1. Fix KDE4.9 build - window grouping disabled.

## 1.8.13-gtk2
1. Fix builing with newer glibs - only glib.h itself can be included.
2. Use squared menus and tooltips if using XRender
3. Minimum 1ms delay for sub menus.

## 1.8.12-KDE4
1. Dont setup KDE4 fonts/colours in constructor - seems to mess things up when
   using proxy styles.
   See http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=638629

## 1.8.12-gtk2
1. Fix first shown menu not having a shadow in firefox.
2. Fix eclipse crash due to resetting of shadows.
3. Disable shadows for SWT where Gtk<2.12.

## 1.8.11-KDE4
1. Fix flashing of clementine's position slider tooltip.

## 1.8.11-gtk2
1. Fix broken titlebars in GIMP, etc.

## 1.8.10-KDE4
1. Fix some toolbutton icon clipping.
2. Korean translation - thanks to YoungUk Kim.
3. Fix kwin button corruption when compositing is disabled under KDE SC4.8
4. Minimum 1ms delay for sub menus.

## 1.8.10-gtk2
1. Fix compilation against Gtk 2.11
2. Implement shadows for KDE SC4.7

## 1.8.9-KDE4
1. Dont force alternate listview colours for `KAboutApplicationDialog`
2. Fix issues with konqueror style menu toolbar buttons and the joined
   toolbar button style.
3. Implement shadows for KDE SC4.7

## 1.8.9-gtk2
1. Disable resize grips in Ubuntu 11.04
2. Remove GimpDock font settings from gtkrc

## 1.8.8-KDE4
1. Disable window dragging from `QGraphicsView`.
2. Respect `_kde_no_window_grab` property to disable window drag for
   certain widgets.
3. Workaround dialog placement issues when using transparent dialogs.
   Attempt to place dialog in centre of its parent - if it has one.
   This only works for dialogs, windows will still (unfortunately)
   be moved to top left :-(

## 1.8.8-gtk2
1. Make timer callbacks threadsafe. Fixes
   https://bugzilla.redhat.com/show_bug.cgi?id=669155
2. Allow scrollbar sliders to be as thin as 5 pixels. At this setting, sliders
   will be squared.

## 1.8.7-KDE4
1. Fix QtDesigner crash when using transparent background - thanks to
   Friedemann Kleint.
2. When installing background images, need to remove and previous files
   first (as `QFile::copy` will not overwrite files)
3. Fix incorrent window-drag trigger when dragging dock widget title
   widgets (set via `QDockWidget::setTitleBarWidget()`)
4. Fix crash when opening style config dialog whilst kwin config dialog is
   open.
5. Allow scrollbar sliders to be as thin as 5 pixels. At this setting, sliders
   will be squared.

## 1.8.7-gtk2
1. Fix some segfaults with evolution under Fedora 14.
2. Don't force widnows style scrollbars for Firefox4 - this workaround
   is no longer required.

## 1.8.6-KDE4
1. Fix potential crash when using translucency and the raster engine.
   (Fix taken from oxygen-transparent)
2. Fix crash when changing QtCurve settings - handle free'ing of colours
   better.

## 1.8.6-gtk2
1. Fix compilation against Gtk2 older than 2.20 - yet again!
2. Fix drawing of background underneath menubars - noticeable if menubar
   gradient uses alpha.
3. Fix highlighted menu items when shading poup menus.
4. Don't use gradient alpha settings for tooltips or rulers.
5. Fix background of popup menus when set to be shaded as per menubars, was
   using inactive and not active titlebar colour!
6. When drawing popup text, where shaded as per titlebar, no need to check
   if window is active - it always is.
7. Improve window drag code.

## 1.8.5-KDE4
1. Fix fill of bottom corners of kwin decoration when not compositing.
2. Fix sunken left background of kwin decoration when not compositing.
3. Don't force toolbar icons to be square.
4. Fix drag issue with MusicScore.
5. Don't use gradient alpha settings for tooltips.
6. Don't do rounded/semi-transparent tooltips for opera.

## 1.8.5-gtk2
1. Fix compilation against Gtk2 older than 2.20
2. Remove some Gtk3 hacks.
3. Dont draw frame when shadow is set to NONE.
4. Use correct shade for highlighted menuitems when not using the highlight
   colour, and the popup is shaded.
5. If using custom menu colours, when drawing selected menuitems always draw as
   such - even if not using the highlight colour.

## 1.8.4-KDE4
1. Fix saving of custom alpha values.
2. Fix crash upon exit - due to double free.

## 1.8.4-gtk2
1. Fix issues with image based backgrounds.
2. Don't start drag on widget tab labels.
3. Don't remove mouse over colour for pressed combo buttons.
4. Use `gtk_rc_parse_string` to hide shortcuts, and not GtkSettings
5. Fix KDE style non-editable combos which have has-frame set to FALSE.

## 1.8.3-KDE4
1. Fix import of ZIP qtcurve files.
2. Fix display of background images, etc, in embedded preview.
3. Fix QMake compile - thanks, once again, to Hugues Delorme.

## 1.8.3-gtk2
1. Better fix for problems with Pidgin's tabs and window dragging.
2. Fix crash when using background gradients/images and built using 'Release'
   build type.
3. Fix titles in GIMP's preferences dialog.

## 1.8.2-KDE4
1. When hiding shortcuts, only show these if widget is enabled.
2. Don't draw titlebar button frame for menu button if this is where the
   window icon is to be. This was already the case for circular buttons,
   but not for square buttons.
3. If drawing sunken background behind titlebar buttons, need to reduce size
   of square buttons. This already happens for circular buttons.
4. Don't draw sunken background around icon based menu button, if it is the
   only button on the left/right.
5. Draw menubar and statusbar kwin toggle buttons square if using square
   titlebar buttons.
6. Draw box around checked Libre/Open Office menu checkboxes.
7. Ukrainian translation - thanks to Yuri Chornoivan
8. Slight drawing fix for square scrollviews.
9. When compiled against KDE, react to style changes.
10. Slightly clean-up code.

## 1.8.2-gtk2
1. Improve window drag code to take care of GtkPizza, and other, widgets.
2. Use Gdk window to determine offsets when drawing background gradients.
3. Use SHADOW_IN for treeviews.
4. Fix background gradients in scrollviews.
5. When drawing background gradients, use the background colour of the top-level
   widget's style.
6. Fix slight glitch with striped scrollbar sliders.
7. Hacky fix for tabs in Thunderbird main window.
8. Better detection of toolbar buttons in Thunderbird.
9. Slightly clean-up code.
10. Can only use KDE-style non-editable combo popup if also using glow focus - as
    Gtk2 modifes the focus rect.
11. Don't recolour checks/radios in listviews when row is selected.

## 1.8.1-KDE4
1. Safer handling of hidden shortcut underscore in popup menus. Only keep
   track of menu widget, not its ancestors.
2. If hiding keyboard shortcut underlines: Keep track of open popup menus,
   and only show keyboard short cut for the current one.
3. Fix corrupted menus when using a gradient that uses an alpha of less than
   100.

## 1.8.1-gtk2
1. Alter CMake linker flags to detect undefined symbols.
2. Fix compile with older Gtk2 versions.

## 1.8.0-KDE4
1. Add option to not display keyboard shortcurt underline until 'Alt' is
   pressed.
2. Add options to specify appearance of toolbar buttons.
3. Allow to use popup menu shade factor when colouring as per menubar.
4. Colour listview arrows on mouse-over.
5. Allow to force alternate colours for combo-box pop-up menus - matches Gtk.
6. Fix background rings alignment on kwin borders.
7. If highlighting scrollviews, and allow mouse-over for entries, also allow
   mouse-over for scrollviews.
8. Allow rounded list-style combo popups.

## 1.8.0-gtk2
1. Add option to not display keyboard shortcurt underline until 'Alt' is
   pressed.
2. Add options to specify appearance of toolbar buttons.
3. Allow to use popup menu shade factor when colouring as per menubar.
4. Colour listview arrows on mouse-over.
5. Fix scrollbar background gradient in flat scrollbar buttons with firefox.
6. Use windows style scrollbars for Firefox 4
7. Fix painting of alternate listview rows in SWT apps. For some reason even
   cells are painted twice, once with the correct "cell_XX" setting, and then
   with an incorrect one!
8. Fix size of focus indicator in treeviews with SWT apps.
9. Use list style pop-up for non-editable combos when not using Gtk-style
   combos - matches KDE behaviour.
10. Fix buttons in chromium's menu - mouse over colour, and text colour when
    using darker colour schemes.
11. Remove `QTC_CAIRO_1_10_HACK` cmake option. For cairo 1.10, an easier fix
    is to just ensure there is no colour stop at 1.0. Thanks to Hugo Pereira
    Da Costa for pointing this out.
12. Allow window drag in toolbars and other areas.
13. Implement hover and focus for scrolled windows.
14. Add support for KDE4 StartDragDist and StartDragTime config items to
    control when window drag starts.
15. Fix rounded border of combo popups when configured to not draw border
    and be square.
16. Use configure-event, and not resize-request, to detect window size changes.
17. Fix splitter background when highlight set to 0.
18. Fix tab mouse-over highlight when tab has a child widget.

## 1.7.2-KDE4
1. Allow editable combo-box popup to be rounded.
2. For windows build, set FramelessWindowHint when setting WA_TranslucentBackground
3. Always use flat background kwin, custom background messes up some aurorae
   decorations.
4. Update Chinese translations.
5. Set/clear widget masks when compositing toggled.
6. Fix kwin custom button icon colours.
7. Fix treeview line dimensions.

## 1.7.2-gtk2
1. Allow editable combo-box popup to be rounded.
2. Fix initial treeview highlight.
3. Fix rounded popup border when using compositing.
4. Add vmware, vmplayer, and gtk to list of app to exclude from opacity
   settings. This also prevents using RGBA for rounded tooltips and popup menus.
5. Fix opacity setting of squared popup menus.
6. Improve group-box label positioning.
7. Fix line-style group box when text is above or below the line.
8. Fix crash when using a tiled image for background appearance, but not for
   menu appearance - was using wrong config item!
9. Fix border of hovered treeview items.
10. Allow combo popupmenus to be rounded if not using compositing.
11. Set/clear widget masks when compositing toggled.
12. Fix treeview line end detection.
13. Fix treeview selection, and line, corruption.
14. Fix treeview line dimensions.
15. Fix size of fonts in GIMPs dock.
16. Fix firefox scrollbar background when using flat buttons and flat
    background.
17. Fix clipping issues with full and filled focus types.
18. For configs older than 1.7.2, disable usage of alpha channel to draw
    rounded tooltips and popup menus. Too many issues with apps to enable
    this by default.

## 1.7.1-KDE4
1. Remove kmix 'fix', and ue a proper fix! Dont access `QWidget`s' winId unless
   `w->testAttribute(Qt::WA_WState_Created) && w->internalWinId()` Thanks to
   Thomas Lübking
2. Set XProperties on show event.
3. Fix saving of 'thin' options.
4. Only disable opacity for titlebar, if using kwin configured setting and
   blending menubars (or using menubar colour).
5. Set KDE SC 4.6 'AbilityUsesBlurBehind' kwin setting if decoration or
   style supports opacity.
6. Fix 'rasied and joined' toolbar buttons.
7. Allow 'menus' of non-editable combos to be rounded.
8. Turn config dialog's 'whats this' help messages into tooltips.
9. Remove box around arrow in listview lines - to match Gtk better.
10. Remove 'new' listview lines setting - always use 'older' style.
11. Add warning about opacity settings and compiz.
12. Fix drawing of background images onto titlebar.

## 1.7.1-gtk2
1. Use rgba colormap to draw rounded tooltips and popup menus - gives nicer
   border. This may be disabled via KDE4 config dialog.
2. Allow 'menus' of non-editable combos to be rounded. (But *only* if the app
   can be set to use an rgba colormap)
3. Remove resize grips - kwin handles this.
4. Add mouse tracking to treeview lists.
5. Indent treeview selection as per KDE.
6. Dont use GtkStyle to draw treeview lines. Use custom code which matches
   previous KDE 'old' style.
7. When loading images for background/menus - check that image was loaded before
   attempting to ascertain its width/height.
8. Fix drawing or progressbar groove in treeview.

## 1.7.0-KDE4
1. Add option to use tiled pixmap as background for windows and menus.
2. Draw line focus for items that cannot use glow focus.
3. Use popup menu gradient's border setting to determine border type of menus.
4. Add option to draw standard, raised, or joined toolbar buttons.
5. Add option to use thinner frames, focus, etc.
6. If not rounding, enable all 'square' options.
7. Store background colour setting in XProperty - so that this can be used by
   QtCurve kwin decoration.
8. Add position setting for background image; top left, top middle, top right,
   bottom left, bottom middle, bottom right, left middle, right middle,
   and centred.
9. Use square tooltips by default. (Problems have been reported with Pidgin and
   rounded tooltip - but I cannot reproduce these!)
10. In preview window, draw background rings/image down from titlebar.
11. Add option to use KDE4 palette's hover colour for kwin button symbol
    mouse-over.
12. Fix slight drawing glitch when not using 'glow' for tab mouse-over.
13. RTL fixes/workarounds for table headers.
14. Workaround kcmshell terminating immediately after QtCurve config dialog is
    closed - increase `KGlobal` reference count.
15. When shading tab background, use `fillRect()` and not `fillPath()` -
    seems to workaround glithches with intel 2.12 graphics driver.
16. Fix kwin option to only apply opacity to titlebar.
17. Add option to control whether window titlebars are filled with the
    background *before* the titlebar gradient is applied.
18. Altered check/radio contents size - so as to better align cehckbox texts
    when used in left part of a form-layout.
19. Slightly better kwin button icon size calculation.
20. Replace "..." in QPrintDialog with "document-open" icon.
21. Don't overwrite user supplied `CMAKE_INSTALL_PREFIX`
22. Don't set background XProperties for kmix - seem to cause a crash on exit
    when using raster graphicssystem.
23. For OpenOffice, fill background before drawing radio buttons - seems to fix
    the 'checkbox' that seems to get painted behind some radios!

## 1.7.0-gtk2
1. Add option to use tiled pixmap as background for windows and menus.
2. Draw line focus for items that cannot use glow focus.
3. Use popup menu gradient's border setting to determine border type of menus.
4. Add option to draw standard, raised, or joined toolbar buttons.
5. Add option to use thinner frames, focus, etc.
6. If not rounding, enable all 'square' options.
7. Store background colour setting in XProperty - so that this can be used by
   QtCurve kwin decoration. (At the moment, this is just the palette background)
8. Add position setting for background image; top left, top middle, top right,
   bottom left, bottom middle, bottom right, left middle, right middle, and
   centred.
9. Use square tooltips by default. (Problems have been reported with Pidgin and
   rounded tooltip - but I cannot reproduce these!)
10. Fix combo-box menu text when shading popup menus.
11. Fix reading of QtCurve supplied default kdeglobals file.
12. Fix rounding of troughs of progressbars within treeview.
13. When using glow focus, dont draw glow focus and line focus on button of
    editable combo - jsut draw line.
14. Improve background gradient performance - only re-draw on size-request
    if size is different from last time.
15. Don't overwrite user supplied `CMAKE_INSTALL_PREFIX`
16. OpenOffice.org fixes:

    1. toolbar buttons when using max rounded buttons.
    2. vertical scrollbar button detection
    3. background of unified spin widgets and combos
