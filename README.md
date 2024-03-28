# Battery Input Manager daemon

Battery Input Manager allows stoping charging your device when uneeded.

## Depends on

- `glib2`
- `meson`
- `ninja`

## Building from Git

```bash
$ meson builddir --prefix=/usr

$ sudo ninja -C builddir install
```
