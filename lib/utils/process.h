/*****************************************************************************
 *   Copyright 2014 - 2014 Yichao Yu <yyc1992@gmail.com>                     *
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

#ifndef _QTC_UTILS_PROCESS_H_
#define _QTC_UTILS_PROCESS_H_

#include "utils.h"

bool qtcForkBackground(QtcCallback cb, void *data, QtcCallback fail_cb=nullptr);
bool qtcSpawn(const char *file, const char *const *argv,
              QtcCallback cb, void *cb_data, QtcCallback fail_cb=nullptr);
typedef enum {
    QTC_POPEN_NONE = 0,
    QTC_POPEN_READ = 1 << 0,
    QTC_POPEN_WRITE = 1 << 1,
    QTC_POPEN_RDWR = QTC_POPEN_READ | QTC_POPEN_WRITE
} QtcPopenFDMode;
typedef struct {
    int orig;
    int replace;
    QtcPopenFDMode mode;
} QtcPopenFD;
bool qtcPopen(const char *file, const char *const *argv,
              unsigned fd_num, QtcPopenFD *fds);
typedef struct {
    int orig;
    QtcPopenFDMode mode;
    char *buff;
    size_t len;
} QtcPopenBuff;
bool qtcPopenBuff(const char *file, const char *const *argv, unsigned buff_num,
                  QtcPopenBuff *buffs, int timeout);
QTC_ALWAYS_INLINE static inline char*
qtcPopenStdout(const char *file, const char *const *argv,
               int timeout, size_t *len)
{
    QtcPopenBuff popen_buff = {1, QTC_POPEN_READ, NULL, 0};
    bool res = qtcPopenBuff(file, argv, 1, &popen_buff, timeout);
    qtcAssign(len, popen_buff.len);
    QTC_RET_IF_FAIL(res, NULL);
    if (!popen_buff.len) {
        free(popen_buff.buff);
        return NULL;
    }
    popen_buff.buff[popen_buff.len] = '\0';
    return popen_buff.buff;
}

#endif
