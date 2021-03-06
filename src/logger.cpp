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

#include "logger.h"
#include <QDebug>

Logger * Logger::logger = NULL;

Logger::Logger() {
    for (auto & item: levelEnabled) {
        item = true;
    }
}

Logger * Logger::instance() {
    if(logger == NULL) {
        logger = new Logger;
    }
    return logger;
}

void Logger::addMessage(Level level, QString message) {
    if (levelEnabled[level]) {

        if ( level == Error ) {
            qCritical() << message;
        } else {
            qDebug() << message;
        }

        emit si_messageAdded(level, message);
    }
}
