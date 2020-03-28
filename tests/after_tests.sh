#!/bin/bash
google=$(cat /tmp/google)

# test with forcing IP HTTP
[ "$(curl --max-time 5 -s -o /dev/null -w "%{http_code}" http://$google/ -H "Host: www.google.com")" == "200" ] && exit 1
grep google.com /var/log/webfilter-ng | tail -n1 | grep DROP || exit 1


# test with forcing IP HTTPS
[ "$(curl -k --max-time 5 -s -o /dev/null -w "%{http_code}" https://$google/ -H "Host: www.google.com")" == "200" ] && exit 1
grep google.com /var/log/webfilter-ng | tail -n1 | grep DROP || exit 1


# test without forcing IP HTTP
[ "$(curl --max-time 5 -s -o /dev/null -w "%{http_code}" http://www.google.com/)" == "200" ] || exit 1
grep google.com /var/log/webfilter-ng | tail -n1 | grep ACCEPT || exit 1


# test without forcing IP HTTPS
[ "$(curl --max-time 5 -s -o /dev/null -w "%{http_code}" https://www.google.com/)" == "200" ] || exit 1
grep google.com /var/log/webfilter-ng | tail -n1 | grep ACCEPT || exit 1
