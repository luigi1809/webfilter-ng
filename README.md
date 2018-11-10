# webfilter-ng
Transparent Web Filtering for Linux (standalone computer or router/internet gateway)


What does it do?
--------------------
Check http and https trafic and deny access to unwanted contents based on domain name of http(s) request.

Main advantages of webfilter-ng is
--------------------

Compared to squid + squidGuard :
* Https trafic does not need to be decrypted to be filtered. No need to install root certificates on client computers.
* Filtering is transparent (not possible with squid). No need to set up proxy settings on clients.
* Filtering of http or https TLS trafic does not depends on TCP port (supported in Squid version 4 only). Useful for example for application that use TLS on TCP port 80.

Compared to DNS filtering :
* webfilter-ng ensure that DNS filtering is not bypassed (usage of a non-filtering DNS, local hosts file, usage of DNSSEC or [DnsCrypt](https://github.com/jedisct1/dnscrypt-proxy)) 

Three options of usage
--------------------
* filtering based on squidGuard
* filtering based on public dns filtering solution (recommanded over squidGuard option)
* filtering based on whitelist / blacklist

How does it work
--------------------

Webfilter-ng reads for :
* unencrypted HTTP Host field as well as the URI that is requested (URI is useful for squidGuard and not for solution based on public dns filtering solution)
* for TLS connections including HTTPS, it reads the SNI field in TLS client Hello. All modern browsers sends the SNI extension in the client Hello. Very old browser does not but almost not anymore capable of browsing. New TLS 1.3 is supported as it still haves unencrypted SNI.


SquidGuard filtering
--------------------
It filters access to websites based on a working installation of squidguard. No need to install squid and configure proxy settings on network computer.

Warning : DNS filtering is recommanded as it does not require maintaining update of squidguard lists. Anyway be careful of the quality of the squidguard list you choose.

DNS filtering
--------------------
* Use a DNS filtering service - a good one is [Yandex DNS](https://dns.yandex.com/)
* DNS usually allows the conversion of domain name (i.e. google.fr) to internet IP address (i.e 172.217.18.195)
* In case an unwanted domain is requested, this kind of DNS responds with a fake IP that did not allow access to the domain website.
* webfilter-ng checks that the DNS filtering service is properly used (protects against bypass of the filter : usage of a non-filtering DNS, local hosts file, usage of DNSSEC or [DnsCrypt](https://github.com/jedisct1/dnscrypt-proxy))
* To do so it compares the IP requested for a website against all the possible IPs provided by the DNS filtering service for the website domain
* If they do not match, the request is blocked.

For a router/gateway (best option as it will protect all machines behind the gateway), it is mandatory to use a local (i.e. on the routeur/gateway) dns resolver such as bind9 and distribute dns setting with a dhcp server. webfilter-ng will read the dns cache of bind9, which is more performant. It is also ensure that webfilter-ng reads IPs that was offered to the computer by DNS, which could avoid blocking non-unwanted content by mistake.

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


Requirements
--------------------

#### Debian/Ubuntu

```sudo apt-get install build-essential git dnsutils iptables iptables-persistent netfilter-persistent libnetfilter-queue-dev libnetfilter-queue1 libnfnetlink-dev libnfnetlink0```


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

#### For filtering based on squidGuard
```make squidguard```

```make install```

#### For filtering based on whitelist / blacklist
```make list```

```make install```
Create whitelist or blacklist file in /etc/webfilter-ng/

```vi etc/webfilter-ng/blacklist```

and add one domain per line. Examples :

```
www.facebook.com
*facebook.com
*.facebook.com
```

#### For all options, set up lists

On a router/gateway that filters trafic (LANINTF is the internal interface - eth0...):

```
iptables -A FORWARD -i LANINTF -o LANINTF -p tcp -j ACCEPT
#Google-QUIC protocol support is experimemental. Use the following if you want to test it :
#iptables -A FORWARD -i LANINTF -p udp -m udp -j NFQUEUE --queue-num 200
iptables -A OUTPUT -p udp -m udp --dport 443 -j DROP
iptables -A FORWARD -i LANINTF -p tcp -m tcp -j NFQUEUE --queue-num 200
iptables-save > /etc/iptables/rules.v4
systemctl start netfilter-persistent.service
systemctl enable netfilter-persistent.service
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

Make also sure that you disable IPv6 as it is not yet supported.

Testing
--------------------
It is recommanded to test installation with a command line tool to avoid getting malware or seeing unwanted content :
```curl -v -L http://something.com```

Work inspired by
--------------------
Thank you to the people that worked on the following project :

* https://github.com/farukuzun/notsodeep (dealing with web trafic part)
* https://github.com/Lochnair/xt_tls (reading the TLS header to catch SNI field)
