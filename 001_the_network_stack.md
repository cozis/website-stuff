# Building web apps from scratch - Introduction to Networks - Part 1

Our web application will use the networking facilities implemented by the operating system to communicate with clients. These facilities, which are used by programs through the socket programming interface, offer a reliable service for the transmission of byte streams. Internally, the OS uses the TCP protocol and other lower level ones such as IP, ethernet, or wifi.

<br />
<br />

This makes things quite easy for us. Many things can go wrong on the network, causing data to be lost. The protocols implemented by the OS solve pretty much all of them. When using the socket interface we essentially hand bytes to the OS and tell where they should go, it handles the rest.

<br />
<br />

Unless we want to really fine-tune our server, knowing the details of the lower level protocols won't help us much. Even then, you may feel like you're missing an important part of the larger picture. That's why I decided to spend some time giving you a sense of what's happening under the hood. I'll go into how networks work with a strong bias towards what we need for our web app.

## The Network Stack

The Internet, or internets in general, are quite a complex beast! So much so that writing a simple explanation without it being overly simplistic is quite a challenge. But maybe that's not a problem: we just want a general framework for how our web app will interact with the external world.

<br />
<br />

The simplest possible network is a Local Area Network (LAN). Here, computers (hosts) are connected either directly or using devices such as busses or switches. The key characteristic is that all hosts can talk directly to each other. This makes LANs relatively reliable.

<br />
<br />

Multiple LANs can come together to form larger networks. This is done by using devices called routers to forward messages between them. Routers can be chained to make arbitrarily large networks. We could say the Internet (with an uppercase "I") is the largest of such networks. Note that in these larger networks it's not true anymore that hosts are directly connected, which introduces the problem of finding paths between hosts that want to communicate (we call this routing).

<br />
<br />

There are a lot of things that go into making networks work. A number of problems must be solved:
1. Multiple hosts may try to write on a single transmission channel (link) at the same time, causing a collision.
1. Determining the paths from one host to another to allow their communication (routing)
1. Make sure no circular paths are created, causing messages to be forwarded in a loop indefinitely
1. Links may break causing message loss or network partitioning (when a network is split in multiple parts)
1. Routers may be overloaded by network traffic causing some messages to be dropped
1. Hosts could trick other hosts into thinking they are someone else
1. Ensuring communication is private
and many others!

<br />
<br />

The solution to all of these problems is protocols: prescribed behaviors that, if followed by all hosts and intermediate devices, allow the network to work correctly. Even then, if we implemented a single protocol to tackle all these problems at once, it would be very complex. To manage this complexity we decided to implement many smaller protocols, each tackling a subset of these problems. More specifically, we use a layered approach.

<br />
<br />

Enter the stack of network protocols:
```
+----------------+
|   Application  | Layer 5
+----------------+
|    Transport   | Layer 4
+----------------+
|     Network    | Layer 3
+----------------+
|    Data-Link   | Layer 2
+----------------+
|    Hardware    | Layer 1
+----------------+
```
Each layer solves a specific set of problems. By adding them up we create the abstraction of a reliable network.

<br />
<br />

Our application sits in layer 5 (L5) and produces messages based on what it does for the user. Messages are passed to the lower layer which adds a header to the message. This header contains information required by the same protocol in other hosts for the correct handling of the message. The message is then forwarded to the lower layer which encapsulates further by adding its own header. The message goes down the stack being incapsulated recursively until it reaches the hardware and its bits are sent off. During transit over the network, intermediate devices (such as switches and routers) will unpack the message in reverse order to get the information necessary for forwarding it to its destination. When the message finally reaches the destination host, it is unpacked all the way up to the application layer, where the destination application is operating.