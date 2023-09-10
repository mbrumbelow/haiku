.. _tun_tap_documentation:

==========================================
TUN/TAP Driver and Interface Documentation
==========================================

.. contents::
	:depth: 2

Introduction
------------

This document provides an overview and technical details about the TUN/TAP driver and interface. The TUN/TAP driver is a virtual network device driver that enables user-space applications to interact with network traffic at layer 2 (for TAP) or layer 3 (for TUN). It is commonly used for creating virtual network interfaces, allowing for various network-related tasks such as tunneling, VPNs, software-defined networking, and packet manipulation.


High Level Overview
-------------------

In this section I want to go over how the driver, interface, and any applications that use the TUN/TAP driver interact with one another. One thing that I think is important to mention before we talk about these interactions is that, as of time of writing, the TUN/TAP driver and interface are implemented seperately from each other in the operating system's kernel compared to most other TUN/TAP driver implementations. This is due to how you write network drivers in Haiku because of the modular and layered networking stack where the driver just takes care of the physical network card interactions while the interface takes care of getting data from the network stack to the driver. The driver is located in ``src/add-ons/kernel/drivers/network/tun/driver.cpp`` and the interface is located in ``src/add-ons/kernel/network/devices/tun/tun.cpp``. One last thing, more information about how the driver and interface work individually are in their respective sections below this section.

Driver Piece
~~~~~~~~~~~~

Now that we have that out of the way, let's get into the details of the interactions. Starting with the driver implementation, the driver has 6 main functions that get used throughout both the interface and applications being ``tun_open``, ``tun_close``, ``tun_ioctl``, ``tun_write``, ``tun_read`` and ``tun_select``.

* ``tun_open`` is how the driver handles the open syscall that the interface and applications will call on it. This function will setup the tun_struct with the queues this open instance will use, what condition variable to keep track of, name, flags, and mutexes. This defaults to application settings but an IOCTL call that the interface makes right after it opens the driver changes that.

* ``tun_close`` is how the driver handles the close syscall.

* ``tun_ioctl`` is for the operations of TUNSETIFF, that only the interface calls to change the tun_struct to interface settings, and B_SET_NONBLOCKING_IO which the application side only calls to enable non-blocking I/O for its side.

* ``tun_write`` and ``tun_read`` are how the driver enqueues and dequeues packets with the BufferQueue's given the tun_struct stored in the drivers cookie.

Read will block until notified that data is there whether its the condition variable for the interface side or select for the application side. Once notified, it will retrieve the packet data from the queue and return if successful.

The ``write`` operation in this context is non-blocking. It appends data to the ``sendQueue`` associated with a specific open instance, provided that there is sufficient space in the queue. When this operation is called from the application side, it also signals the associated interface's condition variable. This signal indicates that new data is available for reading, enabling the interface to fetch and process the data from the queue.

* ``tun_select`` is used to efficiently wait for and handle I/O events made on the driver without the need for busy waiting.

Interface Piece
~~~~~~~~~~~~~~~

Next is how the interface interacts with the driver. The interface is how the user applications data will get to the driver via the network stack. When it's time for the interface to go up, the interface will open the driver where it is statically as of writing being ``/dev/tap/0`` and runs the ``TUNSETIFF`` IOCTL call to change what side this open instance is in the driver. After the interface comes up, the ``tun_receive_data`` function is constantly called by the network stack which is blocked indefinetely by the driver via condition variable until there is data to read in its open instance's ``recvQueue`` and notified by the application side's ``write`` to the driver.

When we use a program like ``ping`` is used to make sure the other side is up, that packet will make it to the interface and written to the driver via ``tun_send_data`` and right before the write finishes in the driver, it will notify the global select pool that the application side is using for its non-blocking I/O to tell the application side that new data has appeared.

Application Piece
~~~~~~~~~~~~~~~~~

Lastly, we have how applications interact with the driver. How every application starts will be very different but some things that will be present are the calls to make the driver non-blocking and the POSIX ``open``, ``close``, ``read``, ``write``, ``ioctl`` calls to interact with the driver. ``ioctl`` will be called after the driver gets opened to set non-blocking I/O and possibly other things depending on the application itself. ``read`` and ``write`` are going to be the main way applications will get the packets from userspace which will then send the edited packets through your devices physical interface that is connected to your network.


Test Procedure
--------------

Here are some of the unit tests that are ran against the driver to make sure that the driver itself is working as it should.

.. note::
	These tests require OpenVPN and its source code to be available on the system before doing these tests.

* One of the easiest tests that come from OpenVPN is ``ifconfig --remote some.random.addr.here --ping 1 --verb 3 --dev tap`` (This one currently doesn't work since it wants one queue to constantly look at to work while this implementation uses two queues)

* If you have the openvpn source on either Linux or Haiku, you can go into the ``/sample/sample-keys`` directory, and run ``openvpn ../sample-config-files/server.conf`` and on another system or vm you can run ``openvpn ../sample-config-files/client.conf``. You have to edit certain aspects of the .conf files like changing ``proto udp`` to ``proto udp4`` and change the IP that you want to connect to.

*


TUN/TAP Driver Overview
-----------------------

This driver serves as an intermediary between a network interface and an application, facilitating the transfer of data packets between them. It acts as a temporary storage buffer, allowing the application to read incoming data and enabling the interface to receive and process data sent by the application.

The tun_struct Data Structure
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* ``name``: Identifies which side (application or interface) initiated the driver.
* ``flags``: Stores various flags obtained through IOCTL operations.
* ``sendQueue``: A queue where data is appended when the application writes to the driver. It can be set to either ``gAppQueue`` or ``gIntQueue``, depending on the direction of data flow.
* ``recvQueue``: The counterpart of ``sendQueue`` as this queue is used for reading data and its assignment depends on being the opposite of what ``sendQueue`` was configured to.
* ``readWait``: A condition variable employed by the interface side to block until data becomes available for reading, at which point it's notified.
* ``readLock``: A mutex used for synchronizing read operations.
* **OPTIONAL** ``select_lock``: Initialized when the application side is created, this mutex prevents deadlocks when the select function is employed. It gets released when the tun_struct transitions from the application side to the interface side.

Understanding sendQueue and recvQueue
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

When the driver is opened by the application, ``sendQueue`` is configured as ``gIntQueue``. This setting designates it for sending data to the interface, while recvQueue is set as ``gAppQueue``. This setup allows the application to read data appended to ``recvQueue`` by the interface.

Conversely, when the driver is initiated by the interface, the configuration flips. ``sendQueue`` becomes ``gAppQueue``, facilitating the interface in sending data to the application, while ``recvQueue`` switches to ``gIntQueue``, enabling the interface to read data from the application.

Non-Blocking I/O Implementation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

For applications that rely on non-blocking I/O and commonly use mechanisms like select, poll, epoll, or kqueue, the driver implements select functionality on the application side. This approach accommodates applications that cannot utilize asynchronous I/O, providing an efficient way to handle I/O events without busy waiting.


TUN/TAP Interface Overview
--------------------------

The TUN/TAP interface serves as an intermediary between network applications and the operating system's network stack as it provides a means for applications to read and write network packets to and from the driver.

The TUN/TAP interface is how the network stack will interact with the driver. When initializing through ``tun_init`` for this interface, it will setup many of the things needed when using an interface like a MAC address, what flags to use to enable certain functionality, etc. All of that is saved in the tun_device object made when initializing and gets used throughout the rest of the functions. It isn't until we establish that we want to bring the interface up when it opens the driver at ``/dev/tap/0`` since it currently does not .

Afterwards, the network stack will start to try and call ``tun_recieve_data`` constantly which is blocked when it hits read via a condition variable in the driver.


TODOs
-----
* Add asynchronous I/O support to the operating system to support AIO functionality
* Add dynamic driver loading
* Add Point-to-Point Protocol to the OS to get the TUN functionality working
* Add seperate function for ethernet bridging for the driver


Conclusion
----------

This documentation provides an overview of the TUN/TAP driver's functionality, features, and implementation details. It also outlines common usage scenarios where the TUN/TAP driver and interface are employed.

For more detailed information and implementation specifics, refer to the source code of the operating system's TUN/TAP driver.
