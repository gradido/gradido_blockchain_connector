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
3. make changes as needed, look for details into the default file config/blockchain_connector.properties
4. on linux create folder /var/log/gradido/ and give write permission to user which will be start GradidoBlockchainConnector
5. run GradidoBlockchainConnector 


## Command Line Options

- to start Gradido Blockchain Connector as daemon in linux use ```--daemon ```
- for starting it on windows as service look here: [Poco::ServerApplication](https://www.appinf.com/docs/poco-2013.2/Poco.Util.ServerApplication.html)
- to record process id if started as deamon use ```--pidfile=<path>``` for example ```--pidfile=/var/run/gradidoBlockchainConnector.pid``` 
- to use another file or folder for config use ```-c=<path>``` or ```--config=<path>```


## Usage Example

Example Datas
- Crypto Box Key Pair (x25519)
     - Public Key:5d8d7e0ab0070a9d2303e36d51e1c6774ce582e941e8134e5559f78aaf26d346
     - Private Key:2acd42ec14e12eecded40e300d420eabbf58f01d3cb3cf1eaf522af49ec92193
- name: testUser1
- password: testUser1 > b379938c857eaa6d480e46666813150cd70d25b289edfcbcdef570d0920a1e19f11fb682999efb5cce4bee78a17021b360c72cb02fd1bb6e67
- groupAlias: testgroup1

To use it simply make JSON Requests to the IP and Port on which GradidoBlockchainConnector is running (default Port: 1271)
Login with username `testUser1`, password `testUser1` encrypted with crypto_box_seal and `5d8d7e0ab0070a9d2303e36d51e1c6774ce582e941e8134e5559f78aaf26d346` as pubkey and from group `testgroup1` 
```bash
curl -X POST \
     -H 'Content-Type: application/json' \
     -d '{"name":"testUser1", "password":"b379938c857eaa6d480e46666813150cd70d25b289edfcbcdef570d0920a1e19f11fb682999efb5cce4bee78a17021b360c72cb02fd1bb6e67", "groupAlias":"testgroup1"}' \
     http://localhost:1271/login
``` 