#!/bin/bash

cp -pf bind/* /etc/bind/

systemctl restart bind9.service
systemctl enable bind9.service
systemctl status bind9.service

categorify=$(dig +noall +answer categorify.org | awk '($4=="A") {print $5}')
printf "$categorify\tcategorify.org\n">>/etc/hosts

cp -p /etc/nghttpx/nghttpx.conf /etc/nghttpx/nghttpx.conf.save

cat>/etc/nghttpx/nghttpx.conf <<\EOF
frontend=127.0.0.1,3000;no-tls
backend=categorify.org,443;;tls
errorlog-syslog=no
backend-keep-alive-timeout=300
#insecure=yes
workers=1
EOF

[ "$DISTRIB" == "xenial" ] && cat>/etc/nghttpx/nghttpx.conf <<\EOF
frontend=127.0.0.1,3000
backend=categorify.org,443
frontend-no-tls=yes
backend-no-tls=no
errorlog-syslog=yes
#insecure=yes
backend-keep-alive-timeout=300
http2-bridge=yes
workers=1
EOF

# nghttpx is used to keep an opened HTTP2 session to categorify.org
# it speeds up the queries to the API

systemctl restart nghttpx.service
systemctl enable nghttpx.service
systemctl status nghttpx.service

PASSWORD=$(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w 30 | head -n 1)
mkdir /etc/webfilter-ng
echo $PASSWORD>/etc/webfilter-ng/redis-password
chmod 600 /etc/webfilter-ng/redis-password

systemctl restart redis-server.service
systemctl enable redis-server.service
systemctl status redis-server.service

redis-cli CONFIG SET requirepass $PASSWORD
