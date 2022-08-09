# CIP-PIR

CIP-PIR is an efficient implementation of multi-server [private information retrieval](https://en.wikipedia.org/wiki/Private_information_retrieval).

Details of the underlying protocols can be found in the paper "[GPU-accelerated PIR with Client-Independent Preprocessing for Large-Scale Applications](https://encrypto.de/papers/GHPS22.pdf)" published at the [31st USENIX Security Symposium (USENIX Security'22)](http://digitalpiglet.org/nsac/ccsw14/) by:
* [Daniel Günther](https://encrypto.de/guenther), Technical University of Darmstadt, [ENCRYPTO](https://encrypto.de)
* Maurice Heymann, Technical University of Darmstadt
* [Benny Pinkas](http://www.pinkas.net/), Bar Ilan University
* [Thomas Schneider](https://encrypto.de/schneider), Technical University of Darmstadt, [ENCRYPTO](https://encrypto.de)

**Warning:** This code is **not** meant to be used for a productive environment and is intended for testing and demonstrational purposes only.

### Requirements
---
* A **Linux distribution** of your choice (The UC compiler was tested with [Ubuntu](https://ubuntu.com) and [Kali](https://kali.org))
* **g++**
* **make**
* **[CMake](https://cmake.org)**
* **[OpenMP](https://www.openmp.org/)**

For GPU acceleration, you additionally need the following:
* **[NVIDIA driver for data center GPUs](https://docs.nvidia.com/datacenter/tesla/index.html)**
* **[CUDA Toolkit](https://developer.nvidia.com/cuda-toolkit)**

### Build

```
mkdir build && cd build
cmake ..
make
```

### Execute
We explain how to run the code locally.

1. Go through the file `src/parameter.h`, put in your configuration, and run
```
make
```

This configuration file also contains flags which enables the GPU version of the code, parallelisation via OpenMP, and the pipelining approach.

2. Create the database with

```
./bin/makeDatabase
```

Because the "makeDatabase" method is not working consistently on all platforms, it has been partly disabled.
To simulate the preprocessing, please run:

```
dd if=/dev/urandom bs=1024 count=1250048 >> ./bin/database/raw.db_preprocess.db
```

Thereby, bs equals the protocols block size and `count = GROUPS * GROUPSIZEP2` with `GROUPSIZEP2=1 << GROUP_SIZE` from `src/parameter.h`

3. Run the PIR servers with
```
./bin/server -i 0 
./bin/server -i 1
```
where `i` specifies the server id. You may run `./bin/server -h` to see a list of further parameters.

4. Run the PIR client with

```
./bin/client
``` 

You may run `./bin/client -h`to see a list of further parameters. You should also specify the server IP addresses and ports via the `party.addServer()` method in `src/application/client.cpp`

