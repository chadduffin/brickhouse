# Brickhouse
MMO infrastructure with encrypted sockets.

## How To Run
In the root directory:

mkdir secrets
cd secrets
openssl req -newkey rsa:2048 -nodes -keyout key.pem -x509 -days 365 -out cert.pem
cat cert.pem > certifications
