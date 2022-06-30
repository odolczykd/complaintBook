# Complaint Book
A project that is an example of using SysV IPC written in C

[ALL PROMPT INFO DURING EXECUTION OF A PROGRAM IS WRITTEN IN POLISH!]

------------------------

**COMPILATION:**

`gcc book-server.c -Wall -o server`

`gcc book-client.c -Wall -o client`

**USAGE:**

*Server file:* `./server FILE ENTRIES` (FILE is some file used for creating IPC key / ENTRIES is a maximum number of entries in Complaint Book)

*Client file:* `./client FILE NICKNAME` (FILE is THE SAME file that was used in server execution / NICKNAME is your nickname in Complaint Book)
