#!/bin/bash
google=$(cat /tmp/google)

# test with forcing IP HTTP
echo test with forcing IP HTTP
[ "$(curl --max-time 5 -s -o /dev/null -w "%{http_code}" --resolve www.google.com:80:$google http://www.google.com/)" == "200" ] && exit 1
grep "http://www.google.com" /var/log/webfilter-ng | tail -n1 | grep DROP || exit 1

sleep 5

# test with forcing IP HTTPS
echo test with forcing IP HTTPS
[ "$(curl --max-time 5 -s -o /dev/null -w "%{http_code}" --resolve www.google.com:443:$google https://www.google.com/)" == "200" ] && exit 1
grep https://www.google.com /var/log/webfilter-ng | tail -n1 | grep DROP || exit 1

sleep 5

# test without forcing IP HTTP
echo test without forcing IP HTTP
[ "$(curl --max-time 5 -s -o /dev/null -w "%{http_code}" http://www.google.com/)" == "200" ] || exit 1
grep http://www.google.com /var/log/webfilter-ng | tail -n1 | grep ACCEPT || exit 1

sleep 5


# test without forcing IP HTTPS
echo test without forcing IP HTTPS
[ "$(curl --max-time 5 -s -o /dev/null -w "%{http_code}" https://www.google.com/)" == "200" ] || exit 1
grep https://www.google.com /var/log/webfilter-ng | tail -n1 | grep ACCEPT || exit 1
