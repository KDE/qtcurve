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

#include <qtcurve-utils/map.h>
#include <assert.h>
#include <ctype.h>

#define STRINGS "jk", "yz", "OP", "LM", "XY", "FG", "MN", "st", "VW", "qr", \
        "vw", "bc", "xy", "gh", "cd", "mn", "op", "ab", "[\\", "WX", "BC", \
        "CD", "IJ", "QR", "Z[", "uv", "_`", "ij", "hi", "no", "HI", "DE", \
        "YZ", "PQ", "ef", "de", "pq", "tu", "fg", "lm", "JK", "kl", "AB", \
        "KL", "TU", "]^", "ST", "EF", "z{", "RS", "wx", "^_", "rs", "`a", \
        "\\]", "UV", "GH", "NO"

#define CASE_STRINGS "m_n", "tpu", "xty", "h`i", "\\l]", "_]`", "vnw", "_q`", \
        "wyx", "oap", "ppq", "isj", "^v_", "k]l", "y_z", "]s^", "e[f", "w]x", \
        "`pa", "^j_", "nro", "uwv", "[e\\", "cud", "cmd", "rns", "ugv", \
        "fvg", "ewf", "rrs", "^b_", "ltm", "e_f", "cad", "t\\u", "vjw", \
        "t`u", "cid", "q[r", "oqp", "gah", "lhm", "r^s", "ikj", "wex", "agb", \
        "]w^", "p\\q", "awb", "acb", "_y`", "`da", "fjg", "qsr", "]g^", \
        "usv", "kel", "p`q", "qcr", "icj", "tdu", "hpi", "^z_", "vvw", "kal", \
        "ekf", "`ha", "akb", "dhe", "dte", "frg", "kul", "nno", "`\\a", \
        "guh", "ffg", "omp", "qor", "hli", "igj", "oyp", "`ta", "ldm", "ywz", \
        "jnk", "jrk", "\\`]", "njo", "mkn", "wix", "zb{", "ttu", "jzk", \
        "oep", "zr{", "dle", "dpe", "xdy", "uov", "[y\\", "kyl", "zv{", \
        "smt", "wax", "thu", "xpy", "d`e", "gih", "lpm", "h\\i", "x\\y", \
        "[a\\", "fzg", "zz{", "sat", "eof", "[]\\", "hti", "v^w", "syt", \
        "mgn", "y[z", "jfk", "s]t", "kil", "esf", "bnc", "oup", "set", "ycz", \
        "aob", "wux", "pxq", "ygz", "mwn", "z^{", "xhy", "dxe", "i_j", "a[b", \
        "q_r", "\\t]", "^^_", "n^o", "zn{", "sit", "bfc", "mcn", "rfs", \
        "brc", "bjc", "\\d]", "ioj", "f^g", "\\h]", "iwj", "nzo", "u_v", \
        "oip", "sqt", "vfw", "`la", "jbk", "hxi", "llm", "fbg", "phq", "jjk", \
        "gyh", "qkr", "_m`", "nfo", "[i\\", "rvs", "bzc", "ukv", "u[v", \
        "nvo", "_u`", "geh", "nbo", "bbc", "dde", "xly", "bvc", "qwr", "_a`", \
        "ecf", "zj{", "tlu", "]k^", "a_b", "kql", "]c^", "][^", "wmx", "l`m", \
        "^r_", "x`y", "rzs", "qgr", "_e`", "hdi", "\\x]", "\\p]", "ced", \
        "sut", "[m\\", "cqd", "fng", "]_^", "d\\e", "kml", "txu", "m[n", \
        "vzw", "gmh", "zf{", "vbw", "lxm", "rbs", "[q\\", "cyd", "rjs", \
        "^n_", "j^k", "pdq", "vrw", "plq", "yoz", "mon", "b^c", "i[j", \
        "l\\m", "\\\\]", "_i`", "egf", "hhi", "ysz", "gqh", "c]d", "xxy", \
        "msn", "``a", "jvk", "asb", "ucv", "]o^", "ptq", "ykz", "o]p", "`xa", \
        "[u\\", "g]h", "wqx", "^f_"

static int
search_map(const char *str)
{
    static const QtCurve::StrMap<int, true> auto_map(STRINGS);
    return auto_map.search(str);
}

static int
search_case_map(const char *str)
{
    static const QtCurve::StrMap<> auto_map(CASE_STRINGS);
    return auto_map.search(str);
}

int
main()
{
    const char *real_order[] = {STRINGS};
    char key[4];
    key[3] = 0;
    for (char i = 0;i < 127;i++) {
        key[0] = i;
        for (char j = 0;j < 127;j++) {
            key[1] = j;
            for (char k = 0;k < 127;k++) {
                key[2] = k;
                int res = search_map(key);
                if (i >= 'A' && i <= 'z' && j == i + 1 && k == 0) {
                    assert(res != -1 && strcmp(real_order[res], key) == 0);
                } else {
                    assert(res == -1);
                }
            }
        }
    }
    const char *case_real_order[] = {CASE_STRINGS};
    for (char i = 0;i < 127;i++) {
        key[0] = i;
        for (char j = 0;j < 127;j++) {
            key[1] = j;
            for (char k = 0;k < 127;k++) {
                key[2] = k;
                int res = search_case_map(key);
                char _i = tolower(i);
                char _j = tolower(j);
                char _k = tolower(k);
                if (_i > 'Z' && _i <= 'z' && _j > 'Z' && _j <= 'z' &&
                    _k == _i + 1 && (_i + _j) % 4 == 0) {
                    assert(res != -1 &&
                           strcasecmp(case_real_order[res], key) == 0);
                } else {
                    assert(res == -1);
                }
            }
        }
    }
    return 0;
}
