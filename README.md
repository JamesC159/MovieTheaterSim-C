# MovieTheaterSim-C
This C program simulates a movie theater simulation using POSIX threads and semaphores for concurrency.
Each customer, box office agent, ticket tearer, and concession stand worker run on their own threads using semaphores to coordinate and communicate their activities among each other.
Each customer can buy a ticket until that ticket is sold out. When a customer tries to buy a ticket that is sold out, they go home.
After buying the ticket, the customer must see the ticket tearer, where he can wait in a queue to have his ticket torn.
After having his ticket torn, he has a probabilistic chance that he will go to the concession stand and order some food before seeing his movie.
Each employee along the way serves the customer as needed on a customer by customer basis.
