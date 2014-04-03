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

#ifndef LOGWINDOW_H
#define LOGWINDOW_H

#include <QPlainTextEdit>
#include "../logger.h"

class LogWindow : public QPlainTextEdit
{
    Q_OBJECT
    typedef QPlainTextEdit super;
public:
    explicit LogWindow(QWidget *parent = 0);

    virtual ~LogWindow();

signals:
    void si_closed();

public slots:
    void sl_messageAdded(Logger::Level level, QString message);

protected:
    void contextMenuEvent(QContextMenuEvent *e) override;
    void closeEvent(QCloseEvent *) override;

private:
    QString levelName(Logger::Level level);
    QString levelColor(Logger::Level level);

    QAction * actionClearLog;
    QAction * actionsEnableLevel[Logger::_levelsCount];
};

#endif // LOGWINDOW_H
