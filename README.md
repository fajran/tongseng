Tongseng
========

Tongseng is a [TUIO](http://tuio.org) multitouch tracker for Mac OS X touchpad devices. 
This application allows to use the internal trackpad (or alternatively an external magic trackpad or magic mouse) 
to send /tuio/2Dcur messages to your TUIO enabled multitouch application. 
The latest version also enables alternative TUIO/TCP and TUIO/WEB transport methods in addition to the default TUIO/UDP transport.

Usage
-----

1. Download [the application package](https://github.com/fajran/tongseng/releases/download/0.7/Tongseng-0.7.zip)  
   (Universal i386/x86_64 binary, minimum MacOS X version 10.6)
2. Uncompress the ZIP archive
3. Run Tongseng
    
    ![Tongseng](tongseng.png)
    
4. Optionally change the TUIO/UDP port and host  
   or select alternative TUIO/TCP or TUIO/WEB transport
5. Optionally select an internal or external trackpad
6. Click the Start button to activate Tongseng

Then ...

1. Run your TUIO-based multitouch application
2. Start touching your touchpad

Compilation
-----------

### GUI Application

1. Retrieve the code. Using git

        $ git clone git://github.com/fajran/tongseng.git
    
    or by clicking [the download
	button](http://github.com/fajran/tongseng/tarball/master).

2. Open `TongsengApp/TongsengApp.xcodeproj` with Xcode.
3. Build the project.

### Command line application

1. Retrieve the code
2. Go to the source directory
3. Type `make` and return.


