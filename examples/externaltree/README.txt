You must copy this directory outside of the source tree or it will
not work properly.

eg.
mkdir ~/tmp/externaltree
cp -R * ~/tmp/externaltree
cd ~/tmp/externaltree
$QPEDIR/bin/qtopiamake
make

For a description of the parts of this tree see the build system
documentation -> advanced tasks -> Create a new project tree.
