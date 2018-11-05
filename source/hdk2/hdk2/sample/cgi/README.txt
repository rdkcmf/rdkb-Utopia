HNAP CGI Sample
===============

The HNAP CGI sample application shows how to integrate the HDK into a
CGI application.  It has been tested with the following HTTP servers:

Cherokee - CGI interface
lighttpd - CGI and FastCGI interface
Apache - CGI (requires conf file change - see below)
mini_httpd - CGI (requires patch)


Setup for Apache on Ubuntu Linux
================================

1. Build the HNAP CGI executable

    make -C hdk2
    make -C hdk2/sample/cgi

2. Install Apache2

    sudo apt-get install apache2

3. Create the symbolic link in the Apache cgi-bin direcotry

    sudo ln -s hdk2/sample/cgi/build/hdk.cgi /usr/lib/cgi-bin/hdk.cgi

4. Copy over the device state for the simulator.

    sudo cp hdk2/libhnap12/unittest/state/default.ds /usr/lib/cgi-bin/simulator.ds

4. Enable the Apache mod_rewrite module

    sudo ln -s /etc/apache2/mods-available/rewrite.load /etc/apache2/mods-enabled/rewrite.load

5. Add the Apache configuration

    sudo gedit /etc/apache2/sites-available/default

   Add the following lines to the end of the file:

    #
    # HNAP
    #

    RewriteEngine on

    # Set the HTTP_AUTHORIZATION environment variable
    RewriteCond %{HTTP:Authorization} !^$
    RewriteRule ^/HNAP1/?$ - [E=HTTP_AUTHORIZATION:%{HTTP:Authorization}]

    # Rewrite /HNAP1/ to hdk.cgi
    RewriteRule ^/HNAP1/?$ /cgi-bin/hdk.cgi [L,PT]

    #
    # HNAP END
    #

6. Restart Apache

    sudo /etc/init.d/apache2 restart

7. Test your HNAP server

    hdk2/bin/hnappost.py localhost -m GetDeviceSettings
