/*
 * (C) Copyright 2012 Kuban State University (http://kubsu.ru/),
 *     Faculty of Physics and Technology, Physics and Information Systems Chair
 *
 * All rights reserved. This file is part of StatistiQ, a program
 * for getting various statistical information from measurement data.
 *
 * Developed by Ivan Novikov (http://github.com/NIA)
 * under the direction of Leontiy Grigorjan
 *
 */

#include "logwindow.h"
#include <QDateTime>
#include <QIcon>
#include <QAction>
#include <QMenu>
#include <QDebug>

LogWindow::LogWindow(QWidget *parent) :
    super(parent)
{
    actionClearLog = new QAction(QIcon(":/icons/images/remove.ico"), tr("Clear log"), this);
    connect(actionClearLog, &QAction::triggered, this, &LogWindow::clear);
    connect(Logger::instance(), &Logger::si_messageAdded, this, &LogWindow::sl_messageAdded);

    setUndoRedoEnabled(false);
    setReadOnly(true);
    setLineWrapMode(super::WidgetWidth);
    setTextInteractionFlags(Qt::NoTextInteraction|Qt::TextSelectableByMouse|Qt::TextSelectableByKeyboard);
    setCenterOnScroll(true);
}

void LogWindow::contextMenuEvent(QContextMenuEvent *e) {
    QMenu * menu = createStandardContextMenu();
    menu->addAction(actionClearLog);
    menu->exec(e->globalPos());
    delete menu;
}

void LogWindow::sl_messageAdded(Logger::Level level, QString message) {
    QString dateTime = QDateTime::currentDateTime().time().toString(Qt::DefaultLocaleShortDate);

    QString formattedMessage = "";
    formattedMessage += QString("<font color='%1'><b>").arg(levelColor(level));
    formattedMessage += QString("[%1] ").arg(dateTime);
    formattedMessage += (levelName(level) + ":</b> ");
    formattedMessage += message.toHtmlEscaped();
    formattedMessage += "</font>";

    QString plainTextMessage = QString("[%1] %2: %3").arg(dateTime).arg(levelName(level)).arg(message);
    if ( level == Logger::Error ) {
        qCritical() << plainTextMessage;
    } else {
        qDebug() << plainTextMessage;
    }

    appendHtml(formattedMessage);
}

QString LogWindow::levelName(Logger::Level level) {
    switch(level) {
    case Logger::Trace:
        return tr("TRACE");
    case Logger::Info:
        return tr("INFO");
    case Logger::Warning:
        return tr("WARNING");
    case Logger::Error:
        return tr("ERROR");
    default:
        return "";
    }
}

QString LogWindow::levelColor(Logger::Level level) {
    switch(level) {
    case Logger::Trace:
        return "gray";
    case Logger::Info:
        return "black";
    case Logger::Warning:
        return "maroon";
    case Logger::Error:
        return "red";
    default:
        return "";
    }
}

void LogWindow::closeEvent(QCloseEvent *) {
    emit si_closed();
}
