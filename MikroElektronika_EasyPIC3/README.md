MikroElektronika EasyPIC3
It's an old board but don't throw it yet because it works very well with an ICSP programmer (I use PICKit3.)
Plug the ICSP wires directly to 1 of the DIP sockets, as per this pinning table:

Pin Name             Description
1   MCLR/Vpp         power
2   Vdd target (Vcc) power
3   Vss (GND)        ground
4   PGD (ICSPDAT)    standard com data
5   PGC (ICSPCLK)    standard com clock

I don't use any resistor or capacitor.
