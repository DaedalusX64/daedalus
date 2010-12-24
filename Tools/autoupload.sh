#! /bin/sh

###############################
#script to upload to an ftp server
#by default I use mine, you can modify it to point to your ftp server
#I'm using lftp as it has the mirror functionnality that the basic ftp utility
#does not seem to include
#
# Has to be run from the trunk directory
#
#Parameter : ftp server
#
#use the ~/.netrc file (for ftp or lftp) to deal with auto-authentication
###############################

WAIT=1h
BUILD_DIRS=./tarballs

function upload {
lftp $1 <<EOF
mirror -R -n $BUILD_DIRS DaedalusX64
quit
EOF
}

if [ $# != 1 ]; then
	echo "you must give the ftp server name" 1>&2
	exit 1
fi

LAST_REV=1 #initialize at first rev

export LC_ALL=C #so that messages for svn info are the svn, independantly
		#from the user's language
while :; do
	svn up || continue
	export LC_ALL=C
	CUR_REV=`svn info | grep Revision | grep -e [0-9]* -o | tr -d '\n'`
	if [ $CUR_REV -gt $LAST_REV ]; then
		LAST_REV=$CUR_REV
		rm Source/SysPSP/UI/AboutComponent.o 2> /dev/null #so About Screen has the actual rev number
		rm PARAM.SFO 2> /dev/null # refresh eboot title
		make zip >/dev/null
		upload $1
		echo "New Revision uploaded."
	fi

	sleep $WAIT
done
