#!/bin/bash

cd "`dirname \"$0\"`"

. ../../repo-scripts/mkdist_funcs.sh

setup_hg
get_version_hg
check_mod_hg
build_all
[ -z "${NOBUILD}" ] && { make -C doc clean ; make -C doc all ; }
get_sources_hg
make_tmpdir
copy_sources ${SOURCES} ${GENSOURCES} ${AUXSOURCES}
copy_dist_files
copy_files doc/API
create_configure

if [[ "${VERSION}" =~ [0-9]{8} ]] ; then
	VERSION_BIN=1
else
	VERSION_BIN="$(printf "0x%02x%02x%02x" $(echo ${VERSION} | tr '.' ' '))"
fi

sed -i "s/<VERSION>/${VERSION}/g" `find ${TOPDIR} -type f`
sed -i "/#define T3_WIDGET_VERSION/c #define T3_WIDGET_VERSION ${VERSION_BIN}" ${TOPDIR}/src/main.h

OBJECTS="`echo \"${SOURCES} ${GENSOURCES} ${AUXSOURCES}\" | tr ' ' '\n' | sed -r 's%\.objects/%%' | egrep '^src/.*\.cc$' | sed -r 's/\.cc\>/.lo/g' | tr '\n' ' '`"

#FIXME: somehow verify binary compatibility, and print an error if not compatible
VERSIONINFO="0:0:0"

sed -r -i "s%<OBJECTS>%${OBJECTS}%g;
s%<VERSIONINFO>%${VERSIONINFO}%g" ${TOPDIR}/Makefile.in

create_tar
