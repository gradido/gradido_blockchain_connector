# Gradido Blockchain Connector

For easy connecting to [Gradido Blockchain](https://github.com/gradido/gradido_blockchain). 

- It generate and stores ed25519 public and private key pair for user, but the private key will be stored only encrypted with user password.
- It sign and send Gradido Transactions via Iota into to blockchain
- store sended transactions in memory and has interface for [Gradido Node](https://github.com/gradido/gradido_node) to tell that pending transaction was confirmed or rejected [Read more](https://gradido.github.io/gradido_blockchain_connector/classmodel_1_1_pending_transactions.html)
- use JWT Token for authenticated json requests

[Doxygen Documentation](https://gradido.github.io/gradido_blockchain_connector/)

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

Result should be something like that: 
```json
{
    "state": "success",
    "token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJleHAiOjE2NTA1NjY5MzIuMTU1MTg0LCJpYXQiOjE2NTA1NjYzMzIuMTU1MTg0LCJuYW1lIjoidGVzdFVzZXIxIiwicHVia2V5IjoiMjVhZGMzZWEwYmZmZmE3ZDAwYmEyMjY4OTJlMDA1MWU5ODJlNGMxOGZiZDM4ZDRjNTE2NzIyMjRkNzM1NGY1YiIsInN1YiI6ImxvZ2luIn0.YsL4F7BUeBa-_yV1mF3aK9DSybwpj_eJH6fOaY_Tn9c"
}
```
On success the request return a jwt token which must be used on the other requests.

After successfully logged in your can register the group if it not already exist on blockchain:
```bash
curl -X POST \
     -H 'Content-Type: application/json' \
     -H 'Authorization: Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJleHAiOjE2NTA1NjY5MzIuMTU1MTg0LCJpYXQiOjE2NTA1NjYzMzIuMTU1MTg0LCJuYW1lIjoidGVzdFVzZXIxIiwicHVia2V5IjoiMjVhZGMzZWEwYmZmZmE3ZDAwYmEyMjY4OTJlMDA1MWU5ODJlNGMxOGZiZDM4ZDRjNTE2NzIyMjRkNzM1NGY1YiIsInN1YiI6ImxvZ2luIn0.YsL4F7BUeBa-_yV1mF3aK9DSybwpj_eJH6fOaY_Tn9c' \
     -d '{"groupName":"test Gruppe ","groupAlias":"testgroup1","coinColor":"","created":"2022-04-21T18:57:59.073Z"}' \
     http://localhost:1271/globalGroupAdd
``` 

If transaction was successfully send via Iota the result should be something like that:
```json
{
     "state":"success",
     "iotaMessageId":"c536f760a31621faf3efcbd07088168738bee4eda52945531e09ae646efc5c18"
}
```
With the iotaMessageId the transaction can be found on Iota Tangle.

Now you can register your public key (generated from GradidoBlockchainConnector) on the blockchain
```bash
curl -X POST \
     -H 'Content-Type: application/json' \
     -H 'Authorization: Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJleHAiOjE2NTA1NjY5MzIuMTU1MTg0LCJpYXQiOjE2NTA1NjYzMzIuMTU1MTg0LCJuYW1lIjoidGVzdFVzZXIxIiwicHVia2V5IjoiMjVhZGMzZWEwYmZmZmE3ZDAwYmEyMjY4OTJlMDA1MWU5ODJlNGMxOGZiZDM4ZDRjNTE2NzIyMjRkNzM1NGY1YiIsInN1YiI6ImxvZ2luIn0.YsL4F7BUeBa-_yV1mF3aK9DSybwpj_eJH6fOaY_Tn9c' \
     -d '{"created":"2022-04-21T19:13:01.506Z","addressType":"HUMAN"}' \
     http://localhost:1271/registerAddress
```

If transaction was successfully send via Iota the result should be something like that:
```json 
{
     "state":"success",
     "iotaMessageId":"07ec60fa60e0fd087bdf77bccb9c702942b61b1bf674fee17d3f258fed1fc8b0"
}
```
With another user you can now create gradidos for this user:

```bash
curl -X POST \
     -H 'Content-Type: application/json' \
     -H 'Authorization: Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJleHAiOjE2NTA1NzIyNjkuMjkwMzE4LCJpYXQiOjE2NTA1NzE2NjkuMjkwMzE4LCJuYW1lIjoidGVzdFVzZXIyIiwicHVia2V5IjoiOGIyMzkxM2VhMjMwMGYyNWVlYjY4NzNmMTE3ZTcwM2Q2ZGE5YjM5ZmNmMjgwMjE3Mzk2MzQ3YWU0NzNiMzdmNiIsInN1YiI6ImxvZ2luIn0.cg9lWwZsSgb-2QiDhz1vuYzHdM9ddkhvFzvKhnqjo4Q' \
     -d '{"memo":"test creation","recipientName":"testUser1","amount":"1000","targetDate":"2022-02-01","created":"2022-04-21T20:08:53.406Z","apolloTransactionId":"1"}' \
     http://localhost:1271/creation
```

Logged in back with this user, you can now send gradidos:

```bash
curl -X POST \
     -H 'Content-Type: application/json' \
     -H 'Authorization: Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJleHAiOjE2NTA1NzI0MzUuNTM0NTgyLCJpYXQiOjE2NTA1NzE4MzUuNTM0NTgyLCJuYW1lIjoidGVzdFVzZXIxIiwicHVia2V5IjoiZmIyNjY4Y2JlNWJmZWViYTNjNzkxOTJkZDI1MGJlNzI3ZjZmYTJiM2RlMjA2OGE4NWQyOTQ3NTFjMmE1Y2MyMCIsInN1YiI6ImxvZ2luIn0.Ymr7-QnSfsWNFOG-IevV3THYukJsiFdG934HgplKrsc' \
     -d '{"memo":"test Transfer","senderName":"testUser1","recipientName":"testUser2","amount":"100","created":"2022-04-21T20:10:59.429Z","apolloTransactionId":"2"}' \
     http://localhost:1271/transfer
```

The result on both requests are the same like for registerAddress and globalGroupAdd


For more Details look here [API](API.md)