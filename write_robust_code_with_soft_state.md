# Write Robust Code With Soft State

While programming, I've noticed that the variables I use to hold state often feel redundant, since some variable values can be inferred from others. I began thinking of this as "redundancy" and developed an intuition for distinguishing between my program's "core state" and "redundant state".

<br />
<br />

Over time, I developed an approach to handle this redundancy that allows for more robust programs. I thought I'd share my intuitions here.

## Programs Have Redundant State

First of all, let me show what I mean by redundant state.

<br />
<br />

Consider this code:

```c
char buffer[100];
int used = 0;
int unused = 100;
```

The two variables count how many used and unused bytes are in the buffer. The buffer size is fixed, so the sum of the variable is always 100. This means each variable can be inferred from the other. For this reason I would call these variables "redundant" as you only need one. I would define as "core state" that which can't be inferred from other state. The core state in this example is the buffer and one of the two variables.

<br />
<br />

Now consider this code:

```c
typedef struct Node Node;
struct Node {
	Node *next;
	int value;
};

Node *list;
int   count;
```

It has a plain old linked list and a counter for the number of nodes. Here, the counter variable is redundant as it can be calculated by iterating over the nodes of the list.

<br />
<br />

We could define redundant state as that which can be computed from the remaining state, which I'm calling core state.

## The Function of Redundancy

I remember asking myself after realizing this: what would happen if I wrote programs with no redundant state at all?

<br />
<br />

I would expect very inefficient programs, but also very robust ones.

<br />
<br />

When you add redundant state to your codebase, you need to add logic to make sure it's always synchronized with the rest of the data. For instance, in the list scenario you need to increment and decrement the counter when the list is modified. Often we forget to add such logic, causing the system to break. If the derived state weren't there, this type of mistake would not be possible.

<br />
<br />

On the other hand, the reason we add redundancy is to avoid unnecessary computations. It's true that we can infer the number of nodes of a list by iterating over it, but a counter variable would avoid that entirely at the cost of incrementing and decrementing the counter when we modify the list. Of course there is the chance we forget to do that, but it's a good trade-off overall.

## Hard & Soft State

I introduced the concepts of "core state" versus "redundant state", but there is another characterization of state that comes in handy here: the difference between soft and hard state.

<br />
<br />

I would define hard state as the data necessary for the program to run, and soft state as that which is not necessary but improves performance. The prime example of soft state is caches, which are the unsung heroes of software architecture.

<br />
<br />

Going back to our initial discussion, both the core and derived state in our program are hard state. The redundant state is assumed to always be up to date with the core state. If not, the program is broken.

<br />
<br />

But there is no reason why the redundant state cannot be treated as soft state. It should be possible to design our program to use redundant state but not require it. This would allow us to use the redundancy when convenient, but then have the option to throw it away when hard to manage.

<br />
<br />

Let me show an example.

## Example

We are implementing a small social network where users can post articles and comment on them.


```c
typedef char Username[60];
typedef char Content[100];

typedef struct {
	int      id;
	Username author;
	Content  content;
	int      upvotes;
	int      downvotes;
} Comment;

typedef struct {
	Username author;
	Content  content;
	int      num_comments;
	Comment  comments[100];
	int      upvotes;
	int      downvotes;
} Post;

int next_comment_id = 0;
Post posts[100];

void upvote_post(Post *post)
{
	post->upvotes++;
}

void downvote_post(Post *post)
{
	post->downvotes++;
}

int add_comment(Post *post, Username author, Content content)
{
	Comment *comment = &post->comments[post->num_comments++];
	int id = init_comment(comment, author, content);
	return id;
}

void remove_comment(Post *post, int comment_id)
{
	int i = comment_by_id(post, comment_id);
	post->comments[i] = post->comments[--post->num_comments];
}

void upvote_comment(Post *post, int comment_id)
{
	int i = comment_by_id(post, comment_id);
	post->comments[i].upvotes++;
}

void downvote_comment(Post *post, int comment_id)
{
	int i = comment_by_id(post, comment_id);
	post->comments[i].downvotes++;
}
```

We later decide that instead of showing posts in chronological order, we want to score them and order them by their scores. Let's say this is the score function we settle upon:

```
post_score = (post_upvotes - post_downvotes) + 4 * comment_count + 2 * sum { (comment_upvotes[i] - comment_downvotes[i]) over i }
```

It involves a number of parameters, like upvotes, downvotes, comments.

## Hard State Solution

The hard state solution would be to add the score variable to our state and make sure that whenever one of the parameters it depends on changes, the score changes accordingly:

```c
typedef char Username[60];
typedef char Content[100];

typedef struct {
	// ...
} Comment;

typedef struct {
	// ...

	// New field
	int score;
} Post;

int next_comment_id = 0;
Post posts[100];

void upvote_post(Post *post)
{
	// ...

	// Update score
	post->score++;
}

void downvote_post(Post *post)
{
	// ...

	// Update score
	post->score--;
}

int add_comment(Post *post, Username author, Content content)
{
	// ...

	// Update score
	post->score += 4;

	return id;
}

void remove_comment(Post *post, int comment_id)
{
	// ...

	// Update score
	post->score -= 4;
	post->score -= 2 * post->comments[i].upvotes;
	post->score += 2 * post->comments[i].downvotes;
}

void upvote_comment(Post *post, int comment_id)
{
	// ...

	// Update score
	post->score += 2;
}

void downvote_comment(Post *post, int comment_id)
{
	// ...

	// Update score
	post->score -= 2;
}

int get_score(Post *post)
{
	return post->score;
}
```

If we forget to update the score or do it incorrectly, our program is essentially broken. If this were not an example, it would be even worse given the score function is something that changes often. I don't know about you, but this is nightmare fuel to me!

## Hard State But No Redundancy Solution

Now let's see what happens when we remove the redundant state entirely. Instead of keeping track of the score, we calculate it on the fly.

<br />
<br />

Our resultant program will be the same as the first version with no scoring, with the addition of this function:

```c
int get_score(Post *post)
{
	int score = post->upvotes - post->downvotes + 4 * post->num_comments;

	for (int i = 0; i < post->num_comments; i++) {
		score += 2 * post->comments[i].upvotes;
		score -= 2 * post->comments[i].downvotes;
	}

	return score;
}
```

Now all computation related to the score is in one function.

## Soft State Solution

Now let's see the soft state solution. This involves storing the score, but allowing the program to work without it.

```c
typedef struct {
	// ...
	int cached_score_value;
} Post;

int next_comment_id = 0;
Post posts[100];

void upvote_post(Post *post)
{
	// ...

	post->cached_score_value = -1;
}

void downvote_post(Post *post)
{
	// ...

	post->cached_score_value = -1;
}

int add_comment(Post *post, Username author, Content content)
{
	// ...

	post->cached_score_value = -1;
	return id;
}

void remove_comment(Post *post, int comment_id)
{
	// ...

	post->cached_score_value = -1;
}

void upvote_comment(Post *post, int comment_id)
{
	// ...

	post->cached_score_value = -1;
}

void downvote_comment(Post *post, int comment_id)
{
	// ...

	post->cached_score_value = -1;
}

int get_score(Post *post)
{
	if (post->cached_score_value == -1) {

		int score = post->upvotes - post->downvotes + 4 * post->num_comments;

		for (int i = 0; i < post->num_comments; i++) {
			score += 2 * post->comments[i].upvotes;
			score -= 2 * post->comments[i].downvotes;
		}

		if (score == -1)
			score = -2;

		post->cached_score_value = score;
	}

	return post->cached_score_value;
}
```

The `cached_score_value` variable holds the current score of the post or -1, which means the score is uncomputed.

<br />
<br />

Regardless of whether the score is computed or not, we can call `get_score` which will return the current score and cache it for next time.

<br />
<br />

All the places where the score would change just cancel the current score, causing it to be recomputed from scratch next time. This completely removes the cost of having to mirror the changes to the core state (the post and comments) to the redundant state (the score).

## Hybrid Approach

The hard state method optimizes for performance by avoiding the cost of computing the score as much as possible. The soft state technique pays the cost of extra computation to reduce complexity.

<br />
<br />

But the nice thing about this strategy is that you don't have to choose between performance and robustness: you can have an hybrid approach!

<br />
<br />

If keeping the redundant state up to date is easy, you can do that. No need to invalidate it unnecessarily. On the other hand if the derived state would be tricky to update, just invalidate it.

<br />
<br />

In conclusion, the soft state approach offers you a way to opt out of complex operations at the cost of extra computation.
