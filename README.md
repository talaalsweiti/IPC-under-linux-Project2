# [IPC-under-linux-Project2](https://drive.google.com/file/d/1XCfKgpm_GU-iUFuKZzFUxaupkhjqXoD7/view?usp=sharing)

## Objective
This project was developed to explore and demonstrate the practical application of interprocess communication techniques under Linux. The objective was to showcase the use of shared memory, semaphores, message queues, and multi-processing to enable efficient and secure communication between multiple processes. By implementing these techniques, the project aimed to provide insights into the benefits and practicality of interprocess communication in various scenarios, such as distributed systems, parallel processing, and resource synchronization.


## Description  
- [Project Description](https://drive.google.com/file/d/1XCfKgpm_GU-iUFuKZzFUxaupkhjqXoD7/view?usp=sharing)

## Key Features
The project encompasses the following key features:

1- Shared Memory: Shared memory is used as a means of communication and data exchange between different processes. Processes can read from and write to a shared memory segment, enabling efficient and fast communication.

2- Semaphores: Semaphores are employed to synchronize access to shared resources. They allow processes to coordinate their actions, ensuring mutually exclusive access to critical sections of code or shared resources.

3- Message Queues: Message queues provide a structured way of exchanging messages between processes. Processes can send messages to a queue and other processes can retrieve and process those messages in a predefined order.

4- Multi-processing: The project utilizes multiple processes running concurrently to simulate a distributed system. Each process has a specific role and performs a set of tasks as part of the overall communication flow.

 ## How 
 - Clone the repository to your local machine.
 - In treminal:  </br>
1- Make sure you have the libfreetype6-dev library installed   </br>
  ```
 sudo apt update
 sudo apt install libfreetype6-dev
 ```
&emsp; &ensp; 2- Then you can run the program using:

 ```
 cd IPC-under-linux-Project2
 make
 ./parent
 ```
 - Monitor the output and check the generated files (**spy.txt** and **receiver.txt**) for the decoded messages
 
## Languages And Tools:

- <img align="left" alt="Visual Studio Code" width="40px" src="https://raw.githubusercontent.com/github/explore/80688e429a7d4ef2fca1e82350fe8e3517d3494d/topics/visual-studio-code/visual-studio-code.png" /> <img align="left" alt=  "OpenGl" width="60px" src="https://upload.wikimedia.org/wikipedia/commons/e/e9/Opengl-logo.svg" /><img align="left" alt="C++" width="50px" src="https://upload.wikimedia.org/wikipedia/commons/1/18/ISO_C%2B%2B_Logo.svg" /><img align="left" alt="GitHub" width="50px" src="https://raw.githubusercontent.com/github/explore/78df643247d429f6cc873026c0622819ad797942/topics/github/github.png" /> <img align="left" alt="Linux" width="50px" src="https://upload.wikimedia.org/wikipedia/commons/thumb/3/35/Tux.svg/800px-Tux.svg.png" /> 

<br/>

## Configuration
You can customize the following parameters in the source code:

- Number of helper processes
- Number of spy processes
- Thresholds for successful and failed decoding operations

Feel free to modify these parameters in **inputVariables.txt** to suit your specific needs.

## Team members :
- [Aseel Sabri](https://github.com/Aseel-Sabri)
- [Shahd Abu-Daghash](https://github.com/shahdDaghash)
- [Tala Alsweiti](https://github.com/talaalsweiti)
