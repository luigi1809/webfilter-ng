# webfilter-ng
Transparent Web Filtering for Linux (standalone computer or router/internet gateway)


What does it do?
--------------------
Filter http(s) web acess based on domain name in the request (basically Host field in http or SNI in https). 

Main advantages of webfilter-ng is
--------------------

Compared to squid + squidGuard :
* Https trafic does not need to be decrypted to be filtered. No need to install root certificates on client computers.
* Filtering is transparent (not possible with squid). No need to set up proxy settings on clients.
* Filtering of http or https TLS trafic does not depends on TCP port (supported in Squid version 4 only). Useful for example for application that use TLS on TCP port 80.

Compared to classic DNS filtering :
* webfilter-ng works well with [modern web browser](https://support.mozilla.org/en-US/kb/firefox-dns-over-https) that integrates encrypted DNS with [DoH](https://en.wikipedia.org/wiki/DNS_over_HTTPS). Classic DNS filtering solution does not work anymore with such new browsers as DNS in operating system global settings is not used for web browsing.
* Also trying to bypass the filtering by implementing other DNS local host files or any kind of encrypted DNS (DoH - DNS over HTTPS, DoT - DNS over TLS...) is not possible with webfilter-ng

Four options of usage
--------------------
* filtering based on squidGuard (deprecated)
* filtering based on public dns filtering solution (recommanded over squidGuard option)
* filtering based on public dns filtering solution + categorify.org API (recommanded over squidGuard option)
* filtering based on whitelist or blacklist

How does it work ?
--------------------

Webfilter-ng reads for :
* unencrypted HTTP Host field as well as the URI that is requested (URI is useful for squidGuard and not for solution based on public dns filtering solution)
* for TLS connections including HTTPS, it reads the SNI field in TLS client Hello. All modern browsers sends the SNI extension in the client Hello. Very old browser does not but they are almost not anymore capable of provide a correct browsing experience. New TLS 1.3 is supported as it still haves unencrypted SNI. Connection is dropped when [SNI is encrypted](https://github.com/luigi1809/webfilter-ng/issues/4) (draft standard).


SquidGuard filtering
--------------------
It filters access to websites based on a working installation of squidguard. No need to install squid and configure proxy settings on network computer.

Warning : DNS filtering is recommanded as it does not require maintaining update of squidguard lists. Anyway be careful of the quality of the squidguard list you choose.

DNS filtering
--------------------
* Use a DNS filtering service - a good one is [Cleanbrowsing DNS](https://cleanbrowsing.org/)
* DNS usually allows the conversion of domain name (i.e. google.fr) to internet IP address (i.e 172.217.18.195)
* In case an unwanted domain is requested, this kind of DNS replies with a fake IP that did notsquidguard allow access to the domain website.
* webfilter-ng checks that the DNS filtering service is properly used (protects against bypass of the filter : usage of a non-filtering DNS, local hosts file, usage encrypted DNS, such as DoT or DoH.
* To do so it compares the IP requested for a website against all the possible IPs provided by the DNS filtering service for the website domain
* If they do not match, the request is blocked.

For a router/gateway (best option as it will protect all machines behind the gateway), it is mandatory to use a local (i.e. on the routeur/gateway) dns resolver such as bind9 and distribute dns setting with a dhcp server. webfilter-ng will read the dns cache of bind9, which is more performant. It is also ensure that webfilter-ng reads IPs that was offered to the computer by DNS, which could avoid blocking non-unwanted content by mistake.

DNS filtering + API categorify.org
--------------------

categorify.org (by cleanbrowsing) is a free API to categorize websites content. It uses a keyword based artificial intelligence algorithm.

This option uses advantages of DNS filtering and categorify.org increases filtering reliability. Since categorify.org seems to limit the number of requests per day, API filtering would not be sufficient.

Current implementation only add reliability to adult content filtering.

Enforce usage of safe search on search engines (Bing, Google)
--------------------
Additionally on a router/gateway, you might want to enforce user to use safe version of search engines, which filter unwanted results and images. 

To do so, it is recommanded to install bind9 rpz zone and distribute dns setting with a dhcp server. See working bind9 configuration in bind directory.

See : https://support.opendns.com/hc/en-us/articles/227986807-How-to-Enforcing-Google-SafeSearch-YouTube-and-Bing
and https://www.cwssoft.com/?p=1577

You can also deny access to search engines that does not offer the possibility to enforce safe search by this method (duckduckgo, yandex...). To do so, add some fake entries to db.rpz :
```duckduckgo.com     A 127.0.0.1```

Whitelist / blacklist filtering
--------------------
* Whitelist : allow access to websites that are on the whitelist. All others are denied.
* Blacklist : deny access to websites that are on the blacklist. All others are permitted.

Requirements
--------------------

#### Debian/Ubuntu

```sudo apt install build-essential git dnsutils iptables iptables-persistent netfilter-persistent libnetfilter-queue-dev libnetfilter-queue1 libnfnetlink-dev libnfnetlink0```

Reply "no" if it asks you to save current ipv4/ipv6 rules.

Installation
--------------------
```
cd /tmp/
git clone https://github.com/luigi1809/webfilter-ng.git
cd webfilter-ng
make
```

#### For filtering based on dns (default)
This option requires that a local dns resolver with cache such as bind9 dns is installed and distributed by dhcp on your local network. Otherwise, if installed on a computer, bind9 should be locally installed and set as the dns resolver.

For a bind9 installation. Make sure it listens on 127.0.0.1

```sudo apt-get install bind9```

and 
```make dns```

#### For filtering based on dns + categorify.org API

/!\ This option is only available for gateway filtering (not standalone linux computer).

Install local dns resolver - see for filtering based on dns

```
sudo apt-get install nghttp2 redis-server jq # debian
sudo apt-get install nghttp2-proxy redis-server jq # ubuntu

cp -p /etc/nghttpx/nghttpx.conf /etc/nghttpx/nghttpx.conf.save

cat>/etc/nghttpx/nghttpx.conf <<\EOF
frontend=127.0.0.1,3000;no-tls
backend=categorify.org,443;;tls
errorlog-syslog=no
backend-keep-alive-timeout=300
insecure=yes
workers=1
EOF

# nghttpx is used to keep an opened HTTP2 session to categorify.org
# it speeds up the queries to the API

systemctl restart nghttpx.service
systemctl enable nghttpx.service

systemctl restart redis-server.service
systemctl enable redis-server.service

make dns_categorify

make install
```

#### For filtering based on squidGuard
```make squidguard```

```make install```

#### For filtering based on whitelist / blacklist
```make list```

```make install```

Create whitelist or blacklist file in /etc/webfilter-ng/ . Note that whitelist and blacklist files cannot be used at the same time.

```/etc/webfilter-ng/blacklist```
or
```/etc/webfilter-ng/whitelist```

and add one domain per line. Do not add url such https://domain.com or https://domain.com/page.html. Examples :

```
www.facebook.com
*facebook.com
*.facebook.com
```

#### For all options, set up lists

On a router/gateway that filters trafic (LANINTF is the internal interface - eth0...):

```
iptables -A FORWARD -i LANINTF -o LANINTF -p tcp -j ACCEPT
#Google-QUIC protocol is a work-in-progress protocol that is used by Google for their websites. 
#It is currently recognized by webfilter-ng but future evolution of the protocol might break the support.
#IETF standard QUIC protocol, which is different from Google-QUIC, is currently not filtered.
#Use the following if you want to test support for Google-QUIC and leave IETF-QUIC unfiltered :
#iptables -A FORWARD -i LANINTF -p udp -m udp -j NFQUEUE --queue-num 200
iptables -A OUTPUT -p udp -m udp --dport 443 -j DROP
iptables -A FORWARD -i LANINTF -p tcp -m tcp -j NFQUEUE --queue-num 200
iptables-save > /etc/iptables/rules.v4
systemctl start netfilter-persistent.service
systemctl enable netfilter-persistent.service
```

Add the following if you use dns based filtering to redirect all dns requests to you router dns resolver. IP_OF_LANINTF is the IP of LANINTF.
Otherwise, usage of specific dns settings (not learnt by DHCP) on client would drop majority of traffic. 
```
iptables -t nat -A PREROUTING -i LANINTF -p udp -m udp --dport 53 -j DNAT --to-destination IP_OF_LANINTF
iptables-save > /etc/iptables/rules.v4
systemctl restart netfilter-persistent.service
```

On a linux computer (not a router/gateway)  :

```
#Google-QUIC protocol support is experimemental. Use the following if you want to test it :
#iptables -A OUTPUT -p udp -m udp --dport 443 -j NFQUEUE --queue-num 200
iptables -A OUTPUT -p udp -m udp --dport 443 -j DROP
iptables -A OUTPUT -p tcp -j NFQUEUE --queue-num 200
iptables-save > /etc/iptables/rules.v4
systemctl start netfilter-persistent.service
systemctl enable netfilter-persistent.service
```

On a linux computer or router, make sure to disable IPv6 as it is not yet supported.

```
ip6tables -I FORWARD -i LANINTF -j REJECT
ip6tables-save > /etc/iptables/rules.v6
systemctl restart netfilter-persistent.service
```

Logs
--------------------
Logs are available at :
```/var/log/webfilter-ng```

Testing
--------------------
It is recommanded to test installation with a command line tool to avoid getting malware or seeing unwanted content :
```curl -v -L http://something.com```

Work inspired by
--------------------
Thank you to the people that worked on the following project :

* https://github.com/farukuzun/notsodeep (dealing with web trafic part)
* https://github.com/Lochnair/xt_tls (reading the TLS header to catch SNI field)
