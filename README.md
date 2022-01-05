# ImageIntegrityController

This is a steganography-based image integrity controller. It works in two steps:
1. Image signing. ImageIntegrityController reads an image, splits it to blocks, calculates pHash for each block and embeds it using steganography.
2. Image verifying. ImageIntegrityController reads a signed image, calculates pHashes again and compares them with the original ones.

It reads any RGB images and saves only PNGs.

Tested with OpenCV 3.4.15 and Qt 5.12.0.

# References

[1] Koch E., Zhao J. Towards Robust and Hidden Image Copyright Labeling // Proc. of 1995 IEEE Workshop on Nonlinear Signal and Image Processing. –– 1995. –– P. 452–455.
