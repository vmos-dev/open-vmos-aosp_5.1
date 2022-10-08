TestingCamera2 is a test application for the Android camera2 API (android.hardware.camera2.*).

1. Overview

The goal of Testingcamera2 is to allow all API primitives and behaviors to be excercised through
a reasonable UI, and allow simple demos and test cases to be built on top of it.

The concepts the application uses match the concepts of the API itself, so any given configuration
in the application can easily be reproduced directly with the camera2 API.

2. UI concepts:

The UI is divided into two panels, organized either side-by-side (in landscape orientations) or
above each other (in portrait orientations).

The left (or above) panel generally displays output from the camera subsystem, while the right
(or below) panel generally has controls for interacting with the camera subsystem.

Each category of object in the API is configured through a pane dedicated to that category; the
panes are grouped in lists by category, and can be added, removed, or collapsed from view
arbitrarily.

The left pane contains target panes along with the app's overall log pane.  The right pane contains
camera, request, burst, and utility panes.

The basic operation of the app is as follows:

  1. Add a camera pane, select camera to use
  2. Add at least one target pane, and for each, select the kind of target desired and its
     configuration options.
  3. Open and configure the camera from the camera pane
  4. Add a request pane, select the desired camera and targets, and the desired use case template.
  5. Start capturing images with either 'capture' (one-shot) or 'repeat' (continuous) buttons on
     the request.

The application also has a set of preset configurations that can be accessed from the top-right
dropdown menu, to speed up operation.

2.1. Camera panes

The camera pane includes device-level controls for a single camera, which can be selected from all
the cameras available on the Android device.  It allows for opening the device, reading out its
camera characteristics information, for configuring a set of targets, and for stopping or flushing
the camera.

2.2. Target panes

Target panes represent various destinations for camera data.  The kind of destination desired, and
the camera this target is meant for, are always available to select.  Each kind of destination
has its own set of controls for configuration and operation.

The pane always has a 'configure' toggle, which specifies whether that target pane will be included
in the selected camera's next configure operation. Until the target pane is included in a camera's
configuration, it cannot be used as a target for capture requests to that camera.

2.2.1. TextureView and SurfaceView targets

These are basic outputs generally used for camera preview; each is simply configured by the size
of buffers it requests from the camera.

2.2.2. ImageReader target

This is the basic still capture output, used for JPEG or uncompressed data that needs to be
application-accessible. It is simply configured with the desired output format, and the size.

2.2.3. MediaCodec and MediaRecorder targes

These are video recording targets, which use the Android device's video encoding APIs to create
video files.

2.2.4. RenderScript target

This target sends camera data to a RenderScript Allocation, from which it can be efficiently
processed by Android's RenderScript APIs.

2.3. Request panes

Request panes are used to set up the configuration for a single camera capture request. It includes
all the settings required for the camera device to capture a frame of data. It contains selectors
for the camera to send the request to, and then which of the configured target panes to use for
image output.  A given request can send data to any subset of the configured targets, but it
must include at least one target.

The camera device provides default values for all the request settings for various use cases; the
starting template can be selected from the dropdown.

Once a camera has been opened and configured, and the correct target(s) has been selected for the
request, the request can be sent to the camera with either the capture or repeat buttons.

Capture is a one-shot operation, which simply instructs the camera to capture a single frame of
data. This is commonly used for high-resolution snapshots, for triggering autofocus, or other
single-occurance events. Captures are queued and processed in FIFO order.

Repeat is a continuous operation, where the camera device continually captures the images using
the same request settings.  These have lower priority than captures, so if some request is actively
repeating, other requests can still use the 'capture' action to trigger single captures. Another
request triggering repeat will pre-empt any previous repeat actions by other requests.

To stop repeating, use the camera pane's stop method.

2.4. Burst panes

Burst panes simply aggregate together a set of requests into a single high-speed burst. Bursts
are captured atomically with no intervening other requests.  Similarly to single requests, they
can be one-shot or continuous.

The burst pane contains the target camera to send the burst to, and then allows including one or
more requests into the burst, from the list of requests that target the chosen camera.

2.5. Utility panes

Utility panes are catchall panes, which can implement various kinds of ease-of-use functionality,
such as implementing simple autofocus operation, still capture sequencing, or other high-level
functions.

2.6. Configuration load/save

TestingCamera2 supports loading a predefined set of panes from an XML definition. The definitions
can either be one of the default included sets, or located on the device SD card.

3. Internal architecture

Each pane is a specialized view, with a few generic methods for setting them up and notifying them
of changes in other panes.  Panes are tracked by a global pane tracker, which allows individual
panes to notify others of status changes (such as a change in the selected camera, so that all
panes targeting that camera pane can update their displayed settings choices), to find panes of
given types, and so on.

The operation of the camera devices is centralized in a single camera ops class, which keeps track
of open devices, notifies panes about changes to camera device status, and allows the utility panes
to intercept/override camera device operations as desired.

