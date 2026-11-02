# Generated intro test fixture

`intro.mpg` is a 2-second, 64x48 MPEG-1/MP2 program stream generated from
FFmpeg's `testsrc2` and `sine` filters, not a third-party promotional video.
Reproduce: `ffmpeg -f lavfi -i testsrc2=size=64x48:rate=25 -f lavfi -i sine=frequency=440:sample_rate=44100 -t 2 -c:v mpeg1video -c:a mp2 -f mpeg intro.mpg`.
It is a decoder test fixture only and is not installed as game content.
