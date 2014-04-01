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

#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>

class Logger : public QObject
{
    Q_OBJECT
public:
    enum Level {
        Trace,
        Info,
        Warning,
        Error,
        _levelsCount
    };

    // It's a singleton! Use this function to get access to the only instance
    static Logger * instance();

    // Four shortcuts for easier adding log messages:
    static void trace(QString message) { instance()->addMessage(Trace, message); }
    static void info (QString message) { instance()->addMessage(Info, message); }
    static void warning(QString message) { instance()->addMessage(Warning, message); }
    static void error(QString message) { instance()->addMessage(Error, message); }

    // A common way for adding log messages
    void addMessage(Level level, QString message);

    // Enable or disable levels:
    bool isLevelEnabled(Level level) const { return levelEnabled[level]; }
    void setLevelEnabled(Level level, bool value) { levelEnabled[level] = value; }

    template<typename _Function>
    static void forEachLevel(_Function f) {
        for (int i = 0; i < _levelsCount; ++i) {
            f( static_cast<Logger::Level>(i) );
        }
    }

signals:
    void si_messageAdded(Level level, QString message);
    
private:
    explicit Logger();
    static Logger * logger;
    bool levelEnabled[_levelsCount];
};

#endif // LOGGER_H
