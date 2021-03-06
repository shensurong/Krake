Krake
=====

Krake provides health monitor functionality for servers. 


Overview
--------

Krake is a health monitor which can check the server's health status.

Feature List:
* Support health monitoring in:
 * ICMP
 * TCP
 * HTTP/HTTPS
* Support callback mechanism that can call user-specified scripts/commands to notify the health status
* Support writing logs into syslog or file

Krake runs as a daemon and can be configured by a xml styled configuration file.
Krake can run in most Unix like environment, incl. Linux and xBSD.


Installation
------------

Requires:
* libevent2
* libmnl

Read the INSTALL file in the source code tree.


General Usage
-------------

The krake daemon supports the following options:

        --config/-c             Assign the configruation file
        --reload/-r             Reload the configruation file
        --show/-s               Show health status, -s all for all monitor, -s monitor_name for one monitor
        --quit/-q               Shutdown krake
        --version/-v            Show Krake version
        --help/-h               Show this help


Configuration
-------------

Default configuration file: $prefix/etc/krake.conf, the $prefix refers to the parameter passed to configure script while 
compile the source code.

Example:

        <?xml version="1.0"?>
        <krk_config>
                <monitor>                               <!--Keyword for krake configuration, monitor can more than one-->
                        <name>monitor1</name>               <!--name of a monitor-->
                        <status>enable</status>             <!--Only can be enable or disable; 
                                                            when enable, also starts the monitor's timer;
                                                            when disable, also stops the monitor's timer-->
                        <checker>http</checker>             <!--specify a checker using by a monitor-->
                        <checker_param>send-file:"~/request" 
                                expected-file:"~/response"</checker_param>    <!--checker's parameters, see below-->
                        <interval>5</interval>              <!--interval of monitor's timer in seconds-->
                        <timeout>3</timeout>                <!--time out value of checked host in seconds-->
                        <failure_threshold>3</failure_threshold>            <!--how many times of failures happen, marking host as down-->
                        <success_threshold>3</success_threshold>            <!--how many times of successes happen, marking host as up-->
                        <node>
                                <host>10.1.1.2</host>               <!--ip address of a checked host, either ipv4 address is valid-->
                                <port>8080</port>                   <!--port number of a checked host, range is 1 ~ 65535-->
                        </node>
                        <node>
                                <host>10.1.1.3</host>               <!--ip address of a checked host, either ipv4 address is valid-->
                                <port>8080</port>                   <!--port number of a checked host, range is 1 ~ 65535-->
                        </node>
                        <script>~/notifier.sh</script>              <!--failure notification, if user specify this option,
                                                              when a failure of a checked host is deteceted, krake will call this script-->
                </monitor>
                <log>                               <!--set logging attributes-->
                        <logtype>syslog</logtype>       <!--set the type of logs that krake sends, value can be file, syslog-->
                        <loglevel>notice</loglevel>          <!--set the level of logs, under which the logs will not be sent out-->
                </log>
        </krk_config>

If don't want to use this file, you can assign another xml file by krake command line

After you make some modifications to the configuration file, you can use "krake -r" to force the daemon reload the 
configuration file without stopping the current detection.


Configuration of the Checkers
-----------------------------

At current stage, only http checker has the checker parameters.

    http checker:
        
        <checker-param>send-file:"/path/to/a/file" expected-file:"/path/to/a/file"</checker-param>

    This let you to specify what to send and what to expected to receive when you use the http checker.
    You can write the http packet into a send file and tell Krake what to send.

    If these parameters are missing, then Krake will send a minimum http request packet as:
        GET / HTTP/1.0
        Host: test
        Connection: Close

    and expect only the 200 status of the http response.
    http checker now only supports HTTP 1.0 protocol.


