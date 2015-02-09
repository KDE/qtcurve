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

#include "fd_utils.h"
#include <sys/socket.h>
#include <fcntl.h>

QTC_EXPORT bool
qtcSendFD(int sock, int fd)
{
    QTC_RET_IF_FAIL(fd >= 0 && sock >= 0, false);
    char buf = 0;
    iovec iov;
    iov.iov_base = &buf;
    iov.iov_len = 1;
    union {
        struct cmsghdr cmsghdr;
        char control[CMSG_SPACE(sizeof(int))];
    } cmsgu;
    memset(&cmsgu, 0, sizeof(cmsgu));
    msghdr msg;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = cmsgu.control;
    msg.msg_controllen = sizeof(cmsgu.control);
    cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_len = CMSG_LEN(sizeof(int));
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    memcpy(CMSG_DATA(cmsg), &fd, sizeof(int));
    return sendmsg(sock, &msg, 0) >= 0;
}

QTC_EXPORT int
qtcRecvFD(int sock)
{
    QTC_RET_IF_FAIL(sock >= 0, -1);
    char buf = 0;
    iovec iov;
    iov.iov_base = &buf;
    iov.iov_len = 1;
    union {
        struct cmsghdr cmsghdr;
        char control[CMSG_SPACE(sizeof(int))];
    } cmsgu;
    memset(&cmsgu, 0, sizeof(cmsgu));
    msghdr msg;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = cmsgu.control;
    msg.msg_controllen = sizeof(cmsgu.control);
    QTC_RET_IF_FAIL(recvmsg(sock, &msg, 0) >= 0, -1);
    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
    QTC_RET_IF_FAIL(cmsg && cmsg->cmsg_len == CMSG_LEN(sizeof(int)) &&
                    cmsg->cmsg_level == SOL_SOCKET &&
                    cmsg->cmsg_type == SCM_RIGHTS, -1);
    int fd;
    memcpy(&fd, CMSG_DATA(cmsg), sizeof(int));
    return fd;
}

QTC_EXPORT bool
qtcFDSetCloexec(int fd, bool cloexec)
{
    long flags;
    flags = fcntl(fd, F_GETFD, 0);
    if (flags == -1) {
        return false;
    }
    if (cloexec) {
        flags |= FD_CLOEXEC;
    } else {
        flags &= ~FD_CLOEXEC;
    }
    return fcntl(fd, F_SETFD, flags) != -1;
}

QTC_EXPORT bool
qtcFDSetNonBlock(int fd, bool nonblock)
{
    long flags;
    flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        return false;
    }
    if (nonblock) {
        nonblock |= O_NONBLOCK;
    } else {
        nonblock &= ~O_NONBLOCK;
    }
    return fcntl(fd, F_SETFL, flags) != -1;
}
