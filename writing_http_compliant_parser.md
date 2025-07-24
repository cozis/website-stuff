# Writing an HTTP compliant parser in C

Recently I published a library called [cHTTP](https://github.com/cozis/cHTTP) on GitHub, which is essentially my private networking code polished and made reusable. Yesterday I decided I would go through the HTTP request parser and make sure it is compliant to spec, so I took the opportunity to write about it. In this post I will go throgh the specification to turn it into an HTTP request parser using C.

<br />
<br />

This article will focus more on following the specification than the actual parsing code. If you're not familiar with string parsing in C, consider reading my post about [the scanner pattern](the_scanner_pattern.html) where I document my way of doing things. Also, if for some reason you're not familiar with HTTP protocol or it's syntax, you should go read [this post](004_http.html).

## How to Read the Spec

The HTTP protocol is standardized by documents called RFCs. Often new documents that obsolete old ones are published. For instance the first RFC for HTTP 1.1 was RFC 2616, but the latest specification at the time of writing is RFC 9112.

<br />
<br />

If we take a look at RFC 9112, we'll find that in section [2](https://www.rfc-editor.org/rfc/rfc9112.html#name-message) the syntax of an "HTTP message" is defined:

```
HTTP-message = start-line CRLF
               *( field-line CRLF )
               CRLF
               [ message-body ]
```

It's expressed using a notation called *Augmented Backus-Naur Form (ABNF)* and this is essentially what we'll need to turn into code. I'll go into its details as we write code, but if you want to learn about it in more detail, read [section 1.2](https://www.rfc-editor.org/rfc/rfc9112.html#name-syntax-notation).

<br />
<br />

The basic idea is that every "token" (start-line, CRLF, field-line, message-body) represents a pattern of characters called "rules". Token representing rules can be grouped into higher level rules, such as this one. Tokens with uppercase names usually are primitive tokens that represent simple characters. The special characters such as parenthesis or the asterisk specify how these tokens combine to make the higher level rule. For instance, the asterisk means that the tokens following it can repeat zero or more times and the square brackets mean that the tokens inside it are optional.

<br />
<br />

So our job will be to read the document, unpack these rules and turn them into code. Now let's dive in!

## Parsing the Request Line

Let's make sense of that `HTTP-message` rule by comparing it to a practical example of HTTP request

```
HTTP-message = start-line CRLF
               *( field-line CRLF )
               CRLF
               [ message-body ]
```

```
GET /index.html HTTP/1.1\r\n
Host: coz.is\r\n
User-Agent: curl/7.81.0\r\n
\r\n
```

A quick string search of CRLF shows that it is defined in section 1.2 as a carriage return and line feed `\r\n`. So intuitively we can observe that:

```
start-line   => GET /index.html HTTP/1.1
field-line   => Host: coz.is
field-line   => User-Agent: curl/7.81.0
message-body => (empty)
```

The `start-line` matches the line where method, path, and version are specified, the `field-line` matches the headers, and the `message-body` matches the body (which is not present here).

<br />
<br />

Let's unpack the rule further by looking at how `start-line` is defined (as always, we do a string search in the document):

```
start-line = request-line / status-line

request-line = method SP request-target SP HTTP-version
```

The slash `/` means only one of the two rules apply. Since we are parsing HTTP requests, we are interested in the `request-line` rule. Just like the CRLF token, the SP token is defined in section 1.2 as a single space.

```
method         => GET
request-target => /index.html
HTTP-version   => HTTP/1.1
```

### Parsing the Method

If we look for the `method` rule, we find that it's definition references some other document called `[HTTP]`

```
method = token

token = <token, see [HTTP], Section 5.6.2>
```

If we follow the reference (look for `[HTTP]` in the [references](https://www.rfc-editor.org/rfc/rfc9112.html#name-references) section) we find that the rule is defined in RFC 9110

```
token = 1*tchar

tchar = "!" / "#" / "$" / "%" / "&" / "'" / "*"
      / "+" / "-" / "." / "^" / "_" / "`" / "|" / "~"
      / DIGIT / ALPHA
      ; any VCHAR, except delimiters
```

Now we have a good idea of how to parse the method. We want to parse one or more tchars followed by a space! That's easy enough:

```c

// DIGIT
bool is_digit(char c)
{
    return c >= '0' && c <= '9';
}

// ALPHA
bool is_alpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

// tchar
bool is_tchar(char c)
{
    return c == '!' || c == '#' || c == '$' || c == '%' || c == '&'
        || c == '\'' || c == '*' || c == '+' || c == '-' || c == '.'
        || c == '^' || c == '_' || c == '`' || c == '|' || c == '~'
        || is_digit(c) || is_alpha(c);
}

int parse_method(Scanner *s, String *method)
{
    int method_off = s->cur;

    // Consume a tchar or fail
    if (s->cur == s->len || !is_tchar(s->src[s->cur]))
        return -1; // Error! Missing token
    s->cur++;

    // Consume additional tchars following the first one
    while (s->cur < s->len && is_tchar(s->src[s->cur]))
        s->cur++;

    // Make a substring of the method
    *method = (String) {
        s->src + method_off,
        s->cur - method_off
    };

    // Consume a space or fail
    if (s->cur == s->len || s->src[s->cur] != ' ')
        return -1;
    s->cur++;

    // All good!
    return 0;
}
```

## Parsing the Request Target

Now that we parsed the method, let's go back to the `request-line` rule:

```
request-line = method SP request-target SP HTTP-version
```

The next rule is `request-target`. I happen to know that this one is quite complex. This rule alone takes more code than the rest of the parser, so we can take a shortcut and say that the request target is anything that goes from the first space to the second one preceding the version. This isn't ideal but most server do it. Note that it's always safer to parse strings by explicitly allowing a set of characters, which is not what we are doing here as we are letting though anything that is not a space.

<br />
<br />

With this simplification parsing the target should be quite straight-forward

```c
int parse_target(Scanner *s, String *target)
{
    int off = s->cur;

    while (s->cur < s->len && s->src[s->cur] != ' ')
        s->cur++;

    *target = (String) {
        s->src + off,
        s->cur - off,
    };

    if (s->cur == s->len)
        return -1;
    s->cur++;

    return 0;
}
```
## Parsing the Version

Following the target comes the HTTP-version rule:

```
HTTP-name = %x48.54.54.50 ; HTTP
HTTP-version = HTTP-name "/" DIGIT "." DIGIT
```

The `%x` notation just means a sequence of character specified by their hex representation. If you look the numbers in the [ASCII table](https://en.wikipedia.org/wiki/ASCII), you will find that they simpli represent the string "HTTP". I think they used this notation to express that the string must be uppercase. Following the "HTTP" string comes the "/" character and then a major version number and a minor version number separated by a dot. Since this is an HTTP 1.1 parser we only allow versions "1.1" and "1.0", which means the rule might as well be this for us

```
HTTP-version = "HTTP/1.0" / "HTTP/1.1"
```

For this, we can rely on our super useful `consume_str` function defined in [the scanner pattern](the_scanner_pattern.html) post

```c
bool consume_str(Scanner *s, String x)
{
    if (x.len == 0)
        return false;

    if (x.len > s.len - s.cur)
        return false;

    for (int i = 0; i < x.len; i++)
        if (s->src[s->cur+i] != x.ptr[i])
            return false;

    s->cur += x.len;
    return true;
}

int parse_version(Scanner *s, int *minor)
{
    if (consume_str(s, S("HTTP/1.0\r\n"))) {
        *minor = 0;
        return 0;
    }

    if (consume_str(s, S("HTTP/1.1\r\n"))) {
        *minor = 1;
        return 0;
    }

    return -1;
}
```

Since the major version is always 1, we just need to return the minor version. Also as we did with the method and target parsing function, we make the version function also consume the CRLF separator that follows it.

<br />
<br />

And this completes are parsing of the request line!

## Parsing Headers

Next come the `field-line` rule

```
HTTP-message = start-line CRLF
               *( field-line CRLF )
               CRLF
               [ message-body ]
```

This rule can occur zero or more times, which means we'll need to write a function to parse a single `field-line` and then call it in a loop.

<br />
<br />

A single `field-line` is defined as this:

```
field-line = field-name ":" OWS field-value OWS
```

We have a name and a value separated by a semicolon. The value is padded by spaces (string search OWS in the document). The name and value rules are defined in RFC 9110

```
field-name = <field-name, see [HTTP], Section 5.1>
field-value = <field-value, see [HTTP], Section 5.5>
```

Following the references we get to these definitions

```
field-name = token

field-value    = *field-content
field-content  = field-vchar
                 [ 1*( SP / HTAB / field-vchar ) field-vchar ]
field-vchar    = VCHAR / obs-text
obs-text       = %x80-FF
```

The name rule is simple enough, while the value rule is quite verbose. Fortunately, that's just a way of saying that the header value can't contain leading and trailing spaces. Other than that it can contain spaces, tabs, any visible ASCII caracters (referred to as VCHARs) or characters with values 0x80 to 0xFF that correspond to UTF-8 character bytes. Unfortunately there is a lot of weirdness with UTF-8 so I prefer to only allow ASCII.

```c
typedef struct {
    String name;
    String value;
} Header;

bool is_vchar(char c)
{
    return c >= ' ' && c <= '~';
}

int parse_header(Scanner *s, Header *header)
{
    // Parse the name

    int name_off = s->cur;
    if (s->cur == s->len || !is_tchar(s->src[s->cur]))
        return -1;
    s->cur++;

    while (s->cur < s->len && is_tchar(s->src[s->cur]))
        s->cur++;
    header->name = (String) { s->src + name_off, s->cur - name_off };

    // Consume the separator
    if (s->cur == s->len || s->src[s->cur] != ':')
        return -1;
    s->cur++;

    // Consume whitespace preceding the value
    while (s->cur < s->len && (s->src[s->cur] == ' ' || s->src[s->cur] == '\t'))
        s->cur++;

    // Parse the value

    int value_off = s->cur;

    // Consume all VCHARs and spaces
    while (s->cur < s->len && (is_vchar(s->src[s->cur]) || s->src[s->cur] == ' ' || s->src[s->cur] == '\t'))
        s->cur++;

    // If the body ended with some spaces, remove them. Note how this loop
    // doesn't have bound checks. We can do this because we know the header
    // contains at least one tchar
    while (s->src[s->cur] == ' ' || s->src[s->cur] == '\t')
        s->cur--;

    // Make a slice for the value
    header->value = (String) { s->src + value_off, s->cur - value_off };

    // Consume any spaces that follow the value
    while (s->cur < s->len && (s->src[s->cur] == ' ' || s->src[s->cur] == '\t'))
        s->cur++;

    // Consume the CRLF following the header field

    if (1 >= s->len - s->cur
        || s->src[s->cur+0] != '\r'
        || s->src[s->cur+1] != '\n')
        return -1;
    s->cur += 2;

    return 0;
}
```

As we were saying earlier, the header rule may apply multiple times until the final `CRLF` that precedes the body is found. Let's define a parsing function for the entire header list:

```c
int parse_header_list(Scanner *s, Header *headers, int max_headers)
{
    int num_headers = 0;
    while (!consume_str(s, S("\r\n"))) {

        if (num_headers == max_headers)
            return -1;

        int ret = parse_header(s, &headers[num_headers]);
        if (ret < 0)
            return -1;

        num_headers++;
    }

    return num_headers;
}
```

## Parsing the Body

The last thing to do is parse the `message-body` rule, which is defined in [section 6](https://www.rfc-editor.org/rfc/rfc9112.html#name-message-body) of RFC 9112

```
message-body = *OCTET
```

by octet the RFC means a generic byte, which means the body is expressed as a generic stream of data. The length of the data depends on the header values. If the request contains the `Content-Length` header, then the body is a single chunk of bytes. If the request contains the `Transfer-Encoding` header, then the body is actually a list of chunks each with its own header that needs parsing. This alone, as the request target parsing does, warrants its own article! So for now, you'll need to try implement body parsing on your own :)

## Putting Everything Together

And here it is, the complete parser with a basic test of our initial input!

```c
#include <stdio.h>
#include <stdbool.h>

#define MAX_HEADERS 128

#define S(X) (String) { (X), (int) sizeof(X)-1 }
#define UNPACK(X) (X).len, (X).ptr
#define COUNT(X) (sizeof(X) / sizeof((X)[0]))

typedef struct {
    char *src;
    int   len;
    int   cur;
} Scanner;

typedef struct {
    char *ptr;
    int   len;
} String;

typedef struct {
    String name;
    String value;
} Header;

typedef struct {
    String method;
    String target;
    int    minor;
    int    num_headers;
    Header headers[MAX_HEADERS];
} Request;

bool is_digit(char c)
{
    return c >= '0' && c <= '9';
}

bool is_alpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool is_tchar(char c)
{
    return c == '!' || c == '#' || c == '$' || c == '%' || c == '&'
        || c == '\'' || c == '*' || c == '+' || c == '-' || c == '.'
        || c == '^' || c == '_' || c == '`' || c == '|' || c == '~'
        || is_digit(c) || is_alpha(c);
}

bool consume_str(Scanner *s, String x)
{
    if (x.len == 0)
        return false;

    if (x.len > s->len - s->cur)
        return false;

    for (int i = 0; i < x.len; i++)
        if (s->src[s->cur+i] != x.ptr[i])
            return false;

    s->cur += x.len;
    return true;
}

int parse_method(Scanner *s, String *method)
{
    int method_off = s->cur;

    // Consume a tchar or fail
    if (s->cur == s->len || !is_tchar(s->src[s->cur]))
        return -1; // Error! Missing token
    s->cur++;

    // Consume additional tchars following the first one
    while (s->cur < s->len && is_tchar(s->src[s->cur]))
        s->cur++;

    // Make a substring of the method
    *method = (String) {
        s->src + method_off,
        s->cur - method_off
    };

    // Consume a space or fail
    if (s->cur == s->len || s->src[s->cur] != ' ')
        return -1;
    s->cur++;

    // All good!
    return 0;
}

int parse_target(Scanner *s, String *target)
{
    int off = s->cur;

    while (s->cur < s->len && s->src[s->cur] != ' ')
        s->cur++;

    *target = (String) {
        s->src + off,
        s->cur - off,
    };

    if (s->cur == s->len)
        return -1;
    s->cur++;

    return 0;
}

int parse_version(Scanner *s, int *minor)
{
    if (consume_str(s, S("HTTP/1.0\r\n"))) {
        *minor = 0;
        return 0;
    }

    if (consume_str(s, S("HTTP/1.1\r\n"))) {
        *minor = 1;
        return 0;
    }

    return -1;
}

bool is_vchar(char c)
{
    return c >= ' ' && c <= '~';
}

int parse_header(Scanner *s, Header *header)
{
    // Parse the name

    int name_off = s->cur;
    if (s->cur == s->len || !is_tchar(s->src[s->cur]))
        return -1;
    s->cur++;

    while (s->cur < s->len && is_tchar(s->src[s->cur]))
        s->cur++;
    header->name = (String) { s->src + name_off, s->cur - name_off };

    // Consume the separator
    if (s->cur == s->len || s->src[s->cur] != ':')
        return -1;
    s->cur++;

    // Consume whitespace preceding the value
    while (s->cur < s->len && (s->src[s->cur] == ' ' || s->src[s->cur] == '\t'))
        s->cur++;

    // Parse the value

    int value_off = s->cur;

    // Consume all VCHARs and spaces
    while (s->cur < s->len && (is_vchar(s->src[s->cur]) || s->src[s->cur] == ' ' || s->src[s->cur] == '\t'))
        s->cur++;

    // If the body ended with some spaces, remove them. Note how this loop
    // doesn't have bound checks. We can do this because we know the header
    // contains at least one tchar
    while (s->src[s->cur] == ' ' || s->src[s->cur] == '\t')
        s->cur--;

    // Make a slice for the value
    header->value = (String) { s->src + value_off, s->cur - value_off };

    // Consume any spaces that follow the value
    while (s->cur < s->len && (s->src[s->cur] == ' ' || s->src[s->cur] == '\t'))
        s->cur++;

    // Consume the CRLF following the header field

    if (1 >= s->len - s->cur
        || s->src[s->cur+0] != '\r'
        || s->src[s->cur+1] != '\n')
        return -1;
    s->cur += 2;

    return 0;
}

int parse_header_list(Scanner *s, Header *headers, int max_headers)
{
    int num_headers = 0;
    while (!consume_str(s, S("\r\n"))) {

        if (num_headers == max_headers)
            return -1;

        int ret = parse_header(s, &headers[num_headers]);
        if (ret < 0)
            return -1;

        num_headers++;
    }

    return num_headers;
}

int parse_request(String src, Request *req)
{
    Scanner s = { src.ptr, src.len, 0 };

    if (parse_method(&s, &req->method) < 0)
        return -1;

    if (parse_target(&s, &req->target) < 0)
        return -1;

    if (parse_version(&s, &req->minor) < 0)
        return -1;

    int ret = parse_header_list(&s, req->headers, (int) COUNT(req->headers));
    if (ret < 0)
        return -1;
    req->num_headers = ret;

    return 0;
}

int main(void)
{
    String str = S("GET /index.html HTTP/1.1\r\nHost: coz.is\r\nUser-Agent: curl/7.81.0\r\n\r\n");

    Request req;
    if (parse_request(str, &req) < 0) {
        printf("Parsing failed\n");
        return -1;
    }

    printf("method: %.*s\n", UNPACK(req.method));
    printf("target: %.*s\n", UNPACK(req.target));
    printf("version: HTTP/1.%d\n", req.minor);
    printf("headers:\n");
    for (int i = 0; i < req.num_headers; i++)
        printf("name: %.*s, value: %.*s\n", UNPACK(req.headers[i].name), UNPACK(req.headers[i].value));

    return 0;
}
```