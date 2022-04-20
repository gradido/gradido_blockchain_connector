# Gradido Blockchain Connector

For easy connecting to [Gradido Blockchain](https://github.com/gradido/gradido_blockchain). 

- It generate and stores ed25519 public and private key pair for user, but the private key will be stored only
encrypted with user password.
- It sign and send Gradido Transactions via Iota into to blockchain
- store sended transactions in memory and has interface for [Gradido Node](https://github.com/gradido/gradido_node) to tell that pending transaction was confirmed or rejected
- use JWT Token for authenticated json requests

## Getting Started

1. Compile GradidoBlockchainConnector, for more details look here: [INSTALL.md](INSTALL.md)
2. copy default config from ./config/blockchain_connector.properties to ~/.gradido/blockchain_connector.properties
3. make changes as needed, look for details into the default file ./config/blockchain_connector.properties
4. on linux create folder /var/log/gradido/ and give write permission to user which will be start GradidoBlockchainConnector
5. run GradidoBlockchainConnector 


## Command Line Options

- to start Gradido Blockchain Connector as daemon in linux use ```--daemon ```
- for starting it on windows as service look here: [Poco::ServerApplication](https://www.appinf.com/docs/poco-2013.2/Poco.Util.ServerApplication.html)
- to record process id if started as deamon use ```--pidfile=<path>``` for example ```--pidfile=/var/run/gradidoBlockchainConnector.pid``` 
- to use another file or folder for config use ```-c=<path>``` or ```--config=<path>```


## Usage Example

To use it simply make JSON Requests to the IP and Port on which GradidoBlockchainConnector is running (default Port: 1271)
```bash
curl 
``` 