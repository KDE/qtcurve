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

    QTC_DEF_LOCAL_BUFF(char, buff7, 10, 10);
    assert(buff7.p == buff7.static_p && buff7.l == buff7.static_l);
    char *m_res7 = QTC_LOCAL_BUFF_PRINTF(buff7, TEST_FORMAT);
    assert(buff7.p == m_res7);
    assert(buff7.p != buff7.static_p);

    QTC_DEF_LOCAL_BUFF(char, buff8, 10, 11);
    assert(buff8.p != buff8.static_p && buff8.l != buff8.static_l);
    char *old_p8 = buff8.p;
    char *m_res8 = QTC_LOCAL_BUFF_PRINTF(buff8, TEST_FORMAT);
    assert(buff8.p == m_res8);
    assert(buff8.p != old_p8);

    QTC_DEF_LOCAL_BUFF(char, buff9, 10, 1024);
    assert(buff9.p != buff9.static_p && buff9.l != buff9.static_l);
    char *old_p9 = buff9.p;
    char *m_res9 = QTC_LOCAL_BUFF_PRINTF(buff9, TEST_FORMAT);
    assert(buff9.p == m_res9);
    assert(buff9.p == old_p9);

    QTC_DEF_LOCAL_BUFF(char, buff10, 2048, 1024);
    assert(buff10.p == buff10.static_p && buff10.l == buff10.static_l);
    char *old_p10 = buff10.p;
    char *m_res10 = QTC_LOCAL_BUFF_PRINTF(buff10, TEST_FORMAT);
    assert(buff10.p == m_res10);
    assert(buff10.p == old_p10);
    assert(buff10.p == buff10.static_p);

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
    QTC_FREE_LOCAL_BUFF(buff7);
    QTC_FREE_LOCAL_BUFF(buff8);
    QTC_FREE_LOCAL_BUFF(buff9);
    QTC_FREE_LOCAL_BUFF(buff10);
    return 0;
}
