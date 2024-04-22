# Battery Input Manager daemon

Battery Input Manager suspends battery input to save battery health.

## Settings ##

To enable service:

`$ systemctl --user enable --now battery-input-manager`

```
# Set max charging threshold, will never charge above this level
$ gsettings set org.adishatz.Bim threshold-max 100 (default)
# Set start charging threshold, will start charging again at this level
$ gsettings set org.adishatz.Bim threshold-start 60
# Set end charging threshold, will stop charging at this level (unless an alarm is pending, then will wait for threshold-max)
$ gsettings set org.adishatz.Bim threshold-end 80
```

Alarm support needs gnome-alarm 46 (flathub version on Droidian).

## Add support for more devices ##

Send me your device /sys path node for input suspend. (devices with start-end threshold are not supported for now)

## Depends on

- `glib2`
- `meson`
- `ninja`

## Building from Git

```bash
$ meson builddir --prefix=/usr

$ sudo ninja -C builddir install
```
