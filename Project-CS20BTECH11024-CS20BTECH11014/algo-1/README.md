### How to compile?
There are two modes in which this code can be compiled on

- With SharedMemory values in log
  ```
  cmake -S . -B build -DSHARED_MEMORY_LOG=1
  pushd build 
  make
  popd
  ```
  This mode can be used to test if the values are read by the transactions from updated SharedMemory. 
  Example: Transaction `5` writes into DataItem `2` a value of `60`, it prints
  ```
  From Transaction 5 - SharedMemory[2] = 60
  ```
  in the log file. 


- Without SharedMemory values in log
  ```
  cmake -S . -B build
  pushd build
  make
  popd
  ```

### How to execute?
After compiling, build directory is created. It has the executable ```opt-test```. It takes file containing the parameters as input.
Sample input ```input.txt``` is provided.
```
./build/opt-test input.txt
```
Gives the log file as output in the current working directory.