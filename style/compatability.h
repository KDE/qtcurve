#if GTK_CHECK_VERSION(2, 90, 0)

    static GtkAllocation qtcWidgetGetAllocation(GtkWidget *widget)
    {
        GtkAllocation alloc;
        gtk_widget_get_allocation(widget, &alloc);
        return alloc;
    }

    static GtkRequisition qtcWidgetGetRequisition(GtkWidget *widget)
    {
        GtkRequisition req;
        gtk_widget_get_requisition(widget, &req);
        return req;
    }

    #define qtcWidgetGetWindow(A) gtk_widget_get_parent_window(A)

    static gboolean qtcIsProgressBar(GtkWidget *w)
    {
        if(GTK_IS_PROGRESS_BAR(w))
        {
            GtkAllocation alloc;
            gtk_widget_get_allocation(w, &alloc);
            return -1!=alloc.x && -1!=alloc.y;
        }
        return FALSE;
    }

    static gfloat qtcFrameGetLabelYAlign(GtkFrame *f)
    {
        gfloat x, y;

        gtk_frame_get_label_align(f, &x, &y);
        return y;
    }
    
    #define qtcBinGetChild(W)           gtk_bin_get_child(W)
    #define qtcWidgetGetParent(W)       gtk_widget_get_parent(W)
    #define qtcWidgetName(W)            gtk_widget_get_name(W)
    #define qtcWidgetGetStyle(W)        gtk_widget_get_style(W)
    #define qtcWidgetGetState(W)        gtk_widget_get_state(W)
    #define qtcWidgetSetState(W, S)     gtk_widget_set_state(W, S)
    #define qtcWidgetHasFocus(W)        gtk_widget_has_focus(W)
    #define qtcWidgetState(W)           gtk_widget_get_state(W)
    #define qtcWidgetVisible(W)         gtk_widget_get_visible(W)
    #define qtcWidgetHasDefault(W)      gtk_widget_has_default(W)
    #define qtcWidgetIsSensitive(W)     gtk_widget_is_sensitive(W)
    #define qtcWidgetNoWindow(W)        (!gtk_widget_get_has_window(W))
    #define qtcWidgetType(W)            G_OBJECT_TYPE(W)
    #define qtcWidgetTopLevel(W)        gtk_widget_is_toplevel(W)
    #define qtcWidgetRealized(W)        gtk_widget_get_realized(W)
    #define qtcMenuGetTopLevel(W)       gtk_widget_get_toplevel(W)  /* TODO ??? */
    #define qtcWindowTransientFor(W)    gtk_window_get_transient_for(GTK_WINDOW(W))
    #define qtcWindowDefaultWidget(W)   gtk_window_get_default_widget(GTK_WINDOW(W))
    #define qtcFrameGetLabel(W)         gtk_frame_get_label_widget(W)
    #define qtcButtonIsDepressed(W)     (GTK_SHADOW_IN==shadow_type)
    #define qtcButtonGetLabelText(W)    gtk_button_get_label(W)
    #define qtcAdjustmentGetValue(W)    gtk_adjustment_get_value(W)
    #define qtcAdjustmentGetLower(W)    gtk_adjustment_get_lower(W)
    #define qtcAdjustmentGetUpper(W)    gtk_adjustment_get_upper(W)
    #define qtcAdjustmentGetPageSize(W) gtk_adjustment_get_page_size(W)
    #define qtcRangeGetAdjustment(W)    gtk_range_get_adjustment(W)
    #define qtcMenuItemGetSubMenu(W)    gtk_menu_item_get_submenu(W)
    #define IS_PROGRESS_BAR(W)          qtcIsProgressBar(W)
    #define IS_PROGRESS                 GTK_IS_PROGRESS_BAR(widget) || DETAIL("progressbar")

    #define qtcStyleApplyDefBgnd(SET_BG, STATE) gtk_style_apply_default_background(style, cr, gtk_widget_get_window(widget), STATE, x, y, width, height)

    #define QtcRegion                   cairo_region_t
    #define qtcRegionRect               cairo_region_create_rectangle
    #define qtcRegionXor                cairo_region_xor
    #define qtcRegionDestroy            cairo_region_destroy

    #define QtcRect                     cairo_rectangle_int_t

    #define QTC_KEY_m GDK_KEY_m
    #define QTC_KEY_M GDK_KEY_M
    #define QTC_KEY_s GDK_KEY_s
    #define QTC_KEY_S GDK_KEY_S

    #define WINDOW_PARAM     cairo_t *cr,
    #define WINDOW_PARAM_VAL cr,
    #define AREA_PARAM
    #define AREA_PARAM_VAL
    #define AREA_PARAM_VAL_L NULL

#else

    #define qtcWidgetGetAllocation(W)   ((W)->allocation)
    #define qtcWidgetGetRequisition(W)  ((W)->requisition)
    #define qtcWidgetGetWindow(W)       ((W)->window)
    #define qtcBinGetChild(W)           ((W)->child)
    #define qtcWidgetGetParent(W)       ((W)->parent)
    #define qtcWidgetName(W)            ((W)->name)
    #define qtcWidgetGetStyle(W)        ((W)->style)
    #define qtcWidgetGetState(W)        ((W)->state)
    #define qtcWidgetSetState(W, S)     ((W)->state=(S))
    #define qtcWidgetHasFocus(W)        GTK_WIDGET_HAS_FOCUS(W)
    #define qtcWidgetState(W)           GTK_WIDGET_STATE(W)
    #define qtcWidgetVisible(W)         GTK_WIDGET_VISIBLE(W)
    #define qtcWidgetHasDefault(W)      GTK_WIDGET_HAS_DEFAULT(W)
    #define qtcWidgetIsSensitive(W)     GTK_WIDGET_IS_SENSITIVE(W)
    #define qtcWidgetNoWindow(W)        GTK_WIDGET_NO_WINDOW(W)
    #define qtcWidgetType(W)            GTK_WIDGET_TYPE(W)
    #define qtcWidgetTopLevel(W)        GTK_WIDGET_TOPLEVEL(W)
    #define qtcWidgetRealized(W)        GTK_WIDGET_REALIZED(W)
    #define qtcMenuGetTopLevel(W)       (GTK_MENU(W)->toplevel)
    #define qtcWindowTransientFor(W)    (GTK_WINDOW(W)->transient_parent)
    #define qtcWindowDefaultWidget(W)   (GTK_WINDOW(W)->default_widget)
    #define qtcFrameGetLabel(W)         ((W)->label_widget)
    #define qtcFrameGetLabelYAlign(W)   ((W)->label_yalign)
    #define qtcButtonIsDepressed(W)     (GTK_BUTTON((W))->depressed)
    #define qtcButtonGetLabelText(W)    ((W)->label_text)
    #define qtcAdjustmentGetValue(W)    ((W)->value)
    #define qtcAdjustmentGetLower(W)    ((W)->lower)
    #define qtcAdjustmentGetUpper(W)    ((W)->upper)
    #define qtcAdjustmentGetPageSize(W) ((W)->page_size)
    #define qtcRangeGetAdjustment(W)    ((W)->adjustment)
    #define qtcMenuItemGetSubMenu(W)    ((W)->submenu)
    #define IS_PROGRESS_BAR(W)          (GTK_IS_PROGRESS_BAR((W)) && (W)->allocation.x != -1 && (W)->allocation.y != -1)
    #define IS_PROGRESS                 GTK_IS_PROGRESS(widget) || GTK_IS_PROGRESS_BAR(widget) || DETAIL("progressbar")

    #define qtcStyleApplyDefBgnd(SET_BG, STATE) gtk_style_apply_default_background(style, window, SET_BG, STATE, area, x, y, width, height)
    
    #define QtcRegion                   GdkRegion
    #define qtcRegionRect               gdk_region_rectangle
    #define qtcRegionXor                gdk_region_xor
    #define qtcRegionDestroy            gdk_region_destroy

    #define QtcRect                     GdkRectangle

    #define QTC_KEY_m GDK_m
    #define QTC_KEY_M GDK_M
    #define QTC_KEY_s GDK_s
    #define QTC_KEY_S GDK_S

    #define WINDOW_PARAM     GdkWindow *window,
    #define WINDOW_PARAM_VAL window,
    #define AREA_PARAM       GdkRectangle *area,
    #define AREA_PARAM_VAL   area,
    #define AREA_PARAM_VAL_L area

#endif