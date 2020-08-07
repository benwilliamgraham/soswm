#!/usr/bin/python3

from Xlib import X, XK
from Xlib.display import Display

class Stack:
    """Stack base class."""
    def __init__(self):
        self.items = []

    def push(self, item):
        self.items.append(item)

    def pop(self, item):
        self.items.pop()

    def swap(self, i, j):
        self.items[i], self.items[j] = self.items[j], self.items[i]

    def roll_l(self):
        self.items = self.items[-1:] + self.items[:-1]

    def roll_r(self):
        self.items = self.items[:1] + self.items[1:]

class MonitorStack(Stack):
    """Stack of monitors."""
    def __init__(self):
        super().__init__()

class WorkspaceStack(Stack):
    """Stack of workspaces."""
    def __init__(self):
        super().__init__()

class WindowStack(Stack):
    """Stack of windows."""
    def __init__(self):
        super().__init__()

class Monitor:
    """Monitor class."""

class Workspace:
    """Workspace class."""

class Window:
    """Window class."""

class Key:
    """Key class"""

class WM:
    display = None

    def __init__(self):
        if self.display is None:
            self.display = Display()


if __name__ == "__main__":
    WM()
