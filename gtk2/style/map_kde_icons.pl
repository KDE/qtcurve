#!/usr/bin/env perl
#   Copyright 2007 - 2010 Craig Drummond <craig.p.drummond@gmail.com>
#   Copyright 2013 - 2015 Yichao Yu <yyc1992@gmail.com>
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU Lesser General Public License as
#   published by the Free Software Foundation; either version 2.1 of the
#   License, or (at your option) version 3, or any later version accepted
#   by the membership of KDE e.V. (or its successor approved by the
#   membership of KDE e.V.), which shall act as a proxy defined in
#   Section 6 of version 3 of the license.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#   Lesser General Public License for more details.
#
#   You should have received a copy of the GNU Lesser General Public
#   License along with this library. If not,
#   see <http://www.gnu.org/licenses/>.

# This perl script is called by the QtCurve Gtk2 theme, it is not intended
# to be useful by itself.
#
# Usage perl map_kde_icons.pl <icon map file> <kde prefix> <kde version> <small toolbar size> <toolbar size> <dnd size> <btn size> <menu size> <dialog size> <icon theme name> <icons map file version>
# @iconSizes = (22, 32, 22, 16, 16, 48);
# KDE Uses 32x32 for dialogs, and 16x16 for buttons
@iconSizes = ($ARGV[3], $ARGV[4], $ARGV[5], $ARGV[6], $ARGV[7], $ARGV[8]);
@gtk = ("gtk-small-toolbar", "gtk-large-toolbar", "gtk-dnd", "gtk-button",
        "gtk-menu", "gtk-dialog" );
$numSizes = $#iconSizes + 1;
$useCustom = 0;

printf "#%s %s %02X%02X%02X%02X%02X%02X%02X\n", $ARGV[10], $ARGV[9], $ARGV[2], $ARGV[3], $ARGV[4], $ARGV[5], $ARGV[6], $ARGV[7], $ARGV[8];
printf "#This file is created, and used by, QtCurve. Alterations may be overwritten.\n";
print "gtk-icon-sizes=\"gtk-small-toolbar=$ARGV[3],$ARGV[3]:gtk-large-toolbar=$ARGV[4],$ARGV[4]:";
print "gtk-dnd=$ARGV[5],$ARGV[5]:gtk-button=$ARGV[6],$ARGV[6]:gtk-menu=$ARGV[7],$ARGV[7]:gtk-dialog=$ARGV[8],$ARGV[8]\"\n";
#print "gtk-dnd=$ARGV[5],$ARGV[5]:gtk-button=$ARGV[6],$ARGV[6]:gtk-menu=$ARGV[7],$ARGV[7]\"\n";

if ($ARGV[1])
{
    $baseDefault=$ARGV[1];
    if($ARGV[9] ne "XX")
    {
        $useCustom=1;
        $baseCustom=join("", $baseDefault, "/");
        $baseCustom=join("", $baseCustom, "$ARGV[9]");
        $baseCustom=join("", $baseCustom, "/");
    }
}

if($ARGV[2] == "3")
{
    $baseDefault=join("", $baseDefault, "/crystalsvg/");
}
else
{
    $baseDefault=join("", $baseDefault, "/oxygen/");
}

open(icons, "$ARGV[0]") || die "Could not open \"$ARGV[0]\"\n";

sub checkSize
{
    $fname=join("", $_[0], $_[1], "x", $_[1], "/", $_[2]);
    if (open(tst, $fname))
    {
        close(tst);
        $fname;
    }
    else
    {
        "";
    }
}

sub printSize
{
    if($_[0])
    {
        print "\t\t{ \"$_[1]x$_[1]/$_[2]\", *, *, \"$_[3]\" },\n";
        $_[4]--;
    }
}

print "\nstyle \"KDE$ARGV[2]-icons\"\n{\n";

while($entry=<icons>)
{
    @iconMap=split(/ /, $entry);

    for($i=0; $i<$#iconMap; $i++)
    {
        $iconMap[$i+1]=~s/\n//;
    }

    if($iconMap[1] == /WHAT/)
    {
        $got=0;
        $use=0;
        if($useCustom)
        {
            for($i=0; $i<$#iconMap && $use == 0; $i++)
            {
                for($index=0; $index<$numSizes; $index++)
                {
                    $files[$index]=checkSize($baseCustom, $iconSizes[$index], $iconMap[$i+1]);
                    if($files[$index])
                    {
                        $got++;
                    }
                }

                if($got)
                {
                    $use=$i+1;
                }
            }
        }

        if($got == 0)
        {
            for($i=0; $i<$#iconMap && $use == 0; $i++)
            {
                for($index=0; $index<$numSizes; $index++)
                {
                    $files[$index]=checkSize($baseDefault, $iconSizes[$index], $iconMap[$i+1]);
                    if($files[$index])
                    {
                        $got++;
                    }
                }

                if($got)
                {
                    $use=$i+1;
                }
            }
        }

        if($got)
        {
            print "\tstock[\"$iconMap[0]\"]={\n";
            if($got > 1)
            {
                for($index=0; $index<$numSizes; $index++)
                {
                    $got=printSize($files[$index], $iconSizes[$index], $iconMap[$use], $gtk[$index], $got);
                }
            }
            $found=0;
            for($index=0; $index<$numSizes && !$found; $index++)
            {
                if($files[$index])
                {
                    print "\t\t{ \"$iconSizes[$index]x$iconSizes[$index]/$iconMap[$use]\" }\n";
                    $found=1;
                }
            }
            print "\t}\n";
        }
        #else
        #{
        #    print "#\tstock[\"$iconMap[0]\"]=<No matching KDE icon>\n";
        #}
    }
}
print "}\nclass \"*\" style \"KDE$ARGV[2]-icons\"\n";

close(icons);
