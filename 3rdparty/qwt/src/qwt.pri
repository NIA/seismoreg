# NIA: list of qwt files to use them as source, as of src.pro
#

# Allow including these headers
INCLUDEPATH += $$PWD

HEADERS += \
    $$PWD/qwt.h \
    $$PWD/qwt_abstract_scale_draw.h \
    $$PWD/qwt_clipper.h \
    $$PWD/qwt_color_map.h \
    $$PWD/qwt_compat.h \
    $$PWD/qwt_column_symbol.h \
    $$PWD/qwt_date.h \
    $$PWD/qwt_date_scale_draw.h \
    $$PWD/qwt_date_scale_engine.h \
    $$PWD/qwt_dyngrid_layout.h \
    $$PWD/qwt_global.h \
    $$PWD/qwt_graphic.h \
    $$PWD/qwt_interval.h \
    $$PWD/qwt_interval_symbol.h \
    $$PWD/qwt_math.h \
    $$PWD/qwt_magnifier.h \
    $$PWD/qwt_null_paintdevice.h \
    $$PWD/qwt_painter.h \
    $$PWD/qwt_painter_command.h \
    $$PWD/qwt_panner.h \
    $$PWD/qwt_picker.h \
    $$PWD/qwt_picker_machine.h \
    $$PWD/qwt_pixel_matrix.h \
    $$PWD/qwt_point_3d.h \
    $$PWD/qwt_point_polar.h \
    $$PWD/qwt_round_scale_draw.h \
    $$PWD/qwt_scale_div.h \
    $$PWD/qwt_scale_draw.h \
    $$PWD/qwt_scale_engine.h \
    $$PWD/qwt_scale_map.h \
    $$PWD/qwt_spline.h \
    $$PWD/qwt_symbol.h \
    $$PWD/qwt_system_clock.h \
    $$PWD/qwt_text_engine.h \
    $$PWD/qwt_text_label.h \
    $$PWD/qwt_text.h \
    $$PWD/qwt_transform.h \
    $$PWD/qwt_widget_overlay.h

SOURCES += \
    $$PWD/qwt_abstract_scale_draw.cpp \
    $$PWD/qwt_clipper.cpp \
    $$PWD/qwt_color_map.cpp \
    $$PWD/qwt_column_symbol.cpp \
    $$PWD/qwt_date.cpp \
    $$PWD/qwt_date_scale_draw.cpp \
    $$PWD/qwt_date_scale_engine.cpp \
    $$PWD/qwt_dyngrid_layout.cpp \
    $$PWD/qwt_event_pattern.cpp \
    $$PWD/qwt_graphic.cpp \
    $$PWD/qwt_interval.cpp \
    $$PWD/qwt_interval_symbol.cpp \
    $$PWD/qwt_math.cpp \
    $$PWD/qwt_magnifier.cpp \
    $$PWD/qwt_null_paintdevice.cpp \
    $$PWD/qwt_painter.cpp \
    $$PWD/qwt_painter_command.cpp \
    $$PWD/qwt_panner.cpp \
    $$PWD/qwt_picker.cpp \
    $$PWD/qwt_picker_machine.cpp \
    $$PWD/qwt_pixel_matrix.cpp \
    $$PWD/qwt_point_3d.cpp \
    $$PWD/qwt_point_polar.cpp \
    $$PWD/qwt_round_scale_draw.cpp \
    $$PWD/qwt_scale_div.cpp \
    $$PWD/qwt_scale_draw.cpp \
    $$PWD/qwt_scale_map.cpp \
    $$PWD/qwt_spline.cpp \
    $$PWD/qwt_scale_engine.cpp \
    $$PWD/qwt_symbol.cpp \
    $$PWD/qwt_system_clock.cpp \
    $$PWD/qwt_text_engine.cpp \
    $$PWD/qwt_text_label.cpp \
    $$PWD/qwt_text.cpp \
    $$PWD/qwt_transform.cpp \
    $$PWD/qwt_widget_overlay.cpp

 
## contains(QWT_CONFIG, QwtPlot)

    HEADERS += \
        $$PWD/qwt_curve_fitter.h \
        $$PWD/qwt_event_pattern.h \
        $$PWD/qwt_abstract_legend.h \
        $$PWD/qwt_legend.h \
        $$PWD/qwt_legend_data.h \
        $$PWD/qwt_legend_label.h \
        $$PWD/qwt_plot.h \
        $$PWD/qwt_plot_renderer.h \
        $$PWD/qwt_plot_curve.h \
        $$PWD/qwt_plot_dict.h \
        $$PWD/qwt_plot_directpainter.h \
        $$PWD/qwt_plot_grid.h \
        $$PWD/qwt_plot_histogram.h \
        $$PWD/qwt_plot_item.h \
        $$PWD/qwt_plot_abstract_barchart.h \
        $$PWD/qwt_plot_barchart.h \
        $$PWD/qwt_plot_multi_barchart.h \
        $$PWD/qwt_plot_intervalcurve.h \
        $$PWD/qwt_plot_tradingcurve.h \
        $$PWD/qwt_plot_layout.h \
        $$PWD/qwt_plot_marker.h \
        $$PWD/qwt_plot_zoneitem.h \
        $$PWD/qwt_plot_textlabel.h \
        $$PWD/qwt_plot_rasteritem.h \
        $$PWD/qwt_plot_spectrogram.h \
        $$PWD/qwt_plot_spectrocurve.h \
        $$PWD/qwt_plot_scaleitem.h \
        $$PWD/qwt_plot_legenditem.h \
        $$PWD/qwt_plot_seriesitem.h \
        $$PWD/qwt_plot_shapeitem.h \
        $$PWD/qwt_plot_canvas.h \
        $$PWD/qwt_plot_panner.h \
        $$PWD/qwt_plot_picker.h \
        $$PWD/qwt_plot_zoomer.h \
        $$PWD/qwt_plot_magnifier.h \
        $$PWD/qwt_plot_rescaler.h \
        $$PWD/qwt_point_mapper.h \
        $$PWD/qwt_raster_data.h \
        $$PWD/qwt_matrix_raster_data.h \
        $$PWD/qwt_sampling_thread.h \
        $$PWD/qwt_samples.h \
        $$PWD/qwt_series_data.h \
        $$PWD/qwt_series_store.h \
        $$PWD/qwt_point_data.h \
        $$PWD/qwt_scale_widget.h 

    SOURCES += \
        $$PWD/qwt_curve_fitter.cpp \
        $$PWD/qwt_abstract_legend.cpp \
        $$PWD/qwt_legend.cpp \
        $$PWD/qwt_legend_data.cpp \
        $$PWD/qwt_legend_label.cpp \
        $$PWD/qwt_plot.cpp \
        $$PWD/qwt_plot_renderer.cpp \
        $$PWD/qwt_plot_xml.cpp \
        $$PWD/qwt_plot_axis.cpp \
        $$PWD/qwt_plot_curve.cpp \
        $$PWD/qwt_plot_dict.cpp \
        $$PWD/qwt_plot_directpainter.cpp \
        $$PWD/qwt_plot_grid.cpp \
        $$PWD/qwt_plot_histogram.cpp \
        $$PWD/qwt_plot_item.cpp \
        $$PWD/qwt_plot_abstract_barchart.cpp \
        $$PWD/qwt_plot_barchart.cpp \
        $$PWD/qwt_plot_multi_barchart.cpp \
        $$PWD/qwt_plot_intervalcurve.cpp \
        $$PWD/qwt_plot_zoneitem.cpp \
        $$PWD/qwt_plot_tradingcurve.cpp \
        $$PWD/qwt_plot_spectrogram.cpp \
        $$PWD/qwt_plot_spectrocurve.cpp \
        $$PWD/qwt_plot_scaleitem.cpp \
        $$PWD/qwt_plot_legenditem.cpp \
        $$PWD/qwt_plot_seriesitem.cpp \
        $$PWD/qwt_plot_shapeitem.cpp \
        $$PWD/qwt_plot_marker.cpp \
        $$PWD/qwt_plot_textlabel.cpp \
        $$PWD/qwt_plot_layout.cpp \
        $$PWD/qwt_plot_canvas.cpp \
        $$PWD/qwt_plot_panner.cpp \
        $$PWD/qwt_plot_rasteritem.cpp \
        $$PWD/qwt_plot_picker.cpp \
        $$PWD/qwt_plot_zoomer.cpp \
        $$PWD/qwt_plot_magnifier.cpp \
        $$PWD/qwt_plot_rescaler.cpp \
        $$PWD/qwt_point_mapper.cpp \
        $$PWD/qwt_raster_data.cpp \
        $$PWD/qwt_matrix_raster_data.cpp \
        $$PWD/qwt_sampling_thread.cpp \
        $$PWD/qwt_series_data.cpp \
        $$PWD/qwt_point_data.cpp \
        $$PWD/qwt_scale_widget.cpp 

greaterThan(QT_MAJOR_VERSION, 4) {

    QT += printsupport
    QT += concurrent
} 

# contains(QWT_CONFIG, QwtSvg)

    QT += svg

    HEADERS += $$PWD/qwt_plot_svgitem.h
    SOURCES += $$PWD/qwt_plot_svgitem.cpp 


# contains(QWT_CONFIG, QwtOpenGL)

    # QT += opengl

    # HEADERS += $$PWD/qwt_plot_glcanvas.h
    # SOURCES += $$PWD/qwt_plot_glcanvas.cpp

    DEFINES += QWT_NO_OPENGL

# contains(QWT_CONFIG, QwtWidgets)

    HEADERS += \
        $$PWD/qwt_abstract_slider.h \
        $$PWD/qwt_abstract_scale.h \
        $$PWD/qwt_arrow_button.h \
        $$PWD/qwt_analog_clock.h \
        $$PWD/qwt_compass.h \
        $$PWD/qwt_compass_rose.h \
        $$PWD/qwt_counter.h \
        $$PWD/qwt_dial.h \
        $$PWD/qwt_dial_needle.h \
        $$PWD/qwt_knob.h \
        $$PWD/qwt_slider.h \
        $$PWD/qwt_thermo.h \
        $$PWD/qwt_wheel.h
    
    SOURCES += \
        $$PWD/qwt_abstract_slider.cpp \
        $$PWD/qwt_abstract_scale.cpp \
        $$PWD/qwt_arrow_button.cpp \
        $$PWD/qwt_analog_clock.cpp \
        $$PWD/qwt_compass.cpp \
        $$PWD/qwt_compass_rose.cpp \
        $$PWD/qwt_counter.cpp \
        $$PWD/qwt_dial.cpp \
        $$PWD/qwt_dial_needle.cpp \
        $$PWD/qwt_knob.cpp \
        $$PWD/qwt_slider.cpp \
        $$PWD/qwt_thermo.cpp \
        $$PWD/qwt_wheel.cpp


