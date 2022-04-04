awk -vcol=$2 '{print(tolower($col))}'<$1|sort|uniq -c|awk '{print($2,$1)}'|sort -rk2 >"1f_output_"$2"_column.freq"
