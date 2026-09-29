HDS / Neoware @workStation with netOS 3.2 (1997), in MAME
==========================================================

The machine is the HDS (Human Designed Systems) network station, later sold
by Neoware as the @workStation: an Intel i960CA processor, a 256 colour
screen of up to 1280x1024, a local IDE disk and netOS, HDS's own UNIX
derived from 4.4BSD. The boot PROM is from 1997 (netOS 3.2, build 563).

The emulator is a MAME driver written for this machine (machine "hdsfx").


Starting
--------

There is one launcher per screen size. Pick the one that fits your screen
and double click it:

  NETOS.bat               1280x1024 (V19C monitor, the factory one)
  NETOS-1152x900.bat      1152x900  (V16C monitor)
  NETOS-1024x768.bat      1024x768  (V14C monitor)
  NETOS-800x600.bat       800x600   (VESA monitor)

On a 1920x1080 screen the 1280x1024 one does not fit next to the task bar
and MAME shrinks it a little; NETOS-1152x900.bat shows at real size there,
pixel for pixel. It is the same machine with another monitor: the boot PROM
reads the monitor type from switches on the board and netOS follows.

A MAME window opens. Booting takes about TWO AND A HALF MINUTES up to the
login dialog. That is what the real machine does, it has not hung:

  1. The boot PROM shows its blue screen with the netOS logo, gets an
     address from the network (DHCP) and loads netOS (3 MB) from the disk:
     "Loading netOS..." with its progress bar.
  2. netOS starts: a black text console with its messages.
  3. The X server comes up with the login dialog, on a teal background.


Logging in
----------

The dialog asks for a user name and password on a Windows NT or Unix server
of your network, where your home directory would be. There is no such
server here, so press "Don't log in". The netOS desktop comes up:

  - bottom left, the "netOS" button opens the menu (Windows Session, Unix
    Session, Applications, Configuration, Run, End Session);
  - on the right, the connection icon bar;
  - bottom right, the clock.

The "Windows" and "Unix" sessions are connections to remote servers (ICA,
telnet, XDMCP) on your local network; without those servers they go nowhere.


Netscape and the Internet
-------------------------

netOS menu, Applications, Internet:

  - "Netscape Navigator" opens the netOS user manual, which is on the disk
    (works with no connection at all).
  - "The Old Net" opens http://theoldnet.com, which serves the web of the
    90s (and archived pages of today) in a form Netscape 3 understands.

Netscape TAKES A WHILE to open: one or two minutes after choosing the menu
entry, with nothing on screen meanwhile. It is a 4 MB program the machine
loads from the disk; do not launch it again while it loads. It comes up in
a maximized window.

It is Netscape Navigator 3.0 for the i960, the one HDS shipped with its
software (it is on the same CD). Its license comes already accepted. Its
preferences, bookmarks and history are kept on the disk, in
///disk1/home/.netscape, from one session to the next.

The machine reaches the Internet through your PC's connection, with nothing
to install and no administrator rights: MAME acts as its router (user mode
networking, the same technique QEMU uses). netOS gets 10.0.2.15 by DHCP and
uses 10.0.2.3 as its DNS server.

Only http:// pages work. Almost all of today's web is https:// with
encryption that did not exist in 1997, and Netscape 3 cannot open it: that
is what theoldnet.com is for. The "Home", "What's New?" and similar buttons
point to Netscape's servers of 1997, which are gone.


Keyboard and mouse
------------------

The keyboard goes straight to netOS from the start.

The mouse is captured by the window. To release it: press Scroll Lock, then
P to pause; now you can move the pointer out. To go on, come back to the
window, P again and Scroll Lock.


Closing
-------

netOS writes to one part of the disk, the one it mounts as ///disk1 (where
Netscape keeps its files). To close without losing anything:

  1. Close Netscape if you opened it (File menu, Exit).
  2. Wait about five seconds: netOS writes what is pending to the disk in
     less than a second, and after that it writes nothing.
  3. Close the MAME window, or Scroll Lock and then Esc.

Closing with Netscape still at work does not break the next boot (tested):
at most the last few seconds are lost. The rest of the system, netOS and
its programs, lives in a part of the disk that is never written.

MAME saves the machine's configuration memory (the NVM) on exit, in the
"state" folder.


Messages that look alarming and are normal
------------------------------------------

  "Invalid NVM present" / "NVM contents is invalid, setting to Factory
  Defaults"
      Only the first time: the configuration memory starts empty and the
      boot PROM fills it with the factory settings.


Good to know
------------

- The emulated video board is the colour FX, with the TLC34075 palette
  DAC. MAME's menu (Scroll Lock, then Tab), "Machine Configuration", has
  the other board netOS knows, the G300; it is in colour too. The change
  takes effect on the next boot.
- Do not start two launchers at once: they share the disk and the second
  one cannot open it.
- There is no sound, PCMCIA or serial port connected.
- To go back to the factory state, delete the "state" folder and take
  disk\netos.hd out of the zip again.
- Screenshots (F12 key) go to "snapshots".


What is in the pack
-------------------

  NETOS*.bat              the launchers, one per screen size
  hdsfx.exe               MAME with the hdsfx driver and user mode
                          networking (libslirp)
  roms\hdsfx\             the netOS 3.2 V32 boot PROM (it is on the CD)
  disk\netos.hd           a 256 MB IDE disk ready to boot by itself
  ini\ui.ini              hides MAME's startup warnings

The disk holds the boot PROM, netOS 3.2 (V32.bin), the netOS tree from the
installation CD (fonts, configuration, keyboards, programs), Netscape
Navigator 3.0 from the same CD, and a writable partition for ///disk1,
formatted by netOS itself with its own newfs.
