# liblzx
liblzx is a fork of wimlib's LZX compressor with changes to make it compatible
with the CAB LZX format.

It mainly exists to help improve the compression of MSI installers on
non-Windows platforms.  Currently, both GNOME msitools and Wine's cabinet.dll
(used by WiX Toolset) only support deflate compression, which is lower-quality
and hampered by a restriction in the cabinet format that prevents
deflate-compressed blocks from referencing data in previous blocks.

# LZX DELTA
liblzx doesn't currently support LZX DELTA, used by the CreatePatchFileExA and
CreatePatchFileExW functions.  It may in the future once some test scenarios
are added.  This isn't currently possible because several internal structures
have 21 bits of precision, which is just enough for CAB LZX's maximum window
size of 2MB.

LZX DELTA supports window sizes of up to 64MB, so liblzx would need an
additional set of compression functions that can handle the increased offset
precision.

# Wine cabinet.dll patch
The "winecabinet" dir is a patched version of Wine's cabinet.dll to support
the updated FCI functions and liblzx's streaming behavior.

# Usage
See the liblzx.h header file for usage.

# Determinism
liblzx uses some floating point math internally for the BT matchfinder.  As
such it is dependent on floating point optimizations like "fast math" and
the state of floating point control registers.

If you care about output determinism, you should disable "fast math"
optimizations and set the floating point control registers to set values
before calling liblzx functions.

# License
liblzx is licensed under GPLv3 or LGPLv3, same as the wimlib license.