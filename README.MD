
## Compilation

### Requirements

- [CMake](https://cmake.org/download/)
- [ZeroMQ](https://github.com/zeromq/libzmq)
- [SFML](https://www.sfml-dev.org/download.php)
- [uvg266](https://github.com/ultravideo/uvg266/tree/visualizer-backend)

In Windows ZeroMQ must be compiled from source. SFML is compiled automatically by the CMake project.

Additionally, in Windows, the DLL need to be manually copied to the binary directory. This includes the DLLs for ZeroMQ and SFML.

Finally, the font file `Ubuntu-M.ttf` must be copied to the output directory.

Compile the `visualizer-backend` branch of `uvg266`, which also requires ZeroMQ.

### Compilation

#### Linux
```bash
mkdir build
cd build
cmake ..
make
```

#### Windows
Use the CMake GUI to generate the Visual Studio project files or use the `CMakeLists.txt` directly with sufficiently recent versions of Visual Studio.

## Usage

The visualizer executable can be simply started, and it will start the ZeroMQ server and start receiving data.

FFmpeg can be used to get the video camera data to the encoder. The following command can be used to check the name of the cameras available in Windows:

```bash
ffmpeg -list_devices true -f dshow -i dummy
```

And to check the output formats of the camera:

```bash
ffmpeg -f dshow -list_options true -i video="Camera name"
```

The following command can be used to stream the camera data to the encoder and to start the encoder:

```bash
ffmpeg -f dshow -video_size 1280x720 -framerate 30 -pixel_format  yuyv422 -i video="Camera name"  -filter:v "format=yuv420p" -pixel_format yuv420p -f rawvideo - | ./uvg266 -i - --input-res 1280x720 --preset veryslow --rd 3 -p 1 --mrl --mip --mts intra --isp --no-cpuid --lfnst --mtt-depth-intra 3 --pu-depth-intra 0-8 -o a --skip-input --threads 12
```

It should be noted that the visualizer currently only accepts `yuv420p` `1280x720` resolution video data. If the camera does not support raw output use `-vformat mjpeg` instead of `-pixel_format yuyv422`. If using an existing video remove the ffmpeg part of the command and instead directly envoke the encoder with the video file as the input by replacing `-i -` with `-i video_name` and remove the `--skip-input` from the command line. Note that uvg266 only accepts raw video data as input in either `yuv420p` or `y4m` formats.

The number of threads should be selected based on the available CPU cores. The encoder is very CPU intensive and can easily saturate the CPU so two cores should be reserved for the visualizer.


The controls for the visualizer are as follows:
````
Visualizer Controls:
    F: Toggle the full screen
    F1: Toggle the help text
    G: Toggle the CU grid
    I: Toggle the intra prediction mode display. Includes planar, DC, angular, MRL, and MIP
    W: Toggle MTS and LFNST display
    E: Toggle ISP display
    H: Cycle through heatmap modes: rate-distortion, distortion, rate, and off
    Space: Pause or unpause the video
    Z: Toggle zoom overlay

Encoder controls:
    S: Toggle MRL on, on the encoder side, Shift+S toggles off
    M: Toggle MIP on, on the encoder side, Shift+M toggles off
    P: Toggle ISP on, on the encoder side, Shift+P toggles off
    T: Toggle MTS on, on the encoder side, Shift+T toggles off
    L: Toggle LFNST on, on the encoder side, Shift+L toggles off
    Ctrl+[0-3]: set the MTT depth for intra prediction
    1-8: Set the sleep amount in the encoder in order to slow down the encoding process
         1 = no sleep
         2 = sleep equal to the time it takes to encode a CU after each CU
         3 = sleep 2× the time it takes to encode a CU after each CU
         4 = sleep 4× the time it takes to encode a CU after each CU
         5 = 8×
         6 = 16×
         7 = 32×
         8 = 64×
````
