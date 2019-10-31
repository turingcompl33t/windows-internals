### Debugging User Mode Processes via a Kernel Debugger

When we fire up a new kernel debugging session, we break into the target system in an arbitrary process context. Which one?

```
kd> !process
```

The command below forces the debugger to immediately execute a context switch to the specified process. That is, it does more than simply change the debugger context, and we don't have to switch the context than release the target system to allow the Windows scheduler to perform the switch. 

```
.process /r /p <EPROCESS addr>
```