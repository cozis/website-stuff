# Soft State Is the Best State

For some time I've noticed how much redundant state I use in my programs.

<br />
<br />

For instance, I may write something like this:
```c
typedef struct {
	char buffer[1024];
	int used;
	int unused;
} MyData;
```
There is a buffer and two variables describing how many bytes in it are used, and how many are not. This is where the spidey sense comes in. The number of used and unused bytes will always total 1024, and therefore one of the two variables is unnecessary as its value can be inferred by the other: `used = 1024 - unused` or `unused = 1024 - used`. In other words, one of the two variables is redundant.

<br />
<br />

Or let's take the case of a linked list:

```c
typedef struct Node Node;
struct Node {
	Node *next;
	int   value;
};

typedef struct {
	Node *head;
	int   count; // redundant
} List;
```

We have a list of nodes pointing at each other and a parent structure `List` with an additional element count. In this case the element count is redundant as it can be inferred by iterating over all nodes and counting them until the NULL next pointer is found.

<br />
<br />

Often we add redundancy to make our program more performant. This is quite intuitive from the linked list example. If we didn't store the counter, we would need to scan the list just to get the number of elements.

<br />
<br />

But there is also a disadvantage. By having redundant state we increase the chances that this state is incoherent, causing the program to get in a deeply broken state. Whenever we change some part of the state we need to make sure the changes are mirrored to all other redundant parts of the state. If we forget to do it in at least one place, our program is broken. In the context of the linked list example this means forgetting to increment or decrement the counter when we add or remove a node.

## Caching & Soft State

What I referred to until now as "state" can be referred to more specifically as "hard state", which is different from "soft state".

<br />
<br />

Soft state is state that is non-authoritative. In other words, it improves how the program works but isn't strictly necessary. An example of soft state are caches, which allow storing the results of computation to avoid computing them again. If for any reason these results are not valid anymore, the cache can simply be "invalidated".

<br />
<br />

Caches offer all the advantages of redundant hard state without the downsides. So what would happen if we stripped down our program's state from all redundancy and then added it back in the form of caches?

## Treating Redundancy as Soft State

My solution to this problem is to determine the minimum amount of hard state required by the program, and then determine the alternative representations need based on how the program's algorithm needs to interact with it.

<br />
<br />

These alternative representations are computed and cached. If the hard state is modified, the cached representations are invalidated and computed on request next time. This works well because it's easier to write a single function to create a copy of the state than implementing write methods on the state that mirror the same change in multiple representations.

<br />
<br />

Recalculating soft state each time is costly but greatly improves correctness. If the cost is too great, then changes to the authoritative state may be mirrored on the soft state when the performance benefits outweigh correctness.

<br />
<br />

In other words, you can implement redundancy as hard or soft state on a case by case basis. The point is being intentional about it. This approach offers a way to opt-out of complexity.

## Event Sourcing 

Since the authoritative state doesn't need a layout optimized for reading it, we can use a layout optimized for simplifying write operations.

<br />
<br />

The best layout depends on the specifics, but in general I would make the case for event sourcing, which consists in encoding state as a list of changes. With this setup, any changes to the state consists of appending a structure to a list.

<br />
<br />

To give you a practical example, imagine a database of account balances. You don't need to store the account balance of every account if you have a log of all transactions. The balance can be calculated by adding up the transaction transfer amounts.

<br />
<br />

Event sourcing has a number of benefits, other than simplifying updating the state. The ones I came to appreciate are:

1. By storing the log on a file you automatically get fault tolerant state since what is in memory is just soft state. When your application crashes and restarts it will recalculate the cache representation. This is called write-ahead logging (WAL).

2. You have a view of the application state in all points in time, with the potential of rolling back the state or changing the past entirely by inserting in the middle of the log. If the log changes, the soft state is simply recalculated.
