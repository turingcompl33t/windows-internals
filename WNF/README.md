## Windows Notification Facility (WNF)

WNF implements a blind publisher-subscriber model that remains (relatively) undocumented.

### WNF State Names

WNF state names are 64-bit numbers that encode information in a data structure. Prior to decoding the data structure from the value, the WNF state name must be XOR'ed with the magic value: `0x41C64E6DA3BC0074`.

WNF state names fall into one of four categories:

- _well-known_
- _permanent_
- _persistent_
- _temporary_

### Programming APIs

Register a new WNF state name:

```
ZwCreateWnfStateName
```

Unregister / remove a WNF state name:

```
ZwDeleteWnfStateName
```

Publishing WNF state data:

```
ZwUpdateWnfStateData
```

Wipe the current state data buffer:

```
ZwDeleteWnfStateData
```

Consuming WNF state data:

```
ZwQueryWnfStateData
```

Receiving WNF notifications via system calls:

- `ZwSetWnfProcessNotificationEvent`
- `ZwSubscribeWnfStateChange`
- `zwGetCompleteWnfStateSubscription`

Higher-level API:

- `RtlSubscribeWnfStateChangeNotification`

### References

- [Github: wnfun](https://github.com/ionescu007/wnfun)
- [Windows Notification Facility: Peeling the Onion of the Most Undocumented Kernel Attack Surface Yet (Video)](https://www.youtube.com/watch?time_continue=90&v=MybmgE95weo&feature=emb_logo)
- [Windows Notification Facility: Peeling the Onion of the Most Undocumented Kernel Attack Surface Yet (Slides)](http://alex-ionescu.com/publications/BlackHat/blackhat2018.pdf)
- [Playing with the Windows Notification Facility](https://blog.quarkslab.com/playing-with-the-windows-notification-facility-wnf.html)
- [Find Which Process is Using the Microphone, from a Kernel-Mode Driver](https://gracefulbits.com/2018/08/13/find-which-process-is-using-the-microphone-from-a-kernel-mode-driver/)
- [Windows Process Injection: Windows Notification Facility](https://modexp.wordpress.com/2019/06/15/4083/)