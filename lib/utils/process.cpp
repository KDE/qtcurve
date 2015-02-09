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

#include "process.h"
#include "fd_utils.h"
#include "timer.h"
#include <unistd.h>
#if !defined(__APPLE__) && !defined(__MACH__)
#include <wait.h>
#else
#include <signal.h>
#endif //__APPLE__
#include <sys/stat.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>

static bool
qtcSignalHandlerSet(int sig)
{
    struct sigaction oact;
    QTC_RET_IF_FAIL(sigaction(sig, NULL, &oact) == 0, false);
    void *handler = ((oact.sa_flags & SA_SIGINFO) ? (void*)oact.sa_handler :
                     (void*)oact.sa_sigaction);
    return qtcNoneOf(handler, SIG_DFL, SIG_IGN);
}

QTC_EXPORT bool
qtcForkBackground(QtcCallback cb, void *data, QtcCallback fail_cb)
{
    QTC_RET_IF_FAIL(cb, false);
    // On linux, waitpid will not accept (discard) SIGCHLD therefore if there is
    // a signal handler registered for SIGCHLD and the child process exit
    // inside waitpid()/wait(), it will be run after the process state is
    // cleared and would therefore block if it call wait() (or waitpid(-1))
    // and if there are other child processes. As a workaround we only call
    // waitpid() if the main program did not set up any signal handlers for
    // SIGCHLD. See (the RATIONALE section of) wait(3P) for more detail.
    pid_t child = fork();
    if (child < 0) {
        return false;
    } else if (child == 0) {
        pid_t grandchild = fork();
        if (grandchild < 0) {
            qtcCall(fail_cb, data);
            _exit(1);
        } else if (grandchild == 0) {
            /* grandchild */
            cb(data);
            _exit(0);
        } else {
            _exit(0);
        }
        return true;
    } else {
        /* parent */
        if (qtcSignalHandlerSet(SIGCHLD)) {
            // If we create a child process, the signal handler will receive
            // the signal anyway (and there is no way to only block SIGCHLD
            // only for our child process). Since the signal handler may
            // hang and should already take care of getting rid of
            // zombie processes, we do not call waitpid in this case....
            return true;
        }
        // If SIGCHLD is ignored, waitpid will return -1 with errno
        // set to ECHILD, treat this as success (good enough for our purpose
        // and not likely to fail anyway...)
        int status = 0;
        return ((waitpid(child, &status, 0) > 0 && status == 0) ||
                errno == ECHILD);
    }
}

typedef struct {
    const char *file;
    char *const *argv;
    QtcCallback cb;
    void *cb_data;
    QtcCallback fail_cb;
} QtcSpawnData;

static void
qtcSpawnCb(void *_data)
{
    const QtcSpawnData *data = (const QtcSpawnData*)_data;
    qtcCall(data->cb, data->cb_data);
    execvp(data->file, data->argv);
}

static void
qtcSpawnFailCb(void *_data)
{
    const QtcSpawnData *data = (const QtcSpawnData*)_data;
    qtcCall(data->fail_cb, data->cb_data);
}

QTC_EXPORT bool
qtcSpawn(const char *file, const char *const *argv, QtcCallback cb,
         void *cb_data, QtcCallback fail_cb)
{
    QtcSpawnData data = {file, (char *const*)argv, cb, cb_data, fail_cb};
    return qtcForkBackground(qtcSpawnCb, &data, qtcSpawnFailCb);
}

typedef struct {
    int ctrl_fd;
    unsigned fd_num;
    QtcPopenFD *fds;
} QtcPopenData;

static void
qtcPopenCb(void *_data)
{
    QtcPopenData *data = (QtcPopenData*)_data;
    for (unsigned i = 0;i < data->fd_num;i++) {
        int mode = data->fds[i].mode & QTC_POPEN_RDWR;
        int ret_fd = -1;
        int replace_fd = -1;
        if (!mode) {
            replace_fd = open("/dev/null", O_RDWR);
            // Make sure a valid fd is sent.
            ret_fd = replace_fd;
        } else {
            // Open socket pairs in the child process and send it back to
            // parent with a unix domain socket so that the write end of the
            // pair is always under control.
            // For writing to sub process, the parent will shutdown the write
            // end when it is done (therefore the client will receive EOF even
            // if the parent forks other subprocesses which keeps the pipes
            // open).
            // For reading from sub process, the write end of the pipe is not
            // shared with any other process so the parent will receive EOF
            // whenever the client closes the pipe or exit.
            // See http://stackoverflow.com/questions/1583005/is-there-any-difference-between-socketpair-and-pair-of-unnamed-pipes
            int socket_fds[2];
            socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds);
            ret_fd = socket_fds[0];
            replace_fd = socket_fds[1];
            if (!(mode & QTC_POPEN_READ)) {
                shutdown(ret_fd, SHUT_RD);
                shutdown(replace_fd, SHUT_WR);
            } else if (!(mode & QTC_POPEN_WRITE)) {
                shutdown(ret_fd, SHUT_WR);
                shutdown(replace_fd, SHUT_RD);
            }
        }
        dup2(replace_fd, data->fds[i].orig);
        close(replace_fd);
        qtcSendFD(data->ctrl_fd, ret_fd);
        close(ret_fd);
    }
    shutdown(data->ctrl_fd, SHUT_RDWR);
    close(data->ctrl_fd);
}

static void
qtcPopenFailCb(void *_data)
{
    QtcPopenData *data = (QtcPopenData*)_data;
    // Notify the parent that sth goes wrong.
    shutdown(data->ctrl_fd, SHUT_RDWR);
    close(data->ctrl_fd);
}

QTC_EXPORT bool
qtcPopen(const char *file, const char *const *argv,
         unsigned fd_num, QtcPopenFD *fds)
{
    if (qtcUnlikely(!fds || !fd_num)) {
        return qtcSpawn(file, argv, NULL, NULL);
    }
    for (unsigned i = 0;i < fd_num;i++) {
        QTC_RET_IF_FAIL(fds[i].orig >= 0, false);
    }
    int socket_fds[2];
    QTC_RET_IF_FAIL(socketpair(AF_UNIX, SOCK_STREAM, 0,
                               socket_fds) == 0, false);
    qtcFDSetCloexec(socket_fds[0], true);
    qtcFDSetCloexec(socket_fds[1], true);
    QtcPopenData cbdata = {socket_fds[0], fd_num, fds};
    bool res = qtcSpawn(file, argv, qtcPopenCb, &cbdata, qtcPopenFailCb);
    if (!res) {
        shutdown(socket_fds[0], SHUT_RDWR);
        close(socket_fds[0]);
        shutdown(socket_fds[1], SHUT_RDWR);
        close(socket_fds[1]);
        return false;
    }
    close(socket_fds[0]);
    for (unsigned i = 0;i < fd_num;i++) {
        if ((fds[i].replace = qtcRecvFD(socket_fds[1])) < 0) {
            res = false;
            for (unsigned j = 0;j < i;j++) {
                if (fds[i].replace) {
                    shutdown(fds[i].replace, SHUT_RDWR);
                    close(fds[i].replace);
                }
            }
            break;
        }
        if (!(fds[i].mode & (QTC_POPEN_READ | QTC_POPEN_WRITE))) {
            close(fds[i].replace);
            fds[i].replace = -1;
            continue;
        }
    }
    shutdown(socket_fds[1], SHUT_RDWR);
    close(socket_fds[1]);
    return res;
}

static bool
qtcPopenReadBuff(QtcPopenBuff *buffs)
{
    buffs->buff = (char*)realloc(buffs->buff, buffs->len + 1024 + 1);
    ssize_t len = read(buffs->orig, buffs->buff + buffs->len, 1024);
    if (len == 0 || (len == -1 && qtcNoneOf(errno, EAGAIN, EINTR,
                                            EWOULDBLOCK))) {
        return false;
    } else if (len > 0) {
        buffs->len += len;
    }
    return true;
}

static bool
qtcPopenWriteBuff(QtcPopenBuff *buffs)
{
    ssize_t len = write(buffs->orig, buffs->buff, buffs->len);
    if (len == 0 || (len == -1 && qtcNoneOf(errno, EAGAIN, EINTR,
                                            EWOULDBLOCK))) {
        return false;
    } else if (len > 0) {
        buffs->buff += len;
        buffs->len -= len;
    }
    return true;
}

static bool
qtcPopenPollCheckTimeout(uint64_t start, int timeout, int *new_timeout)
{
    if (timeout < 0) {
        *new_timeout = -1;
        return true;
    }
    int elapse = qtcGetElapse(start) / 1000000;
    if (elapse > timeout) {
        return false;
    }
    *new_timeout = timeout - elapse;
    return true;
}

QTC_EXPORT bool
qtcPopenBuff(const char *file, const char *const argv[],
             unsigned buff_num, QtcPopenBuff *buffs, int timeout)
{
    if (qtcUnlikely(!buffs || !buff_num)) {
        return qtcSpawn(file, argv, NULL, NULL);
    }
    bool need_poll = false;
    for (unsigned i = 0;i < buff_num;i++) {
        QTC_RET_IF_FAIL(buffs[i].orig >= 0, false);
        QTC_RET_IF_FAIL(!(buffs[i].mode & QTC_POPEN_READ &&
                          buffs[i].mode & QTC_POPEN_WRITE), false);
        if (buffs[i].mode & QTC_POPEN_READ ||
            buffs[i].mode & QTC_POPEN_WRITE) {
            need_poll = true;
        }
    }
    QtCurve::LocalBuff<QtcPopenFD, 16> fds(buff_num);
    for (unsigned i = 0;i < buff_num;i++) {
        fds[i].orig = buffs[i].orig;
        fds[i].replace = -1;
        fds[i].mode = buffs[i].mode;
    }
    bool res = qtcPopen(file, argv, buff_num, fds.get());
    if (!res) {
        return false;
    }
    for (unsigned i = 0;i < buff_num;i++) {
        buffs[i].orig = fds[i].replace;
        if (fds[i].replace >= 0) {
            qtcFDSetNonBlock(fds[i].replace, true);
            qtcFDSetCloexec(fds[i].replace, true);
        }
    }
    if (!need_poll) {
        return true;
    }
    QtCurve::LocalBuff<pollfd, 16> poll_fds(buff_num);
    QtCurve::LocalBuff<int, 16> indexes(buff_num);
    unsigned poll_fd_num = 0;
    for (unsigned i = 0;i < buff_num;i++) {
        if (!(buffs[i].mode & (QTC_POPEN_READ | QTC_POPEN_WRITE))) {
            close(buffs[i].orig);
            continue;
        }
        indexes[poll_fd_num] = i;
        pollfd *cur_fd = &poll_fds[poll_fd_num];
        cur_fd->fd = buffs[i].orig;
        cur_fd->events = (buffs[i].mode & QTC_POPEN_READ) ? POLLIN : POLLOUT;
        poll_fd_num++;
    }
    uint64_t start_time = qtcGetTime();
    int poll_timeout = timeout;
    while (true) {
        int ret = poll(poll_fds.get(), poll_fd_num, poll_timeout);
        if (ret == -1) {
            if (errno == EINTR) {
                if (!qtcPopenPollCheckTimeout(start_time, timeout,
                                              &poll_timeout)) {
                    break;
                }
                continue;
            }
            break;
        } else if (ret == 0) {
            break;
        }
        for (unsigned i = 0;i < poll_fd_num;i++) {
            pollfd *cur_fd = &poll_fds[i];
            if (cur_fd->revents & POLLIN) {
                if (!qtcPopenReadBuff(&buffs[indexes[i]])) {
                    cur_fd->events &= ~POLLIN;
                }
            } else if (cur_fd->revents & POLLOUT) {
                if (!qtcPopenWriteBuff(&buffs[indexes[i]])) {
                    cur_fd->events &= ~POLLOUT;
                }
            }
            if (cur_fd->revents & (POLLERR | POLLHUP | POLLNVAL) ||
                !(cur_fd->events & (POLLIN | POLLOUT))) {
                shutdown(cur_fd->fd, SHUT_RDWR);
                close(cur_fd->fd);
                poll_fd_num--;
                memmove(cur_fd, cur_fd + 1,
                        (poll_fd_num - i) * sizeof(pollfd));
                memmove(indexes.get() + i, indexes.get() + i + 1,
                        (poll_fd_num - i) * sizeof(int));
                i--;
            }
        }
        if (poll_fd_num <= 0 || !qtcPopenPollCheckTimeout(start_time, timeout,
                                                          &poll_timeout)) {
            break;
        }
    }
    for (unsigned i = 0;i < poll_fd_num;i++) {
        pollfd *cur_fd = &poll_fds[i];
        shutdown(cur_fd->fd, SHUT_RDWR);
        close(cur_fd->fd);
    }
    return true;
}
