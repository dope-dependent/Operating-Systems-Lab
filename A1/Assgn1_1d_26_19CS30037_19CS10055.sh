mkdir ./files_mod
for file in ./temp/*;do
nl -n'ln' "./temp/""${file##*/}"|sed 's/[^0-9a-zA-Z\n][^0-9a-zA-Z\n]*/,/g'>"./files_mod/""${file##*/}";done