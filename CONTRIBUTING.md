# Contribution Guidelines

Everyone is welcome to:

- Report Blombly bugs or -at this stage- behavior that you think is unexpected. Do so in GitHub issues.
- I am also open to opinions about language design. Again, open issues so that we can discuss things.
- Create pull requests for improving the documentation in `docs/`.
- Create pull requests for more testing in `tests/`.
- Create pull requests for more core library features in `libs/`.

Minor edits (typo fixes, etc) will be accepted but will not be acknowledged as contributors in the readme. You will still be listed by GitHub though.

## Actively seeking help

I am actively seeking help on how to enforce GCC compilation in Windows when MSVC is the default compiler. I think I a missunderstanding how to use vcpkg from cmake and cause linking errors; definitely a skill issue but I don't have enough time to acquire the necessary skills. As I have reached a steady release pipeline, I am focusing on other areas of language improvement.

## Open source, closed contributions

I am currently *not* open to practices that are well meaning but carry the risk of injecting code that is unprotected by the language's built-in safety features. This would require QA from my part that I presently do not have the time to perform. Mainly:

- Do not open requests with non-trivial source code changes. Please get in touch with me first at maniospas@hotmail.com if you are interested in contributing to the code base. 
