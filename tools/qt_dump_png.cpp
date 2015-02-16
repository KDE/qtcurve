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

#include "stdio.h"

int
main(int argc, char **argv)
{
    if (argc < 4)
        return 1;
    const char *filename = argv[1];
    const char *varname = argv[2];
    const char *outputname = argv[3];
    FILE *outputfile = fopen(outputname, "w");
    fprintf(outputfile, "#ifndef __QTC_IMAGE_HDR_%s__\n"
            "#define __QTC_IMAGE_HDR_%s__\n", varname, varname);
    fprintf(outputfile,
            "static constexpr unsigned char _%s_data[] = {", varname);
    int size = 0;
    unsigned char buff;
    FILE *inputfile = fopen(filename, "r");
    while (fread(&buff, 1, 1, inputfile)) {
        fprintf(outputfile, "%u,", (unsigned)buff);
        size++;
    }
    fclose(inputfile);
    fprintf(outputfile, "};\n"
            "static const QImage %s __attribute__((unused)) = "
            "QImage::fromData(_%s_data, %d);\n"
            "#endif\n", varname, varname, size);
    fclose(outputfile);
    return 0;
}
