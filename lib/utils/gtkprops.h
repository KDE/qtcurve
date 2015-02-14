/*****************************************************************************
 *   Copyright 2014 - 2015 Yichao Yu <yyc1992@gmail.com>                     *
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

#ifndef __QTC_UTILS_GTK_PROPS_H__
#define __QTC_UTILS_GTK_PROPS_H__

#include "gtkutils.h"

namespace QtCurve {

class GtkWidgetProps {
    struct Props {
        GtkWidget *m_w;
        template<typename ObjGetter>
        class SigConn {
            SigConn(const SigConn&) = delete;
        public:
            SigConn() : m_id(0)
            {
                static_assert(sizeof(SigConn) == sizeof(int), "");
            }
            inline
            ~SigConn()
            {
                disconn();
            }
            template<typename Ret, typename... Args>
            inline void
            conn(const char *name, Ret(*cb)(Args...), void *data=nullptr)
            {
                if (qtcLikely(!m_id)) {
                    m_id = g_signal_connect(ObjGetter()(this),
                                            name, G_CALLBACK(cb), data);
                }
            }
            inline void
            disconn()
            {
                if (qtcLikely(m_id)) {
                    GObject *obj = ObjGetter()(this);
                    if (g_signal_handler_is_connected(obj, m_id)) {
                        g_signal_handler_disconnect(obj, m_id);
                    }
                    m_id = 0;
                }
            }
        private:
            int m_id;
        };
#define DEF_WIDGET_SIG_CONN_PROPS(name)                                 \
        struct _SigConn_##name##_ObjGetter {                            \
            constexpr inline GObject*                                   \
            operator()(SigConn<_SigConn_##name##_ObjGetter> *p) const   \
            {                                                           \
                return (GObject*)qtcContainerOf(p, Props, name)->m_w;   \
            }                                                           \
        };                                                              \
        SigConn<_SigConn_##name##_ObjGetter> name

        int blurBehind: 2;
        bool shadowSet: 1;
        bool tabHacked: 1;
        bool entryHacked: 1;
        bool statusBarSet: 1;
        bool wmMoveHacked: 1;
        bool windowHacked: 1;
        bool comboBoxHacked: 1;
        bool tabChildHacked: 1;
        bool treeViewHacked: 1;
        bool menuShellHacked: 1;
        bool scrollBarHacked: 1;
        bool buttonOrderHacked: 1;
        bool shadeActiveMBHacked: 1;
        unsigned widgetMapHacked: 2;
        bool scrolledWindowHacked: 1;

        unsigned short windowOpacity;

        int widgetMask;
        DEF_WIDGET_SIG_CONN_PROPS(shadowDestroy);

        DEF_WIDGET_SIG_CONN_PROPS(entryEnter);
        DEF_WIDGET_SIG_CONN_PROPS(entryLeave);
        DEF_WIDGET_SIG_CONN_PROPS(entryDestroy);
        DEF_WIDGET_SIG_CONN_PROPS(entryUnrealize);
        DEF_WIDGET_SIG_CONN_PROPS(entryStyleSet);

        DEF_WIDGET_SIG_CONN_PROPS(comboBoxDestroy);
        DEF_WIDGET_SIG_CONN_PROPS(comboBoxUnrealize);
        DEF_WIDGET_SIG_CONN_PROPS(comboBoxStyleSet);
        DEF_WIDGET_SIG_CONN_PROPS(comboBoxEnter);
        DEF_WIDGET_SIG_CONN_PROPS(comboBoxLeave);
        DEF_WIDGET_SIG_CONN_PROPS(comboBoxStateChange);

        unsigned menuBarSize;
        DEF_WIDGET_SIG_CONN_PROPS(menuShellMotion);
        DEF_WIDGET_SIG_CONN_PROPS(menuShellLeave);
        DEF_WIDGET_SIG_CONN_PROPS(menuShellDestroy);
        DEF_WIDGET_SIG_CONN_PROPS(menuShellStyleSet);
        DEF_WIDGET_SIG_CONN_PROPS(menuShellButtonPress);
        DEF_WIDGET_SIG_CONN_PROPS(menuShellButtonRelease);

        DEF_WIDGET_SIG_CONN_PROPS(scrollBarDestroy);
        DEF_WIDGET_SIG_CONN_PROPS(scrollBarUnrealize);
        DEF_WIDGET_SIG_CONN_PROPS(scrollBarStyleSet);
        DEF_WIDGET_SIG_CONN_PROPS(scrollBarValueChanged);

        DEF_WIDGET_SIG_CONN_PROPS(scrolledWindowDestroy);
        DEF_WIDGET_SIG_CONN_PROPS(scrolledWindowUnrealize);
        DEF_WIDGET_SIG_CONN_PROPS(scrolledWindowStyleSet);
        DEF_WIDGET_SIG_CONN_PROPS(scrolledWindowEnter);
        DEF_WIDGET_SIG_CONN_PROPS(scrolledWindowLeave);
        DEF_WIDGET_SIG_CONN_PROPS(scrolledWindowFocusIn);
        DEF_WIDGET_SIG_CONN_PROPS(scrolledWindowFocusOut);

        DEF_WIDGET_SIG_CONN_PROPS(tabDestroy);
        DEF_WIDGET_SIG_CONN_PROPS(tabUnrealize);
        DEF_WIDGET_SIG_CONN_PROPS(tabStyleSet);
        DEF_WIDGET_SIG_CONN_PROPS(tabMotion);
        DEF_WIDGET_SIG_CONN_PROPS(tabLeave);
        DEF_WIDGET_SIG_CONN_PROPS(tabPageAdded);

        DEF_WIDGET_SIG_CONN_PROPS(tabChildDestroy);
        DEF_WIDGET_SIG_CONN_PROPS(tabChildStyleSet);
        DEF_WIDGET_SIG_CONN_PROPS(tabChildEnter);
        DEF_WIDGET_SIG_CONN_PROPS(tabChildLeave);
        DEF_WIDGET_SIG_CONN_PROPS(tabChildAdd);

        DEF_WIDGET_SIG_CONN_PROPS(wmMoveDestroy);
        DEF_WIDGET_SIG_CONN_PROPS(wmMoveStyleSet);
        DEF_WIDGET_SIG_CONN_PROPS(wmMoveMotion);
        DEF_WIDGET_SIG_CONN_PROPS(wmMoveLeave);
        DEF_WIDGET_SIG_CONN_PROPS(wmMoveButtonPress);

        DEF_WIDGET_SIG_CONN_PROPS(treeViewDestroy);
        DEF_WIDGET_SIG_CONN_PROPS(treeViewUnrealize);
        DEF_WIDGET_SIG_CONN_PROPS(treeViewStyleSet);
        DEF_WIDGET_SIG_CONN_PROPS(treeViewMotion);
        DEF_WIDGET_SIG_CONN_PROPS(treeViewLeave);

        DEF_WIDGET_SIG_CONN_PROPS(widgetMapDestroy);
        DEF_WIDGET_SIG_CONN_PROPS(widgetMapUnrealize);
        DEF_WIDGET_SIG_CONN_PROPS(widgetMapStyleSet);

        DEF_WIDGET_SIG_CONN_PROPS(windowConfigure);
        DEF_WIDGET_SIG_CONN_PROPS(windowDestroy);
        DEF_WIDGET_SIG_CONN_PROPS(windowStyleSet);
        DEF_WIDGET_SIG_CONN_PROPS(windowKeyRelease);
        DEF_WIDGET_SIG_CONN_PROPS(windowMap);
        DEF_WIDGET_SIG_CONN_PROPS(windowClientEvent);
#undef DEF_WIDGET_SIG_CONN_PROPS
    };

    inline Props*
    getProps() const
    {
        static GQuark name =
            g_quark_from_static_string("_gtk__QTCURVE_WIDGET_PROPERTIES__");
        Props *props = (Props*)g_object_get_qdata(m_obj, name);
        if (!props) {
            props = new Props;
            props->m_w = (GtkWidget*)m_obj;
            g_object_set_qdata_full(m_obj, name, props, [] (void *props) {
                    delete (Props*)props;
                });
        }
        return props;
    }
public:
    template<typename T>
    inline
    GtkWidgetProps(T *obj) : m_obj((GObject*)obj), m_props(nullptr)
    {}
    inline Props*
    operator->() const
    {
        if (!m_props && m_obj) {
            m_props = getProps();
        }
        return m_props;
    }
private:
    GObject *m_obj;
    mutable Props *m_props;
};

}

#endif
