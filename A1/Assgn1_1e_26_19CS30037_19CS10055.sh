export REQ_HEADERS="Host,User-Agent"
curl "https://www.example.com/">"example.html"&&curl -i $2 "http://ip.jsontest.com"
rm -f *.txt .tc.json

arrReq=(${REQ_HEADERS//,/ })
for i in ${arrReq[@]};do
curl -q $2 http://headers.jsontest.com|jq --arg keyvar "$i" '.[$keyvar]';done;

for file in "$1"/*;do
curl $2 -sd "json=`cat $file`" -X POST http://validate.jsontest.com |jq '.validate'|grep -q "false" && echo $file >>invalid.txt||echo $file >>valid.txt
done;