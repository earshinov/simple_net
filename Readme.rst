About simple_net
================

Simple network applications - echo servers and compatible clients,
all implemented using pure Berkley sockets API.
They include TCP and some UDP versions.

Target machine requirements
---------------------------

* can be Windows or Unix-like (tested on Windows XP and Ubuntu Linux 8.10)

Additional requirements for Unix-like machines:

* program ``server`` require support of unnamed POSIX semaphores

List of Programs
----------------

Client and server programs are called ``server`` and ``client``  respectively.

Known vulnerabilities
---------------------

When you run ``server tcp`` passing limit option your server is vulnerable
to DoS attacks as I did not consider timeouts useful in a test program.

Modes using ``select()`` may work incorrectly on platforms where there is
no way to set SO_SNDLOWAT socket option (e.g., on Windows and Linux).

Feadback
--------

Any feedback is appreciated, especially are vulnerability reports and information about how to support more platforms and compilers.
Mail me.

Authors
-------

Author: Eugene Arshinov <earshinov@gmail.com> 
