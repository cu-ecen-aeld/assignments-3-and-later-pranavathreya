export USAGE_STR="usage: finder-app/finder.sh filesdir searchstr"

if [ "$1" = "" ]; then
	echo "$USAGE_STR"
	exit 1

elif [ "$2" = "" ]; then
	echo $USAGE_STR
	exit 1

elif ! [ -d $1 ]; then
	echo "$1 is not a valid directory"
	exit 1
fi

FILE_CNT=$(ls $1 | wc -l)
LINE_CNT=$(grep -ro $2 $1 | wc -l) 

echo "The number of files are $FILE_CNT and the number of matching lines are $LINE_CNT"

