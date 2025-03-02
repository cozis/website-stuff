**NOTE: This post is still a work in progress!!**

## Level 5

This level is where all application-specific protocols are implemented. For web applications, the protocol of choice is HTTP and is usually implemented on top of TCP. This series will revolve arount this layer, so I'll avoid going in too much detail for now.

<br/>
<br/>

The common use case for HTTP is to give the ability to hosts on a network to download resources, such as files, from some other host on the network. The host downloading the resource is referred to as client, while the one providing the resource is called server. The message sent by the client is called request message, while the one sent by the server is called response message. The protocol evolved quite a bit over the years, so this can be considered a quite simplistic description. Nontheless it's a starting point.

<br/>
<br/>

As an example, this is a request I made to my website using curl:
```
GET / HTTP/1.1\r\n
Host: coz.is\r\n
User-Agent: curl/7.81.0\r\n
Accept: */*\r\n
\r\n
```
and this is the response I got:
```
HTTP/1.1 200 OK\r\n
Content-Type: text/html\r\n
Connection: Keep-Alive\r\n
Content-Length: 2438\r\n
\r\n
<html>
	... here comes the actual page ...
</html>
```
Unlike the other protocols we talked about, HTTP is text-based. The `\r\n` at the end of each line represent carriage return and newline characters. We'll go into HTTP's syntax in a future post.

TODO: Mention HTTP/2 and HTTP/3

<br/>
<br/>

In this day and age, it is important to secure communication between applications. It would be otherwise very easy for a malicious actor to steal sensitive information that is sent over the network or other shenanigans. To avoid this, we must use a protocol for encryption. The most common one for the web is TLS, which sits right on top of TCP. When HTTP is build on TLS, we call it HTTPS.

<br/>
<br/>

TODO: Talk about DNS