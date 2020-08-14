#!/usr/bin/python3

from Xlib import X, XK, display, xobject
from Xlib.ext import randr
from typing import Any, Callable, Dict, List, Optional
import subprocess
import sys


class Stack:
    """Stack base class."""

    def __init__(self) -> None:
        self.items: List[Any] = []

    def peak(self, n: int = 0) -> Any:
        """View the item at TOS+n."""
        return self.items[-1 - n]

    def push(self, item: Any) -> None:
        """Add a new item onto TOS."""
        self.items.append(item)

    def pop(self) -> Any:
        """Remove an item from TOS."""
        return self.items.pop()

    def swap(self, n: int) -> None:
        """Swap the position of the items at TOS and TOS+n."""
        self.items[-1 - n], self.items[-1] = self.items[-1], self.items[-1 - n]

    def roll_l(self) -> None:
        """Take the item at TOS and move it to the bottom of the stack."""
        self.items = self.items[-1:] + self.items[:-1]

    def roll_r(self) -> None:
        """Take the item at the bottom of the stack and move it to TOS."""
        self.items = self.items[:1] + self.items[1:]


class MonitorStack(Stack):
    """Stack of monitors."""

    def __init__(self) -> None:
        super().__init__()


class WorkspaceStack(Stack):
    """Stack of workspaces."""

    def __init__(self) -> None:
        super().__init__()


class WindowStack(Stack):
    """Stack of windows."""

    def __init__(self) -> None:
        super().__init__()


class Monitor:
    """Monitor class."""

    def __init__(self, x, y, width, height):
        self.x = x
        self.y = y
        self.width = width
        self.height = height


class Workspace:
    """Workspace class."""

    def __init__(self):
        self.windows: WindowStack = WindowStack()


class Window:
    """Window class."""

    def __init__(self, x_window: xobject.drawable.Window):
        self.x_window: xobject.drawable.Window = x_window

    def transform(
        self, x: int, y: int, width: Optional[int] = None, height: Optional[int] = None
    ):
        if width is None and height is None:
            self.x_window.move_window(x, y)
        else:
            self.x_window.move_resize_window(
                x,
                y,
                width if width is not None else self.x_window.width,
                height if height is not None else self.x_window.height,
            )


class _Chord:
    """Chord class."""

    def __init__(self, keycode, modcode):
        self.keycode = keycode
        self.modcode = modcode

    def __eq__(self, other) -> bool:
        return self.keycode == other.keycode and self.modcode == other.modcode

    def __hash__(self) -> int:
        return hash((self.modcode, self.keycode))

    def grab(self):
        """Capture key when pressed."""
        _ROOT.grab_key(
            self.keycode, self.modcode, True, X.GrabModeAsync, X.GrabModeAsync
        )

    def ungrab(self):
        """Don't capture key when pressed."""
        _ROOT.ungrab_key(self.keycode, self.modcode)


class KeyChord(_Chord):
    """Key class."""

    def __init__(self, key: Optional[int], *modifiers: int):
        modcode = 0
        for modifier in modifiers:
            modcode |= modifier
        super().__init__(_DISPLAY.keysym_to_keycode(key), modcode)


def update():
    """Update root window, capture monitors, and load config."""
    global _ROOT, _MONITORS

    # capture and setup root
    _ROOT = _DISPLAY.screen().root
    _ROOT.set_wm_name("soswm")
    _ROOT.change_attributes(
        event_mask=X.SubstructureNotifyMask | X.SubstructureRedirectMask
    )

    # capture monitors
    # TODO
    _MONITORS = MonitorStack()
    tmp_mons = [[1920, 0, 1920, 1080], [0, 0, 1920, 1080]]
    for monitor in tmp_mons:
        _MONITORS.push(Monitor(monitor[0], monitor[1], monitor[2], monitor[3]))

    # load config
    try:
        exec(open("config.py").read(), globals())
    except Exception as e:
        print(f"Error while loading config file: {e}", file=sys.stderr)


def grab_keys(keys):
    """Clear existing keys and grab new keys."""
    global _KEYMAP

    # clear existing keys
    for key in _KEYMAP.keys():
        key.ungrab()

    # save and grab new keys
    _KEYMAP = keys
    for key in _KEYMAP.keys():
        key.grab()


def _run():
    while True:
        if _DISPLAY.pending_events():
            event = _DISPLAY.next_event()
            try:
                {
                    X.ConfigureRequest: _configure_request,
                    X.KeyPress: _key_press,
                    X.MapRequest: _map_request,
                    X.UnmapNotify: _unmap_notify,
                }[event.type](event)
            except KeyError:
                pass


def _configure_request(event: display.event.ConfigureRequest):
    print(event)


def _key_press(event: display.event.KeyPress):
    try:
        chord = _Chord(event.detail, event.state)
        _KEYMAP[chord]()
    except KeyError:
        print(f"Unknown keychord: {chord}", file=sys.stderr)


def _map_request(event: display.event.MapRequest):
    print(event)
    event.window.map()
    print(dir(event.window))


def _unmap_notify(event: display.event.UnmapNotify):
    print(event)


def launch(cmd, *args):
    """Run a system command."""
    try:
        subprocess.Popen([cmd, *args])
    except Exception as e:
        print(f"Error launching application: {e}", file=sys.stderr)


def kill():
    """Kill the active window."""


def logout():
    """Log out of the WM."""

_DISPLAY: display.Display
_ROOT: xobject.drawable.Window
_MONITORS: MonitorStack
_WORKSPACES: WorkspaceStack
_KEYMAP: Dict[_Chord, Callable]

if __name__ == "__main__":
    try:
        _DISPLAY = display.Display()
    except Exception as e:
        print(f"Error while connecting to display: {e}")
    _KEYMAP = {}
    _WORKSPACES = WorkspaceStack()
    update()
    _run()
