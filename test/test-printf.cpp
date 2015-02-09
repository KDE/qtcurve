/*****************************************************************************
 *   Copyright 2013 - 2013 Yichao Yu <yyc1992@gmail.com>                     *
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

#include <qtcurve-utils/strs.h>
#include <assert.h>

#define TEST_FORMAT "%d ... %x \"%s\" kkk %d", 1023, 999, "als8fausdfas", 40

int
main()
{
    char static_res[1024];
    sprintf(static_res, TEST_FORMAT);

    char *asprintf_res;
    asprintf(&asprintf_res, TEST_FORMAT);

    char *m_res1 = _qtcSPrintf(NULL, NULL, false, TEST_FORMAT);
    size_t size = 10;
    char *m_res2 = _qtcSPrintf((char*)malloc(10), &size, true, TEST_FORMAT);
    assert(size > strlen(m_res2));

    size = strlen(m_res2);
    char *m_res3 = _qtcSPrintf((char*)malloc(size), &size, true, TEST_FORMAT);
    assert(size > strlen(m_res3));

    size = strlen(m_res3) + 1;
    char *buff4 = (char*)malloc(size);
    char *m_res4 = _qtcSPrintf(buff4, &size, true, TEST_FORMAT);
    assert(m_res4 == buff4);
    assert(size == strlen(m_res4) + 1);

    char buff5[size];
    char *m_res5 = _qtcSPrintf(buff5, &size, false, TEST_FORMAT);
    assert(m_res5 == buff5);
    assert(size == strlen(m_res5) + 1);

    size--;
    char buff6[size];
    char *m_res6 = _qtcSPrintf(buff6, &size, false, TEST_FORMAT);
    assert(m_res6 != buff6);
    assert(size > strlen(m_res6) + 1);

    QtCurve::StrBuff<10> buff7(10);
    assert(buff7.is_static() && buff7.size() == 10);
    char *m_res7 = buff7.printf(TEST_FORMAT);
    assert(buff7.get() == m_res7);
    assert(!buff7.is_static());

    QtCurve::StrBuff<10> buff8(11);
    assert(!buff8.is_static() && buff8.size() == 11);
    char *old_p8 = buff8.get();
    char *m_res8 = buff8.printf(TEST_FORMAT);
    assert(buff8.get() == m_res8);
    assert(buff8.get() != old_p8);

    QtCurve::StrBuff<10> buff9(1024);
    assert(!buff9.is_static() && buff9.size() == 1024);
    char *old_p9 = buff9.get();
    char *m_res9 = buff9.printf(TEST_FORMAT);
    assert(buff9.get() == m_res9);
    assert(buff9.get() == old_p9);

    QtCurve::StrBuff<2048> buff10(1024);
    assert(buff10.is_static() && buff10.size() == 1024);
    char *old_p10 = buff10.get();
    char *m_res10 = buff10.printf(TEST_FORMAT);
    assert(buff10.get() == m_res10);
    assert(buff10.get() == old_p10);
    assert(buff10.is_static());

    const char *const results[] = {
        static_res,
        asprintf_res,
        m_res1,
        m_res2,
        m_res3,
        m_res4,
        m_res5,
        m_res6,
        m_res7,
        m_res8,
        m_res9,
        m_res10,
    };
    for (unsigned i = 0;i < sizeof(results) / sizeof(results[0]);i++) {
        for (unsigned j = 0;j < sizeof(results) / sizeof(results[0]);j++) {
            assert(strcmp(results[i], results[j]) == 0);
        }
    }
    free(asprintf_res);
    free(m_res1);
    free(m_res2);
    free(m_res3);
    free(m_res4);
    free(m_res6);
    return 0;
}
