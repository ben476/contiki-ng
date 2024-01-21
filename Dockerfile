FROM contiker/contiki-ng:latest

RUN apt install socat

# CMD ["/bin/sh", "-c", "bash --login"]