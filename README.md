# FlipSS
Is a simple Win32 application that allows you to:

- Start/stop the screen saver.
- Enable/disable the idle timed starting of the screen saver.

Although its a Windows program, it has no UI and will normally be invoked from some script or other application.
For example, the following test batch file exercises the ability to start and stop the screen saver by starting and stopping the screen saver twice every 3 seconds.

~~~~
timeout 3
flipss /start
timeout 3
flipss /stop
timeout 3
flipss /start
timeout 3
flipss /stop
~~~~
_The initial timeout delay allows you to activate a different window (to illustrate that the current window doesn't need to be active for FlipSS to work)_
