# Building web apps from scratch - Getting Started - Part 0

I've always been fascinated by how web applications work. After spending the past few years diving how they are build, I thought it would be fun to share what I've learned! In this series of posts, we'll build a social network-like website from scratch. Think of it as a journey where we'll explore how all the pieces fit together. By the end, you'll have a fully functional website ready for real users!

<br/>
<br/>

This is the first time undertaking such an ambitious project, so I welcome any feedback :)

<br/>
<br/>

We'll be using the C programming language for everything, since it's low level enough to do things efficiently and not very opinionated. The libraries we will use are:
* [BearSSL](https://bearssl.org/) for encryption
* [SQLite](https://www.sqlite.org/) as database
but we can worry about that later.

## Prerequisites

To follow along, you will need:
* A good grasp of C programming: there is a lot to cover and I won't be able to delve into the details of everything, not at first at least. I plan on accompanying the main posts with secondary posts explaining the things I had to gloss over.
* You should have an idea of how HTTP works: what is a request, a response, and their general syntex.

## Environment Setup

Our setup won't be very fancy. We will use a regular code editor to write our code and a terminal to build it. All of the code will work on both Linux and Windows.

<br/>
<br/>

On Linux, you must install:
* `gcc` and `make` for building our application
* `gdb`, `valgrind`, and `strace` for debugging

On Windows, you have two options:
1. Use WSL to simulate a Linux environment
1. Go native
WSL is harder to set up, but you will have access to the tools I listed above. Any considerations made for Linux will also apply to you.

<br/>
<br/>

If you still want to go native, then you should download [w64devkit](https://github.com/skeeto/w64devkit), which is is an unix-like environment for Windows built by the awesome author of [nullprogram.com](https://nullprogram.com/). It contains `gcc`, `gdb`, `make` and some other utilities.

## The Roadmap

The project will be divided 3 parts:
* **Section 1**: we will create a simplified yet complete version of our application. This will include a **blocking web server**, the **database**, and **no encryption**.
* **Section 2**: we will improve our web server by making it **non-blocking**.
* **Section 3**: we will add **encryption**.