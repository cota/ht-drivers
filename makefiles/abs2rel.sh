#!/bin/bash

###############################################################################
# @file  abs2rel
#
# @brief Converts absolute pathes into relative one.
#
# @author Copyright (C) 2009 - 2010 CERN. Yury GEORGIEVSKIY <ygeorgie@cern.ch>
#
# Adapted from heindsight @
# http://ubuntuforums.org/attachment.php?attachmentid=107617&d=1238012714
# Thnxs a'lot!
#
# Script takes 2 parameters and creates relative path from $2 to $1.
# @b NOTE on parameters: If parameter is a directory - it should ends with
# slash ( / ). Otherwise it's considered by the script as a filename.
# Take care not to be confused with this.
#
# @date Created on 20/04/2009
###############################################################################

#DEBUGING:
#set -x

# Convert single argument into an absolute path
abspath() {
    local p tmp tmp1 tmp2;

    if [ "x${1#/}" != "x$1" ]; then
        tmp="$1/"
    else
        tmp="$PWD/$1/"
    fi

    while [ "x$tmp" != "x/" ]; do
        tmp1="${tmp#/*/}"
        tmp2="${tmp%"$tmp1"}"
        if [ "x$tmp2" = "x/../" ]; then
            p="${p:+${p%/*/}/}"
        elif [ "x$tmp2" != "x//" -a "x$tmp2" != "x/./" ]; then
            p="${p%/}$tmp2"
        fi
        tmp="/$tmp1"
    done
    if [ "x${1%/}" = "x$1" ]; then
        p="${p%/}"
    fi
    echo "${p:-/}"
}

# Compute longest common prefix of two paths
common_prefix() {
    local pre1 pre2 p1 p2 tmp;

    pre1=""
    pre2=""

    p1="$1"
    p2="$2"

    while [ "x$pre1" = "x$pre2" -a -n "$p1" -a -n "$p2" ]; do
        tmp="${p1#*/}"
        pre1="$pre1${p1%"$tmp"}"
        p1="$tmp"

        tmp="${p2#*/}"
        pre2="$pre2${p2%"$tmp"}"
        p2="$tmp"
    done
    if [ "x$pre1" != "x$pre2" ]; then
        pre1="${pre1%/*/}"
    fi
    if [ "x${pre1%/}" = "x$pre1" ]; then
        pre1="$pre1/"
    fi
    echo ${pre1:-/}
}

# Construct a relative path from $2 to $1
make_relpath() {
    local b1 p1 p2 tmp prefix

    tmp=$(abspath "$1")
    p1="${tmp%/*}/"
    b1="${tmp#"$p1"}"

    tmp=$(abspath "$2")
    if [ "x${tmp%/}" != "x$tmp" ]; then
        p2="${tmp%/*}/"
    else
        p2="$tmp"
    fi

    prefix=$(common_prefix "$p1" "$p2")

    p1="${p1#"$prefix"}"
    p2="${p2#"$prefix"}"

    while [ "x$p2" != "x${p2#*/}" ]; do
        p2="${p2#*/}"
        p1="../$p1"
    done
    echo $p1$b1
}

RESULT=$(make_relpath "$1" "$2")

echo "$RESULT"
