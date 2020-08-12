import soswm
from soswm import Key
from Xlib import XK

soswm.grab_keys({
    Key(XK.XK_A): print("I've been pressed")
})
