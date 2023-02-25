Graphics
=========

Desktop Initialization
-----------------------

The graphics hardware is abstracted from the rest of the app_server.
When started, the server creates the desktop, which is little more than
a collection of workspaces. The desktop actually creates a DisplayDriver
and then calls the driver's method Inititialize() before calling a few
high-level routines for setup. Below is the process by which the
HWDriver class, which is used to access the primary graphics card in the
system, followed by the steps taken to set up the desktop.

Load Accelerant
...............

The app_server looks in three paths when scanning for an accelerant:

- /fd/beos/system/add-ons/app_server
- /boot/home/config/add-ons/app_server
- /boot/beos/system/add-ons/app_server

When the app_server searches a path, it simply prints to the debug
stream on the serial port a message akin "Attempting to load accelerant
so-and-so" when loading the accelerant. Following this, it is loads the
accelerant via load_add_on(), obtains the hook function
control_graphics_card is via get_image_symbol, and
control_graphics_card(OPEN_GRAPHICS_CARD) is called. If this returns an
error, the image is unloaded after a
control_graphics_card(CLOSE_GRAPHICS_CARD) is called and the server
spits out a message like "So-and-so is not an acceptable driver" to the
serial port. Assuming that the OPEN call succeeds, the app_server serial
prints "Using so-and-so as accelerant." Hook functions are then acquired
through control_graphics_card(B_GET_GRAPHICS_CARD_HOOKS). At this point,
it is a good idea to have a palette generated for 8-bit mode (just in
case we're going to use it), so the server generates the system palette.
The palette on the graphics card is then set through many calls to
control_graphics_card(B_SET_INDEXED_COLOR).

Set up workspaces
.................

Workspace preferences are read in from disk. If they exist, they are
used; otherwise the default of 3 workspace, each with the settings
640x480x256@59.9Hz, is used. Each workspace is initialized to the proper
information (preferences or default). Additionally, all settings are
checked and possibly "clipped" by information gained through the driver
class. With the desktop having been given the proper settings, the
default workspace, 0, is activated.

Display
.......

Provided that everything has gone well so far, the screen is filled to
the user-set workspace color or RGB(51,102,160) Also, the global
clipboard is created, which is nothing more than a BClipboard object.
The Input Server will notify the app_server of its existence, at which
point the cursor will be set to B_HAND_CURSOR and shown on the screen.

Window management
-----------------

Window management is a complicated issue, requiring the cooperation of a
number of different types of elements. Each BApplication, BWindow, and
BView has a counterpart in the app_server which has a role to play.
These objects are Decorators, ServerApps, ServerWindows, Layers, and
WindowBorders.

ServerApps
..........

ServerApp objects are created when a BApplication notifies the
app_server of its presence. In acknowledging the BApplication's
existence, the server creates a ServerApp which will handle future
server-app communications and notifies the BApplication of the port to
which it must send future messages.

ServerApps are each an independent thread which has a function similar
to that of a BLooper, but with additional tasks. When a BWindow is
created, it spawns a ServerWindow object to handle the new window. The
same applies to when a window is destroyed. Cursor commands and all
other BApplication functions which require server interaction are also
handled. B_QUIT_REQUESTED messages are received and passed along to the
main thread in order for the ServerApp object to be destroyed. The
server's Picasso thread also utilizes ServerApp::PingTarget in order to
determine whether the counterpart BApplication is still alive and
running.

ServerWindows
.............

ServerWindow objects' purpose is to take care of the needs of BWindows.
This includes all calls which require a trip to the server, such as
BView graphics calls and sending messages to invoke hook functions
within a window.

Layers
......

Layers are shadowed BViews and are used to handle much BView
functionality and also determine invalid screen regions. Hierarchal
functions, such as AddChild, are mirrored. Invalid regions are tracked
and generate Draw requests which are sent to the application for a
specific BView to update its part of the screen.

WindowBorders
.............

WindowBorders are a special kind of Layer with no BView counterpart,
designed to handle window management issues, such as click tests, resize
and move events, and ensuring that its decorator updates the screen
appropriately.

Decorators
..........

Decorators are addons which are intended to do one thing: draw the
window frame. The Decorator API and development information is described
in the Decorator Development Reference. They are essentially the means
by which WindowBorders draw to the screen.

How It All Works
................

The app_server is one large, complex beast because of all the tasks it
performs. It also utilizes the various objects to accomplish them. Input
messages are received from the Input Server and all messages not
specific to the server (such as Ctrl-Alt-Shift-Backspace) are passed to
the active application, if any. Mouse clicks are passed to the
ServerWindow class for hit testing. These hit tests can result in window
tabs and buttons being clicked, or mouse click messages being passed to
a specific view in a window.

These input messages which are passed to a running application will
sometimes cause things to happen inside it, such as button presses,
window closings/openings, etc. which will cause messages to be sent to
the server. These messages are sent either from a BWindow to a
ServerWindow or a BApplication to a ServerApp. When such messages are
sent, then the corresponding app_server object performs an appropriate
action.

Screen Updates
--------------

Screen updates are done entirely through the BView class or some
subclass thereof, hereafter referred to as a view. A view's drawing
commands will cause its window to store draw command messages in a
message packet. At some point Flush() will be called and the command
packet will be sent to the window's ServerWindow object inside the
server.

The ServerWindow will receive the packet, check to ensure that its size
is correct, and begin retrieving each command from the packet and
dispatching it, taking the appropriate actions. Actual drawing commands,
such as StrokeRect, will involve the ServerWindow object calling the
appropriate command in the graphics module for the Layer corresponding
to the view which sent the command.

Cursor Management
-----------------

The app_server handles all messiness to do with the cursor. The cursor
commands which are members of the BApplication class will send a message
to its ServerApp, which will then call the DisplayDriver's appropriate
function. The DisplayDriver used will actually handle the drawing of the
cursor and whether or not to do so at any given time.

OpenBeOS R1 will also include the advent of an extension of the API:
SetCursor(BBitmap \*), which will accept a BBitmap of color space
RGB(A)32, RGBA16, CMAP8, GRAY8, or GRAY1. Thus, color cursors and
cursors which are not 16x16 are now supported.

Display Drivers
---------------

Unlike the BeOS R5 app_server, OpenBeOS' server will have a special
feature: a modular graphics driver access class. The class is not
actually the graphics driver, but, rather, a generalized interface which
is implemented to interact with various destinations for graphics
output. This allows the server to draw to a BWindow/BView combination, a
BDirectWindow, or the actual frame buffer of a particular graphics card.
All that the rest of the server needs to do is call whichever graphics
function that is needed.

