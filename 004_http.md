# Building web apps from scratch - HTTP - Part 4

With this last post we completed our tour into the kernel's network stack, so now we can chill a bit and reduce the pace! The TCP protocol and the ones beneath it offer the abstraction of a reliable transport of byte streams. Applications can then leverage this abstraction to implement application-specific protocols. The most widely used application-level protocol is HTTP given how it powers the web. The remaining part of this series will basically cover all aspects of HTTP, so instead of listing all of its features in a single post, I will introduce them as needed while building our application. This post will cover HTTP's basic syntax and other things that tech-savvy people will probably be already familiar with. Regardless I'll state things explicitly to make sure we are on the same page!

## A bit of history

The *HyperText Transfer Protocol* (HTTP) protocol was originally designed at CERN to share documents between network nodes. The idea is that some machines called "servers" would catalog files, and "clients" would be able to download these files based on their names. These files were intended to be in the HTML format, which essentially allows adding to documents references to other documents:

```
<html>
	<body>
		I'm a very important document!
		<a href="http://server.com/other_very_important_document.html">link to other very important document</a>
	</body>
</html>
```

this is an example of an HTML document that references some other document using the `<a>` tag. Note that the HTML syntax may have been different at the time! The addition of "links" is what made these text documents "hyper". The idea is that users would be able to navigate the graph of documents distributed over the network by leveraging HTTP. Each document would contain links to other related documents, and upon clicking on the links a new HTTP request would be performed under the hood to then visualize the referenced document.

<br />
<br />

With time HTTP was adopted for things other than scientific research, slowly turning it into a more general application delivery platform. Today we don't think about webpages as textual documents but as applications.

<br />
<br />

Later on HTTP was standardized and version 1.0 was born. The original version was called 0.9 but isn't in use anymore. Over the years with its wide adoption the protocol was improved, introducing version 1.1, 2.0, 3.0. In this series we will start with HTTP 1.0 and then move to HTTP 1.1 when making our server non-blocking.

You can find the HTTP 1.0 standard at [https://www.rfc-editor.org/rfc/rfc1945.html](https://www.rfc-editor.org/rfc/rfc1945.html).

## The HTTP Exchange

At its core, HTTP consists in transferring a blob of bytes with some optional attributes from a server to the client machine. The exchange is initiated by the client, which sends an **HTTP request** message:

```
GET very_important_document.txt HTTP/1.0\r\n
Accept-Language: en\r\n
\r\n
```

the request contains the name of a resource that is expected to be stored on the server and some additional request modifiers. The server then responds with the requested resources or an error. Here are examples of possible responses:

```
HTTP/1.0 200 OK\r\n
Content-Length: 168\r\n
\r\n
<html>
	<body>
		I'm a very important document!
		<a href="http://server.com/other_very_important_document.html">other very important document</a>
	</body>
</html>
```

```
HTTP/1.0 404 Not Found\r\n
Content-Length: 49\r\n
\r\n
This resource doesn't exist on the server. Sorry!
```

The HTTP protocol doesn't really dictate what underlying L4 protocol should be used, but in most cases it will be TCP. In HTTP 1.0 the request will be preceded by a TCP SYN segment to establish a connection and the response will be followed by a FIN segment to tear it down.

## The HTTP Request

Now let's focus on the HTTP request:

```
GET very_important_document.txt HTTP/1.0\r\n
Accept-Language: en\r\n
\r\n
```

An HTTP message is split into lines using the special characters carriage return and newline, respectively indicated here with \r and \n. The first line of a request is called "request line", and specifies the request method, the resource name, and HTTP version. The request method essentially indicates if the request is download or uploading data (I didn't mention it earlier but clients may also upload data to servers). You can think of the resource name as the path of a file, except the underlying resource may be something other than a file. For instance a server could have a resource "sensor_data.bin" which is associated to a function that collects sensor data in real time.

<br />
<br />

In HTTP 1.0, the request methods can be GET, HEAD, or POST. The GET request is used to download a resource from a server. HEAD behaves like GET by asking the server to send a resource over, but the actual payload should not be sent. It's useful to see which headers the server would send for a given resource. The POST method is used for sending form submissions or file uploads.

<br />
<br />

The request line is then followed by zero or more header fields, a list of (name,value) pairs separated by a colon. These fields can either add information to how the HTTP exchange must happen or about the request payload. For instance in this example the "Accept-Language" header tells the server that it should serve content in english.

<br />
<br />

The header list is terminated with an empty line. The request line and header fields make up the "request head". If the request contains some payload, it will be sent after the empty line. Note that a request head is always terminated by the \r\n\r\n sequence: the first \r\n terminates the last header field and the second \r\n is the empty line. If present, the size of the payload can be inferred from the header fields.

## The HTTP Response

Responses are very similar to requests. The only difference is the first line. Instead of the request line, responses have the "status line":

```
HTTP/1.0 200 OK\r\n
Content-Length: 168\r\n
\r\n
<html>
	<body>
		I'm a very important document!
		<a href="http://server.com/other_very_important_document.html">other very important document</a>
	</body>
</html>
```

In the status line first comes the version, then the status code, and then a human readable string describing the status called "reason phrase". Everything else looks like a request.

<br />
<br />

Status codes are listed in the standard alongside a possible reason phrase for it:

* 200 OK
* 201 Created
* 202 Accepted
* 204 No Content
* 301 Moved Permanently
* 302 Moved Temporarily
* 304 Not Modified
* 400 Bad Request
* 401 Unauthorized
* 403 Forbidden
* 404 Not Found
* 500 Internal Server Error
* 501 Not Implemented
* 502 Bad Gateway
* 503 Service Unavailable

Status codes always have 3 digits and can be grouped based on the first one:

* 1xx: Informational responses (not used in HTTP 1.0)
* 2xx: Success responses
* 3xx: Redirection responses
* 4xx: Client error responses
* 5xx: Server error responses

## What's next
In the next post we'll finally start implementing our server. We will go over the functions required to set up the TCP layers.

## Join the Discussion!
Have questions or feedback for me? Feel free to pop in my discord

<br />
<br />

[Join the Discord Server](https://discord.gg/vCKkCWceYP)