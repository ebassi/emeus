## Emeus - Constraint-based layout manager for GTK+

### What is Emeus?

Emeus is a constraint-based layout manager widget for [GTK+][gtk-web],
written using the [Cassowary][cassowary-web] constraint solving algorithm.

### What's the difference between Emeus and GTK+'s layout managers?

GTK+ has two different sorts of layout managers:

 * the boxes-inside-boxes model, represented by [GtkBox][gtk-box-api] and
   which is the preferred layout management mechanism
 * the fixed positioning and sizing model, using the [GtkFixed][gtk-fixed-api]
   and the [GtkLayout][gtk-layout-api] containers

The first model works really well in ensuring that UIs are responsive to
size changes, by avoiding pixel-perfect positioning on the screen, as well
as ensuring that changing the font size or margins and paddings do not break
the user interface; its main down side is that it requires accurate, and
often verbose packing of widgets inside boxes, inside other boxes.

The second model allows a more efficient way to construct a user interface,
at the major costs of either "freezing" it, or requiring constant
recalculations of the relative position and size of each UI element.

Emeus provides a third layout management policy, based on *constraints*;
each UI element binds one of more of its attributes — like its width, or its
position — to other UI elements, in a way that is more natural to describe
from a UI building perspective, and hopefully more efficient that stacking
piles of boxes one inside another.

### Constraints

`EmeusLayoutConstraint` is a [GtkContainer][gtk-container-api] that can
support multiple children; each child, in turn, is associated to one or more
`EmeusConstraint` instances. Each constraint is the expression of a simple
linear equation:

    item1.attr1 = item2.attr2 × multiplier + constant

Where:

  * `item1` is the target widget, that is the widget we want to constraint;
    if unset, the target will be the layout itself
  * `attr1` is an attribute of the target widget, like `width` or `end`,
    that we want to constraint
  * `item2` is the source widget, that is the widget that provides the value
    of the constraint; if unset, the source will be the layout itself
  * `attr2` is an attribute of the source widget that provides the value
    of the constraint
  * `multiplier` is a multiplication factor, expressed as a floating point
    value
  * `constant` is an additional constant factor, expressed as a floating
    point value

Using both notations, then, we can construct user interfaces like:

    +-------------------------------------------+
    |   [ button 1 ] [ button 2 ] [ button 3]   |
    +-------------------------------------------+

By expressing the constraints between the UI elements. For instance, we can
center `button2` within its parent and give it a minimum width of 250
logical pixels:

    button2.width >= 250
    button2.centerX = parent.centerX
    button2.centerY = parent.centerY

Then, we can tie `button1` and `button3` to `button2`, and ensure that the
width and height of all three buttons are the same:

    button1.end = button2.start - 8
    button1.width = button2.width
    button1.height = button2.height

    button3.start = button2.end + 8
    button3.width = button2.width
    button3.height = button2.height

The `EmeusConstraintLayout` widget will attempt to resolve all the
constraints, and lay out its children according to them.

## Building

 * Install [meson](http://mesonbuild.com/)
 * Install [ninja](https://ninja-build.org/)
 * Create a build directory:
  * `$ mkdir _build && cd _build`
 * Run meson:
  * `$ meson`
 * Run ninja:
  * `$ ninja`
  * `$ ninja test`
  * `# ninja install`

## Licensing

Emeus is released under the terms of the GNU Lesser General Public License,
either version 2.1 or, at your option, any later version.

The Cassowary simplex solving algorithm implementation is largely inspired
by the equivalent implementations written in various other language,
including:

 * [Cassowary][cassowary-web] — the original C++ implementation, released
   under the terms of the GNU Lesser General Public License, version 2.1 or
   later
 * [Cassowary.js][cassowary-js-web] — JavaScript implementation, released
   under the terms of the Apache License, Version 2.0
 * [Cassowary][cassowary-py-web] — Python implementation, released under the
   terms of the BSD 3-clause license

Additionally, the automatic layout solving is inspired by
[autolayout.js][autolayout-js-web], a JavaScript automatic constraint-based
layout, which, in turn, is based on the Apple [autolayout][nsautolayout-web]
layout manager.

You can check on the [Overconstrained][overconstrained-web] website for
additional Cassowary implementations in various languages.

[gtk-web]: https://www.gtk.org
[cassowary-web]: http://constraints.cs.washington.edu/cassowary/
[overconstrained-web]: http://overconstrained.io/
[gtk-container-api]: https://developer.gnome.org/gtk3/stable/GtkContainer.html
[gtk-fixed-api]: https://developer.gnome.org/gtk3/stable/GtkFixed.html
[gtk-layout-api]: https://developer.gnome.org/gtk3/stable/GtkLayout.html
[gtk-box-api]: https://developer.gnome.org/gtk3/stable/GtkBox.html
[cassowary-cpp-web]: https://sourceforge.net/projects/cassowary/
[cassowary-js-web]: https://github.com/slightlyoff/cassowary.js
[cassowary-py-web]: https://github.com/pybee/cassowary
[autolayout-js-web]: https://github.com/IjzerenHein/autolayout.js
[nsautolayout-web]: https://developer.apple.com/library/content/documentation/UserExperience/Conceptual/AutolayoutPG/index.html
