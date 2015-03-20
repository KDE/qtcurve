/*****************************************************************************
 *   Copyright 2015 - 2015 Yichao Yu <yyc1992@gmail.com>                     *
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

#ifndef _QTC_UTILS_THREAD_H_
#define _QTC_UTILS_THREAD_H_

#include <pthread.h>

// Replaces thread_local since clang on OSX doesn't really support it.
template<typename T>
class ThreadLocal {
    pthread_key_t m_key;
public:
    ThreadLocal()
    {
        pthread_key_create(&m_key, [] (void *ptr) {
                delete reinterpret_cast<T*>(ptr);
            });
    }
    ~ThreadLocal()
    {
        pthread_key_delete(m_key);
    }
    T*
    get() const
    {
        T *v = reinterpret_cast<T*>(pthread_getspecific(m_key));
        if (!v) {
            v = new T();
            pthread_setspecific(m_key, reinterpret_cast<void*>(v));
        }
        return v;
    }
    T*
    operator->() const
    {
        return get();
    }
};

#endif
