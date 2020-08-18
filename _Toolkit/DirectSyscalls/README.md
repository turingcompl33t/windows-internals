## Windows Direct System Calls

Access system operations and resources without touching any userspace API.

### Resources

Inspired by the post [here](https://outflank.nl/blog/2019/06/19/red-team-tactics-combining-direct-system-calls-and-srdi-to-bypass-av-edr/) from Outflank.

Made possible by the Windows [system call table](https://j00ru.vexillium.org/syscalls/nt/64/) documented by Mateusz "j00ru" Jurczyk. 
