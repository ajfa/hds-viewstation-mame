# Network

## Intel 82596CA

At 0x10000000 (PORT) and 0x10000004 (channel attention), little endian, 32-bit linear
mode. MAME's `i82596` device does the rest, with one change
(`patches/i82596-self-test.patch`): the PORT self-test command now writes its result.

The boot PROM puts 0x12345678 at the result address, issues the self-test, and checks
that the chip overwrote it. The result is two words, the ROM signature and a status
word with one bit per test (ROM, register, bus timer, diagnose, self test) where 0 means
passed. MAME wrote nothing, so the boot PROM printed "82596 Self Test: failed to
complete" and "Station may be disconnected from the network", and gave up on DHCP.

The chip's interrupt pin is not connected; neither the boot PROM nor netOS needed it so
far. XINT1, the only external interrupt netOS installs a handler on besides the known
ones, reads a device at 0xD4000000/0xD4000001 (index and data), most likely the PCMCIA
controller.

## User mode networking

MAME can reach a real network only through TAP or pcap, and both need administrator
rights to set up. `patches/slirp-network.patch` adds a third network provider, `slirp`,
built on [libslirp](https://gitlab.freedesktop.org/slirp/libslirp), the library behind
QEMU's user mode networking:

- the guest sees a 10.0.2.0/24 network with a DHCP server (it hands out 10.0.2.15), the
  gateway at 10.0.2.2 and a DNS forwarder at 10.0.2.3;
- its TCP and UDP connections leave through ordinary sockets of the host;
- nothing listens on the host, and there is nothing to install.

The provider is `src/osd/modules/netdev/slirp.cpp`. It is compiled only with
`USE_SLIRP=1`, and registered before `taptun`, so it is the default where it is built.
It works with libslirp 4.6 (Ubuntu 22.04) and 4.9 (MSYS2), whose socket interface
differs on Windows. The driver attaches the 82596 to network device 0 at start.

With it, the boot PROM gets its address by DHCP, which also shortens the boot: it no
longer waits a minute for an answer and restarts itself. netOS uses the same address,
and Netscape 3.0 browses `http://` sites. `https://` needs encryption that did not
exist in 1997; <http://theoldnet.com> serves the old web, and archived copies of today's,
in a form Netscape 3 can show.

## A LAN with other emulators

Two environment variables let the same provider share a local network with a machine
emulated in QEMU, while netOS keeps its Internet access:

- `MAME_SLIRP_NET=a.b.c` moves the user mode network to a.b.c.0/24, with the gateway at
  a.b.c.2, the DNS forwarder at a.b.c.3 and DHCP addresses from a.b.c.20.
- `MAME_SLIRP_LAN=localport:remotehost:remoteport` (or `localport:remoteport` for
  127.0.0.1) joins a LAN carried as raw Ethernet frames, one per UDP datagram. That is
  the format of QEMU's `-netdev dgram`.

Frames are switched by destination address: those for slirp's own addresses (52:55:...)
go to slirp, unicast frames for any other station go to the LAN, and broadcasts go to
both. Slirp never sees the traffic between the two guests; joined to a plain hub
instead, it saw that traffic and reset connections that were not its own.

For example, with a QEMU guest whose second network card has the fixed address
192.168.77.15:

    MAME_SLIRP_NET=192.168.77 MAME_SLIRP_LAN=7702:127.0.0.1:7701 \
        netos hdsfxv16 -networkprovider slirp ...

    qemu-system-i386 ... \
        -netdev dgram,id=lan,local.type=inet,local.host=127.0.0.1,local.port=7701,remote.type=inet,remote.host=127.0.0.1,remote.port=7702 \
        -device pcnet,netdev=lan

netOS gets 192.168.77.20 by DHCP and reaches the guest at 192.168.77.15. This was tested
with an X11 application server running in QEMU: netOS's WINConnect opened sessions on
it, and closing and starting MAME again kept the LAN. One thing learnt there: ask such a
server for 16 colours. The HDS screen has a single 256 entry colormap, so a session that
installs its own 256 colour palette changes the colours of the rest of the netOS desktop
while it lasts.

## Reopening the network device

`patches/network-reopen.patch` changes MAME itself. When the saved configuration
(`cfg/<system>.cfg`) names a network interface, MAME reopens the device at start, and it
opened the new one before releasing the old one. With the LAN above, the old one still
held the UDP port, so from the second run on netOS had no LAN and the log said
"slirp: cannot bind LAN port". The old device is now released first.

If the port is really taken, by another running instance for example, the provider says
so and carries on with user mode networking only.
