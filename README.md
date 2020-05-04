# soswm

## The stack-of-stacks window manager.

soswm is a small and fast dynamic window manager.

## Installation

Run `[sudo] make clean install` to clean, compile, and install soswm.

## Functionality

As the name suggests, soswm is built around stacks.

Rather than having named workspaces, soswm uses a stack of workspaces, with the
active workspace as the TOS (top-of-stack). Each workspace contains a stack of
windows, with the active window as the TOS. Furthermore, being a dynamic window
manager, soswm chooses the location of all windows and keeps the active
workspace and window in the same location for maximum efficiency. Rather than
moving between windows/workspaces, the active window and workspaces revolve
around you.

Both the window and workspace stack supports several operations to reorder the
stack.

* Push: Add a new item onto TOS
* Pop: Remove an item from TOS 
* Swap(n): Swap the positions of the items at TOS and TOS+n
* Roll left: Take the item at TOS and move it to the bottom of the stack
* Roll right: Take the item at the bottom of the stack and move it to TOS
* Move: Take the TOS item from one stack and put it at the TOS of another

Monitors are also stored on a stack that can be similarly manipulated to set
the active monitor.

Workspaces have further functionality to handle the way in which windows are
drawn. A fullscreen in which only the active window for the given workspace can
be enabled. Also, for cases where there are more than one window in a workspace,
the windows will be drawn with the active window to the left, and each
successive window taking up a ratio of the remaining screen space. This ratio
can be ajusted by 'shrinking' or 'growing' a workspace, with the default value
existing in the configuration. Finally, the gaps between windows can be changed.

## Configuration

The file `config.c` contains the ability to configure the default programs,
keybinds and graphical settings for soswm. Edit the file and re-install. 
