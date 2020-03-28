#!/bin/bash

systemctl status webfilter-ng.service
categorify=$(grep -Ew categorify.org /etc/hosts | awk '{print $1}')
#Google-QUIC protocol support is experimemental. Use the following if you want to test it :
iptables -A OUTPUT -p udp -m udp --dport 443 -j NFQUEUE --queue-num 200
# iptables -A OUTPUT -p udp -m udp --dport 443 -j DROP
iptables -A OUTPUT -d $categorify -p tcp -m tcp --dport 443 -j ACCEPT
iptables -A OUTPUT -p tcp -j NFQUEUE --queue-num 200
iptables -t nat -A OUTPUT -p udp -m udp --dport 53 -j DNAT --to-destination 127.0.0.1
iptables-save
iptables-save > /etc/iptables/rules.v4
systemctl start netfilter-persistent.service
systemctl enable netfilter-persistent.service
systemctl status netfilter-persistent.service

ip6tables -I FORWARD -i LANINTF -j REJECT
ip6tables-save > /etc/iptables/rules.v6
systemctl restart netfilter-persistent.service
systemctl status netfilter-persistent.service