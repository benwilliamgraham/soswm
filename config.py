grab_keys({
    KeyChord(XK.XK_R): lambda: update(),
    KeyChord(XK.XK_A): lambda: print("I've been pressed!"),
    KeyChord(XK.XK_A, X.ShiftMask): lambda: print("Pressed with shift"),
    KeyChord(XK.XK_T): lambda: launch("kitty"),
})

launch("feh", "--bg-scale", "/usr/share/backgrounds/fedora-workstation/zen.jpg")
