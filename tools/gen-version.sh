#!/bin/bash

# call with
# $ gen-version.sh <output file> <orig version> <git executable>

OUTPUT=$1
ORIG_VERSION=$2
GIT=$3

print_file() {
    cat <<EOF
#include <qtcurve-utils/utils.h>
QTC_EXPORT const char*
qtcVersion()
{
    return "$1";
}
EOF
}

if [[ -z $GIT ]]; then
    GIT=$(which git 2> /dev/null)
fi

VERSION=$("$GIT" describe 2> /dev/null)

if [[ -z $VERSION ]]; then
    VERSION=$ORIG_VERSION
else
    echo "QTCURVE_VERSION_FULL=${VERSION}"
fi

print_file "$VERSION" > "$OUTPUT".$$

if [[ -f "$OUTPUT" ]] && diff -q "$OUTPUT" "$OUTPUT".$$ > /dev/null; then
    rm "$OUTPUT".$$
else
    mv "$OUTPUT".$$ "$OUTPUT"
fi
