#!/bin/sh

PATH=.:$PATH
PROJECTDIR=$(dirname $0)/../
arch=$1

PROJECTNAME="upl"

TEMPFILE="/tmp/upl_new.bin"

if [ "$PROJECTDIR" = "" ]; then
	echo "Please run this before make"
	echo ""
	echo ". ./envsetup.sh"
	echo ""
	exit 1
fi

TAR=$PROJECTDIR/dist/$PROJECTNAME.tar.gz
if [ ! -f $TAR ]; then
	echo "Please run 'make dist' firstly to create $TAR!"
	exit 1
fi

POSTINSTALL=$PROJECTDIR/scripts/upl_base.bin
BASE=$(basename $TAR)
sum=`sum $TAR`

index=1
for s in $sum
do
	case $index in
	1) sum1=$s;
		index=2;
		;;
	2) sum2=$s;
		index=3;
		;;
  	esac
done


REVISION_NO=`svn info | grep "Revision" | cut -d: -f2 | sed 's/^ //g'`
MACHINENAME=`uname -m`
if [ "$arch" = "" ]; then
	INSTALLER=upl_${MACHINENAME}_R${REVISION_NO}.bin
else
	INSTALLER=upl_${arch}_R${REVISION_NO}.bin
fi


cat $POSTINSTALL | sed -e s/OUTNAME/$BASE/ -e s/SUM1/$sum1/ -e s/SUM2/$sum2/ > $TEMPFILE
cat $TEMPFILE $TAR > $PROJECTDIR/dist/$INSTALLER
chmod a+x $PROJECTDIR/dist/$INSTALLER
rm -f $TAR
rm -f $TEMPFILE
