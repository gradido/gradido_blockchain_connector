# API for Gradido Blockchain Connector 

<a name="error_reporting"></a>
##  Error Reporting
If an error occure, the Response usually looks like that:
```json
{
    "state": "error",
    "msg":"parameter error"
}
```

and sometimes they contain more infos in details:

```json
{
    "state": "error",
    "msg":"parameter error",
    "details": {
		"what": "contain invalid character",
		"fieldName": "name",
		"fieldType": "string",
	}
}
```


## New Api v2

## Login into Blockchain Connector / Create User if not exist
Login into Blockchain Connector to get JWT-Token for other requests.
Intern the Blockchain Connector load the [Session](https://gradido.github.io/gradido_blockchain_connector/classmodel_1_1_session.html) via [SessionManager](https://gradido.github.io/gradido_blockchain_connector/class_session_manager.html) into memory and keep 
and unencrpyted private key for the user to able to sign transactions. 
If user with name and groupAlias not exist, he will generate a public-private key
pair for this user and save it into db. The private key is saved
encrypted. For decryption password and name are needed. 

### Request
`POST http://localhost:1271/login`

```json
{	
	"name":"testUser1", "password":"b379938c857eaa6d480e46666813150cd70d25b289edfcbcdef570d0920a1e19f11fb682999efb5cce4bee78a17021b360c72cb02fd1bb6e67",
    "groupAlias":"testgroup1"
}
```

- `name`: String identifier for user, can be also a UUID
- `password`: encrypted password, encrypted with [sodium (sealed boxes)](https://libsodium.gitbook.io/doc/public-key_cryptography/sealed_boxes) crypto_box_seal, key pair can be generated with crypto_box_keypair, private key will be stored in Gradido Blockchain Connector properties file under `crypto.jwt.private`
- `groupAlias`: group/community identifier string (can be also a UUID)

### Response
```json 
{
	"state": "success",
	"token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJleHAiOjE2NTA1NjY5MzIuMTU1MTg0LCJpYXQiOjE2NTA1NjYzMzIuMTU1MTg0LCJuYW1lIjoidGVzdFVzZXIxIiwicHVia2V5IjoiMjVhZGMzZWEwYmZmZmE3ZDAwYmEyMjY4OTJlMDA1MWU5ODJlNGMxOGZiZDM4ZDRjNTE2NzIyMjRkNzM1NGY1YiIsInN1YiI6ImxvZ2luIn0.YsL4F7BUeBa-_yV1mF3aK9DSybwpj_eJH6fOaY_Tn9c"
}
```
With this token from the response we can user the other requests.

### Possible errors
Errors have the format like in [Error Reporting](#error_reporting) at the begin of file shown. 
For this request this error message are possible:

- `parameter error`: one of the expected parameters wasn't set
- `invalid group alias`: the group alias has an invalid format
	- this regular expression check the format: `^[a-z0-9-]{3,120}$`
- `Internal Server Error`: something went wrong with Gradido Blockchain Connector, in this case look at the console output or in the logfile from Gradido Blockchain Connector
- `login failed`: wrong password
- `cannot get free cpu core for encryption`: server gets to many login requests, try again later
- `<keyname> is missing from config`: a key wasn't found in config from Gradido Blockchain Connector
- `cannot decrypt password`: check that the password was really encrypted with the public key and the right private key was put into the Gradido Blockchain Server Config under `crypto.jwt.private`
- `invalid ip`: user was already logged in, but from another ip address

## Add Group to Global Group Blockchain
The [Gradido Node](https://github.com/gradido/gradido_node) has blockchains for every group and one blockchain which contain every Gradido group on Earth. 
This is to make sure that every group alias and also each coin color exist only once. 


### Request
`POST http://localhost:1271/globalGroupAdd`

a jwt Token returned from a Login Request transfered as Authorization Header is mandatory

```json
{
	"groupName":"test Gruppe ",
	"groupAlias":"testgroup1",
	"coinColor":"0xffaabbcc",
	"created":"2022-04-21T18:57:59.073Z"
}
```
- `groupName`: a name for the group how it should be shown as title
- `groupAlias`: a unique group alias containing only lowercase letters from a-z, 0-9 and - at least 3 character long, maximal 120 character long
- `coinColor`: (optional) a unique 32 Bit Integer Number or a hex number with maximal 8 characters, will be choosen randomly if let empty
- `created`: The current date and time on creating this transaction

### Possible errors
Errors have the format like in [Error Reporting](#error_reporting) at the begin of file shown. 
For this request this error message are possible:

- `invalid ip`: the login which created this jwt token, came from another ip
- `invalid jwt token`: jwt token couldn't be verified or don't contain expected data, or was timed out
- `no session found`: no session for name in jwt token found, maybe Gradido Blockchain Connector was restarted since creating this jwt token or it was deleted because the time last access was longer than the configured `session.duration_seconds`
- `coin color already exist, please choose another or keep empty`: this coin color already exist on Global Groups Blockchain
- `error by requesting Gradido Node`: error in the communication with Gradido Node
	- is the key `gradidoNode` filled correctly in Gradido Blockchain Connector Properties?
	- is the Gradido Node running and the last Version?
	- for more details look into console or log output from Gradido Blockchain Connector
- `coinColor isn't a valid hex string`: if the field coinColor is a string witz size > 0 but not a valid hex string
- `coinColor has unknown type`: if coinColor is neither a string or a unsigned 32 Bit Integer
- `Internal Server Error`: something went wrong with Gradido Blockchain Connector, in this case look at the console output or in the logfile from Gradido Blockchain Connector
- `transaction validation failed`: transaction is invalid, more infos can be found in details field from result
	#### group_name
	```json 
	"details" : {
		"what": "to short, at least 3",
		"fieldName": "group_name",
		"fieldType": "string"
	}
	```
	```json 
	"details" : {
		"what": "to long, max 255",
		"fieldName": "group_name",
		"fieldType": "string"
	}
	```
	```json 
	"details" : {
		"what": "invalid character, only ascii",
		"fieldName": "group_name",
		"fieldType": "string"
	}
	```
	#### group_alias
	```json
	"details" : {
		"what": "to short, at least 3",
		"fieldName": "group_alias", 
		"fieldType": "string"
	}
	```
	```json
	"details" : {
		"what": "to long, max 255",
		"fieldName": "group_alias", 
		"fieldType": "string"
	}
	```
	```json
	"details" : {
		"what": "invalid character, only ascii",
		"fieldName": "group_alias", 
		"fieldType": "string"
	}
	```
	```json
	"details" : {
		"what": "group alias is used from system",
		"fieldName": "group_alias", 
		"fieldType": "string"
	}
	```	

- `error by calling iota`: by calling iota an error occured, more infos can be found in details field from result
	
	```json
	"details": {
		"what": "no tips",
		"url": "api.lb-0.h.chrysalis-devnet.iota.cafe/api/v1/tips"
	}
	```
	iota hasn't returned previous iota transactions, if that ever happen I don't know what to do

	```json
	"details": {
		"what": "error parsing request answer",
		"parseErrorCode": "The document is empty.",
		"parseErrorPosition": 0,
		"src": "<iota response as raw text>"
	}
	```
	iota returned invalid json, all possible rapidjson parse error codes: [rapidjson error codes](https://rapidjson.org/group___r_a_p_i_d_j_s_o_n___e_r_r_o_r_s.html#ga633f43fd92e6ed5ceb87dbf570647847)
	
	```json
	"details": {
		"what": "data member in response missing",
		"url": "api.lb-0.h.chrysalis-devnet.iota.cafe/api/v1/messages"
	}
	```
	the response for pushing Gradido Transaction as iota message to iota don't contain `data` field, so maybe the iota api changed
	
	```json
	"details": {
		"what": "messageId in response is missing or not a string",
		"url": "api.lb-0.h.chrysalis-devnet.iota.cafe/api/v1/messages"
	}
	```
	the response for pushing Gradido Transaction as iota message to iota don't contain `messageId` field, so maybe the api changed
## 
/creation

## 
/transfer

## 
/registerAddress



##
/notify

##
/listTransactions

##
/listPending



--- 

## Old Api v1 

## Pack Transaction
Pack Transaction as protobuf Array, ready for signing
The returned bodyBytes are in base64 format. 
They must be at first unpacked in a binary array.
Than they can be signed with sodium.crypto_sign_detached (https://sodium-friends.github.io/docs/docs/signing#crypto_sign_detached)
m ist the binary body bytes buffer
sk is the private key of the user

currently every transaction need only one signature but this maybe change in future,
or new transaction types added which need more than one signature

After signing sendTransactionIota can be called for every transaction which consists of bodyBytes and a signature public key pair list
and with the corresponding group alias

For transactions which belong to two blockchains actually two transactions are generated and need to be signed and send

### Request
`POST http://localhost/login_api/packTransaction`

#### Transfer
Needed to be signed from sender user 

```json
{	
	"transactionType": "transfer",
	"created":"2021-01-10 10:00:00",
    "memo": "Danke für deine Hilfe!",
	"senderPubkey":"131c7f68dd94b2be4c913400ff7ff4cdc03ac2bda99c2d29edcacb3b065c67e6",
	"recipientPubkey":"eff7a4a440eb10fa6d5ae5ee47d63240c55ea3e1972e9815c09411e25ab09fdd",
	"amount": 1000000,
}
```

#### Cross-Group Transfer
Needed to be signed from sender user 

```json
{	
	"transactionType": "transfer",
	"created":"2021-01-10 10:00:00",
    "memo": "Danke für deine Hilfe!",
	"senderPubkey":"131c7f68dd94b2be4c913400ff7ff4cdc03ac2bda99c2d29edcacb3b065c67e6",
	"recipientPubkey":"eff7a4a440eb10fa6d5ae5ee47d63240c55ea3e1972e9815c09411e25ab09fdd",
	"amount": 1000000,
	"senderGroupAlias": "gdd1",
	"recipientGroupAlias":"gdd2"
}
```

#### Creation
Needed to be signed from another user from this group which isn't the recipient 
After implementing roles on blockchain, the signing person need to have the correct role for this

```json
{	
	"transactionType": "creation",
	"created":"2021-01-10 10:00:00",
    "memo": "AGE September 2021",
	"recipientPubkey":"eff7a4a440eb10fa6d5ae5ee47d63240c55ea3e1972e9815c09411e25ab09fdd",
	"amount": 10000000,
	"targetDate": "2021-09-01 01:00:00",
}
```

#### Group Member Add
Needed to be signed from the user to add

```json
{
	"transactionType": "groupMemberUpdate",
	"created":"2021-01-10 10:00:00",
	"userRootPubkey":"eff7a4a440eb10fa6d5ae5ee47d63240c55ea3e1972e9815c09411e25ab09fdd",
}
```

#### Group Member Move
Needed to be signed from the user to move

```json
{
	"transactionType": "groupMemberUpdate",
	"created":"2021-01-10 10:00:00",
	"userRootPubkey":"eff7a4a440eb10fa6d5ae5ee47d63240c55ea3e1972e9815c09411e25ab09fdd",
	"currentGroupAlias": "gdd1",
	"newGroupAlias":"gdd2"
}
```

- `transactionType`: type of transaction, currently available: transfer, creation and groupMemberUpdate
- `created`: creation date of transaction
- `memo`: memo for transfer and creation transaction, should be encrypted for example with the sender private key and the recipient public key with sodium.crypto_box_easy maybe with creation date as nonce
		https://sodium-friends.github.io/docs/docs/keyboxencryption#crypto_box_easy
- `senderPubkey`: sender account public key in hex format, need at least `amount` gradidos on this account
- `recipientPubkey`: recipient account public key in hex format, must be added first with a groupMemberUpdate transaction to his or her blockchain
- `amount`: amount of gradido as integer with 4 after comma
- `senderGroupAlias`: only needed for cross group transactions, group alias from sender group
- `recipientGroupAlias`: only needed for cross group transactions, group alias from recipient group
- `userRootPubkey`: root public key from user, all his other addresses are derived from this address https://github.com/bitcoin/bips/blob/master/bip-0032.mediawiki but with ed25519 keys
- `currentGroupAlias`: only needed for moving user, group alias from where the user is from
- `newGroupAlias`: only needed for moving user, group alias from the user where they are moving to

### Response
For Local Transactions which happen only on this group

```json
{
	"state":"success",
	"bodyBytesBase64": "CAEStwEKZgpkCiDs2zemYO1PxD1Odwh5YxyUmDp+lyxgVmiQgiFdPLUahRJAvyYRVASJNvyYiTAT2D8t6QtVgekqnsPIJRAx6jG8tEqxdzwKGg/Jm0gatdY1Ix7DGbHMBRw/9CtoXQXueqqDChJNChBBR0UgT2t0b2JlciAyMDIxEgYIuMWpjQY6MQonCiAdkWfcgRcDfCIg+GbikK6U9Fp4WMTGtAxF7RRdvhisSxCA2sQJGgYIgJ/ZigYaBgjGxamNBiABKiCijBRel5hudg5iZqfeQxjzIMhnOJA+tmHmloMVW+snjTIEyWETAA==",	
}
```

For Cross Group Transactions (and for moving user)
This return at least two transaction, one for every chain involved in this transaction,
inclusive groupAlias from the target blockchain for this transaction

```json
{
	"state":"success",
	"transactions" : 
	[
		{
			"groupAlias": "gdd1", 
			"bodyBytesBase64": "CAUS7QEKZgpkCiDRCqrSJQDyMPnshI+UbSnVU22w+98UJsQ5qwh10VbNCxJAPvxMj7GKxhUf6OfzKRwmZthxXroj/GSLWizDWURDNtL4E8Ez1zqOu9d/ytdZEtY/nQr+1jbLvtsJQ6dfAaDAARKCAQoRVGVzdCBjcm9zcyA0LjEyIDESBgjR366NBjJlEmMKSgomCiDRCqrSJQDyMPnshI+UbSnVU22w+98UJsQ5qwh10VbNCxCI7isSIOzbN6Zg7U/EPU53CHljHJSYOn6XLGBWaJCCIV08tRqFGgdzdGFnaW5nIgwI0d+ujQYQ+LzOwQMaBgjc366NBiABKiBlPZP6a69Kz4JZ87cDcXBARbMXpesRMdDrh5V1PSWRLzIEGIMTAA=="
		},
		{
			"groupAlias": "gdd2",
			"bodyBytesBase64": "CAoS8AEKZgpkCiDRCqrSJQDyMPnshI+UbSnVU22w+98UJsQ5qwh10VbNCxJAf6TTHmhDdP2VoN2eFmMhUHHjSg8ZHsfG2aEgRKVEPwCEVv2NFeSqbYSCbt8xVP16xu2hrEg87qCXq1AJBX7aCBKFAQoRVGVzdCBjcm9zcyA0LjEyIDESBgjS366NBjJoGmYKSgomCiDRCqrSJQDyMPnshI+UbSnVU22w+98UJsQ5qwh10VbNCxCI7isSIOzbN6Zg7U/EPU53CHljHJSYOn6XLGBWaJCCIV08tRqFGgpweXRoYWdvcmFzIgwI0d+ujQYQ+LzOwQMaBgjm366NBiABKiAnFsNGldvfB2QaenWbfdtiCBQ399e81il6VV0kL6CHijIEGYMTAA=="
		}
	]
}
```

## Send Transaction
### Request
`POST http://localhost/login_api/sendTransactionIota`

with 
```json 
{
	"bodyBytesBase64": "CAEStwEKZgpkCiDs2zemYO1PxD1Odwh5YxyUmDp+lyxgVmiQgiFdPLUahRJAvyYRVASJNvyYiTAT2D8t6QtVgekqnsPIJRAx6jG8tEqxdzwKGg/Jm0gatdY1Ix7DGbHMBRw/9CtoXQXueqqDChJNChBBR0UgT2t0b2JlciAyMDIxEgYIuMWpjQY6MQonCiAdkWfcgRcDfCIg+GbikK6U9Fp4WMTGtAxF7RRdvhisSxCA2sQJGgYIgJ/ZigYaBgjGxamNBiABKiCijBRel5hudg5iZqfeQxjzIMhnOJA+tmHmloMVW+snjTIEyWETAA==",
	"signaturePairs": [
		{
			"pubkey": "ecdb37a660ed4fc43d4e770879631c94983a7e972c6056689082215d3cb51a85",
			"signature": "c1b2e8077c206bf78aeaefbdcdfe8a5ae32ddf9adca95ba1f5a93e885b7b3a00f224fe1fb0ea491d20966f7336fd90479c792432dc94b8c8b83dd00510fca508"
		}
	],
	"groupAlias":"gdd1"
}
```

### Response
```json 
{
	"state": "success",
	"iotaMessageId": "acc2ad26e987a787145d20d61a140606b45bfc0857cff5391c784937a5430108"
}
```