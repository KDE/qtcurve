/*****************************************************************************
 *   Copyright 2013 - 2015 Yichao Yu <yyc1992@gmail.com>                     *
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

#ifndef _QTC_UTILS_DIRS_H_
#define _QTC_UTILS_DIRS_H_

/**
 * \file dirs.h
 * \author Yichao Yu <yyc1992@gmail.com>
 * \brief Standard directories and file system related functions.
 */

#include "utils.h"
#include <unistd.h>
#include <sys/stat.h>

namespace QtCurve {
/**
 * Get QtCurve configure directory. The directory will be created if it doesn't
 * exist. The returned string is guaranteed to end with '/'
 */
const char *getConfDir();

/**
 * Get home directory. The returned string is guaranteed to end with '/'
 */
const char *getHome();

/**
 * Get XDG_DATA_HOME directory. This is usually `~/.local/share/`
 * The returned string is guaranteed to end with '/'
 */
const char *getXDGDataHome();

/**
 * Get XDG_CONFIG_HOME directory. This is usually `~/.config/`
 * The returned string is guaranteed to end with '/'
 */
const char *getXDGConfigHome();

/**
 * Return the absolute path of \param file with the QtCurve configure directory
 * as the current directory. If the optional argument \param buff is not NULL
 * it will be realloc'ed to hold the result. If \param path is a relative path
 * the QtCurve configure directory will be created.
 */
char *getConfFile(const char *file, char *buff=nullptr);

static inline std::string
getConfFile(const std::string &file)
{
    if (file[0] == '/')
        return file;
    return getConfDir() + file;
}

static inline std::string
getConfFile(std::string &&file)
{
    if (file[0] == '/')
        return std::move(file);
    return getConfDir() + std::move(file);
}

/**
 * Create directory \param path with mode \param mode. Its parent directories
 * will also be created as needed.
 */
void makePath(const char *path, int mode);

/**
 * Check whether \param path is (or is a symlink that is pointing to)
 * a directory with read and execute permissions.
 */
static inline bool
isDir(const char *path)
{
    struct stat stats;
    return (stat(path, &stats) == 0 && S_ISDIR(stats.st_mode) &&
            access(path, R_OK | X_OK) == 0);
}

/**
 * Check whether \param path is (or is a symlink that is pointing to)
 * a regular file with read permission.
 */
static inline bool
isRegFile(const char *path)
{
    struct stat stats;
    return (stat(path, &stats) == 0 && S_ISREG(stats.st_mode) &&
            access(path, R_OK) == 0);
}

/**
 * Check whether \param path is a symlink.
 */
static inline bool
isSymLink(const char *path)
{
    struct stat stats;
    return lstat(path, &stats) == 0 && S_ISLNK(stats.st_mode);
}

}

#endif
