les=$(find ./data -type f|sed 's/[.]/\t/g'|awk '{print($2)}'|sort|uniq);mkdir "Nil";
for i in $les;do
    mkdir -p "$i"&&find ./data -name "*.""$i"|xargs mv -t "$i";done
find ./data -type f|xargs mv -t "Nil"&&rm -rf ./data/*