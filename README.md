# Simple KaZaA Style P2P Application
A simple KaZaA style p2p application, which uses a super-node approach to improve the performance of searching

##
###Preliminaries
- csapp module (csapp.c & csapp.h) is borrowed from the [page](http://csapp.cs.cmu.edu/public/code.html) of Bryant and O'Hallaron's Computer Systems: A Programmer's Perspective.
- GetOpt module is inspired from the [page](https://www.gnu.org/software/libc/manual/html_node/Getopt-Long-Option-Example.html#Getopt-Long-Option-Example) of GNU

##
### Descriptions
1. Make clean & Make all to clean and then to compile the project

2. General informations :
  - There are only 2 super nodes possibles
  - Child will do all the necessary things (say hello to super and file info sharing) before listening to its socket and stdin. Any error from the necessary things will stop the child immediately.
  - Super will do all the necessary things (say hello to other super if it is precised in the options) before listening to its socket for all the requests.
  - The protocol mostly behaves as stateless protocol, meaning just reply to the received packets (except for the neccessary things and file download for child and necessary things for super). These neccessary things are also refered as connection set-up.
  - No retrying in case of errors.
  - Everytime a packet is sent, a new tcp connection with a different client port is used. This connection will wait for a reply in most cases and then will be terminated.
  - Child will maintain a listening socket only for stdin and file download requests.
  - Super will maintain a listening a socket only for updating each other with other super nodes for its listening port for updating the list of file. It is also for any incoming file query and child hello from its children.
  - All the listening sockets is about 2s of timeout as requested.
  - All files accepted except « directory ».
  - FILE INFO will be shared directly with neighbour super node once it is received from the children.

3. Data structure :
  - All utilised structures are found in the include/data_structure.h.
  - Hashtable with string as file_index (key) is used for storing file's information (filename as the key and the other information of file as the value).
  - Hashtable with integer as file_index is used for storing childs' information (child id as the key and child data structure as the value).
  - Simple hash function is used which is not robust to collisions.
  - A packet data structure is introduced to facilitate of the use different field information of the packet.
  - 'Child' data structure is used to store child's information (ip address and port number)
  - 'Files' data structure is used to store different information about the file (filename (file_index) in string and filesize)

4. Global variables :
  - It can be found in the include/data_structure.h.
  - superContents are used to store Super's list of updated file informations.
  - childsPortNumber are used to store Super's list of connected childs.
  - NeighbourSuper is a variable to store the neighbour's super node information for the other super node.
  - NeighbourSuperIsUpdated is a boolean to indicate if the information of super node is updated (received).
  - SuperNodeIp & superNodePortNumber are the variables (informations) for the child to be connected to its specific super node.

4. Unique ID :
  - ID (unique id) is created from a combination of current time (from Epoch Time 1970) in seconds and port number. 
  - This is garanted unique and without collision within one second of range and for approximately a range of one hour as time is used for 2 bytes and another 2 bytes for port number. 
  - Ip is not used as the id because 4bytes ID  is not enough to store the Ip and port number.
  
 ##
There are still bugs to be resolved, and improvements to be done.
