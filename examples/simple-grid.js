const Emeus = imports.gi.Emeus;
const GObject = imports.gi.GObject;
const Gio = imports.gi.Gio;
const Gtk = imports.gi.Gtk;
const Lang = imports.lang;

Emeus.ConstraintLayout.prototype.pack = function(child, constraints) {
    Gtk.Container.prototype.add.call(this, child);

    let childConstraints = constraints || [];
    for (let constraint of constraints) {
        Emeus.ConstraintLayout.prototype.child_add_constraint.call(this, child, constraint);
    }
}

const MyApplicationWindow = Lang.Class({
    Name: 'MyApplicationWindow',
    Extends: Gtk.ApplicationWindow,

    _init: function() {
        this.set_default_size(400, 500);

        let box = new Gtk.Box({ orientation: Gtk.Orientation.VERTICAL, spacing: 6 });
        this.add(box);
        box.show();

        let layout = new Emeus.ConstraintLayout();
        layout.hexpand = true;
        layout.vexpand = true;
        box.add(layout);
        layout.show();

        this._layout = layout;

        let button = new Gtk.Button({ label: 'Quit' });
        button.hexpand = true;
        box.add(button);
        button.show();

        button.signal_connect('clicked', Lang.bind(this, function() { this.destroy(); }));

        this._buildGrid();
    },

    _buildGrid: function() {
        /* Grid:
         *
         * +--------------------------+
         * | +---------+  +---------+ |
         * | | Child 1 |  | Child 2 | |
         * | +---------+  +---------+ |
         * |                          |
         * | +----------------------+ |
         * | |        Child 3       | |
         * | +----------------------+ |
         * +--------------------------+
         */
        let button1 = new Gtk.Button({ label: 'Child 1' });
        this._layout.pack(button1 [
            new Emeus.Constraint({ target_attribute: Emeus.ConstraintAttribute.WIDTH,
                                   relation: Emeus.ConstraintRelation.GEQ,
                                   constant: 120, }),
        ]);
        button1.show();

        let button2 = new Gtk.Button({ label: 'Child 2' });
        this._layout.pack(button2, [
            new Emeus.Constraint({ target_attribute: Emeus.ConstraintAttribute.WIDTH,
                                   relation: Emeus.ConstraintRelation.EQ,
                                   source_object: button1,
                                   source_attribute: Emeus.ConstraintAttribute.WIDTH, }),
            new Emeus.Constraint({ target_attribute: Emeus.ConstraintAttribute.HEIGHT,
                                   relation: Emeus.ConstraintRelation.EQ,
                                   source_object: button1,
                                   source_attribute: Emeus.ConstraintAttribute.HEIGHT, }),
            new Emeus.Constraint({ target_attribute: Emeus.ConstraintAttribute.START,
                                   relation: Emeus.ConstraintRelation.EQ,
                                   source_object: button1,
                                   source_attribute: Emeus.ConstraintAttribute.END,
                                   constant: 12, }),
        ]);
        button2.show();

        let button3 = new Gtk.Button({ label: 'Child 3' });
        this._layout.pack(button3, [
            new Emeus.Constraint({ target_attribute: Emeus.ConstraintAttribute.START,
                                   relation: Emeus.ConstraintRelation.EQ,
                                   source_object: button1,
                                   source_attribute: Emeus.ConstraintAttribute.START, }),
            new Emeus.Constraint({ target_attribute: Emeus.ConstraintAttribute.END,
                                   relation: Emeus.ConstraintRelation.EQ,
                                   source_object: button2,
                                   source_attribute: Emeus.ConstraintAttribute.END, }),
            new Emeus.Constraint({ target_attribute: Emeus.ConstraintAttribute.TOP,
                                   relation: Emeus.ConstraintRelation.EQ,
                                   source_object: button1,
                                   source_attribute: Emeus.ConstraintAttribute.BOTTOM,
                                   constant: 12, })
        ]);
    },
});

const MyApplication = Lang.Class({
    Name: 'MyApplication',
    Extends: Gtk.Application,

    _init: function() {
        this.parent({ application_id: 'io.bassi.SimpleGrid' });

        this._mainWindow = null;
    },

    vfunc_activate: function() {
        if (!this._mainWindow) {
            this._mainWindow = new MyApplicationWindow({ application: this });
            this._mainWindow.show();
        }

        this._mainWindow.present();
    },
});

let app = new MyApplication();
app.run();
