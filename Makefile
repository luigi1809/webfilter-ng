CC      ?= g++
CFLAGS   = -g -Wall -Wextra -Wformat-security -O3 -fstack-protector-all
LIBS  = -lnetfilter_queue -lnfnetlink
PROGRAM  = webfilter-ng
SOURCE   = webfilter-ng.c

all: webfilter-ng 

webfilter-ng: webfilter-ng.c
	$(CC) $(SOURCE) $(CFLAGS) $(LIBS) -o $(PROGRAM)
	cp -pf webfilter-ng.service /tmp/
squidguard:
	chmod a+x squidGuardWebGuard; rm -f webGuard 2>/dev/null ; cp -pf squidGuardWebGuard webGuard

dns:
	chmod a+x dnsWebGuard; rm -f webGuard 2>/dev/null ; cp -pf dnsWebGuard webGuard

dns_categorify:
	chmod a+x dnsCategorifyWebGuard; rm -f webGuard 2>/dev/null ; cp -pf dnsCategorifyWebGuard webGuard; cp -pf webfilter-ng.service /tmp/ ; sed -e 's/After=/After=redis-server\nAfter=nghttpx\nAfter=/g' -i /tmp/webfilter-ng.service


list:
	chmod a+x listWebGuard; rm -f webGuard 2>/dev/null ; cp -pf listWebGuard webGuard

uninstall:
	systemctl daemon-reload; systemctl stop webfilter-ng; systemctl disable webfilter-ng.service; rm -f /usr/sbin/$(PROGRAM); rm -f /usr/bin/webGuard; rm -f /etc/systemd/system/webfilter-ng.service

install: uninstall
	mkdir /etc/webfilter-ng; cp -pf $(PROGRAM) /usr/sbin/ ; cp -pf /tmp/webfilter-ng.service /etc/systemd/system/ ; systemctl daemon-reload ;mkdir /var/cache/webfilter-ng/ ; (! [ -f webGuard ] ) && cp -pf dnsWebGuard webGuard ; cp -pf webGuard /usr/bin/ ; chmod a+x /usr/bin/webGuard; chmod u+x /usr/sbin/$(PROGRAM) ; systemctl daemon-reload ; systemctl start webfilter-ng; systemctl enable webfilter-ng.service


clean:
	@rm -rf $(PROGRAM); rm -f webGuard
