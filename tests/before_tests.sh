#!/bin/bash
dig +noall +answer www.google.com | awk '($4=="A") {print $5}' | head -n1 >/tmp/google
google=$(cat /tmp/google)

# test with forcing IP HTTP
echo test with forcing IP HTTP
[ "$(curl --max-time 5 -s -o /dev/null -w "%{http_code}" --resolve www.google.com:80:$google http://www.google.com/)" == "200" ] || exit 1

# test with forcing IP HTTPS
echo test with forcing IP HTTPS
[ "$(curl --max-time 5 -s -o /dev/null -w "%{http_code}" --resolve www.google.com:443:$google https://www.google.com/)" == "200" ] || exit 1

# test without forcing IP HTTP
echo test without forcing IP HTTP
[ "$(curl --max-time 5 -s -o /dev/null -w "%{http_code}" http://www.google.com/)" == "200" ] || exit 1

# test without forcing IP HTTPS
echo test without forcing IP HTTPS
[ "$(curl --max-time 5 -s -o /dev/null -w "%{http_code}" https://www.google.com/)" == "200" ] || exit 1
