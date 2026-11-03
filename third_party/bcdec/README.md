# bcdec

Independent block decoder by Sergii Kudlai (iOrange), pinned to
https://github.com/iOrange/bcdec/commit/80859ed3b7afb1c527a2a99d70c61457bea72d0c .
The unmodified header contains its MIT/Unlicense alternatives; TrackRS uses MIT.
`implementation.cpp` enables the implementation in exactly one translation unit.
Used for portable CPU decoding of native BC textures when BPTC is unavailable
and to independently verify TrackRS's original deterministic encoders.
No network download is required to build or run TrackRS.
