# An experience on interpreted numeric optimization (19/1/2025)


Hi all! This is [maniospas](https://github.com/maniospas), Blombly's creator.

<br>

In this development log I will describe my experience in
trying to optimize certain aspects of the language's interpreter.
Work based on the ideas described here started after Blomlby 1.16.0
and is still an ongoing effort. I think I'm at a good
point to give a summary of things I tried and what ended up
working for this codebase. 

!!! warning
    I will try to simplify terminology and considerations
    to make this broadly readable, but I am inevitably dealing
    with lower level concepts.

## Some context

It all started with me remembering that [German strings](https://cedardb.com/blog/german_strings/)
exist.

<br>

This is a memory model for
representing strings that contains a length and a pointer to 
a c-style array of characters. The clever part is that
short enough strings are stored directly in the pointer segment,
which lets them get accessed directly, without lookups at the "heap".
To grossly oversimplify: without needing to follow the pointer elsewhere and make things
more difficult for the memory cache.
Actually, such strings also keep track of a prefix anyway
to enable fast comparisons, but this is beside the point
right now.

```bash
German string memory layout: [len][prefix][ptr]
```

Blombly was already offering performant implementations for
operations like string concatenation, where differences with compiled languages 
were not as significant. This works
by gathering references to concatenated strings,
and running them all at once afterwards. 
So, no, I will NOT talk about string optimizations here, as operations
for strings are performant already...

<br> 

The problem with any interpreted language is that *numerical* operations are slow. 
Blombly did (and does) offer a `vector` data type to basically run C++ for scientific
computations, much like Python has *numpy* or GPU-based libraries.
But I still want generic arithmetics to be fast enough
to allow weaving in a couple of non-matrix stuff without feeling
as if you are waiting for Visual Studio to load a text file.
Techniques like creating computation trees do not help much with single nunmbers,
because keeping track of operations 
is slower than running them; a Just-in-Time (JIT) compiler on
runtime would help, but this is not what this post is about.

<br>

In first Blombly implementations, numbers were C++ objects 
that used polymorphism to change behavior for the same operations
between different types (databases, files, graphics, servers, etc).
To make the language's virtual machine easy to
modify during the first stages of language design,
my implementation was not even a proper polymorphic 
one (in that case, C++'s vtable lookups for functions would be
the only overhead). No, I had a polymorphic implementation of an 
`implement` method that used each operation's code to determine what 
to run.

<br>

Why am I saying all this? I was looking for a way to improve performance
to levels beyond what typical interpreted languages offer before JIT is 
injected; not only had I not done this, I was hovering to being
up to two times as slow as Python and didn't like it at all!
Sure, I could have proper polymorphic implementations with virtual functions, 
but I was not eager to try it for numeric operations
only to still end up being slow. Instead, I was looking for solutions that would
give me speedups in the orders of mangitude.

<br>

It was then that I remembered that German strings exist. 
I figured it would be nice if I could change all my pointers
(yes, I am doing raw pointers with reference counting being implemented
as part of the object class to remove approximately half the counting 
operations) to a type that either points to objects allocated in the heap
or numeric values stored in the stack as part of the pointer.
I am probably hardly the first person to realize that this concept can
also be used when implementing interpreters for programming languages.
It may even be standard in some C++ communities that I am not a part of
(the networking community maybe?). 
It was nonetheless a clever enough thought that I felt 
the spark of motivation to create such an implementation myself.

## Idea prototype

Since I already had a fully fleshed Blombly interpreter,
I set out to first test my idea with a simple proof-of-concept. 
I was looking for something that would keep up with C++ at the level of being several times slower,
but certainly not have the up to 300x slowdown that I was incurring (see below).
Now, the language also has a ton of other bottlenecks, including
simple function calls because it promotes a programming paradigm called
forward-oriented programming that is a pain to optimize for.
But I won't talk about any of those because I
either expect them to happen less frequently compared to normal code execution,
or follow different optimization paths intinsic to the language's semantics (perhaps
I will create a separate develog entry for those if I have something nice going on).
I will focus only on the journey of making numeric computations performant.

<br>

The first thing I did was to create a reference C++ implementation of 
some computations to have *something* to benchmark against. Benchmarks
are largely meaningless when you try to compare small differences between
languages, but certainly not when differences lie in the orders of
magnitude. Below is the first implementation in C++. Standard stuff,
where the only thing to not is sum is marked as `volatile` to prevent the compiler from making
several program-specific improvements; 
I want to compare against what I would like my language doing not
against what is the fastest possible way to run a specific program (if I wanted
a compiled language, I would have left out some features
and made a compiled language). I am compiling C++ with gcc and `-O3` optimizations, 
though I restrict them to `-O2` for Blombly's codebase because 
this is what I use for releases; differences are neglibile.

```cpp
#include <iostream>
#include <vector>
#include <chrono>
#include <cstdint>

int main() {
    const int64_t n = 1'000'000'000;

    auto start = std::chrono::high_resolution_clock::now();
    volatile int64_t sum = 0; // the same datatype as blombly ints
    int64_t i = 0;
    while(i<n) {sum += i;++i;}
    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> duration = end - start;
    std::cout << "Sum: " << sum << "\n"; // sanity check
    std::cout << "Time taken: " << duration.count() << " seconds\n";
    return 0;
}
```

!!! info 
    If you find my code style a bit weird, this is because
    I have started writing C++ for Blombly's code base with the
    same style guide I have for the language, which is very opinionated
    on conciseness as a way to improve maintainability - though maintainability
    is subjective.

This runs in a whooping *0.27 seconds* in my laptop, which is a
pretty good machine but not high-end.
 For sure, compilers and modern CPUs have progressed tremendously! 
But I refuse the temptation of going on a tangent on the evolution of computing, 
so let's leave it a that. 

<br>

Now, I started by creating the pointer structure I wanted, shown below. Since addresses, doubles, and 64-bit
integers all consume 64 bits, they can be stored at the pointer section interchangeably. And then a byte can be
a prefix to indicate the type; 
you can't go less than a byte because memory is aligned to chunks of whole bytes.
I also created a dummy datatype called `Data` to have the implementation be easily pluggable
to the interpreter's codebase where a namesake data structure exists.

```text
Blombly's memory pointers: [8bit type][64 bit data]
```

If first started with some definitions; I wanted to use bitmasks to detect types,
which makes it easy to add optional properties (e.g., versioning?) and type unions
instead of certain types in the future, for example to mark final numbers
or check for multiple types at once if need arises. I am aware the modern C++ prefers
other directives instead of defining stuff, but for some reason (hint: skill issues) 
it felt like I would create hard-to-debug errors while using them so I didn't in this case.
In the past, I have devolved to using generics when following "proper" practice, 
and I wanted to avoid this in this already complex project.

<br>

Also find the implementation of the pointer struct below;
this has various constructors and type casting mechanisms based on different data types
and uses `std::bit_cast<double>` to forcefully consider the bit representation as that of other data.
To keep things safe, I did not implement proper type casting, so as to avoid unexpected
type casting of pointer outcomes when I switch implementations later.


<details>
<summary>
Definitions.
</summary>

```cpp
#include <iostream>
#include <cstdint>
#include <vector>
#include <chrono>
#include <bit>
#include <cstdint>

// DATTYPETYPE controls memory overhead for each pointer 
// vs how many types can be added (char - so up to 8 types is the minimum)
#define DATTYPETYPE char
#define IS_FLOAT static_cast<DATTYPETYPE>(1)
#define IS_INT static_cast<DATTYPETYPE>(2)
#define IS_BOOL static_cast<DATTYPETYPE>(4)
#define IS_PTR static_cast<DATTYPETYPE>(8)
#define IS_NOT_FLOAT ~static_cast<DATTYPETYPE>(1)
#define IS_NOT_INT ~static_cast<DATTYPETYPE>(2)
#define IS_NOT_BOOL ~static_cast<DATTYPETYPE>(4)
#define IS_NOT_PTR ~static_cast<DATTYPETYPE>(8)
```
</details>

<details>
<summary>
First implementation of a DataPtr struct. All methods marked volatile to allow a volatile sum for proper comparison.
</summary>

```cpp
class Data;
struct DataPtr {
    DataPtr(double data) noexcept : data(std::bit_cast<int64_t>(data)), datatype(IS_FLOAT) {}
    DataPtr(Data* data) noexcept : data(std::bit_cast<int64_t>(data)), datatype(IS_PTR) {}
    DataPtr(int64_t data) noexcept : data(data), datatype(IS_INT) {}
    DataPtr(int data) noexcept : data(static_cast<int64_t>(data)), datatype(IS_INT) {}
    DataPtr(bool data) noexcept : data(static_cast<int64_t>(data)), datatype(IS_BOOL) {}
    DataPtr(DataPtr&& other) noexcept : data(other.data), datatype(other.datatype) {
        other.data = 0;
        other.datatype = IS_PTR;
    }
    DataPtr(const DataPtr& other) : data(other.data), datatype(other.datatype) {}
    DataPtr& operator=(Data* other) {
        data = (std::bit_cast<int64_t>(other);
        datatype = IS_PTR;
    }
    DataPtr& operator=(const DataPtr& other) {
        if (this != &other) {
            data = other.data;
            datatype = other.datatype;
        }
        return *this;
    }
    DataPtr& operator=(DataPtr&& other) noexcept {
        if (this != &other) {
            data = other.data;
            datatype = other.datatype;
            other.data = 0;
            other.datatype = IS_PTR;
        }
        return *this;
    }
    DataPtr() {}

    Data* operator->() const volatile {
        if (datatype & IS_NOT_PTR) {
            // will convert to blombly error handling
            std::cerr << "This is not a Data type\n"; 
            return nullptr;
        }
        return std::bit_cast<Data*>(data);
    }

    double tofloat() const volatile {
        if (datatype & IS_PTR) return this->tofloat();
        if (datatype & IS_NOT_FLOAT) return data;
        return std::bit_cast<double>(data);
    }

    int64_t toint() const volatile {
        if (datatype & IS_PTR) return this->toint();
        if (datatype & IS_FLOAT) return std::bit_cast<double>(data);
        return data;
    }

    bool tobool() const volatile {
        if (datatype & IS_PTR) return this->tobool();
        return data;
    }

    inline double unsafe_tofloat() const volatile { return std::bit_cast<double>(data); }
    inline int64_t unsafe_toint() const volatile { return data; }
    inline bool unsafe_tobool() const volatile { return data; }

    inline bool isfloat() const volatile { return datatype & IS_FLOAT; }
    inline bool isint() const volatile { return datatype & IS_INT; }
    inline bool isbool() const volatile { return datatype & IS_BOOL; }
    inline bool isptr() const volatile { return datatype & IS_PTR; }

private:
    DATTYPETYPE datatype;
    int64_t data;
};



class Data {
public:
    Data() {}
    double tofloat() {return 0;}
    int64_t toint() {return 0;}
    bool tobool() {return false;}
    DataPtr add(const DataPtr& other) {return DataPtr(0.0);}
    DataPtr lt(const DataPtr& other) {return DataPtr(false);}
};
```

</details>

Finally the first main implementation. At first this was very performant, until
I realized that I was assuming the type stored in `sum`, the index increment,
and the loop termination check. So I properly implemented all of those in a manner
similar to what I thought blombly would do in practice.
I am hiding the implementation because it just follows the idea of the first main.


<details>
<summary>
First prototype's benchmark.
</summary>

```cpp
int main() {
    const DataPtr n = (1'000'000'000);
    std::cout << "Sanity check on number of elements (you never know with typecasts)" << n.toint() << "\n";

    auto start = std::chrono::high_resolution_clock::now();
    DataPtr sum(0);
    DataPtr value(0);
    DataPtr one(1);
    volatile int64_t reps = 0;
    while(true) {
        DataPtr comparison;
        if(value.isfloat()) comparison = DataPtr(value.unsafe_tofloat()<n.tofloat());
        else if(value.isint()) comparison = DataPtr(value.unsafe_toint()<n.toint());
        else if(value.isptr()) comparison = value->lt(n);
        else std::cout << "invalid operation\n";

        if(!comparison.tobool()) break;

        if(sum.isfloat()) sum = DataPtr(sum.unsafe_tofloat()+value.tofloat());
        else if(sum.isint()) sum = DataPtr(sum.unsafe_toint()+value.toint());
        else if(sum.isptr()) sum = sum->add(value);
        else std::cout << "invalid operation\n";

        if(value.isfloat()) sum = DataPtr(value.unsafe_tofloat()+one.tofloat());
        else if(value.isint()) value = DataPtr(value.unsafe_toint()+one.toint());
        else if(value.isptr()) value = value->add(one);
        else std::cout << "invalid operation\n";
    }
    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> duration = end - start;
    std::cout << "Sum: " << sum.toint() << "\n";
    std::cout << "Time taken: " << duration.count() << " seconds\n";
    return 0;
}
```

</details>

Now, how well are we doing? I compared initial stats of this loop
across C++ with volatile, the prototype, Python 3.12, and Blombly 1.16.0
(note: I have nice-sounding versions, but the language is still maleable and 
not ready for production). Results are from one run, but relative
deviations across multiple runs where always less than 10%.
At any rate, I just needed something to convince myself, not a fully
fledged study.


| Description                              | Time   |
|------------------------------------------|--------|
| Running time with blombly v1.16.0        | 88 sec |
| Direct equivalent in Python 3.12.7       | 43 sec |
| Direct equivalent in C++ (With volatile) | 0.27 sec |
| Prototype (with volatile everywhere)     | 2.4 sec|

Wow, this looks very promising!!! The prototype is some times slower than C++, which is expected, but twice as fast as
normal interpretation. It also uses some additional memory per number/pointer, in particular +25%. But actually I
was already storing the number type and memory counter, which I no longer need to do, so for number I *reduced* required
memory to nearly half. Coming back to speed, improvements are seen only when compiling, so we need to keep that in mind.
For example, this would be the simplest JIT imaginable,
but not actual interpretaion. Still, I felt that there was something there, and decided to proceed (always on a new git
branch first in case I cause irrecoverable damage to the code base with changes). By the way, measuring time
for Blombly looks like this:

```java
tic = time();
sum = 0;
n = 1000000000;
while(i in range(n)) sum += i;
print(time()-tic); // verified with perf too
print(n);
```

## Integrating the prototype

Any apprentice wizard can seem impressive if they point to their teacher's accomplishments.
But the question is, you know, if they can actually do magic.

<br>

So I added an implementation similar to the one above to Blombly's interpreter, changing a
bunch of stuff here and there. And, much like the apprentice that was still a farmer last week,
it didn't go well for creating magic. Of course everything broke in tests, but that was expected 
because I was focusing on only numeric computations so I could focus on a specific hotpath. Let's
say that this is my apprentice version, or version A for short. It *increased* running time by 10%.

<br>

Hm? What's that? I added the pointer mechanism but *completely forgot to actually change numeric
computations to the new system*. You know, I forgot to do the thing that was the whole point! 
To my defense, this happened
because I had gotten happy with some added safety features I could add, such as creating proper errors
for dereferencing null pointers instead of getting segfaults (more on error checking later).
But version A is not completely useless in that it let me know to expect a 10% slowdown at worst as the
cost of my new choices. Fine by me; maybe it could grow into an actual wizard after all!

<br>

Ok, so then I actually made the substitution and got immediate improvements. Let's call those version
B. But then I could also have some low-hanging improvements with returning `DataPtr&` and setting those
on `const DataPtr&` to tell the compiler to reference the same object instead of moving a bunch of stuff
around. These changes are only meaningful for the new model, which needs to move a character and a number
around instead of only a number. Let's call this last version C, which happens to pair well with *const*. 
Totally by clever writing on my part (not). So now I had the following numbers.


| Description                              | Time   |
|------------------------------------------|--------|
| Blombly v1.16.0                          | 88 sec |
| A - worst case demonstration             | 99 sec |
| B - get numbers from address segment     | 77 sec |
| C - low hanging optimizations            | 66 sec |

I saw some nice improvements, sure, but ... what a bummer! 
This was nowhere around the levels I was expecting; I was expecting the outcome to be around the 
30 second mark to be honest. Now, I usually have a very good intuition for stuff like this, which made me 
question why I was not getting the benefits I was hoping for. Being off by 20%-30% is ok in my book, but
when such a difference happens I always try to understand exactly what went wrong to see if I can accept it or
fix it. After all, I could measure
the amount of operations, including memory accesses, and had looked at the assembly of the original (this is standard stuff for C++ development,
but I did not show it to avoid boring you). But I had not looked at the Blombly assembly because, well
I just couldn't ask the [compiler explorer](https://godbolt.org/) to replicate the full breadth of interconnected
computations.

<br>

That last sentence, mainly the *interconnected computation* part, should have tipped me off right there. 
But here I was trying to understand what was going on and execution remained persistently slow. 
Puzzlyingly, trying a bunch of stuff to emulate specific hotpaths saw no improvement. It was then that
I had a growing suspicion. Remember how one of the benefits of German strings is that they avoid cache misses?
Yeah, I had forgotten too.

<br>

Cache is a special fast memory that your preprocessor preloads data into as your program runs. Well, the
preloading gathers a bunch of nearby area segments to the stuff you actually access, with the hopes that
you will be using those too. But if you hop around memory too
much you will cause misses and actual loading of stuff from memory at tens or hundreds of times the delay. 
Caches have levels too, going from L1-L3. CPU registers get data from L1, which prefetches from L2,
which prefetches from L3. This is typically shared across cores and fetches data from memory. Are we good?
Because we are about to measure what happens there when measuring with 1/100 the number of iterations.


```bash
sudo perf stat -e cache-misses,cache-references ./build/blombly playground/test.bb
```

```text
           3122924      cpu_atom/cache-misses/           #   25,53% of all cache refs           (0,08%)
            852007      cpu_core/cache-misses/           #   63,75% of all cache refs           (99,92%)
          12231044      cpu_atom/cache-references/                                              (0,08%)
           1336474      cpu_core/cache-references/                                              (99,92%)

       0,710353165 seconds time elapsed

       0,690228000 seconds user
       0,020006000 seconds sys
```

Yeah, not good. Just ignore everything and look at cpu_core/cache-misses, which is the L3 cache.
This is the main bottleneck, at the speed ranges are looking at (as testament, the first line's
missing percentage varies wildly). 63,75% of memory accesses are misses! To put it in perspective,
most applications should not exceed 15%-20%. Or so I am told.


## Reducing cache misses


So, what actually happened is that, to accommodate the dynamic inlining of code blocks at runtime that
Blombly offers, I was using neither the normal stack-based mechanism of most programming language
to maintain values, nor an emulation of registers.
No, my smarta** was using hashmaps (aka dictionaries) that look up values based on an integer encoding
of their names. I was not a complete fool and was using an implementation optimized for minimizing cache misses. 
But still this is an issue when you want to make millions of reads and writes because your data 
get all over the place. 

<br>


I made some improvements here and there, including in the way I was storing data in my memory contexts, 
having contiguous memory for variable identifiers that were close together (hey! my own L...4 cache) 
and a bit more clever use of const DataPtr& (did not have exceptional improvements, but they all accumulated to something).
It was not dramatic, but I started getting outcomes at the 40 seconds mark.

```text
           2127092      cpu_atom/cache-misses/           #   17,45% of all cache refs           (0,08%)
            881803      cpu_core/cache-misses/           #   50,97% of all cache refs           (99,92%)
          12190757      cpu_atom/cache-references/                                              (0,08%)
           1729940      cpu_core/cache-references/                                              (99,92%)

       4,099552958 seconds time elapsed

       4,078725000 seconds user
       0,012999000 seconds sys
```


At this point I was getting a bit desperate to bring optimization under 40 seconds consistently. 
It was like a challenge from the code to myself, if you will. So I did the oldest trick in the book and assigned 
a common outcome to one variable at the point where I was calculating if optimizations can occur. Apparently it worked, 
because I shaved off a whole second! Still don't know why the compiler was not doing the same optimization. 
Maybe I am trusting it too much




## Switching to dynamic dispatch

But, wait! I was doing minor optimization where I could be doing some big stuff. Like... redesigning the whole interpreter!

<br>

Now, it's not as if this was impossible to do earlier, but I completely lacked the confidence to do it. See, I had made the decision
to not use shared pointers because they were *very* slow and was doing reference counting manually, or rather semi-automatically
by manually tracking scope additions but also keeping pointers alive when
being returned as function results with the `Result` class below. However, changing any core segment of the language had a 
significant risk to it in that breaking (e.g,. mis-referencing) any of the raw pointers used everywhere would create segfaults 
that would be very hard to debug. 


<details>
<summary>
Result class to delay decreasing reference counts; all operation outcomes are wrapped in this, and it is assigned
to variables at the other end to be destroyed only at the end of using code.
</summary>

```cpp
class Result {
private:
    DataPtr data;

public:
    explicit Result(DataPtr data) noexcept;
    explicit Result(Result& other) noexcept;
    Result(Result&& other) noexcept;
    ~Result();
    const DataPtr& get() const;
    Result& operator=(const Result& other);
    Result& operator=(Result&& other) noexcept;
};
```

```cpp
Result::Result(DataPtr data) noexcept : data(data) {data.existsAddOwner();}
Result::Result(Result& other) noexcept : data(other.data) {data.existsAddOwner();}
Result::Result(Result&& other) noexcept : data(std::move(other.data)) {other.data = DataPtr::NULLP;}
Result::~Result() {data.existsRemoveFromOwner();}
const DataPtr& Result::get() const {return data;}

Result& Result::operator=(const Result& other) {
    if (this != &other) {
        auto prevData = data;
        data = other.data;
        data.existsAddOwner();
        prevData.existsRemoveFromOwner();
    }
    return *this;
}

Result& Result::operator=(Result&& other) noexcept {
    if (this != &other) {
        data.existsRemoveFromOwner();
        data = std::move(other.data);
        other.data = DataPtr::NULLP;
    }
    return *this;
}
```

</details>

But now I had whole pointer management class wrapping all memory references! Now I could do type and `nullptr` checks.
This added approximately 5% overhead, but I am fine with it and also added a mechanism to disable it if I want to - I still
haven't decided on whether to keep it in during public release.
Ok, my safety mechanisms do not account for use-after-release, but I was confident in my other code (the `Result` class mostly) 
in preventing those. Instead, I was worried about accidentally accessing the wrong stuff without noticing it.
But the new pointer type not only has type checks but, by merit of having only one out of the datatypes being a pointer, makes
it most likely that accidentally accessing garbage memory will just not return a struct when I am expecting it
to and would thus create a Blombly internal error that I can work with without even needing a debugger (for example, in optimized code!). 
In summary, it would make it very easy to reveal and work with bugs. 

<br>

This actually happened more than ten times and counting already, and I am very happy with how things
turned out; the migration never crashes unexpectedly even under the most extreme memory mismanagement conditions
that arise when calling parts of the codebase that have not been updated to the recent pointer model yet.

<br>

But I digress. The main point here is that I now had enough safety to make sweeping changes to the execution model. The first
thing I did was add dynamic dispatch, which refers to goto statements that change
where they are going programmatically.
Apparently, these are [much more performant than switch statements](https://eli.thegreenplace.net/2012/07/12/computed-goto-for-efficient-dispatch-tables). 
I tried it and indeed I saw drastic improvements. Now the same loop, which after all previous optimizations needed **44 seconds** to conclude, now run in
 **31-34 seconds** (depending on the run and whether sefery features are enabled). At this stage, I had properly overcome
 Python's speed, which I was comparing to for sanity. But it was still a far cry off from the C++'s target.


## Conclusion

I shudder to think what awful bugs await function call orchestration - I'm not running any other tests as I'm making sweeping changes to the codebase, because now is the time for rewrite and then I can pick up the ashes and fix things afterwards... The good news were that throughout the whole process I gained full control of my pointers, so I can add and remove safety checks on demand - in fact I created some #ifdef guards that enable check only if I have the respective definition.

<br>

Bonus points that, since the stuff I'm doing are very sensitive to small errors, I was at the stage where I couldn't use chatgpt to even give me small snippets of code other than basically googling commands. Being in the full driver's seat was a nice refresher that nothing beats the security of carefully hand-crafted code. My codebase is generally a mess from using LLMs a tad too much -this is a side project after all!- but at least the main dispatch loop is a piece of work (at least for my sense of aesthetics in this point in time). The fun part is that used blombly's own opinionated style on c++ and it really helped with the verbosity; I even used macros to emulate code inlining to an extend!

