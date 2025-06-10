# Training for Texas Instruments Phase Light Modulator

Only supports Linux and macOS.

## Getting Started (Linux instructions)
1. Copy this github repository

        git clone https://github.com/tsopeter/VM_NES_Training.git

2.  Download the requirement

    * Raylib 5.0
    * LibTorch 2.7.1
    * Cnpy
    * gcc
    * make
    * Basler Pylon >= 7

    Install Cnpy and Raylib to /usr/local, and install libtorch and Pylon to /opt.

3. Build project

        cd VM_NES_Training
        make

    This will build ./app.

4. Setting up FPGA.

    Go to FrameSync/Zybo/12/ and copy the files from frame_sync_z to an SD-Card. Insert SD-Card to Zybo and turn it on. Initialization sequence will take ~10 seconds.

    Connect FPGA to computer over serial. Baudrate is 115200 bps.

    You can build for other FPGAs if necessary by entering PHC_2 and modifying the hardware.

    For the default board (Zybo), connection to FPGA can be seen in FPGA-Layout.rtf. Note that a rich text file viewer is necessary

