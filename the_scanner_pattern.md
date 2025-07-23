# The Scanner Pattern

Over the years, I have written quite a bit of string manipulation and parsing code in C. Unfortunately, C has no runtime checks that can catch weird behaviors like out-of-bounds array accesses. For this reason, I came up with some safe patterns to follow that allow me to write pretty robust code from the get-go. Of course, one can always make mistakes, but the chances of making them in the first place drop drastically. Not only that, but when one has standard ways of doing things and something breaks, it's quite easy to find the problem by finding where the pattern breaks. The umbrella term I use for these techniques is "The Scanner Pattern", as they all revolve around a "Scanner" object.

## The Problem

As I was saying, the Scanner pattern is used to parse information in textual format, such as JSON or HTTP. We usually want textual and flexible formats as they are easier to work with for us humans, but the same can't be said for machines. That's why programs usually translate text into a more native binary representation.

<br />
<br />

Say we want to write a program that's capable of reading a list of names from a file to determine whether a given name is contained or not. The file uses a human-friendly format:

```
BEGIN
  Linus  Torvalds
  Bill   Gates
  Steve  Jobs
  Mark   Facebook
END
```

It starts and ends with the BEGIN and END keywords, and has a bunch of names in between. The format is insensitive to spaces, so you can have zero or more spaces before and after names and one or more between name and surname.

<br />
<br />

The result should be an array of structs, each containing a name:

```c
typedef struct {
    char *ptr;
    int   len;
} String;

typedef struct {
    String name;
    String surname;
} Person;

#define S(X) (String) { (X), (int) sizeof(X)-1 }

int main(void)
{
    String file_contents = S("BEGIN\n  Linus Torvalds\n  Bill Gates\n  Steve Jobs\n  Mark Facebook\nEND");

    int num_people = 0;
    Person people[MAX_PEOPLE];

    // TODO: parse file_contents and fill up the people array

    // TODO: check that the list contains a given name

    return 0;
}
```

## The Scanner

The Scanner Pattern is called that way because it's based on a Scanner object:

```c
typedef struct {
    char *src;
    int   len;
    int   cur;
} Scanner;
```

The string given by the `src` pointer and `len` length is the input data. The `cur` field contains the next character that needs to be read and is advanced as the program reads the string and extracts information. The cursor starts at 0 and parsing is complete when it reaches the end and is equal to the length of the string.

```c
Scanner s = { file_contents.ptr, file_contents.len, 0 };
```

I usually use a very short name for it as it's referenced a lot.

## Consuming Specific Sequences

```
BEGIN\n  Linus  Torvalds\n  Bill   Gates\n  Steve  Jobs\n  Mark   Facebook\nEND
^
cur
```

The first thing is that we need to check that the file starts with the sequence "BEGIN" followed by a line feed. If it doesn't, we fail the parsing. If it does, we advance the cursor.

```c
if (s.cur == s.len || s.src[s.cur] != 'B')
    return -1; // Error
s.cur++;
```

This code fails if the cursor doesn't point to the B character. This may be because there is a different character or the string is empty. At the end of this code, if it didn't error, the cursor will point to the second character. We can apply this code multiple times, once per character:

```c
if (s.cur == s.len || s.src[s.cur] != 'B')
    return -1; // Error
s.cur++;

if (s.cur == s.len || s.src[s.cur] != 'E')
    return -1; // Error
s.cur++;

if (s.cur == s.len || s.src[s.cur] != 'G')
    return -1; // Error
s.cur++;

if (s.cur == s.len || s.src[s.cur] != 'I')
    return -1; // Error
s.cur++;

if (s.cur == s.len || s.src[s.cur] != 'N')
    return -1; // Error
s.cur++;

if (s.cur == s.len || s.src[s.cur] != '\n')
    return -1; // Error
s.cur++;
```

This works but is quite verbose. We can group all the checks in a single if statement and then increment the cursor when all of them have passed.

```c
if (s.cur+0 == s.len || s.src[s.cur+0] != 'B' ||
    s.cur+1 == s.len || s.src[s.cur+1] != 'E' ||
    s.cur+2 == s.len || s.src[s.cur+2] != 'G' ||
    s.cur+3 == s.len || s.src[s.cur+3] != 'I' ||
    s.cur+4 == s.len || s.src[s.cur+4] != 'N' ||
    s.cur+5 == s.len || s.src[s.cur+5] != '\n')
    return -1; // Error
s.cur += 6;
```

Note that since the last bounds check `s.cur+5 == s.len` implies the previous ones, so we can drop them:

```c
if (s.cur+5 >= s.len
    || s.src[s.cur+0] != 'B'
    || s.src[s.cur+1] != 'E'
    || s.src[s.cur+2] != 'G'
    || s.src[s.cur+3] != 'I'
    || s.src[s.cur+4] != 'N'
    || s.src[s.cur+5] != '\n')
    return -1; // Error
s.cur += 6;
```

If we wanted to be nitpicky, doing bounds checks as `5 >= s.len - s.cur` would reduce the probability of overflowing.

<br />
<br />

Of course this type of check is only possible when the string we are looking for is known at compile time. We can write a more generic function using a for loop:

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
```

This function returns false if the string wasn't found at the cursor's position, while it returns true and advances the cursor otherwise.

<br />
<br />

We can use this function for the first and last tokens:

```c
Scanner s = { file_contents.ptr, file_contents.len, 0 };

if (!consume_str(&s, S("BEGIN\n")))
    return -1; // Error

// .. parse names ...

if (!consume_str(&s, S("END")))
    return -1; // Error
```

## Consuming Optional Sequences

```
BEGIN\n  Linus  Torvalds\n  Bill   Gates\n  Steve  Jobs\n  Mark   Facebook\nEND
       ^
       cur
```

In our example, every name is preceded by two spaces, but we would like this file format to be insensitive to spaces, which means we should also handle any number of spaces or no spaces at all. If there is some space before the element, we need to consume it. If there is no space, the cursor needs to stay still.

<br />
<br />

To consume a single optional character, we can use this if statement.

```c
if (s.cur < s.len && s.src[s.cur] == ' ')
    s.cur++;
```

If the cursor is at the end of the file or there is something other than a space, the cursor is left unchanged, just as we wanted. Since we want to consume an arbitrary number of spaces, we can turn this into a loop:

```c
while (s.cur < s.len && s.src[s.cur] == ' ')
    s.cur++;
```

This loop is useful whenever we want to consume an optional sequence of characters. We only need to define an expression that returns true for that sequence alone:

```c
while (s.cur < s.len && is_sequence(s.src[s.cur]))
    s.cur++;
```

## Consuming Required Sequences

```
BEGIN\n  Linus  Torvalds\n  Bill   Gates\n  Steve  Jobs\n  Mark   Facebook\nEND
         ^
         cur
```

Similarly to spaces, we can see a name as a sequence of letters. Unlike spaces, the name must be present in the file. The way we enforce this is by making sure that there is at least one letter character at the cursor's position. The remaining part of the name can be considered an optional sequence of letters:

```c
if (s.cur == s.len || !is_alpha(s.src[s.cur]))
    return -1; // Missing name
s.cur++;

while (s.cur < s.len && is_alpha(s.src[s.cur]))
    s.cur++;
```

```
BEGIN\n  Linus  Torvalds\n  Bill   Gates\n  Steve  Jobs\n  Mark   Facebook\nEND
              ^
              cur
```

## Extracting Substrings

We managed to identify the start and end position of the first name, but how do we extract that information? This is where our `String` type comes in handy.

<br />
<br />

If we relied on null-terminated strings, we would need to copy the name character by character while we are consuming it into a buffer in order to append a null byte after it. With our string type we don't need to do that. We can create substrings of larger strings by simply pointing inside them!

<br />
<br />

Say our cursor is pointing to the start of a token we are interested in:

```
BEGIN\n  Linus  Torvalds\n  Bill   Gates\n  Steve  Jobs\n  Mark   Facebook\nEND
         ^
         cur = off
```

We can save the first character's position in a second variable called `off` and then consume the token until the end:

```
BEGIN\n  Linus  Torvalds\n  Bill   Gates\n  Steve  Jobs\n  Mark   Facebook\nEND
         ^    ^ cur
         off
```

The pointer to the string "Linus" can be evaluated by adding `off` to the start of the source string, and its length as the new cursor position minus the old one.

```c
// ... consume stuff before the token we are interested in ...

int off = s.cur; // Save the index of the first character of the token

// ... consume the token ...

String name = { s.src + off, s.cur - off };

// ... continue parsing ...
```

This way we can effortlessly extract substrings by simply knowing where each token starts and ends!

## Parsing the Entire Line

You may have noticed that we have everything we need now. We managed to parse the first name of the first item and the spaces preceding it. The name is then followed by more spaces, then a surname, and then more spaces. We just need to apply the same patterns again and again.

<br />
<br />

Let's define a couple of helper functions:

```c
bool is_space(char c)
{
    return c == ' ';
}

bool is_alpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

void consume_spaces(Scanner *s)
{
    while (s->cur < s->len && is_space(s->src[s->cur]))
        s->cur++;
}

bool consume_word(Scanner *s)
{
    if (s->cur == s->len || !is_alpha(s->src[s->cur]))
        return false;
    s->cur++;

    while (s->cur < s->len && is_alpha(s->src[s->cur]))
        s->cur++;

    return true;
}
```

Parsing a line from the file becomes quite easy:

```c
consume_spaces(&s);

int name_off = s.cur;

if (!consume_word(&s))
    return -1; // Error

String name = { s.src + name_off, s.cur - name_off };

consume_spaces(&s);

int surname_off = s.cur;

if (!consume_word(&s))
    return -1;

String surname = { s.src + surname_off, s.cur - surname_off };

consume_spaces(&s);

if (s.cur == s.len || s.src[s.cur] != '\n')
    return -1;
s.cur++;

if (num_people == MAX_PEOPLE)
    return -1;
people[num_people++] = (Person) { name, surname };
```

And to parse the entire file we need to perform this sequence over and over until the END keyword is found.

## Putting Everything Together

And here is the final result! By following these patterns correctly, we can be quite sure that our parser is rock-solid! And we didn't need extra allocations or anything :D

```c
#include <stdio.h>
#include <stdbool.h>

typedef struct {
    char *ptr;
    int   len;
} String;

typedef struct {
    String name;
    String surname;
} Person;

typedef struct {
    char *src;
    int   len;
    int   cur;
} Scanner;

#define MAX_PEOPLE 100

#define S(X) (String) { (X), (int) sizeof(X)-1 }

bool eqstr(String s1, String s2)
{
    if (s1.len != s2.len)
        return false;
    for (int i = 0; i < s1.len; i++)
        if (s1.ptr[i] != s2.ptr[i])
            return false;
    return true;
}

bool is_space(char c)
{
    return c == ' ';
}

bool is_alpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

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

void consume_spaces(Scanner *s)
{
    while (s->cur < s->len && is_space(s->src[s->cur]))
        s->cur++;
}

bool consume_word(Scanner *s)
{
    if (s->cur == s->len || !is_alpha(s->src[s->cur]))
        return false;
    s->cur++;

    while (s->cur < s->len && is_alpha(s->src[s->cur]))
        s->cur++;

    return true;
}

int main(void)
{
    String file_contents = S("BEGIN\n  Linus Torvalds\n  Bill Gates\n  Steve Jobs\n  Mark Facebook\nEND");

    int num_people = 0;
    Person people[MAX_PEOPLE];

    Scanner s = { file_contents.ptr, file_contents.len, 0 };

    if (!consume_str(&s, S("BEGIN\n")))
        return -1; // Error

    for (;;) {

        if (consume_str(&s, S("END")))
            break;

        consume_spaces(&s);

        int name_off = s.cur;

        if (!consume_word(&s))
            return -1; // Error

        String name = { s.src + name_off, s.cur - name_off };

        consume_spaces(&s);

        int surname_off = s.cur;

        if (!consume_word(&s))
            return -1;

        String surname = { s.src + surname_off, s.cur - surname_off };

        consume_spaces(&s);

        if (s.cur == s.len || s.src[s.cur] != '\n')
            return -1;
        s.cur++;

        if (num_people == MAX_PEOPLE)
            return -1;
        people[num_people++] = (Person) { name, surname };
    }

    bool found = false;
    for (int i = 0; i < num_people; i++) {
        if (eqstr(people[i].name, S("Bill")) &&
            eqstr(people[i].surname, S("Gates"))) {
            found = true;
            break;
        }
    }
    if (found)
        printf("We found Bill Gates!\n");
    else
        printf("Not found\n");

    return 0;
}
```