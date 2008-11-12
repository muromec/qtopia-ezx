#!/bin/sh
export QTOPIA_PATH=/opt/Qtopia.user
export QPEDIR=/opt/Qtopia.user
export QTDIR=$QPEDIR
export PATH=/opt/Qtopia/bin:$QPEDIR/bin:$PATH
export LD_LIBRARY_PATH=/opt/Qtopia/lib:$QPEDIR/lib:/lib/lib
export HOME=/root
export QWS_KEYBOARD=i686fbkbdhandler
export QTOPIA_PHONE_DEVICE=""
export QWS_SIZE="640x480"
export HELIX_LIBS=/opt/Qtopia/lib
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH
cd $QPEDIR/bin
chmod 1777 /tmp

touch /tmp/restart-qtopia
rm -f /tmp/pda-screen

rm -Rf $HOME/Applications $HOME/Settings

#while [ -e /tmp/restart-qtopia ]; do
while (true); do
  killall qpe
  killall quicklauncher
  killall mediaserver
  killall mediaplayer
  killall sipagent
  killall sxemonitor
  sleep 3
  chvt 3
  if [ -e /tmp/pda-screen ] ; then
    export QWS_SIZE="320x240"
  else
    export QWS_SIZE="220x176"
  fi  

  if [ -e /tmp/rot-90 ] ; then
    export QWS_DISPLAY=Transformed:LinuxFb:mmWidth34:mmHeight44:Rot90:0
  elif [ -e /tmp/rot-180 ] ; then
    export QWS_DISPLAY=Transformed:LinuxFb:mmWidth34:mmHeight44:Rot180:0
  elif [ -e /tmp/rot-270 ] ; then
    export QWS_DISPLAY=Transformed:LinuxFb:mmWidth34:mmHeight44:Rot270:0
  else
    export QWS_DISPLAY=Transformed:LinuxFb:mmWidth34:mmHeight44:Rot0:0
  fi

  cat /usr/share/clear.fb >/dev/fb
  qpe
  chvt 1
done

