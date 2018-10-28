## Issues

(GitHub does not allow forked repositories to flag new issues, so we're making this as a to-do/issue list.)

### Known Issues:

- Daedalus currently does not detect if it is running on a Vita or not. This causes crashes with async audio.
- Some games do not save correctly, especially ROM hacks.
-

### To-Do:

- Add PS Vita detection. If running on a Vita, make Daedalus ignore the Media Engine PRX - as this is not present on Vita and will simply crash. 
- Vita optimization: if Media Engine is not available on Vita, then is there a way to interact with the the Vita's native hardware?
- Static recompilation! Recompile instructions in advance and give them to the PSP at runtime, saving the dynarec a lot of work and in theory bringing big speedups (Mario 64 ran perfectly on a 166MHz Pentium 3 with static recompilation...)
- Mac: Get the latest version of Daedalus compiling and working.
- Fix stutter issues related to the dynarec.

Note that this list is not conclusive and simply exists to flag new issues and keep a to-do list to fix them.
