This is a simple pre forking webserver.
when the server is started it creates 4 worker process and puts them sleep.
when a request is received, it wakes up a worker process and assigns the request
to it. once the request is complete, worker process goes back to sleep, until
another request is assigned to it.

this repo can serve as an example for Network Socket, Unix Domain Socket and Signal Handling, etc IPC mechanism.

to run the project, simply open the repo folder in codeblock, and hit build and run button :)
