Nordic Thingy-91
================

HTTPS Sample
------------

### Connecting to dual stack server from IPv6

Use the HTTPS sample from Nordic, with some changes:

* Change to use `ipv6-test.com`, with a `GET` request to the IP address API
* Use `ipv6-test.com` certificate
* Output the full response (to see the returned the client address)
* Add Packet Data Network (PDN) configuration for IPv6
* Use AF_INET6 hint for getaddrinfo()
* Use AF_INET6 for creating the socket

Also added some additional debug output:

* Add delay countime timer at start (to attach serial listener)
* Add handler for PDN events
* Output default APN
* Ouput retrieved address


```
Connected via Serial Port with settings /dev/ttyACM0 115200 8n1 rtscts:off

1
2
3
Certificate match
Waiting for network.. Event: PDP context 0 activated
OK
Default APN is telstra.iot
Event: PDP context 0 IPv6 up
Host v4v6.ipv6-test.com address is 2001:41d0:701:1100::29c8
Connecting to v4v6.ipv6-test.com
Sent 79 bytes
Received 245 bytes

> HTTP/1.1 200 OK
Date: Sun, 26 Feb 2023 06:43:31 GMT
Server: Apache/2.4.25 (Debian)
Vary: Accept-Encoding
Connection: close
Transfer-Encoding: chunked
Content-Type: text/html; charset=UTF-8

24
2001:8004:4810:1:3e00:bbed:37fc:7d1c
0

Finished, closing socket.
Event: PDP context 0 deactivated
```

### Connecting to IPv4 only server (via NAT64)

NAT64 / DNS64 allows an IPv6 only connection to connection to IPv4 only hosts.

Note that if using IPv4 you probably are going through carrier grade NAT44 anyway.

```
Connected via Serial Port with settings /dev/ttyACM0 115200 8n1 rtscts:off

1
2
3
Certificate match
Waiting for network.. Event: PDP context 0 activated
OK
Default APN is telstra.iot
Event: PDP context 0 IPv6 up
Host v4.ipv6-test.com address is 2001:8004:11d0:4e2a::334b:4e67
Connecting to v4.ipv6-test.com
Sent 77 bytes
Received 220 bytes

> HTTP/1.1 200 OK
Date: Sun, 26 Feb 2023 06:51:59 GMT
Server: Apache/2.4.25 (Debian)
Vary: Accept-Encoding
Connection: close
Transfer-Encoding: chunked
Content-Type: text/html; charset=UTF-8

c
1.124.26.221
0

Finished, closing socket.
Event: PDP context 0 deactivated
```

Note that the NAT64 returned address `2001:8004:11d0:4e2a::334b:4e67` is using a non-standard prefix
(the standard NAT64 address would be `64:ff9b::334b:4e67`; that last four bytes still encode the IPv4 address).

Note that the address seen by the IPv4 server is the IPv4 NAT address (the same one see in an IPv4 only
connection).


Resources
---------

Getting started with Thingy:91

https://developer.nordicsemi.com/nRF_Connect_SDK/doc/2.2.0/nrf/ug_thingy91_gsg.html


Developing with Thingy:91

https://developer.nordicsemi.com/nRF_Connect_SDK/doc/2.2.0/nrf/ug_thingy91.html


Academy Courses

https://academy.nordicsemi.com/courses/nrf-connect-sdk-fundamentals/
https://academy.nordicsemi.com/courses/cellular-iot-fundamentals/


nRF9160 samples

https://developer.nordicsemi.com/nRF_Connect_SDK/doc/2.2.0/nrf/samples/samples_nrf9160.html


Nordic Zephyr - getting started

https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/zephyr/develop/getting_started/index.html


Zephyr - Introduction

https://docs.zephyrproject.org/latest/introduction/index.html


