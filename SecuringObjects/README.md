### Security Descriptor

In addition to access tokens, security descriptors are the other side of the object security equation
- Specifies who can perform what actions on an object
- The security descriptor is the data structure that maintains this information

Query security descriptors with:
- GetSecurityInfo
- GetKernelObjectSecurity
- GetFileSecurity
- GetNamedSecurityInfo

Security descriptor has embedded access control lists
- DACL -> discretionary access control list
    - Access control entries in this list specify a SID and the rights granted to that SID
- SACL -> system access control list
    - Specifies which actions by which users should be logged
    - If a SACL is null, no auditing occurs for that object 

Kernel debugging
- The pointer to the SD is in the object header of the EPROCESS object
- Use the !sd command to dump the SD
- Caveat here about bitwise-anding the pointer first because some bits of the address are used as flags??? (&-10 on 64 bit system) e.g !sd <addr> & -10 (64 bit)

### Security Identifier (SID)

Instead of using names to identify entities which perform actions within a system, Windows uses SIDs. 

Who has a SID?
- Users
- Local and domain groups
- Local computers
- Domains
- Domain members
- Services

SID components
- ‘S’ prefix
- SID structure revision number
- 48 bit authority value -> who issued the SID
- Variable number of
    - 32 bit sub authority values
    - Relative identifier (RID) values

### Integrity Levels

- Orthogonal to access control?
- We can isolate code and data running within the context of a user account
- 6 levels commonly used
    - 0 = untrusted
    - 5 = protected
- For instance, mark an untrusted process like a web browser with a low integrity level, will prevent it from doing certain things
- Processes have integrity levels, but so do other objects 
- Windows enforces a "No Write Up" policy by default such that a process may not "write" any object that is configured with a higher integrity level
    - e.g. Run notepad.exe at low intergity level and you won't be able to save a new file to any location other than the LocalLow directory
- Implicit medium integrity level system-wide

### Access Tokens (aka Tokens)

What is a token?
- Identify the security context of a process or thread
- Identify a user’s (entity’s?) credentials, and thus their privileges
- When user logs in, LSASS checks if the user is a member of a powerful group or possesses a powerful privilege; if this is the case, LSASS creates a restricted token (a filtered admin token) for the user in addition to the powerful one, and creates a logon session for each

Access Tokens APIs
- LogonUser
- CreateProcessAsUser
- CreateProcessWithLogon

Token Components
- Group SIDs -> identify the groups to which the user represented by the token belongs
- Privilege array
- Default primary group, default DACL -> used when the process associated with this token creates an object, applied as default security policy
- Many other "informational" fields

Trust ACEs
- Allow protected processes and PPL to make objects accessible only to other protected processes
- Identified by well-known SIDs
- Higher trust SID = more powerful token
- Listed in the kernel debugger as "TrustLevelSid" within the token object

Debugging
- Use dt _TOKEN to view token structure
- Use the !token command to examine token for a process