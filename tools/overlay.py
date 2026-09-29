#!/usr/bin/env python3
# Build the overlay: files added to, or replacing, the netOS 3.2 tree on the disk.
#  - Netscape Navigator 3.0 for i960, from the HDSware 3.2.1 tree on the same CD
#  - apps.cfg and system.netOSwmrc with Netscape in the netOS menu
#  - default.cfg and startup.cmd that mount the writable /disk1
# usage: overlay.py <extracted CD> <output directory>
import os, shutil, sys

cd, out = sys.argv[1:3]
net = os.path.join(cd, 'netos32/netOS')
hds = os.path.join(cd, 'hds-fx.3.2.1/hds-fx')
shutil.rmtree(out, ignore_errors=True)

def put(rel, data):
    p = os.path.join(out, rel)
    os.makedirs(os.path.dirname(p), exist_ok=True)
    open(p, 'wb').write(data)

def read(path):
    return open(path, encoding='latin1').read()

put('bin/i960/netscape', open(os.path.join(hds, 'bin/netscape'), 'rb').read())
lib = os.path.join(hds, 'lib/netscape')
for root, dirs, names in os.walk(lib):
    for n in names:
        full = os.path.join(root, n)
        put(os.path.join('lib/netscape', os.path.relpath(full, lib)), open(full, 'rb').read())

# Netscape gets the usual application environment, with its home on /disk1
apps = read(os.path.join(net, 'config/apps.cfg'))
old = 'app.netscape.failApp: msgbox\n'
assert apps.count(old) == 1
apps = apps.replace(old, old + '''app.netscape.menuName: Netscape Navigator
app.netscape.path: ///%netOSdir%/netOS/bin/%cpu%/netscape
app.netscape.minAvailMem: 4000k
app.netscape.defaultEnv: XFILESEARCHPATH=///builtin/%N%S:///%%netOSdir%%/netOS/lib/app-defaults/%N%S\\n\\
	     XBMLANGPATH=///builtin/%B\\n\\
	     XCMSDB=\\n\\
	     XNLSPATH=///builtin\\n\\
	     XMBINDDIR=///builtin\\n\\
	     HOME=///disk1/home\\n\\
	     XENVIRONMENT=///builtin/.Xdefaults
''')
put('config/apps.cfg', apps.encode('latin1'))

# an Internet menu; Netscape opens as a normal, maximized window
wm = read(os.path.join(net, 'config/system.netOSwmrc'))
old = '+ "Accessories%mini-ofolder.xpm%"\tPopup AccessoriesMenu\n'
assert wm.count(old) == 1
wm = wm.replace(old, old + '+ "Internet%mini-ofolder.xpm%"\tPopup InternetMenu\n')
old = 'AddToMenu "MultimediaMenu"\n'
assert wm.count(old) == 1
wm = wm.replace(old, 'AddToMenu "InternetMenu"\n'
	'+ "Netscape Navigator%mini-nscape.xpm%"\tFunction Netscape-Manual\n'
	'+ "The Old Net%mini-nscape.xpm%"\tFunction Netscape-OldNet\n\n' + old)
old = 'Style "*Netscape*"\tTitleIcon mini-nscape.xpm\n'
assert wm.count(old) == 1
wm = wm.replace(old, old + 'Style "*Netscape*"\tStartNormal\n')
wm += """
AddToFunc "Netscape-Manual"
+\t"I" Exec netscape file:///dev/flash/netOS/web/usermanual/usermanual32.html &
+\t"I" Wait Netscape
+\t"I" Next [Netscape*] Maximize 100 95

AddToFunc "Netscape-OldNet"
+\t"I" Exec netscape http://theoldnet.com/ &
+\t"I" Wait Netscape
+\t"I" Next [Netscape*] Maximize 100 95
"""
put('config/system.netOSwmrc', wm.encode('latin1'))

# netOS mounts only wd0d by itself (the read-only devf with netOS); the startup commands
# mount the writable 4.2BSD partition e as /disk1, clear a lock that a Netscape closed
# without Exit leaves behind, and then run the starter as netOS does without this file
cfg = read(os.path.join(net, 'config/default.cfg'))
cfg += 'setup.startup.command.file:\t///%netOSdir%/netOS/config/startup.cmd\n'
put('config/default.cfg', cfg.encode('latin1'))
put('config/startup.cmd', b'mount /dev/dsk/wd0e /disk1\nmkdir /disk1/home\nrm /disk1/home/.netscape/lock\nstarter &\n')

for root, dirs, names in os.walk(out):
    for n in names:
        p = os.path.join(root, n)
        print(os.path.getsize(p), os.path.relpath(p, out))
