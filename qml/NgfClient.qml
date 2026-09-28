import QtQuick 2.0
import Nemo.DBus 2.0

Item {
    function play(event) {
        ngf.typedCall("Play", [
                          { "type": "s", "value": event },
                          { "type": "a{sv}", "value": {} }
                      ])
    }

    DBusInterface {
        id: ngf
        bus: DBus.SystemBus
        service: "com.nokia.NonGraphicFeedback1.Backend"
        path: "/com/nokia/NonGraphicFeedback1"
        iface: "com.nokia.NonGraphicFeedback1"
    }
}
