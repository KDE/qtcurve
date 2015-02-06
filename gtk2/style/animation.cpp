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

/*
 * Stolen from Clearlooks...
 */

/* Yes, this is evil code. But many people seem to like hazardous things, so
 * it exists. Most of this was written by Kulyk Nazar.
 *
 * heavily modified by Benjamin Berg <benjamin@sipsolutions.net>.
 */

/* #define CHECK_ANIMATION_TIME 0.5 */

#include "animation.h"
#include <common/common.h>

namespace QtCurve {

class AnimationInfo {
public:
    GtkWidget *const widget;

    AnimationInfo(const GtkWidget *w, double stop_time);
    ~AnimationInfo();
    double timer_elapsed() const;
    void timer_reset() const;
    bool need_stop() const;
private:
    GTimer *const m_timer;
    const double m_stop_time;
};

struct SignalInfo {
    GtkWidget *widget;
    unsigned long handler_id;
};

static GSList *connected_widgets = nullptr;
static GHashTable *animated_widgets = nullptr;
static int animation_timer_id = 0;

static gboolean animationTimeoutHandler(void *data);

inline
AnimationInfo::AnimationInfo(const GtkWidget *w, double stop_time)
    : widget(const_cast<GtkWidget*>(w)),
      m_timer(g_timer_new()),
      m_stop_time(stop_time)
{
}

inline
AnimationInfo::~AnimationInfo()
{
    g_timer_destroy(m_timer);
}

inline double
AnimationInfo::timer_elapsed() const
{
    return g_timer_elapsed(m_timer, nullptr);
}

inline bool
AnimationInfo::need_stop() const
{
    return m_stop_time != 0 && timer_elapsed() > m_stop_time;
}

inline void
AnimationInfo::timer_reset() const
{
    g_timer_start(m_timer);
}

/* This forces a redraw on a widget */
static void
force_widget_redraw(GtkWidget *widget)
{
    if (GTK_IS_PROGRESS_BAR(widget)) {
        gtk_widget_queue_resize(widget);
    } else {
        gtk_widget_queue_draw(widget);
    }
}

/* ensures that the timer is running */
static void
animationStartTimer()
{
    if (animation_timer_id == 0) {
        animation_timer_id = g_timeout_add(PROGRESS_ANIMATION,
                                           animationTimeoutHandler, nullptr);
    }
}

/* ensures that the timer is stopped */
static void
animationStopTimer()
{
    if (animation_timer_id != 0) {
        g_source_remove(animation_timer_id);
        animation_timer_id = 0;
    }
}

/* This function does not unref the weak reference, because the object
 * is being destroyed currently. */
static void
animationOnWidgetDestruction(void *data, GObject *object)
{
    /* steal the animation info from the hash table(destroying it would
     * result in the weak reference to be unrefed, which does not work
     * as the widget is already destroyed. */
    g_hash_table_steal(animated_widgets, object);
    delete (AnimationInfo*)data;
}

/* This function also needs to unref the weak reference. */
static void
animationDestroyInfoAndWeakUnref(void *data)
{
    AnimationInfo *animation_info = (AnimationInfo*)data;

    /* force a last redraw. This is so that if the animation is removed,
     * the widget is left in a sane state. */
    force_widget_redraw(animation_info->widget);

    g_object_weak_unref(G_OBJECT(animation_info->widget),
                        animationOnWidgetDestruction, data);
    delete animation_info;
}

/* Find and return a pointer to the data linked to this widget, if it exists */
static AnimationInfo*
animationLookupInfo(const GtkWidget *widget)
{
    if (animated_widgets) {
        return (AnimationInfo*)g_hash_table_lookup(animated_widgets, widget);
    }
    return nullptr;
}

/* Create all the relevant information for the animation,
 * and insert it into the hash table. */
static void
animationAdd(const GtkWidget *widget, double stop_time)
{
    /* object already in the list, do not add it twice */
    if (animationLookupInfo(widget)) {
        return;
    }

    if (animated_widgets == nullptr) {
        animated_widgets =
            g_hash_table_new_full(g_direct_hash, g_direct_equal, nullptr,
                                  animationDestroyInfoAndWeakUnref);
    }

    auto *value = new AnimationInfo(widget, stop_time);

    g_object_weak_ref(G_OBJECT(widget), animationOnWidgetDestruction, value);
    g_hash_table_insert(animated_widgets, (GtkWidget*)widget, value);

    animationStartTimer();
}

/* update the animation information for each widget. This will also queue a redraw
 * and stop the animation if it is done. */
static gboolean
animationUpdateInfo(void *key, void *value, void*)
{
    AnimationInfo *animation_info = (AnimationInfo*)value;
    GtkWidget *widget = (GtkWidget*)key;

    if ((widget == nullptr) || (animation_info == nullptr)) {
        g_assert_not_reached();
    }

    /* remove the widget from the hash table if it is not drawable */
    if (!gtk_widget_is_drawable(widget)) {
        return true;
    }

    if (GTK_IS_PROGRESS_BAR(widget)) {
        float fraction =
            gtk_progress_bar_get_fraction(GTK_PROGRESS_BAR(widget));
        /* stop animation for filled/not filled progress bars */
        if (fraction <= 0.0 || fraction >= 1.0) {
            return true;
        }
    } else if (GTK_IS_ENTRY(widget)) {
        float fraction = gtk_entry_get_progress_fraction(GTK_ENTRY(widget));

        /* stop animation for filled/not filled progress bars */
        if (fraction <= 0.0 || fraction >= 1.0) {
            return true;
        }
    }

    force_widget_redraw(widget);

    /* stop at stop_time */
    if (animation_info->need_stop()) {
        return true;
    }
    return false;
}

/* This gets called by the glib main loop every once in a while. */
static gboolean
animationTimeoutHandler(void*)
{
    /* enter threads as animationUpdateInfo will use gtk/gdk. */
    gdk_threads_enter();
    g_hash_table_foreach_remove(animated_widgets, animationUpdateInfo, nullptr);
    /* leave threads again */
    gdk_threads_leave();

    if (g_hash_table_size(animated_widgets) == 0) {
        animationStopTimer();
        return false;
    }
    return true;
}

#if 0
static void
on_checkbox_toggle(GtkWidget *widget, void *data)
{
    AnimationInfo *animation_info = animationLookupInfo(widget);

    if (animation_info != nullptr) {
        animation_info->timer_reset();
    } else {
        animationAdd(widget, CHECK_ANIMATION_TIME);
    }
}
#endif

static void
animationOnConnectedWidgetDestruction(void *data, GObject*)
{
    connected_widgets = g_slist_remove(connected_widgets, data);
    free(data);
}

static void
animationDisconnect()
{
    GSList *item = connected_widgets;
    while (item != nullptr) {
        SignalInfo *signal_info = (SignalInfo*)item->data;

        g_signal_handler_disconnect(signal_info->widget,
                                    signal_info->handler_id);
        g_object_weak_unref(G_OBJECT(signal_info->widget),
                            animationOnConnectedWidgetDestruction,
                            signal_info);
        free(signal_info);

        item = g_slist_next(item);
    }

    g_slist_free(connected_widgets);
    connected_widgets = nullptr;
}

#if 0
/* helper function for animationConnectCheckbox */
static int
animationFindSignalInfo(const void *signal_info, const void *widget)
{
    return ((SignalInfo*)signal_info)->widget != widget;
}

/* hooks up the signals for check and radio buttons */
static void
animationConnectCheckbox(GtkWidget *widget)
{
    if (GTK_IS_CHECK_BUTTON(widget)) {
        if (!g_slist_find_custom(connected_widgets, widget,
                                 animationFindSignalInfo)) {
            SignalInfo *signal_info = qtcNew(SignalInfo);

            signal_info->widget = widget;
            signal_info->handler_id =
                g_signal_connect((GObject*)widget, "toggled",
                                 G_CALLBACK(on_checkbox_toggle), nullptr);
            connected_widgets = g_slist_append(connected_widgets, signal_info);
            g_object_weak_ref(
                G_OBJECT(widget), animationOnConnectedWidgetDestruction,
                signal_info);
        }
    }
}
#endif

}

using namespace QtCurve;

/* external interface */

/* adds a progress bar */
void
qtcAnimationAddProgressBar(GtkWidget *progressbar, gboolean isEntry)
{
    double fraction =
        (isEntry ? gtk_entry_get_progress_fraction(GTK_ENTRY(progressbar)) :
         gtk_progress_bar_get_fraction(GTK_PROGRESS_BAR(progressbar)));

    if (fraction < 1.0 && fraction > 0.0) {
        animationAdd((GtkWidget*)progressbar, 0.0);
    }
}

/* returns the elapsed time for the animation */
double
qtcAnimationElapsed(void *data)
{
    AnimationInfo *animation_info = animationLookupInfo((GtkWidget*)data);

    if (animation_info) {
        return animation_info->timer_elapsed();
    }
    return 0.0;
}

/* cleans up all resources of the animation system */
void
qtcAnimationCleanup()
{
    animationDisconnect();

    if (animated_widgets != nullptr) {
        g_hash_table_destroy(animated_widgets);
        animated_widgets = nullptr;
    }
    animationStopTimer();
}
