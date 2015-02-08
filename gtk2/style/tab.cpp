/*****************************************************************************
 *   Copyright 2003 - 2010 Craig Drummond <craig.p.drummond@gmail.com>       *
 *   Copyright 2013 - 2015 Yichao Yu <yyc1992@gmail.com>                     *
 *                                                                           *
 *   This program is free software; you can redistribute it and/or modify    *
 *   it under the terms of the GNU Lesser General Public License as          *
 *   published by the Free Software Foundation; either version 2.1 of the    *
 *   License, or (at your option) version 3, or any later version accepted   *
 *   by the membership of KDE e.V. (or its successor approved by the         *
 *   membership of KDE e.V.), which shall act as a proxy defined in          *
 *   Section 6 of version 3 of the license.                                  *
 *                                                                           *
 *   This program is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU       *
 *   Lesser General Public License for more details.                         *
 *                                                                           *
 *   You should have received a copy of the GNU Lesser General Public        *
 *   License along with this library. If not,                                *
 *   see <http://www.gnu.org/licenses/>.                                     *
 *****************************************************************************/

#include "tab.h"

#include <qtcurve-utils/gtkprops.h>

#include <vector>
#include <unordered_map>
#include <utility>
#include <tuple>

namespace QtCurve {
namespace Tab {

struct Info {
    int id;
    std::vector<QtcRect> rects;
    Info(GtkWidget *notebook);
};

Info::Info(GtkWidget *notebook)
    : id(-1),
      rects(gtk_notebook_get_n_pages((GtkNotebook*)notebook),
            qtcRect(0, 0, -1, -1))
{
}

static std::unordered_map<GtkWidget*, Info> tabMap;

static Info*
lookupHash(GtkWidget *hash, bool create=false)
{
    auto it = tabMap.find(hash);
    if (it != tabMap.end()) {
        return &it->second;
    } else if (!create) {
        return nullptr;
    }
    auto res = tabMap.emplace(std::piecewise_construct,
                              std::forward_as_tuple(hash),
                              std::forward_as_tuple(hash));
    return &(res.first->second);
}

static Info*
widgetFindTab(GtkWidget *widget)
{
    if (GTK_IS_NOTEBOOK(widget))
        return lookupHash(widget);
    return nullptr;
}

static void
cleanup(GtkWidget *widget)
{
    if (widget) {
        QTC_DEF_WIDGET_PROPS(props, widget);
        qtcDisconnectFromProp(props, tabDestroy);
        qtcDisconnectFromProp(props, tabUnrealize);
        qtcDisconnectFromProp(props, tabStyleSet);
        qtcDisconnectFromProp(props, tabMotion);
        qtcDisconnectFromProp(props, tabLeave);
        qtcDisconnectFromProp(props, tabPageAdded);
        qtcWidgetProps(props)->tabHacked = true;
        tabMap.erase(widget);
    }
}

static gboolean
styleSet(GtkWidget *widget, GtkStyle*, void*)
{
    cleanup(widget);
    return false;
}

static gboolean
destroy(GtkWidget *widget, GdkEvent*, void*)
{
    cleanup(widget);
    return false;
}

static void
setHovered(Info *tab, GtkWidget *widget, int index)
{
    if (tab->id != index) {
        QtcRect updateRect = {0, 0, -1, -1};
        tab->id = index;
        for (auto &rect: tab->rects) {
            qtcRectUnion(&rect, &updateRect, &updateRect);
        }
        gtk_widget_queue_draw_area(widget, updateRect.x - 4, updateRect.y - 4,
                                   updateRect.width + 8, updateRect.height + 8);
    }
}

static gboolean
motion(GtkWidget *widget, GdkEventMotion*, void*)
{
    Info *tab = widgetFindTab(widget);
    if (tab) {
        int px;
        int py;
        gdk_window_get_pointer(gtk_widget_get_window(widget), &px, &py, NULL);

        for (size_t i = 0;i < tab->rects.size();i++) {
            auto &rect = tab->rects[i];
            if (rect.x <= px && rect.y <= py && rect.x + rect.width > px &&
                rect.y + rect.height > py) {
                setHovered(tab, widget, i);
                return false;
            }
        }
        setHovered(tab, widget, -1);
    }
    return false;
}

static gboolean
leave(GtkWidget *widget, GdkEventCrossing*, void*)
{
    Info *prevTab = widgetFindTab(widget);

    if (prevTab && prevTab->id >= 0) {
        prevTab->id = -1;
        gtk_widget_queue_draw(widget);
    }
    return false;
}

static void
unregisterChild(GtkWidget *widget)
{
    QTC_DEF_WIDGET_PROPS(props, widget);
    if (widget && qtcWidgetProps(props)->tabChildHacked) {
        qtcDisconnectFromProp(props, tabChildDestroy);
        qtcDisconnectFromProp(props, tabChildStyleSet);
        qtcDisconnectFromProp(props, tabChildEnter);
        qtcDisconnectFromProp(props, tabChildLeave);
        if (GTK_IS_CONTAINER(widget)) {
            qtcDisconnectFromProp(props, tabChildAdd);
        }
        qtcWidgetProps(props)->tabChildHacked = false;
    }
}

static void updateChildren(GtkWidget *widget);

static gboolean
childMotion(GtkWidget *widget, GdkEventMotion *event, void *user_data)
{
    motion((GtkWidget*)user_data, event, widget);
    return false;
}

static gboolean
childDestroy(GtkWidget *widget, GdkEventCrossing*, void*)
{
    unregisterChild(widget);
    return false;
}

static gboolean
childStyleSet(GtkWidget *widget, GdkEventCrossing*, void*)
{
    unregisterChild(widget);
    return false;
}

static gboolean
childAdd(GtkWidget*, GdkEventCrossing *, void *data)
{
    updateChildren((GtkWidget*)data);
    return false;
}

static void
registerChild(GtkWidget *notebook, GtkWidget *widget)
{
    QTC_DEF_WIDGET_PROPS(props, widget);
    if (widget && !qtcWidgetProps(props)->tabChildHacked) {
        qtcWidgetProps(props)->tabChildHacked = true;
        qtcConnectToProp(props, tabChildDestroy, "destroy",
                         childDestroy, notebook);
        qtcConnectToProp(props, tabChildStyleSet, "style-set",
                         childStyleSet, notebook);
        qtcConnectToProp(props, tabChildEnter, "enter-notify-event",
                         childMotion, notebook);
        qtcConnectToProp(props, tabChildLeave, "leave-notify-event",
                         childMotion, notebook);
        if (GTK_IS_CONTAINER(widget)) {
            qtcConnectToProp(props, tabChildAdd, "add",
                             childAdd, notebook);
            GList *children = gtk_container_get_children(GTK_CONTAINER(widget));
            for (GList *child = children;child;child = g_list_next(child)) {
                registerChild(notebook, GTK_WIDGET(child->data));
            }
            if (children) {
                g_list_free(children);
            }
        }
    }
}

static void
updateChildren(GtkWidget *widget)
{
    if (widget && GTK_IS_NOTEBOOK(widget)) {
        GtkNotebook *notebook = GTK_NOTEBOOK(widget);
        int numPages = gtk_notebook_get_n_pages(notebook);
        for (int i = 0;i < numPages;i++) {
            registerChild(
                widget, gtk_notebook_get_tab_label(
                    notebook, gtk_notebook_get_nth_page(notebook, i)));
        }
    }
}

static gboolean
pageAdded(GtkWidget *widget, GdkEventCrossing*, void*)
{
    updateChildren(widget);
    return false;
}

int
currentHoveredIndex(GtkWidget *widget)
{
    Info *tab = widgetFindTab(widget);
    return tab ? tab->id : -1;
}

void
setup(GtkWidget *widget)
{
    QTC_DEF_WIDGET_PROPS(props, widget);
    if (widget && !qtcWidgetProps(props)->tabHacked) {
        qtcWidgetProps(props)->tabHacked = true;
        lookupHash(widget, true);
        qtcConnectToProp(props, tabDestroy, "destroy-event",
                         destroy, NULL);
        qtcConnectToProp(props, tabUnrealize, "unrealize", destroy, NULL);
        qtcConnectToProp(props, tabStyleSet, "style-set", styleSet, NULL);
        qtcConnectToProp(props, tabMotion, "motion-notify-event",
                         motion, NULL);
        qtcConnectToProp(props, tabLeave, "leave-notify-event",
                         leave, NULL);
        qtcConnectToProp(props, tabPageAdded, "page-added",
                         pageAdded, NULL);
        updateChildren(widget);
    }
}

void
updateRect(GtkWidget *widget, int tabIndex, int x, int y, int width, int height)
{
    Info *tab = widgetFindTab(widget);

    if (tab && tabIndex >= 0) {
        if (tabIndex >= (int)tab->rects.size()) {
            tab->rects.resize(tabIndex + 8, qtcRect(0, 0, -1, -1));
        }
        tab->rects[tabIndex].x = x;
        tab->rects[tabIndex].y = y;
        tab->rects[tabIndex].width = width;
        tab->rects[tabIndex].height = height;
    }
}

bool
isLabel(GtkNotebook *notebook, GtkWidget *widget)
{
    int numPages = gtk_notebook_get_n_pages(notebook);
    for (int i = 0;i < numPages;++i) {
        if (gtk_notebook_get_tab_label(
                notebook, gtk_notebook_get_nth_page(notebook, i)) == widget) {
            return true;
        }
    }
    return false;
}

QtcRect
getTabbarRect(GtkNotebook *notebook)
{
    QtcRect rect = {0, 0, -1, -1};
    QtcRect empty = rect;
    QtcRect pageAllocation;
    unsigned int borderWidth;
    int pageIndex;
    GtkWidget *page;
    GList *children = NULL;
    // check tab visibility
    if (!(gtk_notebook_get_show_tabs(notebook) &&
          (children = gtk_container_get_children(GTK_CONTAINER(notebook))))) {
        return empty;
    }
    g_list_free(children);
    // get full rect
    rect = Widget::getAllocation(GTK_WIDGET(notebook));

    // adjust to account for borderwidth
    borderWidth = gtk_container_get_border_width(GTK_CONTAINER(notebook));

    rect.x += borderWidth;
    rect.y += borderWidth;
    rect.height -= 2 * borderWidth;
    rect.width -= 2 * borderWidth;

    // get current page
    pageIndex = gtk_notebook_get_current_page(notebook);

    if (!(pageIndex >= 0 && pageIndex < gtk_notebook_get_n_pages(notebook))) {
        return empty;
    }
    page = gtk_notebook_get_nth_page(notebook, pageIndex);
    if (!page) {
        return empty;
    }

    // removes page allocated size from rect, based on tabwidget orientation
    pageAllocation = Widget::getAllocation(page);
    switch (gtk_notebook_get_tab_pos(notebook)) {
    case GTK_POS_BOTTOM:
        rect.y += pageAllocation.height;
        rect.height -= pageAllocation.height;
        break;
    case GTK_POS_TOP:
        rect.height -= pageAllocation.height;
        break;
    case GTK_POS_RIGHT:
        rect.x += pageAllocation.width;
        rect.width -= pageAllocation.width;
        break;
    case GTK_POS_LEFT:
        rect.width -= pageAllocation.width;
        break;
    }
    return rect;
}

bool
hasVisibleArrows(GtkNotebook *notebook)
{
    if (gtk_notebook_get_show_tabs(notebook)) {
        int numPages = gtk_notebook_get_n_pages(notebook);
        for (int i = 0;i < numPages;i++) {
            GtkWidget *label = gtk_notebook_get_tab_label(
                notebook, gtk_notebook_get_nth_page(notebook, i));
#if GTK_CHECK_VERSION(2, 20, 0)
            if (label && !gtk_widget_get_mapped(label)) {
                return true;
            }
#else
            if (label && !GTK_WIDGET_MAPPED(label)) {
                return true;
            }
#endif
        }
    }
    return false;
}

}
}
