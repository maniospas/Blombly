# Language Design

## Vision

Here is a summary of Blombly's design and philosophy going forward.
The main point is that the language should stay simple, with emphasis on easily
reusing code without wrestling with its definition scope to get closure right.

<br>

Those interested should be able to read the syntax almost immediately after
going through the basics; hopefully, one afternoon is enough to
get into the language. Implementation details
are also automated by the virtual machine, and you only need to
focus on writting algorithms. Finally, the language addresses the most common
development needs with a few simple interfaces.

<br>

At heart, I want to create a comprehensive language that is easy to pick up
and fun to work with for small and mid-sized projects. I hope it's to your liking. 😊


## Promises

In the spirit of guiding Blombly's evolution, there are a couple of points 
that I promise to respect. 
In case it matters legally, I do not consider any of these binding.
So you will have to take me at my word that I will stick to them to the
best of my ability, and according to my intent to make a good language.

1. Blombly will never introduce a feature that requires understanding 
    implementation details. If non-algorithmic decisions need to
    be made, "good enough" options and automation will be selected. 
    If an existing feature's design crystalizes into something that includes
    implementation details, those will be removed until only the algorithmic
    part remains.

    *The only exception to this rule will be numeric vectors 
    - simply because the gains are too great to ignore.*

- Safety and stability come above everything else. Any feature that is
    found to lead to undefined or unsafe behavior will be promptly patched,
    regardless of what might depend on it. That is, write Blombly according to
    its specification. Request for fixes or certain behavior in GitHub.
    Moreover, anyone executing a Blombly program controls
    how it affects their machine. If new attack surfaces get discovered,
    I will be adding more restrictions to the virtual machine and your
    users/main files may need to add additional permissions to re-enable 
    certain behavior.

- Backwards compatibility comes next, though. The syntax of version 2.0.0
    (expected to be reached withing 2025) will remain mostly stable.
    Barring unforeseen events, the syntax of version 3.0.0 (expected
    to be reached at late 2026) will remain backwards compatible
    barring safety issues. **Compiled bbvm programs are always 
    forward compatibe.** Furthermore, there will be very few new language 
    features other than support for
    more types of IO. There is a chance that some features 
    will be deprecated aefore 3.0.0 if I find ways to merge them
    with other syntax while maintaining expressiveness.

- The standard library (`libs/`) will only be boilerplate code,
    and it will always be simple to implement similar functionality yourself. 
    *Its only features that you need to use
    to keep code simple are the `in` `default` `=>` `assert` macros.*
    These are covered extensively in the user guide and can be basically
    considered part of the language. They are macros because I am trying to move as much
    stuff I can outside the virtual machine to give people the option to
    tinker with the language and create their own flavors.
