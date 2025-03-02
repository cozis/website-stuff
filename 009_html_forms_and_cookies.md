# Building web apps from scratch - HTML Forms and Cookies - Part 4

**NOTE: This post is still a work in progress!!**

We are now ready to write our first web page! We’ll start by implementing a simple login. To do so, we need to understand how HTML forms work.

An HTML form is an element of this kind:

```html
<form action="/some-endpoint" method="POST">
  <input type="text" name="username" />
  <input type="password" name="password" />
  <input type="submit" value="sign up" />
</form>
```

This element contains two text inputs, one for an username and one for a password. The last input is the submit button which will send the request associated to this form. The second input, the one for the password, will show the text with the black dots that are typical of password inputs. This is because we specified `type="password"`.

When we submit, the page will send a request to the server to endpoint `/some-endpoint` with method `POST`. The payload of the request will contain the input values. The request will look similar to this:

```
POST /some-endpoint HTTP/1.1\r\n
Content-Type: application/x-www-form-urlencoded\r\n
Content-Length: 40\r\n
\r\n
username=cozis&password=mysecretpassword
```

this examples assumes I inserted "cozis" as username and "mysecretpassword" as password. You may notice that the password shows in plain text, which makes it possible for people to intercept our request and know about our secret password. This problem is solved by adding encryption to the client-server communication. But we’ll focus on that later.

In the payload each value is associated to the input name, which we specified as `name="username"` and `name="password"`. That' should be pretty intuitive.

When the input names or values contain special characters, they must be properly encoded using “urlencoding“ or “percent encoding“. Special characters must be changed to a token in the form `%XX`, where `XX` is the hexadecimal version of that character. You can find these associations on the [ASCII table](https://www.ascii-code.com/).

When the form request is received, the server processes it based on what the form was for, then redirects the client to a new page. A response to the previous request could look like this:

```
HTTP/1.1 303 See Other\r\n
Content-Length: 45\r\n
Location: /welcome\r\n
\r\n
You are being redirected to the welcome page!
```

The status code 3xx tells us this is a redirect response, and the target is specified in the `Location` header. Upon receiving this request, the client will go to the `/welcome` page.

### Cookies

Upon receiving a request from the client, the server may decide to associated a cookie to the client, which is a piece of information. After the server gave a cookie to the client, the client will start sending that cookie to the server with each new request.

This mechanism allows us to associate multiple requests to a single user, which is essential when building a login system like we are.

Lets see how the previous example can be extended to use cookies.

During the first request, the client has no cookies yet:

```
POST /some-endpoint HTTP/1.1\r\n
Content-Type: application/x-www-form-urlencoded\r\n
Content-Length: 40\r\n
\r\n
username=cozis&password=mysecretpassword
```

The server decides now to associate to the client a cookie with its username. This way it will know upon receiving further requests that it already logged in, ad as which user. To set a cookie, the server adds a `Set-Cookie` header:

```
HTTP/1.1 303 See Other\r\n
Content-Length: 45\r\n
Set-Cookie: username=cozis\r\n
Location: /welcome\r\n
\r\n
You are being redirected to the welcome page!
```

The client then requests `/welcome`, and will do so with its new cookie:

```
GET /welcome HTTP/1.1\r\n
Cookie: username=cozis\r\n
\r\n
```

lets imagine the `/welcome` page can be only viewed by logged in users. It is easy to see how the server can ensure this with cookies: if the client does not send an username cookie, it has not logged in and therefore it can’t access the page.