# Brickhouse
MMO infrastructure with encrypted sockets.

## Generate a new certificate and key with:
openssl req -newkey rsa:2048 -nodes -keyout key.pem -x509 -days 365 -out cert.pem
