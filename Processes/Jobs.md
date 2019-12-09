## Jobs

Jobs are a Windows mechanism that allows processes to be logically grouped and managed collectively. Historically, the primary use of jobs has been to impose limits on the system resources available to groups of processes.

### Job Management

One may create a new job object with the `CreateJobObject()` API (the call may also return `ERROR_ALREADY_EXISTS` in the event a job identified by the name specified already exists). Alternatively, one may acquire a handle to an existing job object with `OpenJobObject()`.

Once a handle to a job object has been obtained, one may add processes to the job with `AssignProcessToJobObject()`. In order to be assignable to a job object, the handle to the process must be acquired with (at least) the `PROCESS_SET_QUOTA` and `PROCESS_TERMINATE` access rights specified in its access mask.

Starting with Windows 8, jobs may be nested arbitrarily. That is, a process may be added to more than one job object, and the order in which the process is added to the job objects creates an implicit hierarchy of jobs. For instance, if process P1 is added to job J1 and subsequently added to job J2, then job J2 becomes an implicit child job of job J1, meaning that J2 cannot impose limits on process P1 that are _less restrictive_ than those imposed by job J1, but it can impose limits that are strictly _more restrictive_. In this way, the parent job J1 controls some of the functionality of job J2.

One may query various information sets regarding job objects with the `QueryInformationJobObject()` API.

Likewise, one may set various information sets regarding job objects with the `SetInformationJobObject()` API.

One may terminate a job object, which effectively terminates all processes therein, with the `TerminateJobObject()` API.

### References

- _Windows 10 System Programming_ Chapter 4