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

============= ============= ========= ==========================
Program Name  Server/Client Protocols Implementation details
============= ============= ========= ==========================
client        Client        TCP, UDP  none
server        Server        TCP, UDP  none
client_select Client        TCP       ``select()`` function used
server_select Server        TCP       ``select()`` function used
============= ============= ========= ==========================

All servers are interchangeable. And are the clients.

Feadback
--------

Any feedback is appreciated, especially are vulnerability reports and information about how to support more platforms and compilers.
Mail me.

Authors
-------

Author: Eugene Arshinov <earshinov@gmail.com> 
