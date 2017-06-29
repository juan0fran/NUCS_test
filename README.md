# NUCS test program

Compile the test program with make.
In order to execute:

```C
    ./comms_test /dev/ttyNAME opt file_prefix_name
```
Execution options (opt) are:
| OPT | Explanation             |
|-----|-------------------------|
| t   | Blind Transmission test |
| r   | Blind Reception test    |
| x   | Retransmission test     |
| w   | Send and wait test      |
| p   | Test UART connection    |
