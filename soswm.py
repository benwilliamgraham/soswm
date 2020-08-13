#!/usr/bin/python3

from Xlib import X, XK, display, xobject
from Xlib.ext import randr
from typing import Any, Callable, Dict, List, Optional


class Stack:
    """Stack base class."""

    def __init__(self) -> None:
        self.items: List[Any] = []

    def push(self, item: Any):
        """Add a new item onto TOS."""
        self.items.append(item)

    def pop(self, item: Any):
        """Remove an item from TOS."""
        self.items.pop()

    def swap(self, n: int):
        """Swap the position of the items at TOS and TOS+n."""
        self.items[-1 - n], self.items[-1] = self.items[-1], self.items[-1 - n]

    def roll_l(self):
        """Take the item at TOS and move it to the bottom of the stack."""
        self.items = self.items[-1:] + self.items[:-1]

    def roll_r(self):
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


class Workspace:
    """Workspace class."""


class Window:
    """Window class."""


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
    global _ROOT

    # capture and setup root
    _ROOT = _DISPLAY.screen().root
    _ROOT.set_wm_name("soswm")
    _ROOT.change_attributes(
        event_mask=X.SubstructureNotifyMask | X.SubstructureRedirectMask
    )

    # capture monitors
    # TODO

    # load config
    try:
        exec(open("config.py").read(), globals())
    except Exception as e:
        print(f"Error while loading config file: {e}")


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
    """Run the main WM loop."""
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
                print(f"Unknown event type: {event}")


def _configure_request(event: display.event.ConfigureRequest):
    pass


def _key_press(event: display.event.KeyPress):
    try:
        chord = _Chord(event.detail, event.state)
        _KEYMAP[chord]()
    except KeyError:
        print(f"Unknown keychord: {chord}")


def _map_request(event: display.event.MapRequest):
    pass


def _unmap_notify(event: display.event.UnmapNotify):
    pass


def launch(cmd, *args):
    """Run a system command."""


def kill():
    """Kill the active window."""


def logout():
    """Log out of the WM."""


if __name__ == "__main__":
    _DISPLAY: display.Display = display.Display()
    _ROOT: xobject.drawable.Window
    _KEYMAP: Dict[_Chord, Callable] = {}
    update()
    _run()
