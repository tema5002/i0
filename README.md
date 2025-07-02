<img src=".readmes/i0.png" align="right" width="150" height="150" alt="i0 logo">

# i0
i0 is a minimal init system where task start, status, and
autostart are implemented using directories and shell scripts.
No daemons, no configs, no magic.

## Quick start

### Create task layout
```
$ i0 new
Create as system task (needs sudo)? [y/N]: n
Task name: myapp
Command to run task: sleep 20
Description for task (optional): this is a test task for i0
Enable autostart? [y/N]: n
Make start template script? [y/N]: n
Make stop template script? [y/N]: n
Make status template script? [y/N]: n
[+] successfully created task at /home/tema5002/.config/i0/tasks/myapp
```

### Run the task
```
$ i0 start myapp
[>] started myapp
$ i0 start myapp
[!] already running with PID 22109
$ i0 status myapp
[+] description: this is a test task for i0
[+] running: PID 22109
[+] started at: 2025-07-02 13:26:26
$ i0 stop myapp
[<] stopped myapp
$ i0 stop myapp
[!] already stopped
$ i0 status myapp
[+] description: this is a test task for i0
[-] not running
```
it doesn't really work yet nothing else to see here
