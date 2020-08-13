grab_keys({
    KeyChord(XK.XK_A): lambda: print("I've been pressed"),
    KeyChord(XK.XK_A, X.ShiftMask): lambda: print("Pressed with shift"),
})
