Macarie Razvan Cristian
======
<p>
Program written in pure C++, using TCP and input multiplexing.
You should be able to test this on any linux machine (or on WSL 2) without
installing anything extra.
Libraries used: 
    Nlohman for JSON https://github.com/nlohmann/json
    Berkley sockets
    TCP and the Linux networking stack
</p>

Preface
======

<p>
    My current priority is getting good grades in the exams to keep
my scholarship and the assignment came in the middle of the exam 
period. I am currently in between exams and I didn't have time to check
the code (I just made it compile) but I decided to send it anyway
more as a proof of concept for some of my knowledge. I wrote this code
in a few hours one night because I really wanted to show something rather
than nothing. 
    I did it in C++ because I already had some experience in making a server
in C++ using sockets. I would have also made a client from which to make 
the requests but I just left it as an open RESTful API anyone can querry.
</p>

Task1.
======
<p>
    Here I wanted to use an SQL database like SQLite for C++ but I didn't have
the knowledge (I only know SQL) and I didn't have the time to research how to 
use the library in C++.
    I opted for fields in a class and to store the objects in a vector inside
a database object. Now that I think of it I should have used JSON objects as
it has low overhead and would have been easier to work with.
</p>

Task2.
======

<p>
    I wanted to use Bearer as I had minimal experience with JWT 
(https://github.com/BlunderBoy/REST-client). In the end just went for basic
and also sent username and passwords in plain-text.
</p>

Task3.
======

<p>
    Rate limiting is done per ip. Even if you make another account as long as
you use the same ip you won't be able to make any request if you exceeded the limit.
The limit I put is 10 requests every 15 minutes. The values can be easily modified
as they are in defines at the top of the code.
    How it's actually implemented: Every time a new ip logs into the server the ip,
the file descriptor and the current time (as UNIX time) are saved in a database.
The program checks if you have requests left and deducts one from your total.
At every point in the main loop we check for each ip adress we have if 15 minutes 
have passed and reset the request allowance.
    If you try to exceed the number of requests you get a 429 Too many requests as
a reply.
</p>

Overview:
======

<p>
    The server is implemented in basic C++ using just the STL. The only external
libraries used are the networking ones from linux, the sockets API and a JSON 
library (because I already knew how to use it and I like JSON). As many people as
the maximum number of clients can connect at the same time (using select on file
descriptor sets). Input from stdin isnt ignored but the only command that the
server can read is "exit".
    The server is written over TCP with the maximum request size of 1.5KB.
It runs on the port designated by the PORT macro, ipv4 and listens for any
incoming adress.
    Everyone can add and querry products from this API.
</p>

The API:
======
```
POST:
    -at api/register to register a new account.
        Returns an warning if the account already exists.
        Returns an error if it doesnt recieve json data.

    -at api/login to log into an existing account.
        Returns an error if it doesnt recieve json data.
        Returns an error if the username isn't in the database.
        Note: i forgot to add a "wrong password reply".
        Returns a cookie (the username + 10 character random string)

    -at api/add_product to add another product.
        Returns an error if it doesnt recieve json data.
        If the product exists already, it gets modified and date updated is reset.
        Returns an error if you dont have a cookie.

GET:
    -at api/products/books to get a json list of all the books.
        Returns an error if you dont have a cookie.
```

Final observations
=========
<p>
    The server has internal checks for databases to not add the same ip twice, to not
add the same user twice and for every send going outside the server I used the DIE macro to
see if anything fails and where. Most of the logic is split up in small functions. The definitions 
for the classes used are split up inside each others files.
</p>
