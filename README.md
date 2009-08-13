Tongseng
========

TUIO wrapper for Mac OS X multitouch events. Now you can use the
multitouch touch pad to send TUIO events to your multitouch
application!

Usage
-----

1. Download [the binary file](http://cloud.github.com/downloads/fajran/tongseng/tongseng-0.2.zip) (Intel only)
2. Extract it
3. Run the `tongseng` file

Then..

1. Run your TUIO-based multitouch application
2. Start touching your touchpad!

By default, `tongseng` will send the TUIO messages to localhost at
port 3333.  If you want to use another host/port, mention them in the
parameter.

    $ ./tongseng host port

Compilation
-----------

1. Retrieve the code. Using git

        $ git clone git://github.com/fajran/tongseng.git
    
    or by clicking the download button.

2. Compile it

        $ cd tongseng
        $ make

