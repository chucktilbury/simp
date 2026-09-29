# Confirms an accessible public base member is selected when another same-named base member is private.
class HiddenPath {
    int marker
}

class VisiblePath {
    int marker
}

class MixedPaths : private HiddenPath, public VisiblePath {
}

start {
    MixedPaths value = MixedPaths()
    value.marker = 9
    print(value.marker)
}
