# Building web apps from scratch - Ethernet and IP - Part 2

In this post we'll continue our peek into the operating system's network stack! We will talk about the data-link and network layers, where the ethernet and IP protocols are located.

## Layer 2: The Data-Link Layer
We call layer 2 the data-link layer. Protocols at this layer, such as ethernet or wifi, manage the communication within LANs. Here messages are referred to as frames, which have a relatively small size that depends on the technology being used. For instance most ethernet frames are 1518 bytes.

<br />
<br />

As we mentioned in the previous post, hosts of a LAN are essentially directly connected with one another (one hop away, as they say). This greatly simplifies the problem of sending messages to their destinations. 

Each host is assigned a MAC address, a 6 byte code which uniquely identifies it. Frames are sent with the MAC address of the destination host in their header. All hosts of the network that read the frame inspect the header and only pick it up if the MAC address matches their own.

## Collisions and Exponential Backoff

One problem that is solved at this layer is collisions. It is possible for hosts to send frames at the same time on a single link, causing them to get all messed up. Hosts must have a way to detect and avoid/recover from such events. This is a fascinating topic, since it requires the hosts to synchronize without being able to communicate.

<br />
<br />

Ethernet solves collisions with exponential backoff, essentially relying on randomness. When a host sends a frame, it also listens to what is being sent. If what is read doesn't match, a collision occurred and it stops sending. The colliding hosts wait a random time before trying again, within a certain limit (of course being random each host will likely wait a different time). If a host collides a second time, it doubles the time limit it's allowed to wait and chooses a random time again. This goes on until no collision occurs. This process may go on indefinitely, but in practice is resolves quickly. With each collision the probability of collision goes down exponentially.

## Ethernet Frames

The format of a frame depends on the L2 protocol, but in general they have header, payload, and footer. Ethernet frames look like this:
```
+-----------------+-----------------+------+-------------------------------------------+------+
| Destination MAC | Source MAC      | type | ... payload ...                           | CRC  |
+-----------------+-----------------+------+-------------------------------------------+------+
```
The first bytes store the MAC of the destination host and the MAC of the source host. Each MAC is 6 bytes long. The type field describes which L3 protocol is contained by the payload. Common L3 protocols are ARP (which is more like a 2.5 protocol), IPv4, and IPv6. The CRC field is a 4 byte hash that allows detecting when the contents of the frame have been changed by some error. If you're unfamiliar with hashes, it is like a summary of the rest of the frame. If the receiver of a frame sees that the summary does not match the frame, something has gone wrong.

## Layer 3: The Network Layer

This layer manages communication between LANs, which is usually divided in two parts:

1. determining routes from hosts of one network to the other (routing)
1. forwarding the messages along those paths

Routing works by having routers communicate to map the network. As application writers this is not very relevant to us, so we'll focus on forwarding. The protocol that implements forwarding is the all-mighty Internet Protocol (IP), which has two declinations: version 4 and 6. The IP protocol implements its own way to address hosts. Instead of MAC addresses, it uses IP addresses. In time, we also started using IP addresses within local networks.

## IP addresses

IPv4 addresses are 32 bits long, and are usually expressed as text using the dotted-decimal notation. The 32 bits are divided into 4 groups of 8, each written in base 10 and separated by dots. Here's an example:
```
168.192.0.2
```
I'm sure you're familiar with these!

<br />
<br />

Since IPv4s use 32 bits, there are around 4'000'000'000 different addresses. At the time IP was being designed, people thought this was going to be more than enough! Turns out, we need many more! Which brings us to the problem of [address exhaustion](https://en.wikipedia.org/wiki/IPv4_address_exhaustion). Around 2010, the last batch of IPv4 addresses was given out. Since then we have managed fine by reusing IPv4 addresses aggressively. The IPv6 protocol was largely introduced to resolve the address exhaustion problem.

<br />
<br />

IPv6 addresses are 128 bits long. Note that the number of addresses grows exponentially with the number of bits of the address. There are so many IPv6 addresses that we can chill for a long time!

<br />
<br />

An IPv6 address looks like this:
```
2001:0db8:0001:0000:0000:0ab9:C0A8:0102
```
the 128 bits are divided into 8 groups of 16. Each group is expressed in base 16 and separated by a colon. When there is a long string of zeros you can omit them by writing `::`
```
2001:0db8:0001::0ab9:C0A8:0102 -> 2001:0db8:0001:0000:0000:0ab9:C0A8:0102

2001:db8::  -> 2001:db8:0000:0000:0000:0000:0000:0000

::1234:5678 -> 0000:0000:0000:0000:0000:0000:1234:5678

::          -> (all zeros)
```

## The Internet Protocol

For this series, I decided to stick with IPv4 since it's the most familiar. IP messages are called packets.

This is how an IP packet looks like:
```
0           1           2           3           4
+-----+-----+-----+-----+-----+-----+-----+-----+
| ver | IHL |    TOS    |         length        |
+-----+-----+-----+-----+-----+-----+-----+-----+
|        packet ID      | flg |     offset      |
+-----+-----+-----+-----+-----+-----+-----+-----+
|    TTL    |   proto   |       checksum        |
+-----+-----+-----+-----+-----+-----+-----+-----+
|              source IP address                |
+-----+-----+-----+-----+-----+-----+-----+-----+
|            destination IP address             |
+-----+-----+-----+-----+-----+-----+-----+-----+
|                                               |
|                                               |
|                ... payload ...                |
|                                               |
|                                               |
+-----+-----+-----+-----+-----+-----+-----+-----+

(each row is 4 bytes long)
```

The most important fields are the source and destination addresses, which allow routers to forward packets across the network.

<br />
<br />

The `length` field contains the length in bytes of the payload. This value is encoded in 16 bits, which allows packets sizes up to 65535 bytes. If you recall, L2 frames are usually much smaller than that.

<br />
<br />

The `packet ID` identifies the packet relative to the host that sent it. Hosts usually keep a global counter they use to generate these IDs.

<br />
<br />

The `TTL` (Time To Live) field is how many times this packet is allowed to be forwarded by routers. When a router forwards a packet, it decreases the TTL. Packets with a TTL of 0 are discarded. This avoids packets from transiting indefinitely in the network when routing loops are formed.

<br />
<br />

The `proto` field contains the ID of the L4 protocol in its payload.

<br />
<br />

The `checksum` contains a hash of the packet headers. This helps to detect when transmission errors occur. When a router or the destination host detect that the checksum is compatible with the packet, they discard it.

## IP Fragmentation

If you recall the section on L2 protocols, we said that ethernet frames are about 1518 bytes. That is much smaller than the maximum length of an IP packet. So how does it work when an IP packet doesn't fit in the L2 frame?

<br />
<br />

This problem is solved by IP fragmentation: a mechanism to split large packets into smaller ones. When the fragmented packets reach the destination, they are assembled back. This is helpful when first sending out the packet, but also when transiting a network since networks use many low level transmission technologies with varying frame sizes.

<br />
<br />

To allow reassembling, the receiver of a fragmented packet needs to know:
1. If a packet is a fragment and, if it is, which larger packet it belongs to
1. Their order
1. When all fragments were received

When a packet is fragmented: all the fragments share the larger packet's ID, the `offset` fields is set to the number of bytes contained by the previous fragments, and all fragments have the `MF` (more fragments) flag set except the last one. With this information the receiver is able to successfully reassemble the original packet.

<br />
<br />

Fragmentation isn't very popular. Host always try to produce IP packets that fit in single frames, and routers prefer to drop them if forwarding isn't possible. This behavior in routers can be forced by setting the `DF` (don't fragment) flag.

## What's next
In the [next posts](003_tcp.html), we'll learn about the transport protocols, with a focus on TCP.

## Join the Discussion!
Have questions or feedback for me? Feel free to pop in my discord

<br />
<br />

[Join the Discord Server](https://discord.gg/vCKkCWceYP)