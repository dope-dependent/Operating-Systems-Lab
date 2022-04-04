for i in {1..100};do
echo $(shuf --input-range=1-2832938 --head-count=10)|sed 's/[^0-9\n]/,/g';done >"$1"
awk -F, -vcol=$2 '{print($col)}' $1|grep $3&&echo YES||echo NO

