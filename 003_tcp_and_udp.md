# Building web apps from scratch - TCP and UDP - Part 3

**NOTE: This post is still a work in progress!!**

## Layer 4: The Transport Layer

As mentioned earlier, L4 protocols work to add varying levels of reliability to the network and to address processes within a host. The most widespread transport protocol is the Transmission Control Protocol (TCP), which is used by the web. TCP solves the problem of packet loss and receiving packets out of order, making the network reliable from the point of view of L5 protocols. The way it works is by basically retrying until the message is received. In TCP there is no concept of a message. The data being transferred is treated as an ordered sequence of bytes. Nonetheless these bytes are sent over the networks in batches, which we call segments.

<br/>
<br/>

TODO: Explain the three-way handshake

<br/>
<br/>

When a host using TCP sends out some data, it always keeps a copy assuming it will need to send it again due to some error. When the peer receives the bytes, it sends an ACK (acknowledgement) back, letting the sender know that everything went ok. Upon receiving the ACK, the sender can discard the received bytes. If no ACK is received within a certain time frame, the segment is sent again. After a number of retries the peer is considered unreachable.

<br/>
<br/>

The ACK system can make communication quite inefficient since it causes the sender to pause after every segment that is sent, effectively doubling the latency. This effect can be greatly reduced with a technique called pipelining: assuming data won't be lost very often, segments can be sent out before receiving ACKs for previous ones. This one simple trick greatly increases throughput! 

<br/>
<br/>

At times, especially due to pipelining, it's possible for networks to get congested. This happens when too many packets are sent through a single router which can't keep up, causing the drops of some packets. To avoid this, TCP implements a congestion control mechanism. This consists of keeping track of the "congestion window", which is the maximum number of bytes that the network is able to handle at once. This is not a static value since it depends on the current load of the network, therefore TCP must infer it dynamically based on the behavior of the network. The congestion window value start small and is increased any time an ACK is received. When three ACKs are not received by the expected time window, a congestion is assumed and the congestion window is reduced. The specifics change based on the implemented algorithm.

<br/>
<br/>

The other features offered by TCP is flow control. Any time a host sends out a segment, it writes in the header how much free memory is available for receiving bytes. This value is usually referred to as "receive window". Each host only sends out segments that are smaller than the destination's receive window.

<br/>
<br/>

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
