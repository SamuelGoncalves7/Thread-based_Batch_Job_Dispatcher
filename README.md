# Thread-based_Batch_Job_Dispatcher

The program accepts n number of jobs specified by the user followed by n number of commands <= 4 and a start time (n seconds away),then puts the jobs into a singly-linked list in a non-decreasing order of submit time + start time, then dispatches them. The scheduling, dispatching, and execution is carried out by three POSIX threads appropriately named, scheduler, dispatcher, and executer. The input ends once a user enters "Ctrl+C" on the keyboard.
