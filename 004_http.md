# Building web apps from scratch - HTTP - Part 4

With the last post we finished covering how the OS offers userspace applications the abstraction of safe channels for the transmission of bytes. Applications use this abstraction to implement their own use case-specific protocols. One of these is HTTP.

HTTP is probably the most used L5 protocol given how it powers the web. For this reason it has evolved a lot over time. The first version was developed at CERN and extremely simple. Later on we standardized it and called it 0.9, even though it's not really used in practice. The first widely used HTTP version was 1.0, which then was extended making version 1.1. There are also HTTP version 2 and 3 which work very differently from 1.x.

In this post I'd like to talk about HTTP 1.0, then we may extend our server to support 1.1 in the future. We won't go too deep into it since we'll have ample opportunities to talk specifics while implementing the protocol. For now I just wanted to give a feel of how the protocol looks like and behaves. Note that the HTTP specification can be found at [https://www.rfc-editor.org/rfc/rfc1945.html](https://www.rfc-editor.org/rfc/rfc1945.html). This is just a very light overview of that document.

Generally speaking, HTTP is used to transfer resources from a host called "server", to a host called "client". This exchange is initiated by the client opening a TCP connection to the server (which is already listening on a specific port, usually port 80) and sending an HTTP request message over it, which includes the name of the resource being requested. The server in turn either responds with the resource contents or a failure response. The TCP connection is then closed.

## Request

Unlike the other lower level protocols, HTTP uses text-based messages (ASCII to be specific). So this is what a request looks like:

```
GET / HTTP/1.0\r\n
Host: coz.is\r\n
User-Agent: curl/7.81.0\r\n
\r\n
```

First of all, HTTP messages are composed of a list of lines separated by a carriage return character and a new line character which are represented here as \r and \n respectively.

The first line, usually referred to as "request line", contains the request method "GET", the resource name "/" and the HTTP version. Then, follow a variable number of "header fields" also commonly referred as "headers". Each header field is made up by a name and a value, separated by the colon character. Then comes an empty new line which signifies the end of the message.

The GET method is used to get resources from the server, but the client can also send data to it. In HTTP 1.0 this is done using the POST method:

```
POST / HTTP/1.0\r\n
Host: coz.is\r\n
User-Agent: curl/7.81.0\r\n
Content-Length: 13\r\n
\r\n
Hello, world!
```

This request is actually sending the "Hello, world!" string to the server. The payload of the request comes after the empty line which signifies the end of the header fields. How many bytes are being sent is specified by the "Content-Length" header field. It may seem strange, but POST requests also refer to a resource, in this case "/". This tells the server what it should do with that data.

The third HTTP 1.0 method is HEAD, which behaves as GET but tells the server to omit the response body. This tells the client which header fields the server would send if the client made a GET request.

## Response

A possible HTTP 1.0 response to the request could be something like this:

```
HTTP/1.0 200 OK\r\n
Content-Length: 22\r\n
\r\n
Hello from the server!
```

the only difference is in the first line, called "status line". Unlike the request line, the HTTP version comes first. Then follows the status code, a three digit code which describes the type of response being sent. The status code is followed by a human-readable message that's usually ignored by clients.

Status codes for HTTP 1.0 are standardized and are available at [https://www.rfc-editor.org/rfc/rfc1945.html#section-6.1.1](https://www.rfc-editor.org/rfc/rfc1945.html#section-6.1.1). They are divided into categories based on the first digit:

* 1xx: Informational responses (not used in HTTP 1.0)
* 2xx: Success responses
* 3xx: Redirection responses
* 4xx: Client error responses
* 5xx: Server error responses

So for instance if a client requested a resource which does not exist on the server, the response status code would be in the 4xx family. If the response failed because the server crashed or some other internal error, then a 5xx status code would be used. If the resource is present but changed location, a 3xx response would be generated with the new location described by one of its headers. If the response succeeded, then a 2xx code would be used as in our previous example.

## HTTPS

These days we usually use HTTPS instead of plain HTTP. HTTPS is a secure version of HTTP (the "S" stands for "secure"). HTTPS works by encrypting all messages before sending them over TCP and by decrypting all messages received over TCP. The encrypted/decrypted messages are just regular HTTP messages as we saw before. This makes sure the communication is both confidential and that the messages we are receiving are actually coming from who we expect.

The encryption/decryption protocol used is called TLS, which is a newer version of the SSL protocol. Other than encrypting and decrypting it also handles authentication and key exchanges between peers.
