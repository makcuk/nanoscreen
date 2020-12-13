Nanoscreen ESP32

What this for?

Sometimes when you do home automation, you need to have a simple display to show something in non-primary 
rooms/locations. For example, I use this device to show some metrics in the garage. 

Workflow

Very simple push model. You get metrics using python script somewhere on your home automation server, generate .bmp image from the template and upload this image to nanoscreen. More often uploads - more real-time values displayed. For time with hour and minutes once on 30 secs is OK. 