# Building web apps from scratch - Getting Started - Part 0

In the past few years, I had some fun studying how web apps are built from the ground up. It's been a great journey, so I decided to share what I learned by making a series of posts showing how one can build a web app from scratch. We will build a simple social network featuring posting articles and following other users. If you follow along, by the end you'll have a fully functional website ready for real users!

<br/>
<br/>

This is the first time undertaking such an ambitious project, so I welcome any feedback :)

<br/>
<br/>

We'll be using the C programming language for everything, since it's low level enough to do things efficiently and not very opinionated. The libraries we will be using:
* [BearSSL](https://bearssl.org/) for encryption
* [SQLite](https://www.sqlite.org/) as database
but we can worry about that later.

## Prerequisites

To follow along, you will need:
* A good grasp of C programming
* An idea of how HTTP works: what is a request, a response, and their general syntax.

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

## Roadmap

We will start by building a **blocking web server** with the **socket interface** provided by the operating system. We will then add a **database** and implement the first version of our application. You could host this version online, but only as a test, since there will be **no encryption**. Then, we will reorganize our server to make it **non-blocking**, greatly increasing how many requests the server can handle at a given time. Finally, we will add encryption.

<br/>
<br/>

But before all of that, I decided to offer an overview of how networks work. This will give you a sense of what the OS is doing for us. So in the [next post](001_the_network_stack.html) we will go over the general structure of the internet and the protocols it uses.