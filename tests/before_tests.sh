#!/bin/bash
dig +noall +answer www.google.com | awk '($4=="A") {print $5}' >/tmp/google
google=$(cat /tmp/google)

echo DEB $google

# test with forcing IP HTTP
curl --max-time 5 -s -o /dev/null -w "%{http_code}" http://$google/ -H "Host: www.google.com"
curl http://$google/ -H "Host: www.google.com"
echo test with forcing IP HTTP
[ "$(curl --max-time 5 -s -o /dev/null -w "%{http_code}" http://$google/ -H "Host: www.google.com")" == "200" ] || exit 1

# test with forcing IP HTTPS
echo test with forcing IP HTTPS
[ "$(curl -k --max-time 5 -s -o /dev/null -w "%{http_code}" https://$google/ -H "Host: www.google.com")" == "200" ] || exit 1

# test without forcing IP HTTP
echo test without forcing IP HTTP
[ "$(curl --max-time 5 -s -o /dev/null -w "%{http_code}" http://www.google.com/)" == "200" ] || exit 1

# test without forcing IP HTTPS
echo test without forcing IP HTTPS
[ "$(curl --max-time 5 -s -o /dev/null -w "%{http_code}" https://www.google.com/)" == "200" ] || exit 1
