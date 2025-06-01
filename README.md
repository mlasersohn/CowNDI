Copyright © 2025 - Mark Lasersohn<br>

NDI™ is a registered trademark of Vizrt NDI AB<br><br>

This is an example of creating a NDI stream in concert with OpenCV, allowing you to turn any USB camera 
like most webcams, or any video file, into a NDI stream and pump the video through your network to any
NDI client, like CowCam or OBS Studio (with a NDI plugin, of course).<br><br>

Compile with:<br><br>

g++ -Wno-write-strings cow_ndi.cpp -o cow_ndi -I/usr/local/include/ndi \`pkg-config --cflags --libs opencv\`<br>
(or simply use the included script: mkit)<br><br>

You must have the OpenCV and NDISDK development libraries installed as this program uses both. It is helpful
to have a program like CowCam or OBS Studio (with a NDI plugin) installed so as to view the created NDI stream.<br><br>

Syntax: cow_ndi camera_path [ndi_stream_name] [video_format]<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;	camera_path is the path to the camera. This may be a device, such as as USB device, in the /dev directory, or it may be a video file. It can be any file OpenCV can use as a capture device.<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;	The optional argument ndi_stream_name is the name listed by NDI™ for the video stream.<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;	The optional argument video_format, is an integer ranging from 0 to 4, used to specify the NDI™ video format for the stream:<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;		0 = BGRX<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;		1 = RGBX<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;		2 = I420<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;		3 = UYVA<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;		4 = UYVY<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;	I420 is the default.<br><br>

Examples:<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;	cow_ndi /dev/video0 ; Stream name defaults to 'Cow NDI' and frame format to I420<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;	cow_ndi /dev/video0 MyStreamName ; Frame format defaults to I420<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;	cow_ndi /dev/video0 MyStreamName 3 ; Stream will use UYVA video frame format<br>
