Unexpected text file lines are not fully tested. For instance:
```
10
12
9a
8
20
```
and
```
11 9
8
20
10
```
etc., are not fully tested and not intended to be handled as this was not a lab on writing a text file parser and input cleanser.
* Infact if you enter only numbers it will fill the array with 0's and overflow the stack because of the recursion
Similarly doubles are not supported though it would be theoretically simple to implement

