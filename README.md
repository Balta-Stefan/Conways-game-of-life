# Conways-game-of-life
Made in C++ with Qt and OpenCL.
![GIF](https://i.imgur.com/0t0273d.gif)

To do:
1. Implement multithreaded CPU simulation (AVX might also be used).
2. Optimize the kernel.All threads within a workgroup share adjacent bytes.It makes sense to move them into shared memory.
