# Inheritance and Polymorphism in Plain C

People often say C is not an object-oriented language because it's missing inheritance and polymorphism, but this is not quite accurate! I would argue that OOP is more about how you structure a solution than what language features you use. You can create the abstraction of objects in C just as well as other languages like C++ and Java, with a bit of extra work.

<br />
<br />

In this post, I want to show how inheritance and polymorphism can be achieved in plain C. Since C has no syntax sugar for this, we will need to build things from scratch using plain old structures and functions. This will have the side-effect of showing how effortlessly dynamic languages work under the hood.

# Inheritance in C++

In case you haven't been on Linkedin recently, inheritance is a way to declare types as extensions of other types. Consider this C++ code:

```c++
// base class
struct Shape {

    Color color;

    void  setColor(Color color_) { color = color_; }
};

// derived class
struct Square : Shape {

    float side;

    float calculateArea() { return side * side; }
};
```

we declared two types: `Shape` and `Square`. The `Shape` object has a color and a method to change it. The `Square` has a side variable and a method to calculate its area. Since we want the `Square` to have a color just as any other shape, we define it to be an extension of the `Shape` type (this is what the `Square : Shape` notation means). The `Square` implicitly inherits all fields and methods of the base type as its own. For instance, you could do:

```c++
int main()
{
    Square square;
    square.setColor(RED);
}
```

The `setColor` method was declared on `Shape` and not `Square`, but it was inherited.

<br />
<br />

Any code implemented for `Shape` will also work on `Square`s now. Say you have a function for drawing a shape:
```c++
void draw(Shape*);

int main()
{
    Square *square = createSquare();

    draw(square);
}
```

The `draw` function was defined to work on a `Shape`, which means it will also work on types derived from it. This simplifies code reuse quite a bit!

<br />
<br />

Now let's peek under the hood to understand how this works. Whenever an object inherits from another, all its fields are implicitly inserted before that type's regular fields:

```c++
// base class
struct Shape {

    Color color;

    void  setColor(Color color_) { color = color_; }
};

// derived class
struct Square : Shape {

    // Color color; <----- Implicit field inserted by the compiler

    float side;

    float calculateArea() { return side * side; }
};
```

This means that the first half of any derived type will match the base class:

```
+---------------------------------+ Square
|+-------+ Shape                  |
||       |                        |
||       |                        |
||       |                        |
|+-------+                        |
+---------------------------------+
 ^
 Address of the Square
```

This implies that any pointer to a `Square` is also a valid pointer to `Shape` at the same time, therefore casting `Square*` to `Shape*` (which is called an "upcast") is a safe operation. The C++ compiler performs this cast implicitly for us, which allows for seamless reuse of code defined on base types for its derived types.

<br />
<br />

The inverse operation of casting base type pointers to derived type pointers (downcast) is generally unsafe. It's not possible to determine without extra information that the base type pointer came original from an upcast of the same derived type.

```c++

struct Shape {

    Color color;

    void  setColor(Color color_) { color = color_; }
};

struct Square : Shape {

    float side;

    float calculateArea() { return side * side; }
};

struct Triangle : Shape {

    float base;
    float height;

    float calculateArea() { return base * height / 2; }
};

int main()
{
    Square   *square   = createSquare();
    Triangle *triangle = createTriangle();

    Shape *shapeA = square;   // upcast
    Shape *shapeB = triangle; // upcast

    Square *downcastA = (Square*) shapeA; // downcast (OK)
    Square *downcastB = (Square*) shapeB; // downcast (ERROR!)
}
```
Ignoring how the `shapeA` and `shapeB` pointers were obtained, the last two lines are identical from the compiler's perspective, which is why it's not possible to detect downcast errors without extra information.

# Inheritance in C

Inheritance really is just a glorified version of composition, which means it's quite easy to replicate in C:

```c
typedef struct {
    Color color;
} Shape;

typedef struct {
    Shape base;
    float side;
} Square;

void draw(Shape *shape);
```

We just need to explicitly add the base type's fields at the start of the derived type. If we do so, all pointer cast consideration we made for C++ apply. The only exception is that the compiler has no notion of upcasts so we'll need to make them explicit:

```c
int main(void)
{
    Square *square = createSquare();
    draw((Shape*) square); // explicit upcast
}
```

Type methods can be written as regular functions that take the object's pointer as first argument:

```c
void Shape_setColor(Shape *shape, Color color)
{
    shape->color = color;
}

float Square_calculateArea(Square *square)
{
    return square->side * square->side;
}
```

# Polymorphism in C++

The inheritance technique we just saw is completely resolved at compile-time. Any time a method is called on an object, the compiler knows exactly which method will be executed based on that object pointer's type. This is obvious from the C code but may be less clear in C++. Consider the following code:

```c++
struct Animal {
    void talk() { printf("yayaya\n"); }
};

struct Cat : Animal {
    void talk() { printf("meow\n"); }
};

int main()
{
    // Create a cat object
    Cat cat;

    // Convert it to an animal object
    Animal *animal = &cat;

    // Run talk() from Animal*
    animal->talk();
}
```

What does this program print? Since inheritance is resolved at compile-time, the `talk()` method is resolved based on the current type of the object, which is `Animal`.

<br />
<br />

Polymorphism allows us to change this behavior. If `Animal` were polymorphic, the `talk()` method would not be called based on the current pointer type, but the original type of the object. 

```c++
struct Animal {
    virtual void talk() { printf("yayaya\n"); }
};

struct Cat : Animal {
    virtual void talk() { printf("meow\n"); }
};

int main()
{
    // Create a cat object
    Cat cat;

    // Convert it to an animal object
    Animal *animal = &cat;

    // Run talk() from Animal*
    animal->talk();
}
```

If we mark the methods as `virtual`, the `Animal*` object will hold internally some information about its original type, which allows the program to call the right method regardless of what is the current type of the pointer.

<br />
<br />

But how does this work under the hood? Let's replicate this behavior using C.

# Polymorphism in C

The way we implement this is by adding to the base type a list of function pointers, one per method. These pointers will refer to the current implementation that should be used when calling a method on the base type. Derived types will inherit the function pointers alongside other base type fields and upon initialization set them to their own implementation. When the derived types are upcased, the base type object will still hold the pointers to the implementations of the derived type.

```c
typedef struct {
    void (*talk)(void);
} Animal;

void talk(Animal *animal)
{
    animal->talk();
}

////////////////////////////////////////

typedef struct {
    Animal base;
} Cat;

void catTalk(void)
{
    printf("Meow\n");
}

Animal *makeCat(void)
{
    Cat *cat = malloc(sizeof(Cat));
    cat->base.talk = catTalk;
    return (Animal*) cat; // upcast
}

////////////////////////////////////////

typedef struct {
    Animal base;
} Dog;

void dogTalk(void)
{
    printf("Bau\n");
}

Animal *makeDog(void)
{
    Dog *dog = malloc(sizeof(Dog));
    dog->base.talk = dogTalk;
    return (Animal*) dog;
}

////////////////////////////////////////

int main(void)
{
    Animal *a = makeCat();
    Animal *b = makeDog();

    talk(a); // Meow
    talk(b); // Bau
}
```

Even though both objects have the `Animal` type, the `talk` operation operates based on the original type. This matches quite closely how C++ does things. One optimization they introduce is the "vtable". Object of the same types will always have the function pointers set to the same implementations, so it makes sense to introduce a global table of implementation associated to each derived type and keep a pointer to that in the object's header.

# Final Note

If you are anything like me, you will take this new found knowledge and make every `struct` of your program polymorphic. That is all fun and good, but it does introduce quite a bit of complexity. In my experience I found that the benefits are rarely worth the cost. I found this [video](https://www.youtube.com/watch?v=tD5NrevFtbU&ab_channel=MollyRocket) on the topic quite fascinating!

<br />
<br />

If you enjoy this type of discussion consider hopping in my [discord server](https://discord.gg/vCKkCWceYP). We talk about this sort of stuff all the time.

<br />
<br />

Thanks for reading :)