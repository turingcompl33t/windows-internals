; Syscalls10.asm
; Direct syscall definitions for Windows 10 systems.

.code

NtOpenProcess10 proc
		mov r10, rcx
		mov eax, 26h
		syscall
		ret
NtOpenProcess10 endp

NtClose10 proc
		mov r10, rcx
		mov eax, 0Fh
		syscall
		ret
NtClose10 endp

NtWriteVirtualMemory10 proc
		mov r10, rcx
		mov eax, 3Ah
		syscall
		ret
NtWriteVirtualMemory10 endp

NtProtectVirtualMemory10 proc
		mov r10, rcx
		mov eax, 50h
		syscall
		ret
NtProtectVirtualMemory10 endp

NtQuerySystemInformation10 proc
		mov r10, rcx
		mov eax, 36h
		syscall
		ret
NtQuerySystemInformation10 endp

NtAllocateVirtualMemory10 proc
		mov r10, rcx
		mov eax, 18h
		syscall
		ret
NtAllocateVirtualMemory10 endp

NtFreeVirtualMemory10 proc
		mov r10, rcx
		mov eax, 1Eh
		syscall
		ret
NtFreeVirtualMemory10 endp

NtCreateFile10 proc
		mov r10, rcx
		mov eax, 55h
		syscall
		ret
NtCreateFile10 endp

NtQueryInformationProcess10 proc
		mov r10, rcx
		mov eax, 19h
		syscall
		ret
NtQueryInformationProcess10 endp

end