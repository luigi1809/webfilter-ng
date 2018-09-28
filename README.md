# webfilter-ng
Web Filtering for Linux (standalone computer or router/internet gateway)


What does it do?
--------------------
Check http and https trafic and deny access to unwanted contents.
Two options :
* filtering based on squidGuard
* filtering based on dns (recommanded)

SquidGuard filtering
--------------------
It filters access to websites based on a working installation of squidguard.

Warning : DNS filtering is recommanded as it does not require maintaining update of squidguard lists. Anyway be careful of the quality of the squidguard list you choose.

DNS filtering
--------------------
* Use a DNS filtering service - a good one is [Yandex DNS](https://dns.yandex.com/)
* DNS usually allows the conversion of domain name (i.e. google.fr) to internet IP address (i.e 172.217.18.195)
* In case an unwanted domain is requested, this kind of DNS responds with a fake IP that did not allow access to the domain website.
* webfilter-ng checks that the DNS filtering service is properly used (protects against bypass of the filter : other DNS usage, local hosts file, usage of DNSSEC or [DnsCrypt](https://github.com/jedisct1/dnscrypt-proxy)) 
* To do so it compares the IP requested for a website against all the possible IPs provided by the DNS filtering service for the website domain
* If they do not match, the request is blocked.

For a router/gateway (best option as it will protect all machines behind the gateway), it is recommanded to use bind9 and distribute dns setting with a dhcp server. webfilter-ng will read the dns cache of bind9, which is more performant. It is also ensure that webfilter-ng reads IPs that was offered to the computer by DNS, which could avoid blocking non-unwanted content by mistake.

Enforce usage of safe search
--------------------
Additionally on a router/gateway, you might want to enforce user to use safe version of search engines, which filter unwanted results and images.

To do so, it is recommanded to install bind9 rpz zone and distribute dns setting with a dhcp server. See working bind9 configuration in bind directory.

See : https://support.opendns.com/hc/en-us/articles/227986807-How-to-Enforcing-Google-SafeSearch-YouTube-and-Bing
and https://www.cwssoft.com/?p=1577

Requirements
--------------------

#### Debian/Ubuntu

```sudo apt-get install build-essential git dnsutils iptables iptables-persistent netfilter-persistent libnetfilter-queue-dev libnetfilter-queue1 libnfnetlink-dev libnfnetlink0```


Installation
--------------------
```cd /tmp/
git clone https://github.com/luigi1809/webfilter-ng.git
cd webfilter-ng
make
```

#### For filtering based on dns (default)
Change IP of DNS from default IP 192.168.2.1 to your setup DNS YOURDNSIP IP (replace YOURDNSIP).
If bind9 dns is locally, put the IP of your internal LAN interface or else put the IP of the filtering DNS you use.

```sed -e 's/192.168.2.1/YOURDNSIP/g' -i dnsWebGuard```

and 
```make dns```

#### For filtering based on squidGuard
```make squidguard```

```make install```

On a router/gateway that filters trafic (LANINTF is the internal interface - eth0...):

```iptables -A FORWARD -i LANINTF -o LANINTF -p tcp -j ACCEPT
iptables -A FORWARD -i LANINTF -p tcp -m tcp -j NFQUEUE --queue-num 200
systemctl start netfilter-persistent.service
systemctl enable netfilter-persistent.service
```

On a linux computer (not a router/gateway)  :

```iptables -A OUTPUT -p tcp -j NFQUEUE --queue-num 200
iptables-save > /etc/iptables/rules.v4/etc/iptables/rules.v4
systemctl start netfilter-persistent.service
systemctl enable netfilter-persistent.service
```

Testing
--------------------
It is recommanded to test installation with a command line tool to avoid getting malware or seeing unwanted content :
```curl -v -L http://something.com```

Work inspired by
--------------------
Thank you to the people that worked on the following project.

https://github.com/farukuzun/notsodeep (dealing with web trafic part)
https://github.com/Lochnair/xt_tls (reading the TLS header to catch SNI field)
