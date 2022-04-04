mkdir ./1.b.files.out
for file in ./1.b.files/*;do
sort -n "./1.b.files/""${file##*/}">"./1.b.files.out/""${file##*/}";done
cat "./1.b.files.out"/*|sort -n|uniq -c|awk '{print($2,$1)}'>./1.b.out.txt