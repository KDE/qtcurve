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

#ifndef _QTC_UTILS_MACROS_H_
#define _QTC_UTILS_MACROS_H_

/**
 * \file macros.h
 * \author Yichao Yu <yyc1992@gmail.com>
 * \brief Definitions of some useful macros.
 */

/** \defgroup qtc_switch Macros for detecting empty arguments
 * \brief Used to implement function overloading and default arguments in c.
 *
 * The idea of this implementation is borrowed from the following
 * [post](https://gustedt.wordpress.com/2010/06/08/detect-empty-macro-arguments)
 * and is modified in order to fit with our usage.
 * @{
 */

#define __QTC_USE_3(_1, _2, _3, ...) _3
// Test if args has one comma
#define __QTC_HAS_COMMA1(ret_true, ret_false, args...)  \
    __QTC_USE_3(args, ret_true, ret_false)
// Convert parentheses to comma, used to check if the next character is "("
#define __QTC_CONVERT_PAREN(...) ,
// Check if arg starts with "("
#define __QTC_IS_PAREN_(ret_true, ret_false, arg)                       \
    __QTC_HAS_COMMA1(ret_true, ret_false, __QTC_CONVERT_PAREN arg)
// Extra layer just to make sure more evaluation (if any) is done than the
// separator path.
#define __QTC_IS_PAREN(ret_true, ret_false, arg)        \
    __QTC_IS_PAREN_(ret_true, ret_false, arg)
// Check if arg is not empty and does not start with "("
// Will not work if arg has comma or is the name of a function like macro
#define __QTC_IS_SEP(ret_true, ret_false, arg)                          \
    __QTC_HAS_COMMA1(ret_false, ret_true, __QTC_CONVERT_PAREN arg())
#define __QTC_IS_EMPTY_PAREN_TRUE(ret_true, ret_false, arg) ret_false
#define __QTC_IS_EMPTY_PAREN_FALSE(ret_true, ret_false, arg)    \
    __QTC_IS_SEP(ret_false, ret_true, arg)

/**
 * \brief Test if \param arg is empty.
 * Evaluate to \param ret_true if it is not empty,
 * evaluate to \param ret_false otherwise.
 *
 * NOTE: this may not work if \param arg is a macro that can be evaluated to a
 * comma separate list without parentheses or is the name of
 * a function like macro.
 */
#define QTC_SWITCH(arg, ret_true, ret_false)                            \
    __QTC_IS_PAREN(__QTC_IS_EMPTY_PAREN_TRUE, __QTC_IS_EMPTY_PAREN_FALSE, arg) \
    (ret_false, ret_true, arg)
/**
 * Evaluate to \param def if \param v is empty and to \param v otherwise.
 * \sa QTC_SWITCH for restrictions.
 **/
#define QTC_DEFAULT(v, def) QTC_SWITCH(v, v, def)

/** @} */

#define qtcMakeVersion(a, b, c...)                      \
    ((a) << 16 | (b) << 8 | (QTC_DEFAULT(c, 0)))
#ifdef __GNUC__
#  define QTC_GCC_VERSION                                               \
    qtcMakeVersion(__GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__)
#else
#  define QTC_GCC_VERSION 0
#endif
#define QTC_CHECK_GCC_VERSION(args...)          \
    (QTC_GCC_VERSION >= qtcMakeVersion(args))

/**
 * \brief Export symbol.
 */
#define QTC_EXPORT __attribute__((visibility("default")))

/**
 * \def QTC_BEGIN_DECLS
 * \brief start declaring c-linked functions.
 */
/**
 * \def QTC_END_DECLS
 * \brief end declaring c-linked functions.
 */
// For public c headers
#ifdef __cplusplus
#  define QTC_BEGIN_DECLS extern "C" {
#  define QTC_END_DECLS }
#else
#  define QTC_BEGIN_DECLS
#  define QTC_END_DECLS
#endif

/**
 * \brief always inline the function.
 *
 * Should only be used for small functions
 */
#define QTC_ALWAYS_INLINE __attribute__((always_inline))

/**
 * Suppress unused parameter warning on variable \param x.
 */
#define QTC_UNUSED(x) ((void)(x))

/**
 * cast the \param member pointer \param ptr of a structure \param type to
 * the containing structure.
 */
#define qtcContainerOf(ptr, type, member)               \
    ((type*)(((char*)(ptr)) - offsetof(type, member)))

/**
 * Tell the compiler that \param exp is likely to be \param var.
 */
#if QTC_CHECK_GCC_VERSION(3, 0)
#  define qtcExpect(exp, var) __builtin_expect(exp, var)
#else
#  define qtcExpect(exp, var) (exp)
#endif

/**
 * Tell the compiler that \param x is likely to be true.
 */
#define qtcLikely(x) qtcExpect(!!(x), 1)

/**
 * Tell the compiler that \param x is likely to be false.
 */
#define qtcUnlikely(x) qtcExpect(!!(x), 0)

#endif
