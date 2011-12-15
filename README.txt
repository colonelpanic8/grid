Marcus Robinson, Ivan Malison, Michael Kinkaid

Design Paradigm: Our objective was to design a lightweight distributed system that can process python jobs and withstand failure. The system is currently implemented on AFS.  Our implementation currently requires that all machines be running on the same binary but it would not be hard to build on the system we already have implemented to make the system more versatile. 

User Commands: 

make Runs the make file. 

Server:

./server <port #> Starts a server and a runner on the current machine. The server listens on the given port. 
./server <port #> <host name/ip> <port #> Adds a server and a runner on the current machine. The server listens on the first port given. Also adds the new server to the already established grid at the given host name/ip.

Client:

./client <port #> Allows a user to add jobs.  

The implementation works as follows:

First a server needs to be established. The established server does a number of things, 
      The server listens on the given port for other servers.
      The server establishes a runner that is prepared to recieve jobs.
      Heartbeats with each server on its server list.

Each server has two queues and a cyclic linked list.
      The first queue is the activeQueue which manages active jobs.
      The second queue is the backupQueue which stores replica jobs.
      The cyclic linked list maintains a list of servers in the grid. 

A server joins an already existing grid by requesting the serverlist from a server that is already established on the grid. The new server then messages all of the other servers and requests the add_lock from each server. If two servers try to join at the same time and find they can't obtain the add_lock, they back off.  Associated with each server is a location in the hash space. A server attempting to join the grid inserts itself into the middle of the largest gap in the hash space. The first server always establishes itself at 0.

The serverlist contains the following information for each server: location, ip, port, and number of jobs in the active queue and time stamp.   

Once a grid is running, a user can submit jobs to an individual server. The server will take the jobs and add them to its active queue. 

Starting a server also establishes a runner (note the number of runners can be easily changed in the code). The runner queries it's local server for jobs. If there are no local jobs, the runner looks at the serverlist. It then selects the server with the most number of jobs and requests a job from the servers local queue. 

Each server is also sending heartbeat pings to the other servers on the list. This helps in the event of failure.



