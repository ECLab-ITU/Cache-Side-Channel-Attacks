#!/bin/sh
openssl genrsa 1024 > server.key
openssl req -new -key server.key > server.csr
openssl x509 -req -in server.csr -out server.crt -signkey server.key
