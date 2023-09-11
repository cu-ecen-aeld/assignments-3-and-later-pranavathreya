USAGE_STR="finder-app/writer.sh writefile writestr"

if [ "$1" = "" ]; then
	echo $USAGE_STR
	exit 1
elif [ "$2" = "" ]; then
	echo $USAGE_STR
	exit 1
fi

dir=$1
dir="${dir%/*}"

if ! [ -d $dir ]; then
	echo "Making $dir"
	mkdir -p $dir
fi

echo $2 > $1

