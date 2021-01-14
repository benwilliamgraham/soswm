# soswm

## The stack-of-stacks window manager.

soswm is a small dynamic window manager built around stacks.

## Installation

After cloning the repository, running `[sudo] make install` will compile and install soswm as well as adding it's desktop image.

Next, a startup script can be added to `~/.config/soswm/startup`.
When present, this script will be run during soswm's startup process and can be used to apply settings and launch other programs.

Finally, an X hotkey program (such as `sxhkd` or `xbindkeys`) can be used to add keybindings for soswm.

## Functionality

As the name suggests, soswm is built around stacks.

The stack behaves as expected, with a few helpful extra features.
Stacks can be manipulation as follows:
* Push: Add a new item onto TOS (top of stack)
* Pop: Remove an item from TOS
* Swap(n): Swap the position of the items at TOS and TOS+n
* Roll top: Take the TOS item and move it to BOS (bottom of stack)
* Roll bottom: Take the BOS item and move it to TOS
* Move: Take the TOS item from one stack and put it at the TOS of another

The inner stacks in soswm are the stacks of windows.
The window stack can be manipulated by all of the aformentioned functions, with pushing being indirectly done through the creation of a new X window, and popping occuring by killing an X window (either in-app or through the wm).

The outer layer of stacks is the stack of window stacks.
This stack can be manipulated in all of the ways mentioned with exception of moving between stacks. 

The active window is the TOS of the TOS stack from the stack-of-stacks.

There is a immutable stack of "monitors" created by splitting up the screen.
The window stacks are drawn in order on these "monitors", with any extra stacks being hidden.

Users can interact with soswm through the sosc (stack-of-stacks client) application.
It sends commands over Unix domain sockets to the window manager.

Commands exist in the form `sosc <action> <actor> [argument]`.

All existing command patterns are as follows:

```
sosc push stack
sosc pop <window | stack>
sosc swap <window | stack> <0...inf>
sosc roll <window | stack> <top | bottom>
sosc move window <0...inf>
sosc set gap <0...inf>
sosc split screen <WxH+X+Y> ...
sosc logout wm
sosc --help
```

They perform the following functions:

* `sosc push stack`: Push a new empty stack to the top of the sos
* `sosc pop window`: Pop the TOS window by sending a kill command
* `sosc pop stack`: Pop the TOS stack if empty
* `sosc swap window <n>`: Swap the TOS window and the window at TOS+n
* `sosc swap stack <n>`: Swap the TOS stack and the stack at TOS+n
* `sosc roll window top`: Take the TOS window and move it to BOS
* `sosc roll window bottom`: Take the BOS window and move it to TOS
* `sosc roll stack top`: Take the TOS stack and move it to BOS
* `sosc roll stack bottom`: Take the BOS stack and move it to TOS
* `sosc move window <n>`: Move the TOS window to the TOS+n stack
* `sosc set gap <n>`: Set the gap between windows in a stack to n pixels
* `sosc split screen <splits>`: Split the window into descending monitors described by the space-separated pattern `"<width>x<height>+<x-offset>+<y-offset> ..."`
* `sosc logout wm`: Exit the window manager
* `sosc --help`: Display the help message

## Acknowledgements:

Thanks to the following window managers for inspiration:
* dwm
* bspwm
* basic_wm
