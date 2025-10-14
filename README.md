# Simple Video Speed Changer

A simple video speed changer application built with Qt6 and ffmpeg.


![Screenshot](data/image.png)

## Tested On
- Ubuntu 22.04, GCC 12.3.0, Qt 6.6.2
- Windows 11, MinGW 13.2.0, Qt 6.9.1

## Build

```bash
mkdir build && cd build
# Replace <QT DIR> with your Qt installation path (e.g. /home/user/Qt/6.6.2/gcc_64/)
cmake .. -DCMAKE_PREFIX_PATH=<QT DIR>
cmake --build . --config Release
```

Alternatively, open the project with Qt Creator and build it from the GUI.
