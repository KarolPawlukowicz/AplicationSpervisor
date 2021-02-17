# AplicationsSpervisor

This is an application which checks wheather or not supervised application is working properly and wheather or not we have access to remote disc. It works in tray.


# Getting started

In order to be able to use this code you need to first import few libraries. I used codeblock and did it this way: setting -> compiler -> linker settings -> and you need to choose:
libodbc32.a
libodbccp32.a
libws2_32.a

example path will look like this: C:\Program Files (x86)\CodeBlocks\MinGW\lib\libodbc32.a




It is done by checking database records. Supervised application should add some specific records to database everyday at 5:45 and at 12:15. Every one minute we just check if those records are in database.To check if we have access to remote disc we use function „GetFileAttributesA”.
