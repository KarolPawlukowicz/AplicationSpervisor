# AplicationsSpervisor

This is an application which checks wheather or not supervised application is working properly or wheather or not we have access to remote disc. It is done by checking database records. Supervised application should add some specific records to database everyday at 5:45 and at 12:15. Every one minute we just check if those records are in database.To check if we have access to remote disc we use function „GetFileAttributesA”.
