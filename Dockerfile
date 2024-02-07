FROM gcc:latest

WORKDIR /app

COPY . .

RUN apt-get update && apt-get install -y make
RUN make fclean -C ./chat-c 
RUN make -C ./chat-c
RUN chmod +x entrypoint.sh

ENTRYPOINT ["./entrypoint.sh"]