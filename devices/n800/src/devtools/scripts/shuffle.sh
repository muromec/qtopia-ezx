#!/bin/sh

cd /media/hdd/Documents/music

find . -name "*.mp3"| \
  sed 's/\.\///g'   | \
  awk '{printf "/media/hdd/Documents/music/"$0"\n"}'> \
  /media/hdd/Documents/music/000-music-shuffle.m3u

cd /
