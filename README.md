# Server-Client Messaging Syste

A simple client-server messaging application written in C, supporting synchronous and asynchronous subscriptions. Clients communicate through a central server.

## Project Structure

- **`server.c`**  
  Contains all server-related functionality:  
  - Handles incoming messages from clients  
  - Forwards messages to the appropriate subscribers  
  - Manages subscriptions and banned users  

- **`client.c`**  
  Contains all client-side functionality:  
  - Two programs: one for sending, one for receiving messages  
  - Communicates with the server to exchange messages  
  - Stores user-related information (e.g., subscriptions, banned accounts)  
  - Each user chooses a unique username when launching the client  

## Compilation Instructions

1. Open a terminal in the project folder.
2. Compile the server and client with the following commands:

   ```bash
   gcc server.c -o server
   gcc client.c -o client
   ```

   > Note: Make sure that the filenames `server.c` and `client.c` refer to the correct source files. You may need to rename or symlink them to `inf155260_155941_s.c` and `inf155260_155941_k.c` respectively.

## Execution Instructions

1. Open **at least two terminal windows** in the project folder.
2. In the first terminal, run the server:

   ```bash
   ./server
   ```

3. In the second (and any additional) terminal(s), run the client with a subscription mode parameter:

   ```bash
   ./client <mode>
   ```

   Where `<mode>` is:
   - `0` – **asynchronous** subscription
   - `1` – **synchronous** subscription

## User Functionality

- Each user selects a **unique username** when launching the client.
- The client application maintains data such as:
  - Banned users
  - Active subscriptions

---

This project was developed for educational purposes as part of a university assignment. Not licensed for production use.
