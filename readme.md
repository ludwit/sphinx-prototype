## SPHINX PROTOTYPE

### Usage 

1. Compile code with make.<br />

`make`

2. Run as many instances of the sphinx prototype as specified by *NETWORK_SIZE* in header.h (the default is 5). Run each instance by passing a one digit address to it starting from 0.<br />

`./spx 0` <br />
`./spx 1` <br />
`./spx 2` <br />
`./spx 3` <br />
`./spx 4` <br />

3. Send a message to another instance by first writing its destination address.<br />
```
./spx 0
1 hello
```

4. Reply to a message by using the alias as an address. Each alias can only be used once to reply.
  ```
  ./spx 1
  Message recieved: hello
  Reply with alias: a
  a how's things?
  ```

### Design

#### Sphinx package format
```
0                             1
0  1  2  3  4  5  6  7  8  9  0  1  
+--+--+--+--+--+--+--+--+--+--+--+--
| Route  |   SURB    | Message ...    
+--+--+--+--+--+--+--+--+--+--+--+--
```
All data is encoded as ASCII.

- **Route** consists of three bytes containing in the following order the addresses of the second and third hop as well as the destination address of the package.<br />
- **SURB**  consists of four bytes containing in the following order the addresses of three hops and the sender of the package. This part is used by the recipient to create the *Route* for a reply to the originator.<br />
- **Message** consists of 142 bytes that encode the message including the line feed and null character. Resulting in an effective message size of 140 characters.<br />

#### Example

Here is described how the prototype works internally by the example of a full message and reply circle.

1. User inputs a destination and message through terminal.
```
./spx 0
1 hello
```

2. Origin builds a sphinx package and sends it to the first hop.

The user input defines the destination address as "1" and message as "hello" (here excluded are the line feed and null character). The origin address is given by the sphinx instance itself. Resulting in a sphinx package with this layout:
```
0                   1
0 1 2 3 4 5 6 7 8 9 0 1 
+-+-+-+-+-+-+-+-+-+-+-+-+
|* * 1|* * * 0|h e l l o|
+-+-+-+-+-+-+-+-+-+-+-+-+
```
The instance pseudo randomly selects three different addresses from the network excluding the destination and origin address. The addresses are used to complete *SURB* and represent the three hops the reply will take before arriving back to the origin.
```
0                   1
0 1 2 3 4 5 6 7 8 9 0 1 
+-+-+-+-+-+-+-+-+-+-+-+-+
|* * 1|3 2 4 0|h e l l o|
+-+-+-+-+-+-+-+-+-+-+-+-+
```
The instance again selects three addresses in the same way. Two of them are saved in the route and represent the second and third hop. The first hop address is where the package is sent to.
```
0                   1
0 1 2 3 4 5 6 7 8 9 0 1 
+-+-+-+-+-+-+-+-+-+-+-+-+   send()
|4 3 1|3 2 4 0|h e l l o|     ->    ./spx 2
+-+-+-+-+-+-+-+-+-+-+-+-+
```
3. First hop forwards the message.

The instance looks for a valid address in *Route*, saves it and forwards the message replacing the saved address with "x".
```
0                   1
0 1 2 3 4 5 6 7 8 9 0 1 
+-+-+-+-+-+-+-+-+-+-+-+-+   send()
|x 3 1|3 2 4 0|h e l l o|     ->    ./spx 4
+-+-+-+-+-+-+-+-+-+-+-+-+
```
4. Second hop forwards the message.

The instance looks for a valid address in *Route*, saves it and forwards the message replacing the saved address with "x".
```
0                   1
0 1 2 3 4 5 6 7 8 9 0 1 
+-+-+-+-+-+-+-+-+-+-+-+-+   send()
|x x 1|3 2 4 0|h e l l o|     ->    ./spx 3
+-+-+-+-+-+-+-+-+-+-+-+-+
```
5. Third hop forwards the message.

The instance looks for a valid address in *Route*, saves it and forwards the message replacing the saved address with "x".
```
0                   1
0 1 2 3 4 5 6 7 8 9 0 1 
+-+-+-+-+-+-+-+-+-+-+-+-+   send()
|x x x|3 2 4 0|h e l l o|     ->    ./spx 0
+-+-+-+-+-+-+-+-+-+-+-+-+
```
6. Destination prints the message.

The instance can't find a valid address in *Route* and thereby knows itself is the destination. It prints the message and saves the *SURB* as an alias.
```
./spx 1
Message recieved: hello
Reply with alias: a
```
7. User at destination replies to message.

`a how's things?`

8. Destination builds a sphinx package and sends it to the first hop

The user input defines the message as "how's things?" and uses an alias as destination. The creation of the *SURB* is equivalent to 2. Resulting in a sphinx package with this layout:
```
0                   1                   2
0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|* * *|3 4 2 1|h o w ' s   t h i n g s ?|
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```
With the alias the instance can access the saved *SURB*.
```
0 1 2 3 
+-+-+-+-+
|3 2 4 0|
+-+-+-+-+
```
Using it to fill in the *Route* and send the package to the first hop address.

```
0                   1                   2
0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+   send()
|2 4 0|0 4 2 1|h o w ' s   t h i n g s ?|     ->    ./spx 3
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```
9. From here it repeats...

