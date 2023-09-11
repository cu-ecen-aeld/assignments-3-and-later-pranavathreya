USAGE_STR="finder-app/writer.sh writefile writestr"

if [ "$1" = "" ]; then
	echo $USAGE_STR
	exit 1
elif [ "$2" = "" ]; then
	echo $USAGE_STR
	exit 1
fi

if ! [ -d /tmp/aeld-data ]; then
	mkdir -p /tmp/aeld-data
fi

echo $2 > $1

