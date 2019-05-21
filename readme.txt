ReadMe

EXTRA CREDIT OPTION

Build with ./PA2 inputfile outputfile

I did the extra credit portion of this PA2 as I found it to be much cleaner and easier to test

I have the parent process be the producer thread as that will always be finished before the consumer thread allowing me to properly have the parent wait for the child

I hardcoded the size of 1024  instead of using #define variable at the top for easier access but I realised this later and did not want to change it.

Both mutexes are located out of their respective loops to properly encase all of the fprintf and fscanf statements. Since The cond_wait released the mutex while waiting it allows for this to work perfectly and have no lock out

I originally programmed with a circular queue struct placed inside of the shared memory struct however I realized it was pointless and caused too much trouble with extra pointers so I moved it into the shared memory segment for easy access. I also had seperate functions for add and remove element from queue but that wasn't great and each thing was simple enought they could be moved simply into their respective thread. 

Since this is the single program fork() version it should never encounter a scenario where the shared memory is already created but just in case there is also the option coded in to just attach to it if this situation arises so that it can work properly.

