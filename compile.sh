
#!/bin/bash
rm P*
gcc -o Monitor Monitor.c citizen.c list.c functions.c skipList.c
gcc -o travelMonitor travelMonitor.c functions.c list.c citizen.c skipList.c

 ./travelMonitor -m 3 -b 10 -s 100 -i input_dir