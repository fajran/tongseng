Tongseng
========

TUIO wrapper for Mac OS X multitouch events. Now you can use the
multitouch touch pad to send TUIO events to your multitouch
application!


Usage
-----

1. Retrieve the code

        $ git clone git://github.com/fajran/tongseng.git

2. Compile it

        $ cd tongseng
        $ make

3. Run it!

        $ ./tongseng

   Touch your touchpad and tongseng will send TUIO/OSC messages to localhost
   port 3333, as usual. To use another host/port, mention them in the
   arguments.
   
        $ ./tongseng host port

Have fun!

