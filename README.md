//Cagan Bakirci
//CruzID: cbakirci
//Assignment 3
//CSE 130: Principles of Computer System Design
//Ethan Miller
//December 7, 2019

//This program is the implementation of a multi-threaded HTTP server.
//The server responds to simple GET, PUT, and PATCH commands. 
//These commands read a specified file from the server and write a file to a server respectively. 
//The program does not use any of the C library FILE * functions. 
//The program does not allocate more than 16KiB of memory for each thread. The Makefile was
//taken from the following address and modified: 
//https://gitlab.soe.ucsc.edu/gitlab/classes/cse130/templates-cse130-02-fall19/blob/master/template-Makefile 

//Notes: The program does not support logging, I could not implement this part for the 
// asgn 2. There is also a bug with the patch command. In order for the patch command to
// work, the hash text file should be created every time. for some reason, open(name, O_CREAT | O_RDWR)
//return -1 if the file exists and this causes the entrie request to crash.

//The project contains the following files:

//httpserver.cpp: The server program.

//DESIGN.pdf: The design document for httpserver.cpp

//WRITEUP.pdf: The write-up describing tests and answering the assigned question.

//MakeFile: For compiling the files.

//----------------------------------------------