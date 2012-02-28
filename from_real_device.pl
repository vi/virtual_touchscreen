#!/usr/bin/perl -w

use strict;

# Usage: something like 
# ssh android "cat /dev/input/event1" | ./from_real_device.pl > /dev/virtual_touchscreen



#define ABS_X                   0x00
#define ABS_Y                   0x01

#define ABS_MT_SLOT             0x2f    /* MT slot being modified */
#define ABS_MT_TOUCH_MAJOR      0x30    /* Major axis of touching ellipse */
#define ABS_MT_TOUCH_MINOR      0x31    /* Minor axis (omit if circular) */
#define ABS_MT_WIDTH_MAJOR      0x32    /* Major axis of approaching ellipse */
#define ABS_MT_WIDTH_MINOR      0x33    /* Minor axis (omit if circular) */
#define ABS_MT_ORIENTATION      0x34    /* Ellipse orientation */
#define ABS_MT_POSITION_X       0x35    /* Center X ellipse position */
#define ABS_MT_POSITION_Y       0x36    /* Center Y ellipse position */
#define ABS_MT_TOOL_TYPE        0x37    /* Type of touching device */
#define ABS_MT_BLOB_ID          0x38    /* Group a set of packets as a blob */
#define ABS_MT_TRACKING_ID      0x39    /* Unique ID of initiated contact */
#define ABS_MT_PRESSURE         0x3a    /* Pressure on contact area */
#define ABS_MT_DISTANCE         0x3b    /* Contact hover distance */

$|=1;

my $need_mouseemu=0;

my $relx = 0;
my $rely = 0;

while (not eof STDIN) {
    sysread STDIN, $_, 16;
    my ($sec, $usec, $type, $code, $val) = unpack "LLssl";
    #print "type=$type, code=$code, val=$val\n"

    if ($type == 1) {
        print "d 0\n" if $val;
        print "u 0\n" unless $val;
    }
    if ($type == 2) {
        # relative positioning
        $relx += $val if $code == 0;
        $rely += $val if $code == 1;
        print "x $relx\ny $rely\nS 0\n";
        next;
    }

    unless ($type == 3) {
        if($need_mouseemu) {
            $need_mouseemu = 0;
            print "e 0\nS 0\n";
        }
    }

    print "s $val\n" if $code == 0x2f;
    print "T $val\n" if $code == 0x39;
    print "X $val\n" and $need_mouseemu=1 if $code == 0x35;
    print "Y $val\n" and $need_mouseemu=1 if $code == 0x36;

    print "a 0\ne 0\nS 0\n" and $need_mouseemu=0 if $code == 0x30 and not $val;  # my SE Xperia X10 driver sends this
    print "a 1\ne 0\nS 0\n" and $need_mouseemu=0 if $code == 0x30 and $val;


}

