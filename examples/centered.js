const ByteArray = imports.byteArray;
const Emeus = imports.gi.Emeus;
const GObject = imports.gi.GObject;
const Gtk = imports.gi.Gtk;
const Lang = imports.lang;

const template = ' \
<interface> \
  <template class="Gjs_MyApplicationWindow" parent="GtkApplicationWindow"> \
    <property name="visible">True</property> \
    <property name="default-width">400</property> \
    <property name="default-height">500</property> \
    <child> \
      <object class="GtkBox" id="box"> \
        <property name="visible">True</property> \
        <property name="orientation">GTK_ORIENTATION_VERTICAL</property> \
        <child> \
          <object class="EmeusConstraintLayout" id="layout"> \
            <property name="visible">True</property> \
            <property name="hexpand">True</property> \
            <property name="vexpand">True</property> \
            <child> \
              <object class="GtkButton" id="button_child"> \
                <property name="visible">True</property> \
                <property name="label">Hello, world</property> \
              </object> \
            </child> \
            <constraints> \
              <constraint target-object="button_child" target-attr="center-x" \
                          relation="eq" \
                          source-object="super" source-attr="center-x" \
                          strength="required"/> \
              <constraint target-object="button_child" target-attr="center-y" \
                          relation="eq" \
                          source-object="super" source-attr="center-y"/> \
              <constraint target-object="button_child" target-attr="width" \
                          relation="ge" \
                          constant="200" \
                          strength="EMEUS_CONSTRAINT_STRENGTH_STRONG"/> \
            </constraints> \
          </object> \
        </child> \
        <child> \
          <object class="GtkButton" id="quit_button"> \
            <property name="visible">True</property> \
            <property name="hexpand">True</property> \
            <property name="label">Quit</property> \
          </object> \
        </child> \
      </object> \
    </child> \
  </template> \
</interface>';

const MyApplicationWindow = new Lang.Class({
    Name: 'MyApplicationWindow',
    Extends: Gtk.ApplicationWindow,
    Template: ByteArray.fromString(template),
    Children: ['quit_button'],

    _init: function(props={}) {
        this.parent(props);

        this.quit_button.connect('clicked', this.destroy.bind(this));
    },
});

const MyApplication = new Lang.Class({
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

GObject.type_ensure(Emeus.ConstraintLayout.$gtype);

let app = new MyApplication();
app.run(['simple-grid'].concat(ARGV));
