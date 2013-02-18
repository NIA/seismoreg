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

Logger * Logger::logger = NULL;

Logger::Logger() {
}

Logger * Logger::instance() {
    if(logger == NULL) {
        logger = new Logger;
    }
    return logger;
}
