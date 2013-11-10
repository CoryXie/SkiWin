SkiWin
======

Android Native Window System Based on Skia Samples.

The following are the steps to build and run this.

1. Installing Sun Java on Newer versions of Ubuntu (10.04 and above)

Open the terminal and type the following:

sudo add-apt-repository ppa:webupd8team/java
sudo apt-get update   
sudo apt-get install oracle-java6-installer

That should install the Sun Java version on your system. To change to it simply do 
the following in case you have other java alternatives:

sudo update-java-alternatives -s java-6-oracle  


2. Installing required packages (Ubuntu 12.04 and above)

You will need a 64-bit version of Ubuntu. Ubuntu 12.04 is recommended. 

# sudo apt-get install git gnupg flex bison gperf build-essential \
  zip curl libc6-dev libncurses5-dev:i386 x11proto-core-dev \
  libx11-dev:i386 libreadline6-dev:i386 libgl1-mesa-glx:i386 \
  libgl1-mesa-dev g++-multilib mingw32 tofrodos \
  python-markdown libxml2-utils xsltproc zlib1g-dev:i386
# sudo ln -s /usr/lib/i386-linux-gnu/mesa/libGL.so.1 /usr/lib/i386-linux-gnu/libGL.so


3. Get Sources

android@android:~/workspace/android$ mkdir coryxie;cd coryxie

# git clone https://github.com/CoryXie/SkiWin.git
# git clone https://github.com/CoryXie/SkiWinSwitcher.git
# git clone https://github.com/CoryXie/ScreenVideo.git

android@android:~/workspace/android/coryxie$ cd ../external

# git clone https://github.com/CoryXie/AndroidCurlBuild.git curl

4. Apply Patches

android@android:~/workspace/android$ cd external/skia/
android@android:~/workspace/android/external/skia$ patch -p1 < ../../coryxie/SkiWin/patches/0001-First-support-for-SkiWin.patch 

android@android:~/workspace/android$ cd system/core/rootdir/
android@android:~/workspace/android/system/core/rootdir$ -p1 < ../../../coryxie/SkiWin/patches/0001-Enable-system-for-input-events.patch 

5. Build

# make -j16

or in any sub-dir, do 

# mm snod

6. Run

# emulator&



