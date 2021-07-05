FROM alpine:latest
WORKDIR /app
COPY *.cpp .
COPY *.hpp .
RUN apk update
RUN apk add g++
RUN g++ -o ConnectFour ConnectFour.cpp -O3
CMD ["./ConnectFour"]