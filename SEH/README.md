## Structured Exception Handling (SEH)

### Filter Expressions

Filter expressions are the expressions used to determine the behavior of an associated exception (`__except()`) block.

### Leaving a `__try` Block

The `__try` block is exited under the following conditions:

- Reaching the end of the `__try` block and "falling through" to the associated termination handler
- Execution of one of the following statements:
    - `return`
    - `break`
    - `goto`
    - `longjmp`
    - `continue`
    - `__leave`
- An exception is raised

### Abnormal Termination

A `__try` block is said to have been "abnormally terminated" in the event that the block is exited in any manner other than "falling through" to the termination handler. 

Note that the `__leave` statement above is an exception (pun intended?) to this rule in that the effect of the `__leave` statement is to effectively jump to the end of the `__try` block and proceed to fall through to the termination handler. Thus, one may prematurely exit a `__try` block without incurring abnormal termination via the `__leave` statement.

One may test the termination condition of the `__try` block by utilizing the following function in the body of the termination handler:

```
BOOL AbnormalTermination(VOID)
```

### Combining Exception and Termination Blocks

The following program structure fails to compile:

```
__try
{
    ...
}
__except (FilterExpression(...))
{
    ...
}
__finally
{
    ...
}
```

This fails to compile because a `__try` block must have one of an associated `__except` block or `__finally` block, but it cannot have both. However, one may achieve a functionally-equivalent result by nesting blocks within one another.

### References

- _Windows System Programming, 4th Edition_ Pages 101-124
- [Unwinding the Stack: Exploring How C++ Exceptions Work on Windows (Video)](https://www.youtube.com/watch?v=COEv2kq_Ht8&t=357s)
- [Unwinding the Stack: Exploring How C++ Exceptions Work on Windows (Slides)](https://github.com/tpn/pdfs/blob/master/2018%20CppCon%20Unwinding%20the%20Stack%20-%20Exploring%20how%20C%2B%2B%20Exceptions%20work%20on%20Windows%20-%20James%20McNellis.pdf)