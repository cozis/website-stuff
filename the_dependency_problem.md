# The Dependency Problem

When talking to other C or C++ programmers, I often find we share the assumption that "dependencies are bad". This doesn't seem to be the case in more mainstream programming communities where this isn't something people think about much. So I decided to take the time and reason about what's happening here. Are dependencies a problem? And if so, how does this issue come to be?

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

In this example, the cost of dependencies grows exponentially as we add layers, doubling each time. But each node is only aware of what's beneath it, so they have no information of how much their dependencies are costing to the actual root program.  This is quite a simplistic and pessimistic example as nodes are often shared, but I think it shows the underlying principle that causes dependency graphs to grow.

Given this model, the "dependencies are bad" heuristic makes more sense as you always design for the worst case.

## Why the Divide?

But why do people in different communities have so different perceptions of the situation?

I think programmers in the C and C++ communities are much more exposed to the problem due to how costly each dependency is and that there is no form of dependency encapsulation. When you use a library, you need to learn about its dependencies and manually add them to the build process.

On the other hand, the Python and Javascript communities have tools for that which greatly reduce the management cost. This makes adding dependencies to your project nearly effortless and the low resistance route whenever a problem comes up. As the software ecosystem grows, the network of dependencies does too, so that when things go wrong, [they go really wrong](https://en.wikipedia.org/wiki/Npm_left-pad_incident).

## C and C++ Programmers Are Wrong

If tools like package managers make the problem worse, does it mean the good way is the C and C++ way?

I'm not sure about that!

Dependency management in C and C++ is problematic, but I think that the perception of the cost doesn't come from the intrinsic cost but by the fact that packages don't have a uniform interface. In other words, the C and C++ way consists of keeping packages hard to install to avoid things like dependency chain attacks. That definitely works, but isn't the optimal solution either.

## Javascript and Python Programmers Are Wrong Too

Okay. So package manager ARE good?

Not quite!

Package managers do provide uniform interfaces for installation and management, but have the problem of discounting transitive dependencies by implementing a form of dependency encapsulation. In other words, when you ask `npm` to install a package, it will implicitly install all transitive dependencies and pretty much hide it from you.

## A Better Way

So basically C and C++ programmers are fighting the wrong war, while Javascript and Python programmers aren't even aware they are in one.

Now that I successfully put myself on everyone's bad side, I wonder: what is the better way?

Generally speaking, we should find ways to remind ourselves of what dependencies are costing us, both in terms of complexity and how much we are exposing ourselves to risks of other organizations failing to delivering what's expected.

A good starting point is the package manager. I'm convinced a good package manager should:
* Implement a uniform package interface for easily installing and updating dependencies
* Not discount transitive dependencies by treating them as direct dependencies (users should need to manually install dependencies at all levels of indirection)
* Be transparent and remind how much code each package is adding

## Final Thoughts

Ultimately, this article is about making better software. While tools and practices matter, the most impactful factor is the individual's ability to write good code and make sound decisions. We rarely discuss this because there's no overnight solution. Building skills requires time.

But how we spend that time matters a lot. Avoiding dependencies has the side-effect of making us grow as we try to solve hard problems, fail, and learn in the process. The software ecosystem is such that same problem often presents itself multiple times in different flavours. So as time goes by, we tend to be in a much better spot than last time.

So, in conclusion, the dependency problem isn't just about complexity, but also about us growing as programmers.

## Join the Discussion!
Have questions or feedback for me? Feel free to pop in my discord

[Join the Discord Server](https://discord.gg/vCKkCWceYP)