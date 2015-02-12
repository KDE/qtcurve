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

#include <qtcurve-utils/process.h>
#include <assert.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>

static char buff1[1024];
static char buff2[1024];

static void
fillBuffer(char *buff, size_t size)
{
    for (unsigned i = 0;i < size - 1;i++) {
        while (!buff[i]) {
            buff[i] = random();
        }
    }
    buff[size - 1] = '\0';
}

static void
subProcess(int, char **argv)
{
    char buff3[1024] = {0};
    read(0, buff3, sizeof(buff3));
    assert(memcmp(argv[1], buff3, sizeof(buff3)) == 0);
    write(1, argv[2], strlen(argv[2]) + 1);
}

static void
mainProcess(int, char **argv)
{
    srandom(time(nullptr));
    fillBuffer(buff1, sizeof(buff1));
    fillBuffer(buff2, sizeof(buff2));
    QtcPopenFD fds[] = {{
            .orig = 0,
            .replace = 0,
            .mode = QTC_POPEN_WRITE
        }, {
            .orig = 1,
            .replace = 0,
            .mode = QTC_POPEN_READ
        }
    };
    alarm(1);
    const char* const args[] = {argv[0], buff1, buff2, nullptr};
    qtcPopen(argv[0], args, sizeof(fds) / sizeof(fds[0]), fds);
    write(fds[0].replace, buff1, sizeof(buff1));
    shutdown(fds[0].replace, SHUT_RDWR);
    close(fds[0].replace);
    char buff3[1024] = {0};
    read(fds[1].replace, buff3, sizeof(buff3));
    assert(memcmp(buff2, buff3, sizeof(buff3)) == 0);
}

int
main(int argc, char **argv)
{
    if (argv[1]) {
        subProcess(argc, argv);
    } else {
        mainProcess(argc, argv);
    }
    return 0;
}
