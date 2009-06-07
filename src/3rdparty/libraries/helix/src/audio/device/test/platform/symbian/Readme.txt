greg2 is a simple GUI app for symbian that will drive the audio test
.lib generated in audio/device/test.

As it sits right now, since it was developed when we didn't have
support for generating .apps with the build system, it is a MSVC
project (greg2\group\greg2.dsw). Since the .dsw and .dsp files have
hardcoded path names you will have to copy the greg2 folder to
C:\Symbian\6.1\Series60\Epoc32Ex\greg2 in order for it to compile
and run.

The audio unit test .lib must be copied to somewhere where the
linker can find it (testauddevice.lib). Greg2 links against this
.lib in order to run.

The reason we need this GUI app under symbian is because if you 
just compile and run a .exe under the emulator the media server
is not started by the emulator and all audio calls will fail.