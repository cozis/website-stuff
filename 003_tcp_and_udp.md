# Building web apps from scratch - TCP and UDP - Part 3

It this post, we'll talk about the all mighty TCP! The protocol which powers the web and more. Knowing how TCP works really is a superpower.

<br/>
<br/>

**NOTE: This post is still a work in progress!!**

## Layer 4: The Transport Layer

As mentioned earlier, L4 protocols work to add varying levels of reliability to the network and to address processes within a host. The most widespread transport protocol is the Transmission Control Protocol (TCP), which is used by the web. TCP solves the problem of packet loss and receiving packets out of order, creating the abstraction of a reliable network from the point of view of applications. The way it works is by basically retrying until the message is received. In TCP there is no concept of a message. The data being transferred is treated as an ordered sequence of bytes. Nonetheless these bytes are sent over the networks in batches, which we call segments. The other L4 protocol is UDP, which is very minimal in the guarantees it offers. You can think of it as a L4 version of the IP protocol with the addition of ports and a checksum over the content (the IP checksum only covers its header). This protocol is very useful when the application already manages communication unreliability internally, or it just doesn't care. Sometimes it's better to have less packets reaching the destination on time than having all of them reach it a bit late. The web uses mostly TCP, so we will put UDP on the side and focus on TCP.

## The Three-way Handshake

For hosts to talk using TCP, a connection needs to be established. In other words, each host must make sure its peer is open to the exchange. To do this, TCP uses a process called the three-way handshake. The host initiating the connection sends a segment with the SYN flag set which we call SYN segment. Upon receiving a SYN segment, a host decides if it's open to communicate (is a process listening on this port? does the peer look malicios?). If it is, it replies with a segment with both the SYN and ACK flags set. The ACK flag tells the peer that the host is sending the SYN segment as a response to the first SYN segment. Upon receiving the SYN+ACK segment, the first host response with an ACK segment which concludes the handshake. Once the handshake is concluded, hosts can send segments with the actual data. As we will see in a bit, TCP uses the notion of a sequence number to keep track of which data was received. TCP's handshake allows each host to tell its peer what is their first sequence number.

<br/>
<br/>

Although the handshake almost always requires three steps

```
Host A                       Host B
  |                            |
  | ---------- SYN ----------> |
  |                            |
  | <------- SYN+ACK --------- |
  |                            |
  | ---------- ACK ----------> |
  |                            |
```

it doesn't need to be that way. The way the handshake is defined naturally allows simultaneous SYN segments. In fact, you could have a four-hand handshake:

```
Host A                       Host B
  |                            |
  | ---------- SYN ----------> |
  |                            |
  | <--------- SYN ----------- |
  |                            |
  | <--------- ACK ----------- |
  |                            |
  | ---------- ACK ----------> |
  |                            |
```

such a wonderful design! We will look deeper into the handshake once we've peeked into TCP's segment structure. It will make much more sense!

## Retransmission

When a host using TCP sends out some data, it always keeps a copy assuming it will need to send it again due to some error. When the peer receives the bytes, it sends an ACK (acknowledgement) back, letting the sender know that everything went ok. Upon receiving the ACK, the sender can discard the received bytes. If no ACK is received within a certain time frame, the segment is sent again. After a number of retries the peer is considered unreachable.

<br/>
<br/>

The ACK system can make communication quite inefficient since it causes the sender to pause after every segment that is sent, effectively doubling the latency. This effect can be greatly reduced with a technique called pipelining: assuming data won't be lost very often, segments can be sent out before receiving ACKs for previous ones. This one simple trick greatly increases throughput! 

## Congestion Control

At times, especially due to pipelining, it's possible for networks to get congested. This happens when too many packets are sent through a single router which can't keep up, causing the drops of some packets. To avoid this, TCP implements a congestion control mechanism. This consists of keeping track of the "congestion window", which is the maximum number of bytes that the network is able to handle at once. This is not a static value since it depends on the current load of the network, therefore TCP must infer it dynamically based on the behavior of the network. The congestion window value start small and is increased any time an ACK is received. When three ACKs are not received by the expected time window, a congestion is assumed and the congestion window is reduced. The specifics change based on the implemented algorithm.

## Flow Control

The other features offered by TCP is flow control. Any time a host sends out a segment, it writes in the header how much free memory is available for receiving bytes. This value is usually referred to as "receive window". Each host only sends out segments that are smaller than the destination's receive window.

## The Four-Way Termination

Terminating a TCP connection is a similar process to creating one, where we use FIN segments instead of SYN segments.

When a host decides it will not send more data over the connection, it sends a segment with the FIN flag set. Upon receiving a FIN segment, a host responds with an ACK segment. As for regular data transmission, when no ACK is received, the FIN segment is sent again.

Sending a FIN segment mean that the host is only willing to listen for incoming data, not sending it. The peer is free to send data if it hasn't finished.

When the second host has no more data to send, it too sends a FIN segment, expecting an ACK for it.

When both peers have received each other's FIN segment and ACKs, the connection is closed. Of course, peers could just stop responding to each other, but this is not considered a graceful termination as the connection's resources will only be freed after a timeout bas been expired.

The four-hand termination looks like this:

```
Host A                       Host B
  |                            |
  | ---------- FIN ----------> |
  |                            |
  | <--------- ACK ----------- |
  |                            |
  | <----- optional data ----- |
  |                            |
  | <--------- FIN ----------- |
  |                            |
  | ---------- ACK ----------> |
  |                            |
```

but similarly to handshakes, it can be expressed as a three-way termination:

```
Host A                       Host B
  |                            |
  | ---------- FIN ----------> |
  |                            |
  | <------- FIN+ACK --------- |
  |                            |
  | ---------- ACK ----------> |
  |                            |
```

## TCP Segment Layout

This is the layout of a TCP segment:
```
0           1           2           3           4
+-----+-----+-----+-----+-----+-----+-----+-----+
|      source port      |    destination port   |
+-----+-----+-----+-----+-----+-----+-----+-----+
|                sequence number                |
+-----+-----+-----+-----+-----+-----+-----+-----+
|             acknowledgement number            |
+-----+-----+-----+-----+-----+-----+-----+-----+
|   offset  |   flags   |      window size      |
+-----+-----+-----+-----+-----+-----+-----+-----+
|        checksum       |     urgent pointer    |
+-----+-----+-----+-----+-----+-----+-----+-----+
|                                               |
|                                               |
|                ... payload ...                |
|                                               |
|                                               |
+-----+-----+-----+-----+-----+-----+-----+-----+
```
The port fields allow the addressing of specific processes within a host. When a TCP connection is initiated, the process must specify the port it is sending it from and the port it must reach of the receiving host. 

<br/>
<br/>

The `sequence number` identifies which bytes are being sent in the stream. You can think of it as the index of the segment's first byte in the stream, except the index doesn't start at zero but at a random number for security purposes.

<br/>
<br/>

The `acknowledgement number` is the next sequence number that is expected from the peer. Lets say a host receives a segment of size 100 with sequence number of 450. The next segment sent by the host will contain an `acknowledgement number` of `100 + 450 = 550`. When the ACK flag is set, this tells the peer what we are ACK-ing. This also shows that the ACKs are cumulative. If we receive multiple segments one after the other, we can just ACK all of them at once by sending the latest `acknowledgement number`. This feature plays well with pipelining.

<br/>
<br/>

The `window size` field is receive window used for flow control.

<br/>
<br/>

TODO: Talk again about the three-way handshake, four-way termination, and ghost bytes in reference to the actual fields