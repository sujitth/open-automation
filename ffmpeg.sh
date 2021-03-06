sudo apt-get install libtool-bin libfftw3-dev pkg-config autoconf automake libtool unzip

cd /usr/src
wget http://tipok.org.ua/downloads/media/aacplus/libaacplus/libaacplus-2.0.2.tar.gz
tar -xzf libaacplus-2.0.2.tar.gz
cd libaacplus-2.0.2
sudo ./autogen.sh --enable-shared --enable-static
sudo make
sudo make install
ldconfig

cd /usr/src
git clone git://git.videolan.org/x264
cd x264
sudo ./configure --host=arm-unknown-linux-gnueabi --enable-static --disable-opencl
sudo make
sudo make install

cd cd /usr/src
git clone git://source.ffmpeg.org/ffmpeg.git
cd ffmpeg
sudo ./configure --enable-cross-compile --cross-prefix=${CCPREFIX} --arch=armel --target-os=linux --prefix=/usr/src --enable-gpl --enable-libx264 --enable-nonfree --enable-libaacplus --extra-cflags="-I/usr/src" --extra-ldflags="-L/usr/src" --extra-libs=-ldl
sudo make
sudo make install

ffmpeg -f alsa -ac 2 -i hw:1,0 -f v4l2 -s 1280x720 -r 10 -i /dev/video1 -vcodec libx264 -pix_fmt yuv420p -preset ultrafast -r 25 -g 20 -b:v 2500k -codec:a libmp3lame -ar 44100 -threads 6 -b:a 11025 -bufsize 512k -f flv rtmp://a.rtmp.youtube.com/live2/
