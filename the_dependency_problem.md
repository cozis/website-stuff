# The Dependency Problem

When talking to other C or C++ programmers, I often find we share the assumption that "dependencies are bad". But when I look at other more mainstream programming communities I find that this is a much less popular opinion. For this reason I decided to write down my thoughts on the topic of dependencies to either disprove my assumptions or offer a better framing of the problem.

## Benefits of Code Reuse

Before diving in, I think it's worth reminding ourselves why dependencies are good. They allow us to build upon each other's work, creating new value and improving society overall. I don't think anyone would seriously doubt that. When you read "dependencies are bad" online, that is just an expression of the intuition that many dependencies will degrade software quality.

## The Complexity Tax

So what's wrong with using other people's software? If I had to boil it down to one point, it's that it increases complexity, which is a pretty good proxy for software quality.

Complexity is introduced by the interface. When you use general-purpose libraries, you are adding complexity to your program. Of course this happens whenever your program grows, but code reuse comes with its own inherent costs. You are relying on general solutions designed to fit a number of use cases that you don't have, which add complexity that isn't required to solve your specific problem. 

Another source of complexity is introduced by having different parts of your program being maintained by different organizations. Your program may become incompatible with dependencies over time, they may introduce issues at distribution time if not bundled, they may even introduce vulnerabilities or be compromised altogether in an avoidable way if their development process isn't good.

These issues wouldn't normally be too problematic. After all, it's a compromise that could go either way based on the specific circumstances. The thing that tips the balance is how fast dependency graphs grow, causing the complexity to get worse and worse in what feels like an exponential way. 

## Dependency Graph Degradation

But why do dependency graphs grow so quickly?

Let's take a generic library A. Due to the benefits of code reuse, A's developer uses libraries B and C

```
  A
 / \
B   C
```

B and C also want the benefits of code reuse, so they add their own sets of dependencies:

```
      A
    /   \
   /     \
  B       C
 / \     / \
D   E   F   G
```

The developers of D, E, F, G also want to leverage code reuse by adding their own dependencies:

```
              A
            /   \
          /       \
        /           \
      B               C
    /   \           /   \
   /     \         /     \
  D       E       F       G
 / \     / \     / \     / \
H   I   J   K   L   M   N   O
```

In this example, the cost of dependencies grows exponentially as we add layers, doubling each time. But each node is only aware of what's beneath it, so they have no information of how much their dependencies are costing to the actual root program.  This is quite a simplistic example, but I think it shows the underlying principle that causes dependency graphs to grow.

Given this model the "dependencies are bad" heuristic makes more sense as you always design for the worst case.

## Conclusion

The root cause of this situation is assuming that code reuse has no downsides. The costs outweigh the benefits as projects grow larger. There needs to be a mindset shift from "I know a library for that" to first trying to solve problems yourself, and then looking for someone else's solution if necessary. If you do choose to add a dependency, you'll have a much better idea of what it's doing for you.