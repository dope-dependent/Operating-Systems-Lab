ip=$1
for((factor=2;factor<=$ip;factor++));do
    while((!$(($ip%$factor))));do
    printf $factor"%b"'\40'&&ip=$(($ip/$factor));done;done;echo