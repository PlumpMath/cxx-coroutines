This is a personal experiment with home-brew coroutines in C++.

The target use for this project is in an embedded ARM platform, without
operating system or anything like that, which is why some things are
solved with static allocations instead of things like std::vector.

This is based on `Simon Tatham’s Duff’s-device based coroutines
<http://www.chiark.greenend.org.uk/~sgtatham/coroutines.html>`_.

Have fun poking around.
