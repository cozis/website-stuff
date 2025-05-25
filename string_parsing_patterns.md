# String Parsing Patterns

Over the years, I came up with several patterns for parsing strings without a tokenization step. I found that following these patterns consciously made it easy to write robust string parsing code.

<br />
<br />

In this post I'll list them and show how to use them to make a simplified JSON parser. Code snippets will use the C language, but the logic applies to any C-like language.

## General Structure

Let's assume `char *src` is a string of length `int len` we want to extract information from. We will use the `int cur` cursor variable to keep track of how many characters we already inspected from the string. The cursor is such that `src[cur]` is the next character that hasn't been inspected yet. If `cur == len`, all characters have been inspected. Note that I sometimes refer to inspecting characters as "skipping" or "consuming" them.

## Consuming an Optional Sequence

To move the cursor over a sequence of characters, you write a loop like this:

```c
while (cur < len && test(src[cur]))
	cur++;
```

the sequence is defined by the `test` function. All and only characters which pass the test are consumed.

If no characters pass the test, the cursor is left unchanged. This is why I call this "consuming an optional sequence".

## Consuming a Required Sequence

What if we want to consume a non optional sequence? In other words, the sequence must contain at least one character.

The way this is done is by adding a first if statement:

```c
if (cur == len || !test(src[cur]))
	return; // Error
do
	cur++;
while (cur < len && test(src[cur]));
```

we added an if statement which checks the negated version of the loop's condition. If the if's check passes, we know there is at least one character, and therefore can use a `do..while` instead of a regular `while` loop.

## Consuming a Specific Character

We can consume an optional character like this:

```c
if (cur < len && src[cur] == 'X')
	cur++;
```

and we can consume a required character like this:

```c
if (cur == len || src[cur] != 'X')
	return; // Error
cur++;
```

## Consuming a Specific String

To consume a specific string, we can of course consume the single characters:

```c
if (cur == len || src[cur] != 'A')
	return; // Error
cur++;

if (cur == len || src[cur] != 'B')
	return; // Error
cur++;

if (cur == len || src[cur] != 'C')
	return; // Error
cur++;
```

but we can do better:

```c
if (2 >= len - cur
	|| src[cur+0] != 'A'
	|| src[cur+1] != 'B'
	|| src[cur+2] != 'C')
	return; // Error
cur += 3;
```

This way we perform a single bounds check and it's more compact overall.

This isn't very intuitive for me either, so this is what I think when writing this.

<br/>
<br/>

First, I ignore the string's bounds and check the characters:

```c
if (
	|| src[cur+0] != 'A'
	|| src[cur+1] != 'B'
	|| src[cur+2] != 'C')
	return; // Error
cur += 3;
```

the index furthest away from the cursor is `cur+2`. If this offset is out of bounds, they all are. Therefore the checks are valid only if `cur+2 < len`, and they are surely not valid when `cur+2 >= len`. We add this to our expression:

```c
if (cur+2 >= len
	|| src[cur+0] != 'A'
	|| src[cur+1] != 'B'
	|| src[cur+2] != 'C')
	return; // Error
cur += 3;
```

if we want to be extra careful here we can improve the bounds check to avoid overflow

```c
if (2 >= len - cur
	|| src[cur+0] != 'A'
	|| src[cur+1] != 'B'
	|| src[cur+2] != 'C')
	return; // Error
cur += 3;
```

this is because `len - cur` can never underflow, while `cur+2` may overflow. This is very nitpicky though.

## Parsing Strings

What if we want to not only consume characters, but extract information from the source string? Fear not! We can easily use our previous lessons for this. Let's say we want to parse a string in the form `<spaces> <name> <spaces> <nickname>`:

```c
int off;

while (cur < len && is_whitespace(src[cur]))
	cur++;

off = cur;
while (cur < len && !is_whitespace(src[cur]))
	cur++;
char *name_ptr = src + off;
int   name_len = cur - off;

while (cur < len && is_whitespace(src[cur]))
	cur++;

off = cur;
while (cur < len && !is_whitespace(src[cur]))
	cur++;
char *nick_ptr = src + off;
int   nick_len = cur - off;

while (cur < len && is_whitespace(src[cur]))
	cur++;
```

with this, we end up with pointers to name and nickname and their relative lengths, with no memory allocation! Note that the strings are not null-terminated. If you want to print them with `printf`, you'll have to use this format specifier:

```c
printf("Hello, I'm %.*s and my nickname is %.*s\n", name_len, name_ptr, nick_len, nick_ptr);
```

but note that using `!is_whitespace(src[cur])` (with the negation) is inherently unsafe. The robust solution is to always specify which characters are allowed, not the other way around. The robust version should use something like this instead of just allowing non-whitespace characters:

```c
char is_name_or_nick(char c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}
```

## Parsing Integers

This is how we can scan a sequence of digits and build the equivalent integer representation of it:

```c
#include <limits.h> // Defines INT_MAX

typedef struct {
	char *src;
	int   len;
	int   cur;
} Scanner;

int is_digit(char c)
{
	return c >= '0' && c <= '9';
}

int parse_int(Scanner *s, int *out)
{
	if (s->cur == s->len || !is_digit(s->src[s->cur]))
		return 0; // Error

	int x = 0;
	do {
		int d = s->src[s->cur++] - '0';
		if (x > (INT_MAX - d) / 10)
			return 0; // Overflow
		x = x * 10 + d;
	} while (s->cur < s->len && is_digit(s->src[s->cur]));

	*out = x;
	return 1;
}
```

## Parsing Floats

This is how one can parse floats:

```c
typedef struct {
	char *src;
	int   len;
	int   cur;
} Scanner;

int is_digit(char c)
{
	return c >= '0' && c <= '9';
}

int parse_float(Scanner *s, float *out)
{
	if (s->cur == s->len || !is_digit(s->src[s->cur]))
		return 0; // Error

	float x = 0;
	do {
		int d = s->src[s->cur++] - '0';
		x = x * 10 + d;
	} while (s->cur < s->len && is_digit(s->src[s->cur]));

	if (s->cur == s->len || s->src[s->cur] != '.')
		return 0; // Error
	s->cur++;

	if (s->cur == s->len || !is_digit(s->src[s->cur]))
		return 0; // Error
	float q = 1;
	do {
		q /= 10;
		x += q * (s->src[s->cur++] - '0');
	} while (s->cur < s->len && is_digit(s->src[s->cur]));

	*out = x;
	return 1;
}
```

this code looks quite involved, but actually it's just the same pattern applied over and over.

## Example

This is an example JSON parser written using the previous patterns. For simplicity I just output the parsed values instead of building an in-memory representation.

```c
#define ASSERT(X) {if (!(X)) __builtin_trap();}

typedef struct {
	char *src;
	int   len;
	int   cur;
} Scanner;

int is_digit(char c)
{
	return c >= '0' && c <= '9';
}

int is_whitespace(char c)
{
	return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

int parse_string(Scanner *s)
{
	ASSERT(s->cur < s->len && s->src[s->cur] == '"');
	s->cur++;

	int off = s->cur;
	while (s->cur < s->len && s->src[s->cur] != '"')
		s->cur++;
	char *p = s->src + off;
	int   l = s->cur - off;

	if (s->cur == s->len)
		return 0; // Error
	s->cur++;

	printf("Parsed string \"%.*s\"\n", l, p);

	return 1;
}

int parse_object(Scanner *s)
{
	ASSERT(s->cur < s->len && s->src[s->cur] == '{');
	s->cur++;

	while (s->cur < s->len && is_whitespace(s->src[s->cur]))
		s->cur++;

	if (s->cur < s->len && s->src[s->cur] != '}') {

		for (;;) {

			if (!parse_string(s))
				return 0; // Propagate error

			while (s->cur < s->len && is_whitespace(s->src[s->cur]))
				s->cur++;

			if (s->cur == s->len || s->src[s->cur] != ':')
				return 0; // Error
			s->cur++;

			while (s->cur < s->len && is_whitespace(s->src[s->cur]))
				s->cur++;

			if (!parse_value(s))
				return 0; // Propagate error

			while (s->cur < s->len && is_whitespace(s->src[s->cur]))
				s->cur++;

			if (s->cur < s->len && s->src[s->cur] == '}')
				break;

			if (s->cur == s->len || s->src[s->cur] != ',')
				return 0; // Error
			s->cur++;

			while (s->cur < s->len && is_whitespace(s->src[s->cur]))
				s->cur++;
		}
	}
	ASSERT(s->cur < s->len && s->src[s->cur] == '}');
	s->cur++;
	return 1;
}

int parse_array(Scanner *s)
{
	ASSERT(s->cur < s->len && s->src[s->cur] == '[');
	s->cur++;

	while (s->cur < s->len && is_whitespace(s->src[s->cur]))
		s->cur++;

	if (s->cur < s->len && s->src[s->cur] != ']') {

		for (;;) {

			if (!parse_value(s))
				return 0; // Propagate error

			while (s->cur < s->len && is_whitespace(s->src[s->cur]))
				s->cur++;

			if (s->cur < s->len && s->src[s->cur] == ']')
				break;

			if (s->cur == s->len || s->src[s->cur] != ',')
				return 0; // Error
			s->cur++;

			while (s->cur < s->len && is_whitespace(s->src[s->cur]))
				s->cur++;
		}
	}
	ASSERT(s->cur < s->len && s->src[s->cur] == ']');
	s->cur++;
	return 1;
}

int parse_int(Scanner *s)
{
	if (s->cur == s->len || !is_digit(s->src[s->cur]))
		return 0; // Error

	int x = 0;
	do {
		int d = s->src[s->cur++] - '0';
		if (x > (INT_MAX - d) / 10)
			return 0; // Overflow
		x = x * 10 + d;
	} while (s->cur < s->len && is_digit(s->src[s->cur]));

	printf("Parsed integer %d\n", x);
	return 1;
}

int parse_float(Scanner *s)
{
	if (s->cur == s->len || !is_digit(s->src[s->cur]))
		return 0; // Error

	float x = 0;
	do {
		int d = s->src[s->cur++] - '0';
		x = x * 10 + d;
		// Note that parse_number already made sure
		// the sequence of digits terminated with a dot,
		// so we can omit the cur<len check
	} while (s->src[s->cur] != '.');

	if (s->cur == s->len || s->src[s->cur] != '.')
		return 0; // Error
	s->cur++;

	if (s->cur == s->len || !is_digit(s->src[s->cur]))
		return 0; // Error
	float q = 1;
	do {
		q /= 10;
		x += q * (s->src[s->cur++] - '0');
	} while (s->cur < s->len && is_digit(s->src[s->cur]));

	printf("Parsed float %f\n", x);
	return 1;
}

int parse_number(Scanner *s)
{
	int peek = s->cur;
	while (peek < s->len && is_digit(s->src[peek]))
		peek++;

	if (peek == s->len || s->src[peek] != '.')
		return parse_int(s);

	return parse_float(s);
}

// Parses any type of value. The cursor must point to the first
// character of the value. Any preceding whitespace must be
// consume by the caller.
int parse_value(Scanner *s)
{
	if (s->cur == s->len)
		return 0; // Error

	if (s->src[s->cur] == '"')
		return parse_string(s);

	if (s->src[s->cur] == '{')
		return parse_object(s);

	if (s->src[s->cur] == '[')
		return parse_array(s);

	if (is_digit(s->src[s->cur]))
		return parse_number(s);

	if (3 < s->len - s->cur
		&& s->src[s->cur+0] == 't'
		&& s->src[s->cur+1] == 'r'
		&& s->src[s->cur+2] == 'u'
		&& s->src[s->cur+3] == 'e') {
		s->cur += 4;
		printf("Parsed true\n");
		return 1;
	}

	if (3 < s->len - s->cur
		&& s->src[s->cur+0] == 'n'
		&& s->src[s->cur+1] == 'u'
		&& s->src[s->cur+2] == 'l'
		&& s->src[s->cur+3] == 'l') {
		s->cur += 4;
		printf("Parsed null\n");
		return 1;
	}

	if (4 < s->len - s->cur
		&& s->src[s->cur+0] == 'f'
		&& s->src[s->cur+1] == 'a'
		&& s->src[s->cur+2] == 'l'
		&& s->src[s->cur+3] == 's'
		&& s->src[s->cur+4] == 'e') {
		s->cur += 5;
		printf("Parsed false\n");
		return 1;
	}

	// Invalid character
	return 0;
}

// Returns 0 on error, 1 on success
int parse_json(char *src, int len)
{
	Scanner s = {src, len, 0};

	while (s.cur < s.len && is_whitespace(s.src[s.cur]))
		s.cur++;

	if (!parse_value(&s))
		return 0;

	if (s.cur == s.len)
		return 0;

	return 1;
}
```